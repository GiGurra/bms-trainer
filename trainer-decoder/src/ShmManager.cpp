/*
 * ShmManager.cpp
 *
 *  Created on: 27 okt 2013
 *      Author: GiGurra
 */

#include "ShmManager.h"
#include "libgurra/Mutex.h"

class ShmManager::Impl {
public:

	virtual ~Impl() {
	}

	Impl() :
					m_index(0) {
	}

	Sh<Shm> openShm(const int size) {
		ScopedLock lock(m_mutex);
		return ShWrap(
				new Shm(
						std::string("TrainerShm").append(std::to_string(m_index++)),
						size));
	}

private:
	int m_index;
	Mutex m_mutex;
};

ShmManager::ShmManager() :
				impl(new Impl) {

}

ShmManager::~ShmManager() {
}

Sh<Shm> ShmManager::openShm(const int size) {
	return impl->openShm(size);
}

Uq<ShmManager> ShmManager::INSTANCE(new ShmManager);
