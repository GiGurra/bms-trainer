#include "ID3D9Wrapper_Device.h"
#include "ScopedSurface.h"
#include "../libgurra/Logger.h"
#include "HookCfg.h"
#include "GetSurfaceDescr.h"

static Opt<Size> mkGpuSize(const Size& sz, const bool onGpu) {
	return onGpu ? sz : Opt<Size>::INVALID;
}

static Size getOutputSz(const std::string& streamName, const Size& defaultSz) {
	const auto wOpt = HookCfg::CFG.readInt(streamName, "res_x");
	const auto hOpt = HookCfg::CFG.readInt(streamName, "res_y");
	if (!wOpt.isDefined()) {
		LOG("Could not parse config file width for stream: " << streamName);
		return defaultSz;
	}
	if (!hOpt.isDefined()) {
		LOG("Could not parse config file height for stream: " << streamName);
		return defaultSz;
	}
	return Size(wOpt.getOrElse(640), hOpt.getOrElse(400));
}

static Size getBbOutputSz() {
	return getOutputSz("d3d_backbuffer", Size(640, 400));
}

static Size getSensorsOutputSz() {
	return getOutputSz("falcon_sensors", Size(400, 400));
}

static bool resizeOnGpu(const std::string& streamName) {
	return HookCfg::CFG.readInt(streamName, "resize_on_gpu").getOrElse(1);
}

static std::string getPlayerName() {
	return HookCfg::CFG.readString("misc", "callsign").getOrElse("unnamed_player");
}

static std::string getStreamTarget() {
	return HookCfg::CFG.readString("misc", "stream_to").getOrElse("127.0.0.1");
}

static int getFramerate() {
	return HookCfg::CFG.readInt("misc", "frame_rate").getOrElse(25);
}

static int getBitrate(const std::string& streamName) {
	return HookCfg::CFG.readInt(streamName, "bit_rate").getOrElse(500000);
}

static int getBbBitrate() {
	return getBitrate("d3d_backbuffer");
}

static int getSensorsBitrate() {
	return getBitrate("falcon_sensors");
}

static int getStreamPort() {
	return HookCfg::CFG.readInt("misc", "port").getOrElse(12346);
}

/******************************************************************************
 *
 *
 *							MODIFIED DX FCNS
 *
 ******************************************************************************/

Direct3DDevice9Wrapper::Direct3DDevice9Wrapper(IDirect3DDevice9 *pDirect3DDevice9, IDirect3D9 *pDirect3D9,
		D3DPRESENT_PARAMETERS *pPresentationParameters) :
				m_backingDevice(pDirect3DDevice9),
				m_backingD3dIfc(pDirect3D9),
				m_comRefCount(1),
				m_sensorsCopyer(this, mkGpuSize(getSensorsOutputSz(), resizeOnGpu("falcon_sensors"))),
				m_backbufferCopyer(this, mkGpuSize(getBbOutputSz(), resizeOnGpu("d3d_backbuffer"))),
				m_sensorsSrc(0),
				m_tcpMirror(getPlayerName(), getFramerate()) {
	m_tcpMirror.addStream(&m_sensorsCopyer, "falcon_sensors", getSensorsOutputSz(), getSensorsBitrate());
	m_tcpMirror.addStream(&m_backbufferCopyer, "d3d_backbuffer", getBbOutputSz(), getBbBitrate());
	m_tcpMirror.addTarget(getStreamTarget(), getStreamPort());
	m_tcpMirror.start();
}

Direct3DDevice9Wrapper::~Direct3DDevice9Wrapper() {
}

HRESULT Direct3DDevice9Wrapper::QueryInterface(REFIID riid, void** ppvObj) {
	HRESULT hRes = m_backingDevice->QueryInterface(riid, ppvObj);
	if (hRes == S_OK) {
		*ppvObj = this;
	} else {
		*ppvObj = NULL;
	}
	return hRes;
}

ULONG Direct3DDevice9Wrapper::AddRef() {
	m_comRefCount++;
	return m_backingDevice->AddRef();
}

ULONG Direct3DDevice9Wrapper::Release() {
	HRESULT out = m_backingDevice->Release();
	m_comRefCount--;
	if (m_comRefCount == 0) {
		delete this;
	}
	return out;
}

HRESULT Direct3DDevice9Wrapper::GetDirect3D(IDirect3D9** ppD3D9) {
	*ppD3D9 = m_backingD3dIfc;
	m_backingD3dIfc->AddRef();
	return D3D_OK;
}

HRESULT Direct3DDevice9Wrapper::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
	if (m_sensorsSrc == NULL) {
		auto desc = getDescrForSurface(pRenderTarget);
		if ((desc.Width == 600 && desc.Height == 600) || (desc.Width == 1200 && desc.Height == 1200)) {
			m_sensorsSrc = pRenderTarget;
		}
	}
	return m_backingDevice->SetRenderTarget(RenderTargetIndex, pRenderTarget);
}

HRESULT Direct3DDevice9Wrapper::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) {
	m_backbufferCopyer.handleD3dDeviceLost();
	m_sensorsCopyer.handleD3dDeviceLost();
	m_sensorsSrc = 0;
	return m_backingDevice->Reset(pPresentationParameters);
}

static auto getBackbuffer = [] (IDirect3DDevice9 * device) {
	IDirect3DSurface9 * backbuffer;
	device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	return scopedSurface(backbuffer);
};

HRESULT Direct3DDevice9Wrapper::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride,
CONST RGNDATA* pDirtyRegion) {

	auto bb = getBackbuffer(this);

	if (bb)
		m_backbufferCopyer.mirrorVramFrom(bb.get());

	HRESULT out = m_backingDevice->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	if (m_sensorsSrc)
		m_sensorsCopyer.mirrorVramFrom(m_sensorsSrc);

	if (m_sensorsSrc)
		m_sensorsCopyer.copyToSysRam();

	if (bb)
		m_backbufferCopyer.copyToSysRam();

	return out;
}

/******************************************************************************
 *
 *
 *						UNMODIFIED DX FCNS
 *
 ******************************************************************************/

HRESULT Direct3DDevice9Wrapper::BeginScene() {
	return m_backingDevice->BeginScene();
}

HRESULT Direct3DDevice9Wrapper::TestCooperativeLevel() {
	return m_backingDevice->TestCooperativeLevel();
}

UINT Direct3DDevice9Wrapper::GetAvailableTextureMem() {
	return m_backingDevice->GetAvailableTextureMem();
}

HRESULT Direct3DDevice9Wrapper::EvictManagedResources() {
	return m_backingDevice->EvictManagedResources();
}

HRESULT Direct3DDevice9Wrapper::GetDeviceCaps(D3DCAPS9* pCaps) {
	return m_backingDevice->GetDeviceCaps(pCaps);
}

HRESULT Direct3DDevice9Wrapper::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) {
	return m_backingDevice->GetDisplayMode(iSwapChain, pMode);
}

HRESULT Direct3DDevice9Wrapper::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) {
	return m_backingDevice->GetCreationParameters(pParameters);
}

HRESULT Direct3DDevice9Wrapper::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {
	return m_backingDevice->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
}

void Direct3DDevice9Wrapper::SetCursorPosition(int X, int Y, DWORD Flags) {
	m_backingDevice->SetCursorPosition(X, Y, Flags);
}

BOOL Direct3DDevice9Wrapper::ShowCursor(BOOL bShow) {
	return m_backingDevice->ShowCursor(bShow);
}

HRESULT Direct3DDevice9Wrapper::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,
		IDirect3DSwapChain9** pSwapChain) {
	return m_backingDevice->CreateAdditionalSwapChain(pPresentationParameters, pSwapChain);
}

HRESULT Direct3DDevice9Wrapper::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) {
	return m_backingDevice->GetSwapChain(iSwapChain, pSwapChain);
}

UINT Direct3DDevice9Wrapper::GetNumberOfSwapChains() {
	return m_backingDevice->GetNumberOfSwapChains();
}

HRESULT Direct3DDevice9Wrapper::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type,
		IDirect3DSurface9** ppBackBuffer) {
	return m_backingDevice->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer);
}

HRESULT Direct3DDevice9Wrapper::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
	return m_backingDevice->GetRasterStatus(iSwapChain, pRasterStatus);
}

HRESULT Direct3DDevice9Wrapper::SetDialogBoxMode(BOOL bEnableDialogs) {
	return m_backingDevice->SetDialogBoxMode(bEnableDialogs);
}

void Direct3DDevice9Wrapper::SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
	m_backingDevice->SetGammaRamp(iSwapChain, Flags, pRamp);
}

void Direct3DDevice9Wrapper::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) {
	m_backingDevice->GetGammaRamp(iSwapChain, pRamp);
}

HRESULT Direct3DDevice9Wrapper::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
		IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage,
		D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
		IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
		IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
		IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface,
			pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface,
		HANDLE* pSharedHandle) {
	return m_backingDevice->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface,
			pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
		IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) {
	return m_backingDevice->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
}

HRESULT Direct3DDevice9Wrapper::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {
	return m_backingDevice->UpdateTexture(pSourceTexture, pDestinationTexture);
}

HRESULT Direct3DDevice9Wrapper::GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) {
	return m_backingDevice->GetRenderTargetData(pRenderTarget, pDestSurface);
}

HRESULT Direct3DDevice9Wrapper::GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) {
	return m_backingDevice->GetFrontBufferData(iSwapChain, pDestSurface);
}

HRESULT Direct3DDevice9Wrapper::StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
		IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {
	return m_backingDevice->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
}

HRESULT Direct3DDevice9Wrapper::ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) {
	return m_backingDevice->ColorFill(pSurface, pRect, color);
}

HRESULT Direct3DDevice9Wrapper::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
		IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
	return m_backingDevice->CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, pSharedHandle);
}

HRESULT Direct3DDevice9Wrapper::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) {
	return m_backingDevice->GetRenderTarget(RenderTargetIndex, ppRenderTarget);
}

HRESULT Direct3DDevice9Wrapper::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) {
	return m_backingDevice->SetDepthStencilSurface(pNewZStencil);
}

HRESULT Direct3DDevice9Wrapper::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) {
	return m_backingDevice->GetDepthStencilSurface(ppZStencilSurface);
}

HRESULT Direct3DDevice9Wrapper::EndScene() {
	return m_backingDevice->EndScene();
}

HRESULT Direct3DDevice9Wrapper::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
	return m_backingDevice->Clear(Count, pRects, Flags, Color, Z, Stencil);
}

HRESULT Direct3DDevice9Wrapper::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
	return m_backingDevice->SetTransform(State, pMatrix);
}

HRESULT Direct3DDevice9Wrapper::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
	return m_backingDevice->GetTransform(State, pMatrix);
}

HRESULT Direct3DDevice9Wrapper::MultiplyTransform(D3DTRANSFORMSTATETYPE param0, CONST D3DMATRIX * param1) {
	return m_backingDevice->MultiplyTransform(param0, param1);
}

HRESULT Direct3DDevice9Wrapper::SetViewport(CONST D3DVIEWPORT9* pViewport) {
	return m_backingDevice->SetViewport(pViewport);
}

HRESULT Direct3DDevice9Wrapper::GetViewport(D3DVIEWPORT9* pViewport) {
	return m_backingDevice->GetViewport(pViewport);
}

HRESULT Direct3DDevice9Wrapper::SetMaterial(CONST D3DMATERIAL9* pMaterial) {
	return m_backingDevice->SetMaterial(pMaterial);
}

HRESULT Direct3DDevice9Wrapper::GetMaterial(D3DMATERIAL9* pMaterial) {
	return m_backingDevice->GetMaterial(pMaterial);
}

HRESULT Direct3DDevice9Wrapper::SetLight(DWORD Index, CONST D3DLIGHT9* param1) {
	return m_backingDevice->SetLight(Index, param1);
}

HRESULT Direct3DDevice9Wrapper::GetLight(DWORD Index, D3DLIGHT9* param1) {
	return m_backingDevice->GetLight(Index, param1);
}

HRESULT Direct3DDevice9Wrapper::LightEnable(DWORD Index, BOOL Enable) {
	return m_backingDevice->LightEnable(Index, Enable);
}

HRESULT Direct3DDevice9Wrapper::GetLightEnable(DWORD Index, BOOL* pEnable) {
	return m_backingDevice->GetLightEnable(Index, pEnable);
}

HRESULT Direct3DDevice9Wrapper::SetClipPlane(DWORD Index, CONST float* pPlane) {
	return m_backingDevice->SetClipPlane(Index, pPlane);
}

HRESULT Direct3DDevice9Wrapper::GetClipPlane(DWORD Index, float* pPlane) {
	return m_backingDevice->GetClipPlane(Index, pPlane);
}

HRESULT Direct3DDevice9Wrapper::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
	return m_backingDevice->SetRenderState(State, Value);
}

HRESULT Direct3DDevice9Wrapper::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
	return m_backingDevice->GetRenderState(State, pValue);
}

HRESULT Direct3DDevice9Wrapper::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) {
	return m_backingDevice->CreateStateBlock(Type, ppSB);
}

HRESULT Direct3DDevice9Wrapper::BeginStateBlock() {
	return m_backingDevice->BeginStateBlock();
}

HRESULT Direct3DDevice9Wrapper::EndStateBlock(IDirect3DStateBlock9** ppSB) {
	return m_backingDevice->EndStateBlock(ppSB);
}

HRESULT Direct3DDevice9Wrapper::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus) {
	return m_backingDevice->SetClipStatus(pClipStatus);
}

HRESULT Direct3DDevice9Wrapper::GetClipStatus(D3DCLIPSTATUS9* pClipStatus) {
	return m_backingDevice->GetClipStatus(pClipStatus);
}

HRESULT Direct3DDevice9Wrapper::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) {
	return m_backingDevice->GetTexture(Stage, ppTexture);
}

HRESULT Direct3DDevice9Wrapper::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) {
	return m_backingDevice->SetTexture(Stage, pTexture);
}

HRESULT Direct3DDevice9Wrapper::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
	return m_backingDevice->GetTextureStageState(Stage, Type, pValue);
}

HRESULT Direct3DDevice9Wrapper::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
	return m_backingDevice->SetTextureStageState(Stage, Type, Value);
}

HRESULT Direct3DDevice9Wrapper::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) {
	return m_backingDevice->GetSamplerState(Sampler, Type, pValue);
}

HRESULT Direct3DDevice9Wrapper::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {
	return m_backingDevice->SetSamplerState(Sampler, Type, Value);
}

HRESULT Direct3DDevice9Wrapper::ValidateDevice(DWORD* pNumPasses) {
	return m_backingDevice->ValidateDevice(pNumPasses);
}

HRESULT Direct3DDevice9Wrapper::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
	return m_backingDevice->SetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT Direct3DDevice9Wrapper::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) {
	return m_backingDevice->GetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT Direct3DDevice9Wrapper::SetCurrentTexturePalette(UINT PaletteNumber) {
	return m_backingDevice->SetCurrentTexturePalette(PaletteNumber);
}

HRESULT Direct3DDevice9Wrapper::GetCurrentTexturePalette(UINT *PaletteNumber) {
	return m_backingDevice->GetCurrentTexturePalette(PaletteNumber);
}

HRESULT Direct3DDevice9Wrapper::SetScissorRect(CONST RECT* pRect) {
	return m_backingDevice->SetScissorRect(pRect);
}

HRESULT Direct3DDevice9Wrapper::GetScissorRect(RECT* pRect) {
	return m_backingDevice->GetScissorRect(pRect);
}

HRESULT Direct3DDevice9Wrapper::SetSoftwareVertexProcessing(BOOL bSoftware) {
	return m_backingDevice->SetSoftwareVertexProcessing(bSoftware);
}

BOOL Direct3DDevice9Wrapper::GetSoftwareVertexProcessing() {
	return m_backingDevice->GetSoftwareVertexProcessing();
}

HRESULT Direct3DDevice9Wrapper::SetNPatchMode(float nSegments) {
	return m_backingDevice->SetNPatchMode(nSegments);
}

float Direct3DDevice9Wrapper::GetNPatchMode() {
	return m_backingDevice->GetNPatchMode();
}

HRESULT Direct3DDevice9Wrapper::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
	return m_backingDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT Direct3DDevice9Wrapper::DrawIndexedPrimitive(D3DPRIMITIVETYPE param0, INT BaseVertexIndex, UINT MinVertexIndex,
		UINT NumVertices, UINT startIndex, UINT primCount) {
	return m_backingDevice->DrawIndexedPrimitive(param0, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT Direct3DDevice9Wrapper::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
	return m_backingDevice->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT Direct3DDevice9Wrapper::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices,
		UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride) {
	return m_backingDevice->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData,
			IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT Direct3DDevice9Wrapper::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
		IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {
	return m_backingDevice->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
}

HRESULT Direct3DDevice9Wrapper::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,
		IDirect3DVertexDeclaration9** ppDecl) {
	return m_backingDevice->CreateVertexDeclaration(pVertexElements, ppDecl);
}

HRESULT Direct3DDevice9Wrapper::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) {
	return m_backingDevice->SetVertexDeclaration(pDecl);
}

HRESULT Direct3DDevice9Wrapper::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) {
	return m_backingDevice->GetVertexDeclaration(ppDecl);
}

HRESULT Direct3DDevice9Wrapper::SetFVF(DWORD FVF) {
	return m_backingDevice->SetFVF(FVF);
}

HRESULT Direct3DDevice9Wrapper::GetFVF(DWORD* pFVF) {
	return m_backingDevice->GetFVF(pFVF);
}

HRESULT Direct3DDevice9Wrapper::CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) {
	return m_backingDevice->CreateVertexShader(pFunction, ppShader);
}

HRESULT Direct3DDevice9Wrapper::SetVertexShader(IDirect3DVertexShader9* pShader) {
	return m_backingDevice->SetVertexShader(pShader);
}

HRESULT Direct3DDevice9Wrapper::GetVertexShader(IDirect3DVertexShader9** ppShader) {
	return m_backingDevice->GetVertexShader(ppShader);
}

HRESULT Direct3DDevice9Wrapper::SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
	return m_backingDevice->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT Direct3DDevice9Wrapper::GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
	return m_backingDevice->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT Direct3DDevice9Wrapper::SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {
	return m_backingDevice->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT Direct3DDevice9Wrapper::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
	return m_backingDevice->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT Direct3DDevice9Wrapper::SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) {
	return m_backingDevice->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT Direct3DDevice9Wrapper::GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {
	return m_backingDevice->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT Direct3DDevice9Wrapper::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes,
		UINT Stride) {
	if (StreamNumber == 0)
		m_Stride = Stride;
	return m_backingDevice->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
}

HRESULT Direct3DDevice9Wrapper::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes,
		UINT* pStride) {
	return m_backingDevice->GetStreamSource(StreamNumber, ppStreamData, pOffsetInBytes, pStride);
}

HRESULT Direct3DDevice9Wrapper::SetStreamSourceFreq(UINT StreamNumber, UINT Setting) {
	return m_backingDevice->SetStreamSourceFreq(StreamNumber, Setting);
}

HRESULT Direct3DDevice9Wrapper::GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) {
	return m_backingDevice->GetStreamSourceFreq(StreamNumber, pSetting);
}

HRESULT Direct3DDevice9Wrapper::SetIndices(IDirect3DIndexBuffer9* pIndexData) {
	return m_backingDevice->SetIndices(pIndexData);
}

HRESULT Direct3DDevice9Wrapper::GetIndices(IDirect3DIndexBuffer9** ppIndexData) {
	return m_backingDevice->GetIndices(ppIndexData);
}

HRESULT Direct3DDevice9Wrapper::CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
	return m_backingDevice->CreatePixelShader(pFunction, ppShader);
}

HRESULT Direct3DDevice9Wrapper::SetPixelShader(IDirect3DPixelShader9* pShader) {
	return m_backingDevice->SetPixelShader(pShader);
}

HRESULT Direct3DDevice9Wrapper::GetPixelShader(IDirect3DPixelShader9** ppShader) {
	return m_backingDevice->GetPixelShader(ppShader);
}

HRESULT Direct3DDevice9Wrapper::SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
	return m_backingDevice->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT Direct3DDevice9Wrapper::GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
	return m_backingDevice->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT Direct3DDevice9Wrapper::SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {
	return m_backingDevice->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT Direct3DDevice9Wrapper::GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
	return m_backingDevice->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT Direct3DDevice9Wrapper::SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) {
	return m_backingDevice->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT Direct3DDevice9Wrapper::GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {
	return m_backingDevice->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT Direct3DDevice9Wrapper::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
	return m_backingDevice->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT Direct3DDevice9Wrapper::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
	return m_backingDevice->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}

HRESULT Direct3DDevice9Wrapper::DeletePatch(UINT Handle) {
	return m_backingDevice->DeletePatch(Handle);
}

HRESULT Direct3DDevice9Wrapper::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) {
	return m_backingDevice->CreateQuery(Type, ppQuery);
}
