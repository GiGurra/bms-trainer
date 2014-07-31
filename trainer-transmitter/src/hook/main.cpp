#include "windows.h"
#include "detours.h"
#include "ID3D9Wrapper.h"

typedef IDirect3D9 *(WINAPI*CreateD3D9DevFcn)(UINT SDKVersion);
static CreateD3D9DevFcn s_realCreateD3D9IfFcn = 0;

static IDirect3D9* WINAPI Mine_Direct3DCreate9(UINT SDKVersion) {
	return new Direct3D9Wrapper(s_realCreateD3D9IfFcn(SDKVersion));
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(
		HMODULE hModule,
		DWORD ul_reason_for_call,
		LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		s_realCreateD3D9IfFcn = (CreateD3D9DevFcn) DetourFunction(
				(PBYTE) Direct3DCreate9,
				(PBYTE) Mine_Direct3DCreate9);
	}
	return true;
}

