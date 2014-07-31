/*
 * DxSurfaceStream.h
 *
 *  Created on: 27 okt 2013
 *      Author: GiGurra
 */

#ifndef DXSURFACESTREAM_H_
#define DXSURFACESTREAM_H_

#include <string>

#include "../libgurra/smartptrs.h"
#include "../libgurra/NonCopyable.h"
#include "../libgurra/Size.h"

class DxSurfaceCopyer;

class DxSurfaceStream {
public:

	DxSurfaceStream(
			DxSurfaceCopyer * src,
			const std::string& playerName,
			const std::string& streamName,
			const Size& streamSize,
			const int bitrate);

	void addClient(const std::string& ip, const int port);

	void cycle();

	virtual ~DxSurfaceStream();

private:
	class Impl;
	Uq<Impl> impl;

	DxSurfaceStream();
};

#endif /* DXSURFACESTREAM_H_ */
