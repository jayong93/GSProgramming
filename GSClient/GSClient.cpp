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

		bool isDone{ false };
		auto startTime{ std::chrono::high_resolution_clock::now() };
		while (!isDone)
		{
			// 기본 메시지 루프입니다.
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) {
					isDone = true;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			auto lastTime{ std::chrono::high_resolution_clock::now() };
			auto du = std::chrono::duration_cast<std::chrono::milliseconds>(lastTime - startTime).count();
			if (du >= 1000 / 60) {
				startTime = lastTime;
				InvalidateRect(global.mainWin, nullptr, FALSE);
			}
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

		if (global.myInstance != nullptr) {
			TCHAR coordStr[21];
			{
				auto locked = global.objManager.GetUniqueCollection();
				int leftTopCoordX, leftTopCoordY;
				auto& myData = *global.myInstance;
				const auto halfViewSize = VIEW_SIZE / 2;
				const auto framedX = min(BOARD_W - halfViewSize - 1, max(myData.x, halfViewSize));
				const auto framedY = min(BOARD_H - halfViewSize - 1, max(myData.y, halfViewSize));
				leftTopCoordX = max(0, framedX - VIEW_SIZE / 2);
				leftTopCoordY = max(0, framedY - VIEW_SIZE / 2);

				// 배경 그리기
				for (int i = 0; i < VIEW_SIZE; ++i) {
					const auto coordY = leftTopCoordY + i;
					for (int j = 0; j < VIEW_SIZE; ++j) {
						const auto coordX = leftTopCoordX + j;
						const auto isColored = (coordY % 8 == 0) && (coordX % 8 == 0);
						HGDIOBJ old = 0;
						if (isColored) old = SelectObject(memDC, GetStockObject(BLACK_BRUSH));
						Rectangle(memDC, j*CELL_W, i*CELL_H, (j + 1)*CELL_W, (i + 1)*CELL_H);
						if (isColored) SelectObject(memDC, old);
					}
				}

				auto oldPen = SelectObject(memDC, GetStockObject(NULL_PEN));
				// 플레이어 그리기
				auto currTime = std::chrono::high_resolution_clock::now();
				for (auto& p : *locked) {
					auto& obj = *p.second.get();
					if (p.first == global.myInstance->id) continue;
					DrawPlayer(memDC, leftTopCoordX, leftTopCoordY, obj);

					if (obj.chat.isValid) {
						if (chrono::duration_cast<chrono::milliseconds>(currTime - obj.chat.lastChatTime).count() < 1000) {
							RECT textRect;
							textRect.left = (obj.x - leftTopCoordX)*CELL_W;
							textRect.bottom = (obj.y - leftTopCoordY)*CELL_H;
							textRect.right = textRect.left + CELL_W;
							textRect.top = textRect.bottom - CELL_H / 2;
							DrawText(memDC, obj.chat.chatMsg, lstrlen(obj.chat.chatMsg), &textRect, DT_BOTTOM | DT_CENTER);
						}
						else {
							obj.chat.isValid = false;
						}
					}
				}
				DrawPlayer(memDC, leftTopCoordX, leftTopCoordY, myData);
				SelectObject(memDC, oldPen);

				// 좌표 표시
				wsprintf(coordStr, TEXT("(%d, %d)"), (int)myData.x, (int)myData.y);
			}
			const auto coordStrLen = lstrlen(coordStr);
			RECT coordStrRect{ 0,0,0,0 };
			DrawText(memDC, coordStr, coordStrLen, &coordStrRect, DT_CALCRECT);
			OffsetRect(&coordStrRect, winSize.right - coordStrRect.right, winSize.bottom - coordStrRect.bottom);
			DrawText(memDC, coordStr, coordStrLen, &coordStrRect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
		}

		BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
		DeleteObject(memBit);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if (global.myInstance != nullptr) {
				SendMovePacket(global.myInstance->s, wParam);
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

void SendMovePacket(SOCKET sock, UINT_PTR key) {
	key -= VK_LEFT;
	int isAxisY = key % 2;
	int delta = ((key / 2) * 2 - 1);
	char dx = (!isAxisY) * delta;
	char dy = isAxisY * delta;

	global.networkManager.SendNetworkMessage(sock, *new MsgInputMove{ dx, dy });
}

INT_PTR InitDlgFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, IDC_IPADDRESS1, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
		SendDlgItemMessage(hWnd, IDC_EDIT_ID, EM_LIMITTEXT, MAX_GAME_ID_LEN, 0);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			SendDlgItemMessage(hWnd, IDC_IPADDRESS1, IPM_GETADDRESS, 0, (LPARAM)&serverIp);
			GetDlgItemText(hWnd, IDC_EDIT_ID, global.gameID, sizeof(global.gameID) / sizeof(TCHAR));
			if (lstrlen(global.gameID) <= 0) {
				MessageBox(hWnd, TEXT("ID를 입력해야 합니다."), TEXT("에러"), MB_OK);
				return TRUE;
			}
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

	auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sock) err_quit_wsa(TEXT("socket"));
	BOOL isNoDelay = TRUE;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay)) == SOCKET_ERROR) err_quit_wsa(TEXT("setsockopt"));

	if (connect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) err_quit_wsa(TEXT("connect"));

	send(sock, (const char*)global.gameID, sizeof(global.gameID), 0);

	global.myInstance = new Client{ sock };
	CreateIoCompletionPort((HANDLE)sock, global.iocpObject, 0, 0);
	global.networkManager.RecvNetworkMessage(*global.myInstance);
	return true;
}

void DrawPlayer(HDC memDC, const int leftTopCoordX, const int leftTopCoordY, const Object & p)
{
	const auto relativeX = p.x - leftTopCoordX;
	const auto relativeY = p.y - leftTopCoordY;
	HBRUSH brush = CreateSolidBrush(p.color);
	auto old = SelectObject(memDC, brush);
	Ellipse(memDC, relativeX*CELL_W, relativeY*CELL_H, (relativeX + 1)*CELL_W, (relativeY + 1)*CELL_H);
	SelectObject(memDC, old);
	DeleteObject(brush);
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
	while (true) {
		DWORD error{ 0 };
		auto isSuccess = GetQueuedCompletionStatus(global.iocpObject, &bytes, &key, (LPOVERLAPPED*)&ov, INFINITE);
		if (FALSE == isSuccess) error = GetLastError();
		if (ov->isRecv) { RecvCompletionCallback(error, bytes, ov); }
		else { SendCompletionCallback(error, bytes, ov); }
	}
}

void RemoveClient(Client & c)
{
	auto locked = global.objManager.GetUniqueCollection();

	auto id = c.id;
	auto localPtr = std::move((*locked)[id]);
	locked->erase(c.id);
}

void ClientMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	switch (msg.type) {
	case MsgType::SC_PUT_OBJ:
	{
		auto& rMsg = *(MsgPutObject*)&msg;
		auto locked = global.objManager.GetUniqueCollection();
		if (rMsg.id == global.myInstance->id) {
			global.myInstance->color = RGB(rMsg.color.r, rMsg.color.g, rMsg.color.b);
			global.myInstance->x = rMsg.x;
			global.myInstance->y = rMsg.y;
		}
		else {
			locked->emplace(rMsg.id, std::make_unique<Object>(rMsg.x, rMsg.y, rMsg.color));
		}
	}
	break;
	case MsgType::SC_REMOVE_OBJ:
	{
		auto& rMsg = *(MsgRemoveObject*)&msg;
		auto locked = global.objManager.GetUniqueCollection();
		locked->erase(rMsg.id);
	}
	break;
	case MsgType::SC_MOVE_OBJ:
	{
		auto& rMsg = *(MsgMoveObject*)&msg;
		auto locked = global.objManager.GetUniqueCollection();
		auto it = locked->find(rMsg.id);
		if (it != locked->end()) {
			it->second->x = rMsg.x;
			it->second->y = rMsg.y;
		}
	}
	break;
	case MsgType::SC_GIVE_ID:
	{
		auto& rMsg = *(MsgGiveID*)&msg;

		auto locked = global.objManager.GetUniqueCollection();
		this->client->id = rMsg.id;
		locked->emplace(rMsg.id, std::unique_ptr<Object>{this->client});
	}
	break;
	case MsgType::SC_CHAT:
	{
		auto& rMsg = *(MsgChat*)&msg;
		auto locked = global.objManager.GetUniqueCollection();
		auto it = locked->find(rMsg.from);
		if (it == locked->end()) break;
		lstrcpyn(it->second->chat.chatMsg, rMsg.msg, MAX_CHAT_LEN);
		it->second->chat.lastChatTime = std::chrono::high_resolution_clock::now();
		it->second->chat.isValid = true;
	}
	break;
	}
}
