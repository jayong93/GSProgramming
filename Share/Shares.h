#pragma once

enum class MsgType { NONE, INPUT_MOVE, MOVE_CHARA };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 8, BOARD_H = 8;

void err_quit(LPCTSTR msg);

struct MsgBase;

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	std::unique_ptr<MsgBase> msg;

	explicit ExtOverlapped(std::unique_ptr<MsgBase> msg) : msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

class MsgReconstructor {
	std::vector<byte> backBuf;
	std::vector<byte> buf;
	int bufLength;
	std::function<void(const MsgBase*)> msgHandler;
public:
	MsgReconstructor(int bufLength, std::function<void(const MsgBase*)> msgHandler) : bufLength{ bufLength }, msgHandler{msgHandler} { backBuf.reserve(this->bufLength); buf.reserve(this->bufLength); }
};

#pragma pack(push, 1)

struct MsgBase {
	short len{ 0 };
	MsgType type{ MsgType::NONE };

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
	virtual ~MsgBase() {}
};

struct MsgInputMove : public MsgBase {
	byte dx, dy;

	MsgInputMove(byte dx, byte dy) : MsgBase{sizeof(MsgInputMove), MsgType::INPUT_MOVE}, dx{ dx }, dy{ dy } {}
	virtual ~MsgInputMove() {}
};

#pragma pack(pop)