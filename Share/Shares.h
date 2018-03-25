#pragma once

enum class MsgType { NONE, INPUT_MOVE, MOVE_CHARA, CLIENT_OUT };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 8, BOARD_H = 8;

void err_quit_wsa(LPCTSTR msg);
void err_quit_wsa(DWORD errCode, LPCTSTR msg);

struct MsgBase;

class MsgReconstructor {
	std::vector<char> backBuf;
	std::vector<char> buf;
	size_t backBufMaxLen;
	size_t backBufSize;
	size_t bufMaxLen;
	size_t bufSize;
	size_t preRemainSize;
	std::function<void(SOCKET, const MsgBase&)> msgHandler;

public:
	MsgReconstructor(size_t bufLength, std::function<void(SOCKET, const MsgBase&)> msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, preRemainSize{ 0 }, msgHandler { msgHandler } { backBuf.reserve(bufMaxLen); buf.reserve(bufMaxLen); }

	void Recv(SOCKET s);
	void Reconstruct(SOCKET s);
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen-bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};

#pragma pack(push, 1)

struct MsgBase {
	short len;
	MsgType type;

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
};

struct MsgInputMove : public MsgBase {
	char dx, dy;

	MsgInputMove(char dx, char dy) : MsgBase{ sizeof(MsgInputMove), MsgType::INPUT_MOVE }, dx{ dx }, dy{ dy } {}
};

struct MsgMoveCharacter : public MsgBase {
	char x, y;
	MsgMoveCharacter(char x, char y) : MsgBase{ sizeof(MsgMoveCharacter), MsgType::MOVE_CHARA }, x{ x }, y{ y } {}
};

struct MsgClientOut : public MsgBase {
	MsgClientOut() : MsgBase{ sizeof(MsgClientOut), MsgType::CLIENT_OUT } {}
};
#pragma pack(pop)