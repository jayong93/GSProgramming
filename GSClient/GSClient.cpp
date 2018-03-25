// GSClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include <stdafx.h>
#include "GSClient.h"
#include "resource.h"

#define MAX_LOADSTRING 100

using namespace std;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
LPCTSTR winClassName = TEXT("GSClient");
LPCTSTR winTitle = TEXT("GSClient");
DWORD serverIp;
SOCKET sock;
WSAEVENT netEvent;
constexpr DWORD WIN_STYLE{ WS_OVERLAPPEDWINDOW };
map<unsigned int, Player> playerList;
deque<tuple<unique_ptr<MsgBase>, int>> msgList;
MsgReconstructor msgRecon{ 100, ProcessMessage };
HWND mainWin;
bool canSend{ false };

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

	MSG msg;
	if (InitNetwork()) {
		bool done{ false };

		// 응용 프로그램 초기화를 수행합니다.
		if (!InitInstance(hInstance, nCmdShow))
		{
			return FALSE;
		}

		// 기본 메시지 루프입니다.
		while (!done)
		{
			const int retval = MsgWaitForMultipleObjectsEx(1, &netEvent, INFINITE, QS_ALLINPUT, 0);
			switch (retval) {
			case WAIT_OBJECT_0 + 1:
				GetMessage(&msg, nullptr, 0, 0);
				if (msg.message == WM_QUIT) {
					done = true;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			case WAIT_IO_COMPLETION:
				break;
			case WAIT_OBJECT_0:
				NetworkHandler();
				break;
			default:
				err_quit_wsa(TEXT("MsgWait"));
				break;
			}
		}

		if (sock) {
			shutdown(sock, SD_BOTH);
			closesocket(sock);
		}
		WSACleanup();
		return (int)msg.wParam;
	}
	return 0;
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
	switch (message)
	{
	case WM_CREATE:
		{
			mainWin = hWnd;

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
			for (auto& p : playerList) {
				HBRUSH brush = CreateSolidBrush(p.second.color);
				auto old = SelectObject(memDC, brush);
				Ellipse(memDC, p.second.x*CELL_W, p.second.y*CELL_H, (p.second.x + 1)*CELL_W, (p.second.y + 1)*CELL_H);
				SelectObject(memDC, old);
				DeleteObject(brush);
			}
			SelectObject(memDC, oldPen);

			BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
			DeleteObject(memBit);
			DeleteDC(memDC);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
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

bool InitNetwork()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { return false; }
	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_INIT), NULL, InitDlgFunc) == 0) { WSACleanup(); return 0; }

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
	if (INVALID_SOCKET == sock) err_quit_wsa(TEXT("socket"));
	BOOL isNoDelay = TRUE;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay)) == SOCKET_ERROR) err_quit_wsa(TEXT("setsockopt"));

	sockaddr_in servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ipStr.c_str(), &servAddr.sin_addr);
	servAddr.sin_port = htons(GS_PORT);

	if (connect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) err_quit_wsa(TEXT("connect"));
	netEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(sock, netEvent, FD_WRITE | FD_READ | FD_CLOSE)) err_quit_wsa(TEXT("WSAEventSelect"));

	return true;
}

void NetworkHandler()
{
	WSANETWORKEVENTS ne;
	WSAEnumNetworkEvents(sock, netEvent, &ne);
	if (FD_WRITE & ne.lNetworkEvents) {
		if (0 != ne.iErrorCode[FD_WRITE_BIT]) ErrorQuit();
		canSend = true;
		while (!msgList.empty()) {
			auto& msg = msgList.front();
			auto buf = std::get<0>(msg).get();
			auto& sended = std::get<1>(msg);
			int remain = buf->len - sended;

			int retval;
			if (remain > (retval = SendAll(sock, ((char*)buf) + sended, remain))) {
				sended += retval;
				break;
			}

			msgList.pop_front();
		}
	}
	if (FD_READ & ne.lNetworkEvents) {
		if (0 != ne.iErrorCode[FD_READ_BIT]) ErrorQuit();
		msgRecon.Recv(sock);
	}
	if (FD_CLOSE & ne.lNetworkEvents) {
		ErrorQuit();
	}
	if (SOCKET_ERROR == WSAEventSelect(sock, netEvent, FD_WRITE | FD_READ | FD_CLOSE)) err_quit_wsa(TEXT("WSAEventSelect"));
}

void SendMovePacket(SOCKET sock, UINT_PTR key) {
	key -= VK_LEFT;
	int isAxisY = key % 2;
	int delta = ((key / 2) * 2 - 1);
	char dx = (!isAxisY) * delta;
	char dy = isAxisY * delta;

	if (!canSend)
		msgList.emplace_back(new MsgInputMove{ dx, dy }, 0);
	else {
		MsgInputMove msg{ dx, dy };
		int sended;
		if (msg.len > (sended = SendAll(sock, (char*)&msg, msg.len))) {
			msgList.emplace_back(new MsgInputMove{ msg }, sended);
		}
	}
}

int SendAll(SOCKET sock, const char * buf, size_t len)
{
	int retval{ 0 };
	int sendedLen{ 0 };
	while (true) {
		if (SOCKET_ERROR == (retval = send(sock, buf + sendedLen, len - sendedLen, 0)))
		{
			int error = WSAGetLastError();
			if (WSAEWOULDBLOCK == error) { canSend = false; return sendedLen; }
			err_quit_wsa(error, TEXT("send"));
		}
		sendedLen += retval;
		if (sendedLen >= len) return 0;
	}
	return 0;
}

void ProcessMessage(SOCKET s, const MsgBase & msg, void*)
{
	switch (msg.type) {
	case MsgType::CLIENT_IN:
		{
			auto& rMsg = *(MsgClientIn*)(&msg);
			playerList.emplace(std::make_pair(rMsg.id, Player{ rMsg.x, rMsg.y, rMsg.color }));
			InvalidateRect(mainWin, nullptr, FALSE);
		}
		break;
	case MsgType::CLIENT_OUT:
		{
			auto& rMsg = *(MsgClientOut*)(&msg);
			playerList.erase(rMsg.id);
			InvalidateRect(mainWin, nullptr, FALSE);
		}
		break;
	case MsgType::MOVE_CHARA:
		{
			auto& rMsg = *(MsgMoveCharacter*)(&msg);
			auto& player = playerList[rMsg.id];
			player.x = rMsg.x; player.y = rMsg.y;
			InvalidateRect(mainWin, nullptr, FALSE);
		}
		break;
	}
}

void ErrorQuit()
{
	MessageBox(NULL, TEXT("연결이 끊겼습니다."), TEXT("ERROR"), MB_OK);
	WSACleanup();
	exit(1);
}
