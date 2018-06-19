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
		SetBkMode(memDC, TRANSPARENT);

		int leftTopCoordX, leftTopCoordY;
		auto& myData = *global.myInstance;
		short myX, myY;
		const bool amIExist = global.objManager.Update(myData.GetID(), [&myX, &myY](auto& obj) {
			std::tie(myX, myY) = obj.GetPos();
		});

		if (amIExist)
		{
			const auto halfViewSize = VIEW_SIZE / 2;
			const auto framedX = min(BOARD_W - halfViewSize - 1, max(myX, halfViewSize));
			const auto framedY = min(BOARD_H - halfViewSize - 1, max(myY, halfViewSize));
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
			global.objManager.Access([&](ObjectMap& map) {
				for (auto& p : map) {
					auto& obj = *p.second.get();
					if (p.first == global.myInstance->GetID()) continue;
					DrawObject(memDC, leftTopCoordX, leftTopCoordY, obj);
				}
				for (auto& p : map) {
					auto& obj = *p.second.get();
					if (p.first == global.myInstance->GetID()) continue;
					DrawHP(memDC, leftTopCoordX, leftTopCoordY, obj);
				}
			});
			global.objManager.Update(myData.GetID(), [&](auto& obj) {
				DrawObject(memDC, leftTopCoordX, leftTopCoordY, obj);
				DrawHP(memDC, leftTopCoordX, leftTopCoordY, obj);
			});
			SelectObject(memDC, oldPen);

			// 좌표 표시
			TCHAR coordStr[21];
			wsprintf(coordStr, TEXT("(%d, %d)"), (int)myX, (int)myY);
			const auto coordStrLen = lstrlen(coordStr);
			RECT coordStrRect{ 0,0,0,0 };
			DrawText(memDC, coordStr, coordStrLen, &coordStrRect, DT_CALCRECT);
			OffsetRect(&coordStrRect, winSize.right - coordStrRect.right, winSize.bottom - coordStrRect.bottom);
			DrawText(memDC, coordStr, coordStrLen, &coordStrRect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
		}

		int height{ 1 };
		const auto right = (winSize.left + winSize.right) / 2;
		if (global.myInstance->isChatting) {
			RECT chatInsertBox = winSize;
			height = DrawText(memDC, global.chatMsg, global.chatLen, &chatInsertBox, DT_CALCRECT);
			if (height == 1) height = 16;
			UINT flag = DT_BOTTOM;
			if (right <= chatInsertBox.right) flag |= DT_RIGHT;
			else flag |= DT_LEFT;
			chatInsertBox.right = right;
			chatInsertBox.bottom = winSize.bottom;
			chatInsertBox.top = chatInsertBox.bottom - height;

			Rectangle(memDC, chatInsertBox.left, chatInsertBox.top, chatInsertBox.right, chatInsertBox.bottom);
			DrawText(memDC, global.chatMsg, global.chatLen, &chatInsertBox, flag);
		}

		if (global.myInstance->isChatting)
		{
			DrawChatLog(memDC, right, height, winSize);
		}
		else if (auto lastTime = global.lastChatWindowClose.GetClone(); lastTime) {
			using namespace std::chrono;
			auto du = duration_cast<milliseconds>(high_resolution_clock::now() - *lastTime).count();
			if (du < 3000)
				DrawChatLog(memDC, right, height, winSize);
		}
		BitBlt(hdc, 0, 0, winSize.right, winSize.bottom, memDC, 0, 0, SRCCOPY);
		DeleteObject(memBit);
		DeleteDC(memDC);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KEYDOWN:
	{
		if (global.myInstance->IsConnected()) {
			switch (wParam) {
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
				SendMovePacket(global.myInstance->GetSocket(), wParam);
				break;
			case 'A':
				if (!global.myInstance->isChatting)
					global.networkManager.SendNetworkMessage(global.myInstance->GetSocket(), *new MsgAttack{});
				break;
			}
		}
		switch (wParam) {
		case VK_RETURN:
			global.myInstance->isChatting = !global.myInstance->isChatting;
			if (global.myInstance->isChatting == false && global.chatLen != 0) {
				global.chatMsg[global.chatLen] = 0;
				global.networkManager.SendNetworkMessage(global.myInstance->GetSocket(), *new MsgSendChat{ global.chatMsg });
				global.chatLen = 0;
				global.chatMsg[0] = 0;
			}
			global.lastChatWindowClose.Access([](auto& time) { time = std::chrono::high_resolution_clock::now(); });
			break;
		}
	}
	break;
	case WM_CHAR:
	{
		if (!global.myInstance->isChatting) break;
		switch (wParam) {
		case VK_RETURN:
			break;
		case VK_BACK:
			if (global.chatLen > 0) {
				global.chatMsg[--global.chatLen] = 0;
			}
			break;
		case VK_ESCAPE:
			global.myInstance->isChatting = false;
			global.chatLen = 0;
			global.chatMsg[0] = 0;
			global.lastChatWindowClose.Access([](auto& time) { time = std::chrono::high_resolution_clock::now(); });
			break;
		case VK_TAB:
		case 0x0A: // line feed
			break;
		default: // 문자들
			if (global.chatLen < MAX_CHAT_LEN - 1) {
				global.chatMsg[global.chatLen++] = wParam;
			}
			break;
		}
	}
	break;
	case WM_DESTROY:
		global.networkManager.SendNetworkMessage(global.myInstance->GetSocket(), *new MsgLogout);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void SendMovePacket(SOCKET sock, UINT_PTR key) {
	BYTE direction;
	switch (key) {
	case VK_UP:
		direction = 0;
		break;
	case VK_DOWN:
		direction = 1;
		break;
	case VK_LEFT:
		direction = 2;
		break;
	case VK_RIGHT:
		direction = 3;
		break;
	default:
		return;
	}

	global.networkManager.SendNetworkMessage(sock, *new MsgInputMove{ direction });
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

	auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sock) err_quit_wsa(TEXT("socket"));
	BOOL isNoDelay = TRUE;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay)) == SOCKET_ERROR) err_quit_wsa(TEXT("setsockopt"));

	if (connect(sock, (const sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR) err_quit_wsa(TEXT("connect"));

	CreateIoCompletionPort((HANDLE)sock, global.iocpObject, 0, 0);
	global.myInstance = new Client{ sock };
	global.networkManager.SendNetworkMessage(sock, *new MsgLogin{ L"" });
	global.networkManager.RecvNetworkMessage(*global.myInstance);
	return true;
}

void DrawObject(HDC memDC, const int leftTopCoordX, const int leftTopCoordY, Object & obj)
{
	const auto[x, y] = obj.GetPos();
	auto rect = obj.GetRelativeRect(leftTopCoordX, leftTopCoordY);

	HBRUSH brush = CreateSolidBrush(obj.GetColor());
	auto old = SelectObject(memDC, brush);
	switch (obj.GetType()) {
	case ObjectType::PLAYER:
		Ellipse(memDC, rect.left, rect.top, rect.right, rect.bottom);
		break;
	case ObjectType::MELEE:
	{
		const auto wOffset = CELL_W / 6;
		const auto hOffset = CELL_H / 6;
		Rectangle(memDC, rect.left + wOffset, rect.top + hOffset, rect.right - wOffset, rect.bottom - hOffset);
	}
	break;
	case ObjectType::RANGE:
	{
		POINT points[] = { {rect.left, (rect.bottom + rect.top) / 2}, {(rect.left + rect.right) / 2, rect.top}, {rect.right, (rect.bottom + rect.top) / 2}, {(rect.left + rect.right) / 2, rect.bottom} };
		Polygon(memDC, points, 4);
	}
	break;
	}
	SelectObject(memDC, old);
	DeleteObject(brush);
}

void DrawHP(HDC memDC, const int leftTopX, const int leftTopY, Object & obj)
{
	// 체력 표시
	const auto maxHP = obj.GetMaxHP();
	const auto hp = obj.GetHP();
	auto rect = obj.GetRelativeRect(leftTopX, leftTopY);
	OffsetRect(&rect, 0, -CELL_H);
	rect.top = rect.bottom - CELL_H / 4;
	auto oldBrush = SelectObject(memDC, GetStockObject(BLACK_BRUSH));
	Rectangle(memDC, rect.left, rect.top, rect.right, rect.bottom);
	auto oldPen = SelectObject(memDC, GetStockObject(NULL_PEN));
	auto redBrush = CreateSolidBrush(RGB(255, 0, 0));
	SelectObject(memDC, redBrush);
	InflateRect(&rect, -2, -2);
	const auto width = rect.right - rect.left;
	Rectangle(memDC, rect.left, rect.top, rect.left + width * (hp / (float)maxHP), rect.bottom);
	SelectObject(memDC, oldPen);
	SelectObject(memDC, oldBrush);
	DeleteObject(redBrush);
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

void DrawChatLog(HDC memDC, int right, int height, const RECT & winSize)
{
	global.chatLogs.Access([memDC, right, &height, &winSize](auto& logs) {
		if (logs.empty()) return;

		std::vector<RECT> drawBox;
		for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
			RECT box = winSize;
			box.right = box.left + right;
			const auto hDelta = DrawText(memDC, it->c_str(), it->size(), &box, DT_CALCRECT | DT_WORDBREAK);
			box.bottom = winSize.bottom - height;
			box.top = box.bottom - hDelta;
			height += hDelta;
			drawBox.emplace_back(box);
		}

		Rectangle(memDC, 0, drawBox.back().top, right, drawBox.front().bottom);
		for (int i = logs.size() - 1; i >= 0; --i) {
			RECT& box = drawBox[logs.size() - i - 1];
			DrawText(memDC, logs[i].c_str(), logs[i].size(), &box, DT_WORDBREAK | DT_VCENTER | DT_LEFT);
		}
	});
}

void ClientMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	auto rType = (MsgTypeSC)msg.type;
	switch (rType) {
	case MsgTypeSC::LOGIN_OK:
	{
		auto& rMsg{ (MsgLoginOK&)msg };
		client->SetID(rMsg.id);
		auto objPtr = make_unique<Object>(rMsg.id, rMsg.x, rMsg.y);
		objPtr->SetType(ObjectType::PLAYER);
		objPtr->SetLevel(rMsg.level);
		objPtr->SetExp(rMsg.exp);
		objPtr->SetHP(rMsg.hp);
		objPtr->SetMaxHP(rMsg.hp);
		global.objManager.Insert(move(objPtr));
	}
	break;
	case MsgTypeSC::LOGIN_FAIL:
		MessageBox(global.mainWin, TEXT("접속이 거절되었습니다."), TEXT("접속 거부"), MB_OK);
		closesocket(client->GetSocket());
		WSACleanup();
		exit(1);
		break;
	case MsgTypeSC::POSITION:
	{
		auto& rMsg = (MsgSetPosition&)msg;
		global.objManager.Update(rMsg.id, [x{ rMsg.x }, y{ rMsg.y }](auto& obj) {
			obj.SetPos(x, y);
		});
	}
	break;
	case MsgTypeSC::CHAT:
	{
		auto& rMsg = *(MsgChat*)&msg;
		global.chatLogs.Access([&rMsg](deque<wstring>& logs) {
			if (logs.size() >= global.MAX_CHAT_LOG_NUM) {
				logs.pop_front();
			}
			wstring text{ L"[System] " };
			text += rMsg.msg;
			logs.emplace_back(std::move(text));
			global.lastChatWindowClose.Access([](auto& tp) {
				tp = std::chrono::high_resolution_clock::now();
			});
		});
	}
	break;
	case MsgTypeSC::OTHER_CHAT:
	{
		auto& rMsg = *(MsgOtherChat*)&msg;
		global.chatLogs.Access([&rMsg](deque<wstring>& logs) {
			if (logs.size() >= global.MAX_CHAT_LOG_NUM) {
				logs.pop_front();
			}
			wstring text{ rMsg.from };
			text += L" : ";
			text += rMsg.msg;
			logs.emplace_back(std::move(text));
			global.lastChatWindowClose.Access([](auto& tp) {
				tp = std::chrono::high_resolution_clock::now();
			});
		});
	}
	break;
	case MsgTypeSC::STAT_CHANGE:
	{
		auto& rMsg{ (MsgStatChange&)msg };
		global.objManager.Update(client->GetID(), [&rMsg](Object& obj) {
			obj.SetHP(rMsg.hp);
			if (obj.GetMaxHP() < rMsg.hp)
				obj.SetMaxHP(rMsg.hp);
			obj.SetLevel(rMsg.level);
			obj.SetExp(rMsg.exp);
		});
	}
	break;
	case MsgTypeSC::OTHER_STAT_CHANGE:
	{
		auto& rMsg{ (MsgOtherStatChange&)msg };
		global.objManager.Update(rMsg.id, [&rMsg](Object& obj) {
			obj.SetHP(rMsg.hp);
			obj.SetMaxHP(rMsg.maxHP);
			obj.SetLevel(rMsg.level);
			obj.SetExp(rMsg.exp);
		});
	}
	break;
	case MsgTypeSC::REMOVE_OBJ:
	{
		auto& rMsg = *(MsgRemoveObject*)&msg;
		if (rMsg.id == global.myInstance->GetID()) {
			global.myInstance->Connect(false);
			global.objManager.Access([](auto& map) {map.clear(); });
		}
		else
			global.objManager.Remove(rMsg.id);
	}
	break;
	case MsgTypeSC::ADD_OBJ:
	{
		auto& rMsg = (MsgAddObject&)msg;
		if (!global.myInstance->IsConnected() && rMsg.id == global.myInstance->GetID()) global.myInstance->Connect(true);
		auto objPtr = make_unique<Object>(rMsg.id, rMsg.x, rMsg.y);
		objPtr->SetType((ObjectType)rMsg.objType);
		global.objManager.Insert(move(objPtr));
	}
	break;
	case MsgTypeSC::SET_COLOR:
	{
		auto& rMsg = (MsgSetColor&)msg;
		global.objManager.Update(rMsg.id, [&rMsg](Object& obj) {
			obj.SetColor(rMsg.color);
		});
	}
	break;
	}
}
