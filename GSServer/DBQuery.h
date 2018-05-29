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

class DBGetUserData : public DBQueryBase {
public:
	// ���� �����͸� ���������� �޾��� �� ȣ���� callback type
	struct Result {
		virtual void doWithResult(SQLWCHAR name[], SQLINTEGER xPos, SQLINTEGER yPos) = 0;
	};

	DBGetUserData(SQLHSTMT hstmt, const std::wstring& gameID, std::unique_ptr<Result>&& result) : DBQueryBase(hstmt, queryStr + gameID), result{ std::move(result) } {};

protected:
	virtual bool postprocess();

private:
	static const std::wstring queryStr;
	std::unique_ptr<Result> result;
};

class DBSetUserData : public DBQueryBase {
public:
	DBSetUserData(SQLHSTMT hstmt, const std::wstring& gameID, short xPos, short yPos) : DBQueryBase{ hstmt, getQueryStr(gameID, xPos,yPos)} {}

protected:
	virtual bool postprocess() { return true; };

private:
	static const std::wstring queryStr;
	static std::wstring getQueryStr(const std::wstring& gameID, short x, short y) {
		auto str = queryStr;
		str += gameID;
		str += std::to_wstring(x);
		str += L" ";
		str += std::to_wstring(y);
		return str;
	}
};