enum class ClientMsg { KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN };
enum class ServerMsg { MOVE_CHARA };

constexpr u_short GS_PORT = 9011;
constexpr int boardW = 8, boardH = 8;

int RecvAll(SOCKET sock, char * buf, size_t len, int flag)
{
	int receivedLen{ 0 };
	int retval;
	while (receivedLen < len) {
		if ((retval = recv(sock, buf + receivedLen, len - receivedLen, flag)) == SOCKET_ERROR) return retval;
		receivedLen += retval;
	}
	return receivedLen;
}
