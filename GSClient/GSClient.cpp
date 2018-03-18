// GSClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <atomic>
#include <thread>
#include <cstdlib>
#include "GSClient.h"
#include "resource.h"

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
LPCTSTR winClassName = TEXT("GSClient");
LPCTSTR winTitle = TEXT("GSClient");
DWORD serverIp;
int cx, cy;

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
	static HBRUSH redBrush;
	static SOCKET sock;
	switch (message)
	{
	case WM_CREATE:
		{
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) DestroyWindow(hWnd);
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_INIT), hWnd, InitDlgFunc) == 0) DestroyWindow(hWnd);

			std::string ipStr;
			char ipNum[4];
			itoa(FIRST_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			itoa(SECOND_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			itoa(THIRD_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;
			itoa(FOURTH_IPADDRESS(serverIp), ipNum, 10);
			ipStr += ipNum;

			sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
			BOOL isNoDelay = TRUE;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay));

			sockaddr_in servAddr;
			ZeroMemory(&servAddr, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			servAddr.sin_addr.s_addr = inet_addr(ipStr.c_str());
			servAddr.sin_port = htons(GS_PORT);

			if (WSAConnect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR) DestroyWindow(hWnd);

			std::thread recvThread{ RecvThreadFunc, hWnd, sock };

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
		if (sock) closesocket(sock);
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
				SendMovePacket(sock, wParam);
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

void SendMovePacket(SOCKET sock, UINT_PTR key) {
	char packet[sizeof(short) + sizeof(ClientMsg)];
	*(short*)packet = sizeof(ClientMsg);
	ClientMsg& msg = *(ClientMsg*)(packet + sizeof(short));
	switch (key) {
	case VK_LEFT:
		msg = ClientMsg::KEY_LEFT;
		break;
	case VK_UP:
		msg = ClientMsg::KEY_UP;
		break;
	case VK_RIGHT:
		msg = ClientMsg::KEY_RIGHT;
		break;
	case VK_DOWN:
		msg = ClientMsg::KEY_DOWN;
		break;
	}

	send(sock, packet, sizeof(packet), 0);
}

void RecvThreadFunc(HWND hWnd, SOCKET sock) {
	// TODO: MOVE 메세지에서 좌표 받아서 설정. 동적 배열 할당 받아서 남은 메세지 전부 RecvAll 한번으로 받음.
	while (true) {
		short len;
		RecvAll(sock, (char*)&len, sizeof(len), 0);
		ServerMsg msg;
		RecvAll(sock, (char*)&msg, sizeof(msg), 0);

		switch (msg) {
		case ServerMsg::MOVE_CHARA:
			InvalidateRect(hWnd, nullptr, FALSE);
			break;
		}
	}
}
