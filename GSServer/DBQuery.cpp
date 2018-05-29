#include "stdafx.h"
#include "DBQuery.h"

const std::wstring DBGetUserData::queryStr{L"EXEC dbo.get_user_data "};

bool DBQueryBase::execute()
{
	auto retval = SQLExecDirect(this->state, (SQLWCHAR *)L"EXEC dbo.select_high_level 100", SQL_NTS);
	if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval) {
		return false;
	}

	auto isSucess = this->postprocess();
	if (isSucess) SQLCancel(state);
	return isSucess;
}

bool DBGetUserData::postprocess()
{
	SQLWCHAR name[11]; // ID는 최대 10자
	SQLINTEGER xPos, yPos;
	SQLLEN nameLen, xLen, yLen;

	SQLBindCol(state, 1, SQL_C_CHAR, name, sizeof(name)/sizeof(SQLWCHAR), &nameLen);
	SQLBindCol(state, 2, SQL_INTEGER, &xPos, sizeof(xPos), &xLen);
	SQLBindCol(state, 3, SQL_INTEGER, &yPos, sizeof(yPos), &yLen);

	auto retval = SQLFetch(state);
	if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval)
		return false;

	// PostQueuedStatus();

	return true;
}
