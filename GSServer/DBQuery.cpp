#include "stdafx.h"
#include "DBQuery.h"
#include "../Share/Shares.h"

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];

	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}

	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

const std::wstring DBGetUserData::queryStr{ L"EXEC dbo.get_user_data " };
const std::wstring DBSetUserData::queryStr{ L"EXEC dbo.set_user_data " };

bool DBQueryBase::execute()
{
	auto retval = SQLExecDirect(this->state, (SQLWCHAR *)this->query.c_str(), SQL_NTS);
	if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval) {
		HandleDiagnosticRecord(this->state, SQL_HANDLE_STMT, retval);
		return false;
	}

	auto isSucess = this->postprocess();
	if (!isSucess)
		HandleDiagnosticRecord(this->state, SQL_HANDLE_STMT, retval);
	SQLCancel(state);
	return isSucess;
}

bool DBGetUserData::postprocess()
{
	SQLWCHAR name[MAX_GAME_ID_LEN + 1];
	SQLSMALLINT xPos, yPos;
	SQLLEN nameLen, xLen, yLen;

	SQLBindCol(state, 1, SQL_WCHAR, name, sizeof(name) / sizeof(*name), &nameLen);
	SQLBindCol(state, 2, SQL_SMALLINT, &xPos, sizeof(xPos), &xLen);
	SQLBindCol(state, 3, SQL_SMALLINT, &yPos, sizeof(yPos), &yLen);

	auto retval = SQLFetch(state);
	if (SQL_SUCCESS != retval && SQL_SUCCESS_WITH_INFO != retval)
		return false;

	if (result) {
		result->doWithResult(name, xPos, yPos);
	}

	return true;
}
