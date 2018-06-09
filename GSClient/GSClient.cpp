// GSClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "GSClient.h"
#include "NetworkManager.h"
#include "ObjectManager.h"
#include "Globals.h"
#include "resource.h"

#define MAX_LOADSTRING 100

using namespace std;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
LPCTSTR winClassName = TEXT("GSClient");
LPCTSTR winTitle = TEXT("GSClient");
DWORD serverIp;
sockaddr_in servAddr;
constexpr DWORD WIN_STYLE{ WS_OVERLAPPEDWINDOW };

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
		// 응용 프로그램 초기화를 수행합니다.
		if (!InitInstance(hInstance, nCmdShow))
		{
			return FALSE;
		}

		// 기본 메시지 루프입니다.
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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

	global.mainWin = hWnd;

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
		srand(time(NULL));

		RECT clientRect;
		SetRect(&clientRect, 0, 0, VIEW_SIZE*CELL_W, VIEW_SIZE*CELL_H);
		AdjustWindowRect(&clientRect, WIN_STYLE, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOMOVE);
		SetTimer(hWnd, 1, 100, nullptr);
		SetTimer(hWnd, 3, 10, nullptr);
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

		{
			auto locked = global.objManager.GetUniqueCollection();
			auto& objMap = locked.data;
			// UI 그리기
			TCHAR text[100];
			wsprintf(text, TEXT("Connected User Num : %d"), objMap.size());
			TextOut(memDC, 0, 0, text, lstrlen(text));
			// 플레이어 상황 그리기
			for (auto& player : objMap) {
				DrawPlayer(memDC, player.second->x, player.second->y);
			}
		}

		BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
		DeleteObject(memBit);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_TIMER:
	{
		switch (wParam) {
		case 1:
		{
			auto locked = global.objManager.GetUniqueCollection();
			auto& objMap = locked.data;
			InitDummyClients(1);
			if (objMap.size() >= MAX_DUMMY_NUM) {
				KillTimer(hWnd, 1);
			}
			if (!global.clientsInitialized) {
				SetTimer(hWnd, 2, 300, nullptr);
			}
		}
		break;
		case 2:
		{
			auto locked = global.objManager.GetUniqueCollection();
			auto& objMap = locked.data;

			// PUT_OBJ 메세지를 모두 무시하고 GET_ID를 받는 객체들만 넣어놨기 때문에 모두 Player이다.
			for (auto& obj : objMap) {
				auto& client = *reinterpret_cast<Client*>(obj.second.get());
				short dx = rand() % 3 - 1;
				short dy = rand() % 3 - 1;
				global.networkManager.SendNetworkMessage(client.s, *new MsgInputMove{ dx, dy });
			}
		}
		break;
		case 3:
		{
			InvalidateRect(hWnd, nullptr, FALSE);
		}
		break;
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
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
	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_INIT), NULL, InitDlgFunc) == 0) { WSACleanup(); return false; }

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

	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	inet_pton(AF_INET, ipStr.c_str(), &servAddr.sin_addr);
	servAddr.sin_port = htons(GS_PORT);

	global.iocpObject = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	auto thread_num = std::thread::hardware_concurrency();
	if (thread_num == 0) thread_num = 1;
	for (auto i = 0; i < thread_num; ++i) {
		std::thread t{ WorkerThreadFunc };
		t.detach();
	}

	return true;
}

void InitDummyClients(int client_num)
{
	for (auto i = 0; i < client_num; ++i) {
		auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock) err_quit_wsa(TEXT("socket"));
		BOOL isNoDelay = TRUE;
		if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay)) == SOCKET_ERROR) err_quit_wsa(TEXT("setsockopt"));

		if (connect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) err_quit_wsa(TEXT("connect"));

		auto client = new Client{ sock };
		CreateIoCompletionPort((HANDLE)sock, global.iocpObject, global.clientId++, 0);

		global.networkManager.RecvNetworkMessage(*client);
	}
}

void DrawPlayer(HDC memDC, const int x, const int y)
{
	const int worldX = x*1.5;
	const int worldY = y*1.5;
	const auto clientSize = 5;
	const auto left = worldX - clientSize;
	const auto top = worldY - clientSize;
	const auto right = worldX + clientSize;
	const auto bottom = worldY + clientSize;
	Ellipse(memDC, left, top, right, bottom);
}

void ErrorQuit()
{
	MessageBox(NULL, TEXT("연결이 끊겼습니다."), TEXT("ERROR"), MB_OK);
	WSACleanup();
	exit(1);
}

void WorkerThreadFunc() {
	DWORD bytes;
	ULONG_PTR key;
	ExtOverlapped* ov;
	DWORD error{ 0 };
	while (true) {
		auto isSuccess = GetQueuedCompletionStatus(global.iocpObject, &bytes, &key, (LPOVERLAPPED*)&ov, INFINITE);
		if (FALSE == isSuccess) error = GetLastError();
		if (ov->isRecv) { RecvCompletionCallback(error, bytes, ov); }
		else { SendCompletionCallback(error, bytes, ov); }
	}
}

void RemoveClient(Client & c)
{
	auto locked = global.objManager.GetUniqueCollection();
	auto& objMap = locked.data;

	auto id = c.id;
	auto localPtr = std::move(objMap[id]);
	objMap.erase(c.id);
}

void ClientMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	switch (msg.type) {
	case MsgType::SC_PUT_OBJ:
	{
		auto& rMsg = *(MsgPutObject*)&msg;
		if (rMsg.id == client->id) {
			client->x = rMsg.x;
			client->y = rMsg.y;
		}
	}
	break;
	case MsgType::SC_REMOVE_OBJ:
	{
	}
	break;
	case MsgType::SC_MOVE_OBJ:
	{
		auto& rMsg = *(MsgMoveObject*)&msg;
		if (rMsg.id == client->id) {
			client->x = rMsg.x;
			client->y = rMsg.y;
		}
	}
	break;
	case MsgType::SC_GIVE_ID:
	{
		auto& rMsg = *(MsgGiveID*)&msg;

		auto locked = global.objManager.GetUniqueCollection();
		auto& objMap = locked.data;
		this->client->id = rMsg.id;
		objMap.emplace(rMsg.id, std::unique_ptr<Object>{this->client});
	}
	break;
	}
}
