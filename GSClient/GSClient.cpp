// GSClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "Windows.h"
#include "GSClient.h"

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
LPCTSTR winClassName = TEXT("GSClient");
LPCTSTR winTitle = TEXT("GSClient");

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	MSG msg;

	// 기본 메시지 루프입니다.
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  목적: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = winClassName;
	wcex.hIconSm = NULL;

	return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   목적: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   설명:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(winClassName, winTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  목적:  주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 응용 프로그램 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int cx, cy;
	static HBRUSH redBrush;
	switch (message)
	{
	case WM_CREATE:
		{
			cx = 0; cy = 0;
			redBrush = CreateSolidBrush(RGB(255, 0, 0));
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT winSize;
			GetClientRect(hWnd, &winSize);
			HDC memDC = CreateCompatibleDC(hdc);
			HBITMAP memBit = CreateCompatibleBitmap(hdc, winSize.right - winSize.left, winSize.bottom - winSize.top);
			SelectObject(memDC, memBit);
			FillRect(memDC, &winSize, WHITE_BRUSH);
			for (int i = 0; i < boardH; ++i) {
				for (int j = 0; j < boardW; ++j) {
					const int color = (i % 2 + j % 2) % 2;
					HGDIOBJ old = 0;
					if (color == 0) old = SelectObject(memDC, GetStockObject(BLACK_BRUSH));
					Rectangle(memDC, j*cellW, i*cellH, (j + 1)*cellW, (i + 1)*cellH);
					if (color == 0) SelectObject(memDC, old);
				}
			}
			auto old = SelectObject(memDC, redBrush);
			Ellipse(memDC, cx*cellW, cy*cellH, (cx + 1)*cellW, (cy + 1)*cellH);
			SelectObject(memDC, old);

			BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
			DeleteObject(memBit);
			DeleteDC(memDC);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		DeleteObject(redBrush);
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		{
			switch (wParam) {
			case VK_UP:
				if (cy > 0) --cy;
				break;
			case VK_DOWN:
				if (cy < boardH - 1) ++cy;
				break;
			case VK_LEFT:
				if (cx > 0) --cx;
				break;
			case VK_RIGHT:
				if (cx < boardW - 1) ++cx;
				break;
			}
			InvalidateRect(hWnd, nullptr, false);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
