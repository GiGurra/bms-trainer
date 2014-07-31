/*
 * ScopedSurface.h
 *
 *  Created on: 18 okt 2013
 *      Author: GiGurra
 */

#ifndef SCOPEDSURFACE_H_
#define SCOPEDSURFACE_H_

#include "../libgurra/smartptrs.h"

#define scopedSurface(surface) UqdWrap(surface, [](IDirect3DSurface9 * s){s->Release();;})

#endif /* SCOPEDSURFACE_H_ */
