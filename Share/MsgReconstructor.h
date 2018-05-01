#pragma once
#include <vector>
#include <memory>

struct MsgBase;

/*
MSG_HANDLER 타입은 호출 가능한 타입이어야 한다.
호출 형식은 다음과 같다.
void MSG_HANDLE(SOCKET s, const MsgBase& msg);
*/
template <typename MSG_HANDLER>
class MsgReconstructor {
	std::vector<char> backBuf;
	std::vector<char> buf;
	size_t backBufMaxLen;
	size_t backBufSize;
	size_t bufMaxLen;
	size_t bufSize;
	size_t preRemainSize;
	MSG_HANDLER msgHandler;

public:
	MsgReconstructor() {}
	MsgReconstructor(size_t bufLength, const MSG_HANDLER& msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, preRemainSize{ 0 }, msgHandler{ msgHandler } { backBuf.reserve(bufMaxLen); buf.reserve(bufMaxLen); }
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

	int Recv(SOCKET s)
	{
		int retval = recv(s, (char*)buf.data() + bufSize, bufMaxLen - bufSize, 0);
		if (SOCKET_ERROR == retval) return WSAGetLastError();
		bufSize += retval;
		this->Reconstruct(s);
		return 0;
	}
	void Reconstruct(SOCKET s)
	{
		char* curPos = buf.data();
		while (bufSize > 0) {
			// 미완성 패킷이 있는 경우
			if (preRemainSize > 0) {
				// 재조립 가능 시
				if (bufSize >= preRemainSize) {
					memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, preRemainSize);
					msgHandler(s, *reinterpret_cast<MsgBase*>(backBuf.data()));
					bufSize -= preRemainSize; curPos += preRemainSize;
					backBufSize = 0; preRemainSize = 0;
				}
				// 아직 미완성인 경우
				else {
					memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, bufSize);
					backBufSize += bufSize; preRemainSize -= bufSize;
					bufSize = 0;
					return;
				}
			}
			// 패킷사이즈조차 알 수 없을만큼 데이터가 적은 경우
			else if (bufSize < sizeof(short)) {
				if (curPos != buf.data()) {
					memcpy_s(buf.data(), bufMaxLen, curPos, bufSize);
				}
				return;
			}
			else {
				short packetSize = *reinterpret_cast<short*>(curPos);
				// 패킷이 잘려서 온 경우
				if (packetSize > bufSize) {
					if (backBufMaxLen < packetSize) {
						backBuf.reserve(packetSize);
						backBufMaxLen = packetSize;
					}
					memcpy_s(backBuf.data(), backBufMaxLen, curPos, bufSize);
					preRemainSize = packetSize - bufSize;
					bufSize = 0;
					return;
				}
				// 정상 패킷 처리
				else {
					msgHandler(s, *reinterpret_cast<MsgBase*>(curPos));
					bufSize -= packetSize; curPos += packetSize;
				}
			}
		}
	}
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen - bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};
