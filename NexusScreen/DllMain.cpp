#include "include/zmq.hpp"
#pragma comment(lib, "lib/libzmq-v120-mt-gd-4_0_4.lib")

#include "include/NexusHook.h"
#pragma comment(lib, "lib/NexusHook.lib")

#include "D3DX11.h"
#pragma comment(lib, "D3DX11.lib")

typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
NexusHook *hkMngr;

// Takes a screenshot
bool SaveTexture() {

	// Get device and swapchain from NexusHook
	IDXGISwapChain* pSwapChain = hkMngr->hMngr.pSwapChain;
	ID3D11Device* pDevice = hkMngr->hMngr.pDevice;
	ID3D11DeviceContext* pContext = hkMngr->hMngr.pContext;

	// Init backbuffer description and pointer
	D3D11_TEXTURE2D_DESC TextureDesc;
	ID3D11Texture2D *pBackBuffer = NULL;
	ID3D11Texture2D *pBackBufferDesc = NULL;

	// Get buffer of current swapchain
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// Get description of backbuffer
	pBackBuffer->GetDesc(&TextureDesc);

	// Modify backbuffer description
	TextureDesc.Usage = D3D11_USAGE_STAGING;			// Support data transfer
	TextureDesc.BindFlags = 0;							// Reset flags
	TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;	// Read access for CPU

	// Create texture from new description
	HRESULT CreateTextureResult = pDevice->CreateTexture2D(&TextureDesc, NULL, &pBackBufferDesc);
	if (FAILED(CreateTextureResult)) {
		std::cout << "Couldn't create texture from new description" << std::endl;
		return false;
	}

	// Copy backbuffer into texture
	pContext->CopyResource(pBackBufferDesc, pBackBuffer);

	// Save to file
	/*
	CreateTextureResult = D3DX11SaveTextureToFileA(pContext, pBackBufferTexture, D3DX11_IFF_BMP, "C:\\Users\\Nakroma\\Documents\\Visual Studio 2017\\Projects\\test.bmp");
	if (FAILED(CreateTextureResult)) {
		std::cout << "Couldn't save texture to file" << std::endl;
		return false;
	}*/

	// Release resources
	pBackBuffer->Release();
	pBackBufferDesc->Release();

	return true;
}

// Present hook
HRESULT __stdcall SwapChainPresentHook(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags) {

	// Take screenshot
	SaveTexture();

	// Call original function
	return ((D3D11PresentHook)hkMngr->oFunctions[SC_PRESENT])(pThis, SyncInterval, Flags);
}

void InitDll() {

	// Set up console
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	std::cout << "Dll attached" << std::endl;

	// Setup ZMQ
	/*
	zmqContext = zmq::context_t(1);
	zmqPublisher = zmq::socket_t(zmqContext, ZMQ_PUB);
	zmqPublisher.bind("tcp://localhost:5050");*/

	// Setup hook manager
	hkMngr = (NexusHook*)calloc(1, sizeof(NexusHook));
	*hkMngr = NexusHook();

	// Init hook manager
	hkMngr->Init();

	// Hook present
	hkMngr->HookSwapChain((DWORD_PTR)SwapChainPresentHook, SC_PRESENT);
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fwdReason, LPVOID lpvReserved) {

	if (fwdReason == DLL_PROCESS_ATTACH) {
		// Create new thread
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitDll, NULL, NULL, NULL);
	}

	return TRUE;
}