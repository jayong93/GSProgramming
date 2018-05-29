#pragma once

class DBQueryBase {
public:
	DBQueryBase(SQLHSTMT hstmt, const std::wstring& query, unsigned int id) : state{ hstmt }, query{ query }, id{ id } {}
	virtual bool execute();

protected:
	SQLHSTMT state;
	std::wstring query;
	unsigned int id;

	virtual bool postprocess() = 0;
};

class DBGetUserData : DBQueryBase {
public:
	DBGetUserData(SQLHSTMT hstmt, unsigned int id, const std::wstring& gameID) : DBQueryBase(hstmt, queryStr + gameID, id) {};

protected:
	virtual bool postprocess();

private:
	std::wstring gameID;
	static const std::wstring queryStr;
};