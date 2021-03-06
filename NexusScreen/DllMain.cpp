#include "include/NexusHook.h"
#pragma comment(lib, "lib/NexusHook.lib")

#include "D3DX11.h"
#pragma comment(lib, "D3DX11.lib")

typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
NexusHook *hkMngr;
HANDLE hPipe;
DWORD dwPipeThreadId = 0;

// Takes a screenshot
bool SaveTexture(HANDLE hTempPipe) {

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

	char *sContent = "HAllo :D";
	DWORD dwWritten;
	if (!WriteFile(hTempPipe, sContent, strlen(sContent), &dwWritten, NULL)) {
		std::cout << "Failed to write input pipe " << std::hex << GetLastError() << std::endl;
		return false;
	}

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
	//SaveTexture();

	// Call original function
	return ((D3D11PresentHook)hkMngr->oFunctions[SC_PRESENT])(pThis, SyncInterval, Flags);
}

// Pipe thread
DWORD WINAPI InstanceThread(LPVOID lpvParam) {

	std::cout << "Started pipe thread" << std::endl;

	// Get pipe from param
	HANDLE hTempPipe = NULL;
	if (lpvParam == NULL) return 0;
	else hTempPipe = (HANDLE)lpvParam;

	// Accept incoming requests
	while (1) {

		// Check for connected pipe
		if (ConnectNamedPipe(hTempPipe, NULL)) {

			std::cout << "Got request on pipe..." << std::endl;

			// Send screenshot to pipe
			SaveTexture(hTempPipe);
		}
		else std::cout << "Failed on accepting request to pipe" << std::endl;

		// Disconnect client
		DisconnectNamedPipe(hTempPipe);
	}

	return 1;
}

// Inits request pipe
void InitPipe() {

	// Create pipe
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\NexusScreen"),	// Name
		PIPE_ACCESS_OUTBOUND,									// Send only
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,		// Bytestream
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	if (hPipe == INVALID_HANDLE_VALUE) std::cout << "Failed on pipe creation" << std::endl;
	else std::cout << "Created request pipe" << std::endl;

	// Start pipe thread
	HANDLE hPipeThread = CreateThread(NULL,	// No security attributes
		0,				// Default stack size
		InstanceThread,	// Thread function
		(LPVOID)hPipe,	// Thread params
		0,				// Not suspended
		&dwPipeThreadId);	// Save thread id

	if (hPipeThread == NULL) std::cout << "Failed on pipe thread creation" << std::endl;
	else CloseHandle(hPipeThread);
}

// Inits Dll
void InitDll() {

	// Set up console
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	std::cout << "Dll attached" << std::endl;

	// Setup hook manager
	hkMngr = (NexusHook*)calloc(1, sizeof(NexusHook));
	*hkMngr = NexusHook();

	// Init hook manager
	hkMngr->Init();

	// Hook present
	hkMngr->HookSwapChain((DWORD_PTR)SwapChainPresentHook, SC_PRESENT);

	// Setup pipe
	InitPipe();
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fwdReason, LPVOID lpvReserved) {

	if (fwdReason == DLL_PROCESS_ATTACH) {

		// Create new thread
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InitDll, NULL, NULL, NULL);
	}

	else if (fwdReason == DLL_PROCESS_DETACH) {

		// Delete pipe
		DisconnectNamedPipe(hPipe);
	}

	return TRUE;
}