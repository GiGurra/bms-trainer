/*
 * GetSurfaceDescr.h
 *
 *  Created on: 24 okt 2013
 *      Author: GiGurra
 */

#ifndef GETSURFACEDESCR_H_
#define GETSURFACEDESCR_H_

inline D3DSURFACE_DESC getDescrForSurface(IDirect3DSurface9* source) {
	D3DSURFACE_DESC descr;
	source->GetDesc(&descr);
	return descr;
}

#endif /* GETSURFACEDESCR_H_ */
