/// CannonBall.cpp
///
/// 2019-2025 kbEngine 2.0

#define KFBX_DLLINFO
#include <dxgi1_6.h>
#include "stdafx.h"
#include "main.h"
#include "blk_core.h"
#include "DX11/kbRenderer_DX11.h"
#include "kbEditor.h"
#include "CannonGame.h"
#include "kbGameEntityHeader.h"
#include "renderer.h"
#include "dx11/kbRenderer_DX11.h"
#include "Renderer_Dx12.h"
#include "sw/renderer_sw.h"

#define MAX_LOADSTRING 100

using namespace blk;

extern bool g_UseEditor;
int g_screen_width = 1920;
int g_screen_height = 1080;
int WindowStartX = 0;
int MonitorIdx = 0;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
HWND hWnd;

bool destroyCalled = false;

/// MyRegisterClass
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMEBASE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDI_GAMEBASE);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

/// EnumDisplayMonitorsCB
BOOL CALLBACK EnumDisplayMonitorsCB(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	static bool firstMonitor = false;
	if (firstMonitor == false) {
		firstMonitor = true;
		WindowStartX += lprcMonitor->left;
	}

	return TRUE;
}

/// InitInstance
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance; // Store instance handle in our global variable
	//g_UseEditor = true;
	if (g_UseEditor == false) {
		WNDCLASS wc = {};
		memset(&wc, 0, sizeof(wc));
		wc.lpszClassName = L"kbEngine";
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WndProc;
		wc.cbWndExtra = 0;
		RegisterClass(&wc);

		DWORD wsStyle = WS_POPUP | WS_OVERLAPPEDWINDOW;

		if (MonitorIdx > 0) {
			EnumDisplayMonitors(nullptr, nullptr, EnumDisplayMonitorsCB, 0);
		}

		RECT winSize = { 0, 0, g_screen_width, g_screen_height };

		AdjustWindowRect(&winSize, wsStyle, false);
		hWnd = CreateWindowA(
			"kbEngine", "kbEngine",
			wsStyle | WS_VISIBLE,
			WindowStartX, 0,
			winSize.right - winSize.left, winSize.bottom - winSize.top,
			nullptr, nullptr, hInstance, nullptr
		);
	}

	return TRUE;
}

/// WndProc
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
		case WM_COMMAND:
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId) {
				case IDM_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			destroyCalled = true;
			//		PostQuitMessage(0);
			break;

		case WM_LBUTTONDOWN: {
			return 0;
		}

		case WM_RBUTTONDOWN: {
			return 0;
		}

		case WM_MOUSEWHEEL: {
		}
						  return 0;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/// About
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

/// _tWinMain
int APIENTRY _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDI_GAMEBASE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	std::string mapName;
	// mapName = "test";
	// mapName = "pinky";
	mapName = "kuwahara";
	// mapName = "sheep";
	 mapName = "characters";
//	mapName = "kungfusheep";
	//mapName = "destructibles";

		// Toggles
	g_UseEditor = 1;
	const u32 use_d3d12 = 1;
	const u32 use_sw = 0;

	// Perform application initialization
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDI_GAMEBASE));

	blk::initialize_engine();

	CannonGame* pGame = nullptr;
	kbEditor* applicationEditor = nullptr;

	if (g_UseEditor) {
		applicationEditor = new kbEditor();
		pGame = new CannonGame();
		applicationEditor->SetGame(pGame);
		g_pRenderer->SetRenderWindow(nullptr);

		if (use_d3d12) {
			g_renderer = new Renderer_Dx12();
		} else if (use_sw) {
			g_renderer = new Renderer_Sw();
		}
		if (g_renderer != nullptr) {
			g_renderer->initialize(applicationEditor->main_viewport_hwnd(), g_screen_width, g_screen_height);
		}

		if (mapName.length() > 0) {
			applicationEditor->LoadMap(mapName);
		}
	} else {
		if (use_d3d12) {
			g_renderer = new Renderer_Dx12();
		} else if (use_sw) {
			g_renderer = new Renderer_Sw();
		}
		if (g_renderer != nullptr) {
			g_renderer->initialize(hWnd, g_screen_width, g_screen_height);
		}

		g_pRenderer = new kbRenderer_DX11();
		g_pRenderer->Init(hWnd, g_screen_width, g_screen_height);
		pGame = new CannonGame();
		std::vector<const kbGameEntity*> GameEntitiesList;
		pGame->InitGame(hWnd, g_screen_width, g_screen_height, GameEntitiesList);
		pGame->LoadMap(mapName);
	}

	// Main message loop
	PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
	while ((applicationEditor == nullptr || applicationEditor->IsRunning()) && (pGame == nullptr || pGame->IsRunning()) && msg.message != WM_QUIT && destroyCalled == false) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		try {
			if (g_renderer != nullptr) {
				g_renderer->render();
			}

			if (g_UseEditor) {
				if (!applicationEditor->IsRunning()) {
					break;
				}
				applicationEditor->Update();
			} else {
				pGame->Update();
			}
		} catch (char* const string) {
			// todo : output error to console
			blk::log(string);
		}
	}

	if (g_pRenderer != nullptr) {
		g_pRenderer->WaitForRenderingToComplete();
	}

	{//if (!use_dx12) {
		pGame->StopGame();
		delete pGame;
		delete applicationEditor;
	}
	g_ResourceManager.Shutdown();
	g_pRenderer->Shutdown();
	delete g_pRenderer;
	g_pRenderer = nullptr;

	blk::shutdown_engine();

	kbConsoleVarManager::DeleteConsoleVarManager();

	delete g_renderer;

	return (int)msg.wParam;
}