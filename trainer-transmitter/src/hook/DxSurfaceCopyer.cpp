/*
 * DxSurfaceCopyer.cpp
 *
 *  Created on: 18 okt 2013
 *      Author: GiGurra
 */

#include "DxSurfaceCopyer.h"
#include "../libgurra/Logger.h"
#include "d3d9.h"
#include "GetSurfaceDescr.h"
#include <string.h>

class DxSurfaceCopyer::Impl: public NonCopyable {

public:

	Impl(IDirect3DDevice9 * parent, const Opt<Size>& sizeOverride) :
					m_parent(parent),
					m_videoRamCopy(0),
					m_systemRamCopy(0),
					m_pBits(0),
					m_sizeOverride(sizeOverride) {
		memset(&m_descr, 0, sizeof(m_descr));
	}

	virtual ~Impl() {
		if (m_systemRamCopy != 0) {
			m_systemRamCopy->Release();
			m_systemRamCopy = 0;
		}
		if (m_videoRamCopy != 0) {
			m_videoRamCopy->Release();
			m_videoRamCopy = 0;
		}
	}

	void mirrorVramFrom(IDirect3DSurface9 * source) {
		if (m_videoRamCopy == 0) {
			m_descr = getDescrForSurface(source);
			m_parent->CreateRenderTarget( //
					outputWidth(), //
					outputHeight(), //
					m_descr.Format, //
					D3DMULTISAMPLE_NONE, // m_descr.MultiSampleType, //
					0, //m_descr.MultiSampleQuality, //
					false, //
					&m_videoRamCopy, //
					0);

			LOG("Made vram copy: " << m_descr.Width << "x" << m_descr.Height << " --> " << outputWidth() << "x" << outputHeight());
		}

		m_parent->StretchRect(source, NULL, m_videoRamCopy, NULL, D3DTEXF_NONE);

	}

	void copyToSysRam() {

		if (m_systemRamCopy == 0 && m_videoRamCopy != 0) {

			m_parent->CreateOffscreenPlainSurface( //
					outputWidth(),  //
					outputHeight(), //
					m_descr.Format, //
					D3DPOOL_SYSTEMMEM, //
					&m_systemRamCopy, //
					0);

			LOG("Made sysram copy: " << outputWidth() << "x" << outputHeight());

			D3DLOCKED_RECT rect;
			m_systemRamCopy->LockRect(&rect, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY);
			m_pBits = rect.pBits;
			m_systemRamCopy->UnlockRect();
			LOG("pBits at: " << m_pBits);
		}

		if (m_videoRamCopy != 0 && m_systemRamCopy != 0)
			m_parent->GetRenderTargetData(m_videoRamCopy, m_systemRamCopy);

	}

	void handleD3dDeviceLost() {
		if (m_videoRamCopy != 0) {
			m_videoRamCopy->Release();
			m_videoRamCopy = 0;
		}
	}

	int outputWidth() const {
		return m_sizeOverride.map<int>([](const Size& sz) {return sz.x();}).getOrElse(m_descr.Width);
	}

	int outputHeight() const {
		return m_sizeOverride.map<int>([](const Size& sz) {return sz.y();}).getOrElse(m_descr.Height);
	}

	const byte * output() const {
		return m_systemRamCopy != 0 ? (const byte*) m_pBits : 0;
	}

private:

	IDirect3DDevice9 * m_parent;
	IDirect3DSurface9 * m_videoRamCopy;
	IDirect3DSurface9 * m_systemRamCopy;
	D3DSURFACE_DESC m_descr;
	void * m_pBits;

	Opt<Size> m_sizeOverride;

};

void DxSurfaceCopyer::mirrorVramFrom(IDirect3DSurface9* source) {
	impl->mirrorVramFrom(source);
}

DxSurfaceCopyer::DxSurfaceCopyer(IDirect3DDevice9* parent, const Opt<Size>& sizeOverride) :
				impl(new Impl(parent, sizeOverride)) {
}

DxSurfaceCopyer::~DxSurfaceCopyer() {
}

void DxSurfaceCopyer::copyToSysRam() {
	impl->copyToSysRam();
}

void DxSurfaceCopyer::handleD3dDeviceLost() {
	impl->handleD3dDeviceLost();
}

int DxSurfaceCopyer::outputWidth() const {
	return impl->outputWidth();
}

int DxSurfaceCopyer::outputHeight() const {
	return impl->outputHeight();
}

const byte* DxSurfaceCopyer::output() const {
	return impl->output();
}
