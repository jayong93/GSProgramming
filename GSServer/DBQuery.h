#pragma once

class DBQueryBase {
public:
	DBQueryBase(SQLHSTMT hstmt, const std::wstring& query) : state{ hstmt }, query{ query } {}
	virtual bool execute();

protected:
	SQLHSTMT state;
	std::wstring query;

	virtual bool postprocess() = 0;
};

template <typename Func>
class DBGetUserData : public DBQueryBase {
public:
	DBGetUserData(SQLHSTMT hstmt, const std::wstring& gameID, Func&& result) : DBQueryBase(hstmt, queryStr + gameID), result{ std::forward<Func>(result) } {};

protected:

	virtual bool postprocess() {
		SQLWCHAR name[MAX_GAME_ID_LEN + 1];
		SQLSMALLINT xPos, yPos;
		SQLLEN nameLen, xLen, yLen;

		SQLBindCol(state, 1, SQL_WCHAR, name, sizeof(name) / sizeof(*name), &nameLen);
		SQLBindCol(state, 2, SQL_SMALLINT, &xPos, sizeof(xPos), &xLen);
		SQLBindCol(state, 3, SQL_SMALLINT, &yPos, sizeof(yPos), &yLen);

		auto retval = SQLFetch(state);
		if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval)
			return false;

		result(name, xPos, yPos);

		return true;
	}

private:
	static const std::wstring queryStr;
	Func result;
};

template <typename Func>
const std::wstring DBGetUserData<Func>::queryStr = L"EXEC dbo.get_user_data ";

template <typename Func>
std::unique_ptr<DBQueryBase> MakeGetUserDataQuery(SQLHSTMT hstmt, const std::wstring& gameID, Func&& result) {
	return std::unique_ptr<DBQueryBase>{ new DBGetUserData<Func>(hstmt, gameID, std::forward<Func>(result)) };
}

class DBSetUserData : public DBQueryBase {
public:
	DBSetUserData(SQLHSTMT hstmt, const std::wstring& gameID, short xPos, short yPos) : DBQueryBase{ hstmt, getQueryStr(gameID, xPos,yPos) } {}

protected:
	virtual bool postprocess() { return true; };

private:
	static const std::wstring queryStr;
	static std::wstring getQueryStr(const std::wstring& gameID, short x, short y) {
		auto str = queryStr;
		str += gameID;
		str += L", ";
		str += std::to_wstring(x);
		str += L", ";
		str += std::to_wstring(y);
		return str;
	}
};