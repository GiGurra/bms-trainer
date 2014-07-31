/*
 * DxSurfaceStream.cpp
 *
 *  Created on: 27 okt 2013
 *      Author: GiGurra
 */

#include "DxSurfaceStream.h"
#include "DxSurfaceCopyer.h"

#include "../libgurra/ReconnectingTcpClient.h"
#include "../libgurra/StreamPacket.h"
#include "../libgurra/VideoEncoder.h"
#include "../libgurra/Logger.h"

class DxSurfaceStream::Impl: public NonCopyable {
public:

	~Impl() {
	}

	Impl(
			DxSurfaceCopyer * src,
			const std::string& playerName,
			const std::string& streamName,
			const Size& streamSize,
			const int bitrate) :
					m_dxCopyer(src),
					m_playerName(playerName),
					m_streamName(streamName),
					m_streamSize(streamSize),
					m_bitrate(bitrate) {
	}

	void addClient(const std::string& ip, const int port) {
		auto client = ShWrap(new ReconnectingTcpClient(ip, port));
		client->start();
		m_clients.push_back(client);
	}

	void cycle() {

		encodeFrame().foreach(
				[&](const Sh<EncodedFrame>& packet) {

					StreamPacketHeader header = StreamPacketHeader::construct(
							packet->getWidth(),
							packet->getHeight(),
							m_playerName,
							m_streamName,
							packet->getByteSize(),
							packet->getFrameNbr()
					);

					for (auto c : m_clients) {
						if (c->isConnected()) {

							c->sendPodBlocking(header, 2);
							c->sendBlocking(packet->getData(), packet->getByteSize(), 2);
						}
					}

				});

		if (!getLastEncodeErr().empty()) {
			LOG("Failed to encode video, err: " << getLastEncodeErr());
		}
	}

private:

	std::vector<Sh<ReconnectingTcpClient> > m_clients;
	Uq<VideoEncoder> m_encoder;
	DxSurfaceCopyer * m_dxCopyer;
	std::string m_playerName;
	std::string m_streamName;
	Size m_streamSize;
	int m_bitrate;

	int dxCopyWidth() const {
		return m_dxCopyer->outputWidth();
	}

	int dxCopyHeight() const {
		return m_dxCopyer->outputHeight();
	}

	Opt<Sh<EncodedFrame> > encodeFrame() {
		if (m_dxCopyer->output() != 0) {
			if (!m_encoder) {

				m_encoder = UqWrap(new VideoEncoder( //
						dxCopyWidth(), //
						dxCopyHeight(), //
						m_streamSize.x(),  //
						m_streamSize.y(), //
						m_bitrate, //
						25));

				if (!m_encoder->getStatus()) {
					LOG(
							"Encoder creation failed with error: " << m_encoder->getLastErrMsg());
				}
			}
			if (m_encoder->getStatus()) {
				return m_encoder->encodeFrame(m_dxCopyer->output());
			} else {
				LOG("encoder status: " << m_encoder->getLastErrMsg());
			}
		}
		return Opt<Sh<EncodedFrame> >::INVALID;
	}

	const std::string getLastEncodeErr() const {
		return m_encoder ? m_encoder->getLastErrMsg() : std::string("");
	}

	Impl();
};

DxSurfaceStream::DxSurfaceStream(
		DxSurfaceCopyer * src,
		const std::string& playerName,
		const std::string& streamName,
		const Size& streamSize,
		const int bitrate) :
		impl(new Impl(src, playerName, streamName, streamSize, bitrate)) {
}

void DxSurfaceStream::addClient(const std::string& ip, const int port) {
	impl->addClient(ip, port);
}

void DxSurfaceStream::cycle() {
	impl->cycle();
}

DxSurfaceStream::~DxSurfaceStream() {
}

