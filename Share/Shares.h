#pragma once

enum class MsgType { NONE, INPUT_MOVE, MOVE_CHARA };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 8, BOARD_H = 8;

void err_quit_wsa(LPCTSTR msg);

struct MsgBase;

class MsgReconstructor {
	std::vector<char> backBuf;
	std::vector<char> buf;
	size_t backBufMaxLen;
	size_t backBufSize;
	size_t bufMaxLen;
	size_t bufSize;
	size_t preRemainSize;
	std::function<void(const MsgBase&)> msgHandler;

	void Reconstruct();
public:
	MsgReconstructor(size_t bufLength, std::function<void(const MsgBase&)> msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, preRemainSize{ 0 }, msgHandler { msgHandler } { backBuf.reserve(bufMaxLen); buf.reserve(bufMaxLen); }

	void Recv(SOCKET s);
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen-bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};

#pragma pack(push, 1)

struct MsgBase {
	short len;
	MsgType type;

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
	virtual ~MsgBase() {}
};

struct MsgInputMove : public MsgBase {
	byte dx, dy;

	MsgInputMove(byte dx, byte dy) : MsgBase{ sizeof(MsgInputMove), MsgType::INPUT_MOVE }, dx{ dx }, dy{ dy } {}
	~MsgInputMove() {}
};

struct MsgMoveCharacter : public MsgBase {
	byte x, y;
	MsgMoveCharacter(byte x, byte y) : MsgBase{ sizeof(MsgMoveCharacter), MsgType::MOVE_CHARA }, x{ x }, y{ y } {}
	~MsgMoveCharacter() {}
};
#pragma pack(pop)