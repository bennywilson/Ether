// kbToonSpriteMaker.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <Windows.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include "kbToonSpriteMaker.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

class A {
public:
	A() { abc =20; }

	int abc;
};
class B : public A {
public:
	B() { def = 23; abc =12; }

	int def;

};
class D : public A {
public:
	D() { ghi = 34; }
	int ghi;
};

void ProcessImage( const char * fileName ) {

	A * a = new A();
	A * b = new B();
	A* d = new D();

//	B v = (*b );
	//const char fileName[] = "../assets/23.bmp";
	FILE * bitmapFile = fopen( fileName, "rb" );

	if ( bitmapFile == NULL ) {
		return;
	}

	BITMAPFILEHEADER bitmapHeader;
	BITMAPINFOHEADER bitmapInfo;

	fread( &bitmapHeader, sizeof( BITMAPFILEHEADER ), 1, bitmapFile );

	if ( bitmapHeader.bfType != 19778 ) {
		fclose( bitmapFile );
		return;
	}

	fread( &bitmapInfo, sizeof( BITMAPINFOHEADER ), 1, bitmapFile );

	if ( bitmapInfo.biBitCount != 24 || bitmapInfo.biCompression != 0 ) {
		fclose( bitmapFile );
		return;
	}

	const int imageWidth = bitmapInfo.biWidth;
	const int imageHeight = bitmapInfo.biHeight;
	const int padSize = ( ( ( imageWidth * 3 ) + 3 ) & 0xfffffffc ) - ( imageWidth * 3 );
	const int dataSize = bitmapHeader.bfSize - bitmapHeader.bfOffBits;

	BYTE * fileImageData = new BYTE[dataSize];

	fread( fileImageData, sizeof( BYTE ), dataSize, bitmapFile );
	fclose( bitmapFile );

	int curPixel = 0;

	// copy file image data into a buffer w/o the padding for convenience
	BYTE * imageData = new BYTE[ imageWidth * imageHeight *3];
	int localIndex = 0;
	for ( int pix = 0; pix < dataSize ; pix += 3, curPixel++, localIndex += 3 ) {

		if ( curPixel == imageWidth ) {
			curPixel = 0;

			pix += padSize;
		} else {
			imageData[localIndex + 0] = fileImageData[pix+0];
			imageData[localIndex + 1] = fileImageData[pix+1];
			imageData[localIndex + 2] = fileImageData[pix+2];
		}
	}

	// transparency
/*	for ( int y = 0; y < imageHeight; y++ ) {
		if (y > 64 && y < 864 - 64)
		{
			continue;
		}
		for ( int x = 0; x < imageWidth; x++ ) {
			int index = ( ( y * imageWidth ) + x) * 3;
			imageData[index+0] = 0;
			imageData[index+1] = 0;
			imageData[index+2] = 0;

		}
	}*/
	/*for ( int i = 0; i < imageHeight * imageWidth * 3; i += 3 ) {
		float x = imageData[i + 0] / 255.0f;
		float y = imageData[i + 1] / 255.0f;
		float z = imageData[i + 2] / 255.0f;

		if ( x*x+y*y+z*z > 0.25f ) {
			imageData[i+0] = 255;
			imageData[i+1] = 0;
			imageData[i+2] = 255;
		}
	}*/

	// determine mean and variance of each pixel
	struct meanAndVariance_t {
		meanAndVariance_t() { mean[0] = 0.0f, mean[1] = 0.0f, mean[2] = 0.0f, variance = 0.0f; }
		float	mean[3];
		float	variance;
	};

	meanAndVariance_t * meanAndVariance = new meanAndVariance_t[imageWidth * imageHeight];
	const int Kuwahara_Half_Size = 2;

	for ( int y = 0; y < imageHeight; y++ ) {
		for ( int x = 0; x < imageWidth; x++ ) {
			int numSamples = 0;
			int totalRGB[3] = { 0, 0, 0 };

			for ( int filterY = y - Kuwahara_Half_Size; filterY <= y + Kuwahara_Half_Size; filterY+=(Kuwahara_Half_Size *2)) {
				if ( filterY < 0 || filterY >= imageHeight ) {
					continue;
				}

				for ( int filterX = x - Kuwahara_Half_Size; filterX <= x + Kuwahara_Half_Size; filterX+=(Kuwahara_Half_Size*2)) {
					if ( filterX < 0 || filterX >= imageWidth ) {
						continue;
					}

					numSamples++;

					int index = ( ( filterY * imageWidth ) + x) * 3;
					totalRGB[0] += imageData[index+0];
					totalRGB[1] += imageData[index+1];
					totalRGB[2] += imageData[index+2];
				}
			}

			int currentIndex = (y * imageWidth) + x;
			meanAndVariance[currentIndex].mean[0] = (float) totalRGB[0] / (float)numSamples;
			meanAndVariance[currentIndex].mean[1] = (float) totalRGB[1] / (float)numSamples;
			meanAndVariance[currentIndex].mean[2] = (float) totalRGB[2] / (float)numSamples;

			float variance[3] = { 0.0f, 0.0f, 0.0f };

			for ( int filterY = y - Kuwahara_Half_Size; filterY <= y + Kuwahara_Half_Size; filterY++ ) {
				if ( filterY < 0 || filterY >= imageHeight ) {
					continue;
				}

				for ( int filterX = x - Kuwahara_Half_Size; filterX <= x + Kuwahara_Half_Size; filterX++ ) {
					if ( filterX < 0 || filterX >= imageWidth ) {
						continue;
					}
					
					
					int index = ( ( filterY * imageWidth ) + filterX ) * 3;
					for ( int i = 0; i < 3; i++ ) {
						float value = (float)imageData[index+i];
						variance[i] += (value - meanAndVariance[currentIndex].mean[i])*( value - meanAndVariance[currentIndex].mean[i]);
					}
				}
			}
			
			meanAndVariance[currentIndex].variance = (variance[0]*variance[0]+variance[1]*variance[1]+variance[2]*variance[2]);

			if ( meanAndVariance[currentIndex].variance > 0.0f ) {
				static int stopHere;
				stopHere++;
			}
		}
	}

	for ( int y = 0; y < imageHeight; y++ ) {
		for ( int x = 0; x < imageWidth; x++ ) {
			float rgb[3] = { 0.0f, 0.0f, 0.0f };
			float lowestVariance = FLT_MAX;

			for ( int filterY = y - Kuwahara_Half_Size; filterY <= y + Kuwahara_Half_Size; filterY++ ) {
				if ( filterY < 0 || filterY >= imageHeight ) {
					continue;
				}

				for ( int filterX = x - Kuwahara_Half_Size; filterX <= x + Kuwahara_Half_Size; filterX++ ) {
					if ( filterX < 0 || filterX >= imageWidth ) {
						continue;
					}

					float currentVariance = meanAndVariance[(filterY * imageWidth) + filterX].variance;
					if ( currentVariance < lowestVariance ) {
						lowestVariance = currentVariance;
						rgb[0] = meanAndVariance[(filterY * imageWidth) + filterX].mean[0];
						rgb[1] = meanAndVariance[(filterY * imageWidth) + filterX].mean[1];
						rgb[2] = meanAndVariance[(filterY * imageWidth) + filterX].mean[2];
					}
				}
			}

			int finalImageIndex = ( ( y * imageWidth ) + x ) * 3;
			if ( rgb[0] > 255.0f ) {
				rgb[0] = 255.0f;
			}
			
			if ( rgb[1] > 255.0f ) {
				rgb[1] = 255.0f;
			}

			if ( rgb[2] > 255.0f ) {
				rgb[2] = 255.0f;
			}

			imageData[finalImageIndex + 0] = rgb[0];
			imageData[finalImageIndex + 1] = rgb[1];
			imageData[finalImageIndex + 2] = rgb[2];
		}
	}

	// copy image back to padded format
	localIndex = 0;
	curPixel = 0;
	for ( int pix = 0; pix < dataSize ; pix += 3, curPixel++, localIndex += 3 ) {

		if ( curPixel == imageWidth ) {
			curPixel = 0;

			pix += padSize;
		} else {
			fileImageData[pix+0] = imageData[localIndex + 0];
			fileImageData[pix+1] = imageData[localIndex + 1];
			fileImageData[pix+2] = imageData[localIndex + 2];
		}
	}

	// write it out
	char outputFileName[256];
	sprintf( outputFileName, "%s", fileName );
	char * end = strstr(outputFileName, ".bmp" );
	*end = '\0';
	sprintf( outputFileName, "%s_processed.bmp", outputFileName );
	bitmapFile = fopen( outputFileName, "wb" );
	fwrite( &bitmapHeader, sizeof( BITMAPFILEHEADER ), 1, bitmapFile );
	fwrite( &bitmapInfo, sizeof( BITMAPINFOHEADER ), 1, bitmapFile );
	fwrite( fileImageData, sizeof( BYTE ), dataSize, bitmapFile ),
	fclose( bitmapFile );

	delete [] meanAndVariance;
	delete [] fileImageData;
	delete [] imageData;
}
 
void ProcessMain() {
	ProcessImage( "../assets/SheepAndFoxTitle.bmp" );
	//sprintf( fileName, "%s/swing_%d.bmp", fileName, i );
	/*for ( int i = 1; i < 101; i++ ) {
		char fileName[256] = "../assets/swing";
		sprintf( fileName, "%s/swing_%d.bmp", fileName, i );
		ProcessImage( fileName );
	}*/

	//ProcessImage( "../assets/pose_test.bmp" );
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_KBTOONSPRITEMAKER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KBTOONSPRITEMAKER));

	ProcessMain();


	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KBTOONSPRITEMAKER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_KBTOONSPRITEMAKER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
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
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
