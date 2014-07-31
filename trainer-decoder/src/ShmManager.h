/*
 * ShmManager.h
 *
 *  Created on: 27 okt 2013
 *      Author: GiGurra
 */

#ifndef SHMMANAGER_H_
#define SHMMANAGER_H_

#include "libgurra/smartptrs.h"
#include "libgurra/Shm.h"

class ShmManager {
public:
	static Uq<ShmManager> INSTANCE;

	virtual ~ShmManager();

	Sh<Shm> openShm(const int size);

private:
	class Impl;
	Sh<Impl> impl;

	ShmManager();

};

#endif /* SHMMANAGER_H_ */
