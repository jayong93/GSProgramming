#pragma once

enum class MsgType { NONE, INPUT_MOVE, MOVE_CHARA, CLIENT_OUT, CLIENT_IN };

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
	std::function<void(SOCKET, const MsgBase&, void*)> msgHandler;

public:
	MsgReconstructor(size_t bufLength, std::function<void(SOCKET, const MsgBase&, void*)> msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, preRemainSize{ 0 }, msgHandler{ msgHandler } { backBuf.reserve(bufMaxLen); buf.reserve(bufMaxLen); }
	MsgReconstructor(MsgReconstructor&& o) : backBuf{ std::move(o.backBuf) }, buf{ std::move(o.buf) }, backBufMaxLen{ o.backBufMaxLen }, backBufSize{ o.backBufSize }, bufMaxLen{ o.bufMaxLen }, bufSize{ o.bufSize }, preRemainSize{ o.preRemainSize }, msgHandler{ std::move(o.msgHandler) } {}
	MsgReconstructor& operator=(MsgReconstructor&& o) {
		backBuf = std::move(o.backBuf);
		buf = std::move(o.buf);
		backBufMaxLen = o.backBufMaxLen;
		backBufSize = o.backBufSize;
		bufMaxLen = o.bufMaxLen;
		bufSize = o.bufSize;
		preRemainSize = o.preRemainSize;
		msgHandler = std::move(o.msgHandler);
	}

	void Recv(SOCKET s);
	void Reconstruct(SOCKET s, void* ov = nullptr);
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen - bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};

#pragma pack(push, 1)
struct Color {
	unsigned char r, g, b;

	Color(unsigned char r, unsigned char g, unsigned char b) : r{ r }, g{ g }, b{ b } {}
};

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
	unsigned int id;
	char x, y;
	MsgMoveCharacter(unsigned int id, char x, char y) : MsgBase{ sizeof(MsgMoveCharacter), MsgType::MOVE_CHARA }, id{ id }, x{ x }, y{ y } {}
};

struct MsgClientOut : public MsgBase {
	unsigned int id;
	explicit MsgClientOut(unsigned int id) : MsgBase{ sizeof(MsgClientOut), MsgType::CLIENT_OUT }, id{ id } {}
};

struct MsgClientIn : public MsgBase {
	unsigned int id;
	char x, y;
	Color color;

	MsgClientIn(unsigned int id, char x, char y, Color color) : MsgBase{ sizeof(MsgClientIn), MsgType::CLIENT_IN }, id{ id }, x{ x }, y{ y }, color{ color } {}
};
#pragma pack(pop)