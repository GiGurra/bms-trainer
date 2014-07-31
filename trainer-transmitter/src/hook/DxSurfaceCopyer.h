/*
 * DxSurfaceCopyer.h
 *
 *  Created on: 18 okt 2013
 *      Author: GiGurra
 */

#ifndef DXSURFACECOPYER_H_
#define DXSURFACECOPYER_H_

#include "../libgurra/NonCopyable.h"
#include "../libgurra/smartptrs.h"
#include "../libgurra/Option.h"
#include "../libgurra/Size.h"
#include "../libgurra/byte.h"

class IDirect3DDevice9;
class IDirect3DSurface9;

class DxSurfaceCopyer: public NonCopyable {

public:
	DxSurfaceCopyer(IDirect3DDevice9 * parent, const Opt<Size>& sizeOverride);
	virtual ~DxSurfaceCopyer();

	void mirrorVramFrom(IDirect3DSurface9 * source);
	void copyToSysRam();

	void handleD3dDeviceLost();

	int outputWidth() const;
	int outputHeight() const;

	const byte * output() const;

private:
	class Impl;
	Uq<Impl> impl;

	DxSurfaceCopyer();
};

#endif /* DXSURFACECOPYER_H_ */
