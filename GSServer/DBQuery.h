#pragma once
#include "typedef.h"

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
		SQLSMALLINT xPos, yPos, hp;
		SQLCHAR level;
		SQLINTEGER exp;
		SQLLEN len;

		SQLBindCol(state, 1, SQL_WCHAR, name, sizeof(name) / sizeof(*name), &len);
		SQLBindCol(state, 2, SQL_SMALLINT, &xPos, sizeof(xPos), &len);
		SQLBindCol(state, 3, SQL_SMALLINT, &yPos, sizeof(yPos), &len);
		SQLBindCol(state, 4, SQL_TINYINT, &level, sizeof(level), &len);
		SQLBindCol(state, 5, SQL_SMALLINT, &hp, sizeof(hp), &len);
		SQLBindCol(state, 6, SQL_INTEGER, &exp, sizeof(exp), &len);

		auto retval = SQLFetch(state);
		bool isSuccess;
		if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval)
			isSuccess = false;
		else
			isSuccess = true;

		result(isSuccess, {name, xPos, yPos, level, hp, exp});

		return isSuccess;
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
	DBSetUserData(SQLHSTMT hstmt, const DBData& data) : DBQueryBase{ hstmt, getQueryStr(data) } {}

protected:
	virtual bool postprocess() { return true; };

private:
	static const std::wstring queryStr;
	static std::wstring getQueryStr(const DBData& data) {
		const auto&[gameID, x, y, level, hp, exp] = data;
		auto str = queryStr;
		str += gameID;
		str += L", ";
		str += std::to_wstring(x);
		str += L", ";
		str += std::to_wstring(y);
		str += L", ";
		str += std::to_wstring(level);
		str += L", ";
		str += std::to_wstring(hp);
		str += L", ";
		str += std::to_wstring(exp);
		return str;
	}
};

class DBAddUser : public DBQueryBase {
public:
	DBAddUser(SQLHSTMT hstmt, const DBData& data) : DBQueryBase{ hstmt, getQueryStr(data) } {}

protected:
	virtual bool postprocess() { return true; };

private:
	static const std::wstring queryStr;
	static std::wstring getQueryStr(const DBData& data) {
		const auto&[gameID, x, y, level, hp, exp] = data;
		auto str = queryStr;
		str += gameID;
		str += L", ";
		str += std::to_wstring(x);
		str += L", ";
		str += std::to_wstring(y);
		str += L", ";
		str += std::to_wstring(level);
		str += L", ";
		str += std::to_wstring(hp);
		str += L", ";
		str += std::to_wstring(exp);
		return str;
	}
};
