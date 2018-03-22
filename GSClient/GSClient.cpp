// GSClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#pragma comment(lib, "ws2_32")

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <deque>
#include "GSClient.h"
#include "resource.h"

#define MAX_LOADSTRING 100

using namespace std;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
LPCTSTR winClassName = TEXT("GSClient");
LPCTSTR winTitle = TEXT("GSClient");
DWORD serverIp;
constexpr DWORD WIN_STYLE{ WS_OVERLAPPEDWINDOW };
int cx, cy;
deque<unique_ptr<MsgBase>> msgQueue;

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    InitDlgFunc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

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

	HWND hWnd = CreateWindowW(winClassName, winTitle, WIN_STYLE,
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
	static HBRUSH redBrush;
	static SOCKET sock;
	switch (message)
	{
	case WM_CREATE:
		{
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { DestroyWindow(hWnd); return 0; }
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_INIT), hWnd, InitDlgFunc) == 0) { DestroyWindow(hWnd); return 0; }

			std::string ipStr;
			char ipNum[4];
			_itoa_s(FIRST_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			ipStr += '.';
			_itoa_s(SECOND_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			ipStr += '.';
			_itoa_s(THIRD_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			ipStr += '.';
			_itoa_s(FOURTH_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;

			sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			BOOL isNoDelay = TRUE;
			if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay)) == SOCKET_ERROR) err_quit(TEXT("setsockopt"));

			sockaddr_in servAddr;
			ZeroMemory(&servAddr, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			inet_pton(AF_INET, ipStr.c_str(), &servAddr.sin_addr);
			servAddr.sin_port = htons(GS_PORT);

			if (connect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) err_quit(TEXT("WSAConnect"));

			cx = 0; cy = 0;
			redBrush = CreateSolidBrush(RGB(255, 0, 0));

			RECT clientRect;
			SetRect(&clientRect, 0, 0, BOARD_W*CELL_W, BOARD_H*CELL_H);
			AdjustWindowRect(&clientRect, WIN_STYLE, FALSE);
			SetWindowPos(hWnd, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOMOVE);
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
			auto oldPen = SelectObject(memDC, GetStockObject(NULL_PEN));
			FillRect(memDC, &winSize, WHITE_BRUSH);
			for (int i = 0; i < BOARD_H; ++i) {
				for (int j = 0; j < BOARD_W; ++j) {
					const int color = (i % 2 + j % 2) % 2;
					HGDIOBJ old = 0;
					if (color == 0) old = SelectObject(memDC, GetStockObject(BLACK_BRUSH));
					Rectangle(memDC, j*CELL_W, i*CELL_H, (j + 1)*CELL_W, (i + 1)*CELL_H);
					if (color == 0) SelectObject(memDC, old);
				}
			}
			auto old = SelectObject(memDC, redBrush);
			Ellipse(memDC, cx*CELL_W, cy*CELL_H, (cx + 1)*CELL_W, (cy + 1)*CELL_H);
			SelectObject(memDC, old);
			SelectObject(memDC, oldPen);

			BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
			DeleteObject(memBit);
			DeleteDC(memDC);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		DeleteObject(redBrush);
		if (sock) {
			shutdown(sock, SD_BOTH);
			closesocket(sock);
		}
		WSACleanup();
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		{
			switch (wParam) {
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
				SendMovePacket(sock, hWnd, wParam);
				break;
			}
		}
		break;
	case UM_NETALLOW:
		{
			if (WSAGETSELECTERROR(lParam) != 0) {
				MessageBox(hWnd, TEXT("The network subsystem failed."), TEXT("ERROR"), MB_OK);
				DestroyWindow(hWnd);
				return 0;
			}

			auto s = static_cast<SOCKET>(wParam);
			switch (WSAGETSELECTEVENT(lParam)) {
			case FD_WRITE:
				for (auto& msg : msgQueue) {
					send(s, (char*)msg.get(), msg->len, 0);
				}
				msgQueue.clear();
				break;
			case FD_READ:

				break;
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR InitDlgFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, IDC_IPADDRESS1, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			SendDlgItemMessage(hWnd, IDC_IPADDRESS1, IPM_GETADDRESS, 0, (LPARAM)&serverIp);
			EndDialog(hWnd, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void SendMovePacket(SOCKET sock, HWND hWnd, UINT_PTR key) {
	key -= VK_LEFT;
	int isAxisY = key % 2;
	int delta = ((key / 2) * 2 - 1);
	byte dx = (!isAxisY) * delta;
	byte dy = isAxisY * delta;

	msgQueue.emplace_back(make_unique<MsgInputMove>(dx, dy));
	WSAAsyncSelect(sock, hWnd, UM_NETALLOW, FD_WRITE);
}

void ReconstructMsg(const MsgBase & msg)
{
}
