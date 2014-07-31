/*
 * DxTcpMirror.cpp
 *
 *  Created on: 21 okt 2013
 *      Author: GiGurra
 */

#include "DxTcpMirror.h"
#include "DxSurfaceCopyer.h"
#include "DxSurfaceStream.h"
#include "../libgurra/NonCopyable.h"
#include "../libgurra/OsTime.h"
#include "../libgurra/Thread.h"
#include <vector>

class DxTcpMirror::Impl: public Thread {
public:

	virtual ~Impl() {
		kill();
		join();
	}

	Impl(const std::string& playerName, const int frameRate) :
					m_frameRate(frameRate),
					m_lastFrameTime(getCurTimeSeconds()),
					m_playerName(playerName) {
	}

	void addStream(
			DxSurfaceCopyer * src,
			const std::string& streamName,
			const Size& streamSize,
			const int bitrate) {
		m_streams.push_back(
				ShWrap(
						new DxSurfaceStream(
								src,
								m_playerName,
								streamName,
								streamSize,
								bitrate)));
	}

	void addTarget(const std::string& ip, const int port) {
		for (auto s : m_streams)
			s->addClient(ip, port);
	}

	void run() {

		while (isToLive()) {

			const double frameRateDt = 1.0 / double(m_frameRate);

			while (isToLive()
					&& ((getCurTimeSeconds() - m_lastFrameTime) < frameRateDt)) {
				Thread::sleep(1);
			}
			m_lastFrameTime += frameRateDt;

			for (auto s : m_streams) {
				s->cycle();
			}

		}
	}

private:
	int m_frameRate;
	double m_lastFrameTime;
	std::string m_playerName;
	std::vector<Sh<DxSurfaceStream> > m_streams;

};

DxTcpMirror::DxTcpMirror(const std::string& playerName, const int frameRate) :
		impl(new Impl(playerName, frameRate)) {
}

DxTcpMirror::~DxTcpMirror() {
}

void DxTcpMirror::addStream(
		DxSurfaceCopyer * src,
		const std::string& streamName,
		const Size& streamSize,
		const int bitrate) {
	impl->addStream(src, streamName, streamSize, bitrate);
}

void DxTcpMirror::addTarget(const std::string& ip, const int port) {
	impl->addTarget(ip, port);
}

void DxTcpMirror::start() {
	impl->start();
}
