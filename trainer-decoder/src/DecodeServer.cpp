/*
 * DecodeServer.cpp
 *
 *  Created on: 26 okt 2013
 *      Author: GiGurra
 */

#include "DecodeServer.h"
#include "ShmManager.h"
#include "libgurra/Logger.h"
#include "libgurra/TcpServer.h"
#include "libgurra/VideoDecoder.h"
#include "libgurra/Blob.h"
#include "libgurra/StreamPacket.h"
#include "libgurra/ReconnectingTcpClient.h"
#include "libgurra/OsTime.h"
#include "libgurra/Shm.h"
#include <map>
#include <string.h>

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

class ClientHandler: public NonCopyable {
public:

	virtual ~ClientHandler() {
	}

	ClientHandler(ServersideTcpClient * client) :
					m_client(client) {
		client->sendPodBlocking(1, 2);
	}

	void handleNewData(Sh<Blob> data) {

		m_bufferedData.add(*data);
		Opt<StreamPacketHeader> header = m_bufferedData.tryPeekPod<StreamPacketHeader>();

		header.foreach(
				[&](const StreamPacketHeader& header) {
					if (header.getWholeSizeNative() <= m_bufferedData.size()) {
						m_bufferedData.removeFirst(sizeof(StreamPacketHeader));
						Blob streamPacket = m_bufferedData.getAndRemoveFirst(header.getDataSizeNative());
						streamPacket.resize(streamPacket.size() + 32);
						handlePacket(header, streamPacket);
					}
				});

		if (m_bufferedData.size() > 1024 * 1024) {
			LOG("ClientHandler::handleNewData overflow, resetting");
			m_bufferedData.clear();
		}

	}

	void handlePacket(const StreamPacketHeader& header, Blob& data) {

		std::function<byte*(int, int)> outAddrGetter =
				[&](int w, int h) {return getOutPtr(w,h);};

		Opt<DecodedFrame> frame = m_decoder.decodeFrame(header, data, outAddrGetter);

		frame.foreach([&](const DecodedFrame& frame) {
			notifyDrawer(header, frame);
		});

	}

	byte * getOutPtr(int w, int h) {
		if (!m_shm) {
			const int reqSz = w * h * 4 + sizeof(StreamPacketHeader);
			m_shm = ShmManager::INSTANCE->openShm(reqSz);
		}

		if (!m_shm->isStatusOk()) {
			return 0;
		}

		return ((byte*) m_shm->getPtr()) + sizeof(StreamPacketHeader);
	}

	void notifyDrawer(const StreamPacketHeader& header, const DecodedFrame& frame) {
		memcpy(m_shm->getPtr(), &header, sizeof(header));
		m_shm->flush();

		if (!m_notificationClient) {
			m_notificationClient = ShWrap(new ReconnectingTcpClient("127.0.0.1", 12345));
			m_notificationClient->start();
		}

		if (m_notificationClient->isConnected()) {
			m_notificationClient->sendBlobBlocking( //
					Blob() //
					.addPod(htonl(123456789)) //
					.addPod(htonl(m_shm->getName().size())) //
					.addString(m_shm->getName()), //
					1);
		}

	}

private:
	ServersideTcpClient * m_client;
	VideoDecoder m_decoder;
	Blob m_bufferedData;
	Sh<Shm> m_shm;
	Sh<ReconnectingTcpClient> m_notificationClient;

	ClientHandler();
};

class DecodeServer::Impl: public TcpServer {
public:

	virtual ~Impl() {
		kill();
		join();
	}

	Impl(const int port, const int sockBufSz, const int recvBufSz) :
					TcpServer(port, sockBufSz, recvBufSz) {

	}

protected:

	void handleConnect(Sh<ServersideTcpClient> client) {
		LOG("ServerSide-client:" << client << " connected, finding his decoder...");
		m_handlers[client->getId()] = ShWrap(new ClientHandler(client.get()));
	}

	void handleDisconnect(Sh<ServersideTcpClient> client) {
		LOG("ServerSide-client: " << client << " disconnected");
		m_handlers[client->getId()] = 0;
	}

	void handleMsg(Sh<ServersideTcpClient> client, Sh<Blob> data) {
		m_handlers[client->getId()]->handleNewData(data);
	}

private:
	std::map<int, Sh<ClientHandler> > m_handlers;

	Impl();
};

void DecodeServer::start() {
	impl->start();
}

void DecodeServer::kill() {
	impl->kill();
}

void DecodeServer::join() {
	impl->join();
}

DecodeServer::~DecodeServer() {
}

DecodeServer::DecodeServer(const int port, const int sockBufSz, const int recvBufSz) :
				impl(new Impl(port, sockBufSz, recvBufSz)) {
}

