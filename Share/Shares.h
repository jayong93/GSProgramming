#pragma once

enum class MsgType { NONE, INPUT_MOVE, GIVE_ID, MOVE_OBJ, PUT_OBJ, REMOVE_OBJ };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 100, BOARD_H = 100; // unsigned로 선언하지 말 것. 플레이어 좌표를 min 하는 과정에서 unsigned로 변환되어 -1이 99가 됨
constexpr int VIEW_SIZE = 11;
constexpr int PLAYER_VIEW_SIZE = 7;

void err_quit_wsa(LPCTSTR msg);
void err_quit_wsa(DWORD errCode, LPCTSTR msg);

struct MsgBase;

struct MsgHandler {
	virtual void ProcessMessage(SOCKET s, const MsgBase& msg) = 0;
	virtual ~MsgHandler() {}
};

class MsgReconstructor {
	std::vector<char> backBuf;
	std::vector<char> buf;
	size_t backBufMaxLen;
	size_t backBufSize;
	size_t bufMaxLen;
	size_t bufSize;
	size_t preRemainSize;
	std::unique_ptr<MsgHandler> msgHandler;

public:
	MsgReconstructor() {}
	MsgReconstructor(size_t bufLength, MsgHandler& msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, preRemainSize{ 0 }, msgHandler{ &msgHandler } { backBuf.reserve(bufMaxLen); buf.reserve(bufMaxLen); }
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
	void Reconstruct(SOCKET s);
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen - bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};

#pragma pack(push, 1)
struct Color {
	unsigned char r, g, b;

	Color(unsigned char r, unsigned char g, unsigned char b) : r{ r }, g{ g }, b{ b } {}
};

// 가상 함수는 선언하지 말 것. vtable 때문에 자료구조 크기가 바뀜
struct MsgBase {
	short len;
	MsgType type;

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
};

struct MsgInputMove : public MsgBase {
	char dx, dy;

	MsgInputMove(char dx, char dy) : MsgBase{ sizeof(MsgInputMove), MsgType::INPUT_MOVE }, dx{ dx }, dy{ dy } {}
};

struct MsgGiveID : public MsgBase {
	unsigned int id;

	explicit MsgGiveID(unsigned int id) : MsgBase{ sizeof(MsgGiveID), MsgType::GIVE_ID }, id{ id } {}
};

struct MsgMoveObject : public MsgBase {
	unsigned int id;
	char x, y;
	MsgMoveObject(unsigned int id, char x, char y) : MsgBase{ sizeof(MsgMoveObject), MsgType::MOVE_OBJ }, id{ id }, x{ x }, y{ y } {}
};

struct MsgRemoveObject : public MsgBase {
	unsigned int id;
	explicit MsgRemoveObject(unsigned int id) : MsgBase{ sizeof(MsgRemoveObject), MsgType::REMOVE_OBJ }, id{ id } {}
};

struct MsgPutObject : public MsgBase {
	unsigned int id;
	char x, y;
	Color color;

	MsgPutObject(unsigned int id, char x, char y, Color color) : MsgBase{ sizeof(MsgPutObject), MsgType::PUT_OBJ }, id{ id }, x{ x }, y{ y }, color{ color } {}
};
#pragma pack(pop)