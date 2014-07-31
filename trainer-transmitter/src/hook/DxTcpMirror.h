/*
 * DxTcpMirror.h
 *
 *  Created on: 21 okt 2013
 *      Author: GiGurra
 */

#ifndef DXTCPMIRROR_H_
#define DXTCPMIRROR_H_

#include <string>

#include "../libgurra/smartptrs.h"
#include "../libgurra/Size.h"

class DxSurfaceCopyer;

class DxTcpMirror {
public:
	DxTcpMirror(const std::string& playerName, const int frameRate);
	virtual ~DxTcpMirror();

	void addStream(
			DxSurfaceCopyer * src,
			const std::string& streamName,
			const Size& streamSize,
			const int bitrate);

	void addTarget(const std::string& ip, const int port);

	void start();

private:
	class Impl;
	Uq<Impl> impl;

};

#endif /* DXTCPMIRROR_H_ */
