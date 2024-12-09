#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>
#include "BaseSQL.h"

using namespace std;

column::column(const string& name, const string& type) : name(name), type(type) {}

tableInfo::tableInfo(const string& tableName, const initializer_list<column>& cols) : name(tableName), columns(cols) {}
tableInfo::tableInfo(const string& tableName, const vector<column>& cols) : name(tableName), columns(cols) {}

void textLog(sqlite3 *db, string eventName, string object, string subject, string eventStatus)
{
	printf("SQLite error:%s ", sqlite3_errmsg(db));
	time_t now = time(0);
	printf("Data: {\neventName = %s\nobject = %s\nsubject = %s\neventStatus = %s\neventDateTime = %s}\n", eventName.c_str(), object.c_str(), subject.c_str(), eventStatus.c_str(),ctime(&now));
}

int Log(sqlite3 *db, string eventName, string object, string subject, string eventStatus, string *errString)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "INSERT INTO Log(eventName, object, subject, eventStatus, eventDateTime) VALUES(?, ?, ?, ?, ?)", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		textLog(db, eventName, object, subject, eventStatus);
		errString->append("_Log-FAIL_ERROR-SQLite:" + string(sqlite3_errmsg(db)));
		sqlite3_finalize(res);
		return -1;
	}
	time_t now = time(0);
	char time[30];
	strftime(time, 30, "%Y-%m-%d %H:%M:%S", localtime(&now));
	if((sqlite3_bind_text(res, 1, eventName.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_text(res, 2, object.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_text(res, 3, subject.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_text(res, 4, eventStatus.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_text(res, 5, time, -1, SQLITE_STATIC)) == SQLITE_OK) //OK
	{
		if(sqlite3_step(res) == SQLITE_DONE)
		{
			errString->append("_Log-OK");
			sqlite3_finalize(res);
			return 0;
		}
	}
	errString->append("_Log-FAIL_ERROR-SQLite:" + string(sqlite3_errmsg(db)));
	textLog(db, eventName, object, subject, eventStatus);
	sqlite3_finalize(res);
	return -2;
}

int checkTable(sqlite3 *db, string tableName, vector<column> columns, string *errString)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT name, type FROM pragma_table_info(?)", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR:SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -1;
	}
	rc = sqlite3_bind_text(res, 1, tableName.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) //TODO
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -2;
	}
	for(int i = 0; i < columns.size(); i++)
	{
		rc = sqlite3_step(res);
		if(rc == SQLITE_ROW)
		{
			if(string(reinterpret_cast<const char*>(sqlite3_column_text(res, 0))) != columns[i].name) //OK
			{
				Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:column name is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 0))) + "\" != \"" + columns[i].name + "\")", errString);
				errString->append("_checkTable-FAIL:column name is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 0))) + "\" != \"" + columns[i].name + "\")");
				sqlite3_finalize(res);
				return -3;
			}
			if(string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) != columns[i].type) //OK
			{
				Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:column type is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) + "\" != \"" + columns[i].type + "\")", errString);
				errString->append("_checkTable-FAIL:column type is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) + "\" != \"" + columns[i].type + "\")");
				sqlite3_finalize(res);
				return -4;
			}
		}
		else if(rc == SQLITE_DONE) //OK
		{
			Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:number of columns is less than expected", errString);
			errString->append("_checkTable-FAIL:number of columns is less than expected");
			sqlite3_finalize(res);
			return -5;
		}
		else
		{
			const char *errmsg = sqlite3_errmsg(db);
			Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
			errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
			sqlite3_finalize(res);
			return -6; //TODO
		}
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_DONE) //OK
	{
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "OK", errString);
		errString->append("_checkTable-OK");
		sqlite3_finalize(res);
		return 1;
	}
	if(rc == SQLITE_ROW) //OK
	{
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:the number of columns is greater than expected", errString);
		errString->append("_checkTable-FAIL:the number of columns is greater than expected");
		sqlite3_finalize(res);
		return -7;
	}
	const char *errmsg = sqlite3_errmsg(db);
	Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
	errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
	sqlite3_finalize(res);
	return -8; //TODO
}

int checkTable(sqlite3 *db, tableInfo table, string *errString)
{
	return checkTable(db, table.name, table.columns, errString);
}

tableInfo LogInfo("Log", {column("id", "INTEGER"), column("eventName", "TEXT"), column("object", "TEXT"), column("subject", "TEXT"), column("eventStatus", "TEXT"), column("eventDateTime", "TEXT")});
//int Log(sqlite3 *db, string eventName, string object, string subject, string eventStatus, string *errString)

int initBaseSQL(sqlite3 **db, string databaseName, string *errString)
{
	if (sqlite3_open(databaseName.c_str(), db) != SQLITE_OK)
	{
		textLog(*db, "initBaseSQL", "DATABASE", "SYSTEM", "FAIL_ERROR-SQLite");
		errString->append("_initBaseSQL-FAIL_ERROR-SQLite");
		return -1;
	}
	if(checkTable(*db, LogInfo, errString) > 1)
	{
		textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "FAIL:table in an unexpected way");
		errString->append("_initBaseSQL-FAIL:table in an unexpected way");
		return -2;
	}
	if(checkTable(*db, LogInfo, errString) > 1)
	{
		textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "FAIL_ERROR-checkTable:");
		errString->append("_initBaseSQL-FAIL:table in an unexpected way");
		return -3;
	}
	Log(*db, "initBaseSQL", "DATABASE", "SYSTEM", "OK", errString);
	errString->append("_initBaseSQL-OK");
	return 0;
}
