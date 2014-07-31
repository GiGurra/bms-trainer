/*
 * DecodeServer.h
 *
 *  Created on: 26 okt 2013
 *      Author: GiGurra
 */

#ifndef DECODESERVER_H_
#define DECODESERVER_H_

#include "libgurra/smartptrs.h"
#include "libgurra/NonCopyable.h"

class DecodeServer: public NonCopyable {
public:

	DecodeServer(
			const int port = 12346,
			const int sockBufSz = 512 * 1024,
			const int recvBufSz = 512 * 1024);

	void start();
	void kill();
	void join();

	virtual ~DecodeServer();

private:
	class Impl;
	Uq<Impl> impl;

};

#endif /* DECODESERVER_H_ */
