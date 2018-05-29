#pragma once
#include <vector>
#include <memory>

struct MsgBase;

/*
MSG_HANDLER Ÿ���� ȣ�� ������ Ÿ���̾�� �Ѵ�.
ȣ�� ������ ������ ����.
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
	MsgReconstructor(size_t bufLength, const MSG_HANDLER& msgHandler) : bufMaxLen{ bufLength }, bufSize{ 0 }, backBufMaxLen{ bufLength }, backBufSize{ 0 }, preRemainSize{ 0 }, msgHandler{ msgHandler } { backBuf.resize(bufMaxLen); buf.resize(bufMaxLen); }
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

	bool Recv(SOCKET s)
	{
		int retval = recv(s, (char*)buf.data() + bufSize, bufMaxLen - bufSize, 0);
		if (SOCKET_ERROR == retval) return WSAGetLastError();
		bufSize += retval;
		return this->Reconstruct(s);
	}
	bool Reconstruct(SOCKET s)
	{
		bool isConstructed{ false };
		char* curPos = buf.data();

		while (bufSize > 0) {
			// �̿ϼ� ��Ŷ�� �ִ� ���
			if (preRemainSize > 0) {
				// ������ ���� ��
				if (bufSize >= preRemainSize) {
					memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, preRemainSize);
					msgHandler(s, *reinterpret_cast<MsgBase*>(backBuf.data()));
					bufSize -= preRemainSize; curPos += preRemainSize;
					backBufSize = 0; preRemainSize = 0;
					isConstructed = true;
				}
				// ���� �̿ϼ��� ���
				else {
					memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, bufSize);
					backBufSize += bufSize; preRemainSize -= bufSize;
					bufSize = 0;
					break;
				}
			}
			// ��Ŷ���������� �� �� ������ŭ �����Ͱ� ���� ���
			else if (bufSize < sizeof(short)) {
				if (curPos != buf.data()) {
					memcpy_s(buf.data(), bufMaxLen, curPos, bufSize);
				}
				break;
			}
			else {
				short packetSize = *reinterpret_cast<short*>(curPos);
				// ��Ŷ�� �߷��� �� ���
				if (packetSize > bufSize) {
					if (backBufMaxLen < packetSize) {
						backBuf.resize(packetSize);
						backBufMaxLen = packetSize;
					}
					memcpy_s(backBuf.data(), backBufMaxLen, curPos, bufSize);
					backBufSize = bufSize;
					preRemainSize = packetSize - bufSize;
					bufSize = 0;
					break;
				}
				// ���� ��Ŷ ó��
				else {
					msgHandler(s, *reinterpret_cast<MsgBase*>(curPos));
					bufSize -= packetSize; curPos += packetSize;
					isConstructed = true;
				}
			}
		}

		return isConstructed;
	}
	char* GetBuffer() { return reinterpret_cast<char*>(buf.data() + bufSize); }
	size_t GetSize() const { return bufMaxLen - bufSize; }
	void AddSize(size_t size) { bufSize += size; }
};
