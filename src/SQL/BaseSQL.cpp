#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>
#include "SQL/BaseSQL.h"

using namespace std;

FILE* textLogFile; //for future use by the textLog function

column::column(const string& name, const string& type) : name(name), type(type) {}

tableInfo::tableInfo(const string& tableName, const initializer_list<column>& cols) : name(tableName), columns(cols) {}
tableInfo::tableInfo(const string& tableName, const vector<column>& cols) : name(tableName), columns(cols) {}

void textLog(sqlite3 *db, string eventName, string object, string subject, string eventStatus)
{
	time_t now = time(0);
	fprintf(textLogFile, "Data: {\neventName = %s\nobject = %s\nsubject = %s\neventStatus = %s\neventDateTime = %s}\n", eventName.c_str(), object.c_str(), subject.c_str(), eventStatus.c_str(),ctime(&now));
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

int dropTable(sqlite3 *db, string tableName, string *errString)
{
	string sql = "DROP TABLE IF EXISTS " + tableName;

	sqlite3_stmt *stmt;
	int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "dropTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_dropTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(stmt);
		return -1;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "dropTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_dropTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(stmt);
		return -2;
	}

	Log(db, "dropTable", "TABLE:" + tableName, "SYSTEM", "OK", errString);
	errString->append("_dropTable-OK");
	sqlite3_finalize(stmt);
	return 0;
}

int dropTable(sqlite3 *db, tableInfo table, string *errString)
{
	return dropTable(db, table.name, errString);
}

int checkTable(sqlite3 *db, string tableName, vector<column> columns, string *errString)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?", -1, &res, 0);
	if (rc != SQLITE_OK) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -1;
	}

	rc = sqlite3_bind_text(res, 1, tableName.c_str(), -1, SQLITE_STATIC);
	if (rc != SQLITE_OK) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -2;
	}

	rc = sqlite3_step(res);
	if (rc == SQLITE_ROW) {
		int count = sqlite3_column_int(res, 0);
		if (count == 0) {
			Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "OK(WARN):table does not exist", errString);
			errString->append("_checkTable-OK(WARN):table does not exist");
			sqlite3_finalize(res);
			return 1;
		}
	} else {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -3;
	}
	sqlite3_finalize(res);
	rc = sqlite3_prepare_v2(db, "SELECT name, type FROM pragma_table_info(?)", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR:SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -4;
	}
	rc = sqlite3_bind_text(res, 1, tableName.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) //TODO
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -5;
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
				return 2;
			}
			if(string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) != columns[i].type) //OK
			{
				Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:column type is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) + "\" != \"" + columns[i].type + "\")", errString);
				errString->append("_checkTable-FAIL:column type is not equal to expected(\"" +
				string(reinterpret_cast<const char*>(sqlite3_column_text(res, 1))) + "\" != \"" + columns[i].type + "\")");
				sqlite3_finalize(res);
				return 3;
			}
		}
		else if(rc == SQLITE_DONE) //OK
		{
			Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:the number of columns is lesser than expected", errString);
			errString->append("_checkTable-FAIL:the number of columns is lesser than expected");
			sqlite3_finalize(res);
			return 4;
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
		return 0;
	}
	if(rc == SQLITE_ROW) //OK
	{
		Log(db, "checkTable", "TABLE:" + tableName, "SYSTEM", "FAIL:the number of columns is greater than expected", errString);
		errString->append("_checkTable-FAIL:the number of columns is greater than expected");
		sqlite3_finalize(res);
		return 5;
	}
	const char *errmsg = sqlite3_errmsg(db);
	Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
	errString->append("_checkTable-FAIL_ERROR-SQLite:" + string(errmsg));
	sqlite3_finalize(res);
	return -7; //TODO
}

int checkTable(sqlite3 *db, tableInfo table, string *errString)
{
	return checkTable(db, table.name, table.columns, errString);
}

tableInfo LogInfo("Log", {column("id", "INTEGER"), column("eventName", "TEXT"), column("object", "TEXT"), column("subject", "TEXT"), column("eventStatus", "TEXT"), column("eventDateTime", "TEXT")});

int createTable(sqlite3 *db, string tableName, vector<column> columns, string *errString)
{
	int rc = checkTable(db, tableName, columns, errString);
	if(rc == 0)
	{
		Log(db, "createTable", "TABLE:" + tableName, "SYSTEM", "OK", errString);
		errString->append("_createTable-OK");
		return 0;
	}
	if(rc > 1)
	{
		dropTable(db, tableName, errString);
	}
	if(rc < 0)
	{
		Log(db, "createTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-checkTable:" + to_string(rc), errString);
		errString->append("_createTable-FAIL_ERROR-checkTable:" + to_string(rc));
	}
	// Формируем SQL-запрос для создания таблицы
	string sql = "CREATE TABLE " + tableName + " (";
	for (size_t i = 0; i < columns.size(); ++i) {
		sql += columns[i].name + " " + columns[i].type;
		if (i < columns.size() - 1) {
			sql += ", ";
		}
	}
	sql += ")";

	// Подготовка и выполнение запроса
	sqlite3_stmt *res;
	rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
	if (rc != SQLITE_OK) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "createTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_createTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -1;
	}

	rc = sqlite3_step(res);
	if (rc != SQLITE_DONE) {
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "createTable", "TABLE:" + tableName, "SYSTEM", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_createTable-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -2;
	}

	// Успешное создание
	Log(db, "createTable", "TABLE:" + tableName, "SYSTEM", "OK", errString);
	errString->append("_createTable-OK");
	sqlite3_finalize(res);
	return 0;
}

int createTable(sqlite3 *db, tableInfo table, string *errString)
{
	return createTable(db, table.name, table.columns, errString);
}



int initBaseSQL(sqlite3 **db, string databaseName, string *errString)
{
	textLogFile = fopen("textLog", "a");
	if(textLogFile == NULL)
	{
		printf("ERROR:Disabble to open file\n");
		return -1;
	}
	if (sqlite3_open(databaseName.c_str(), db) != SQLITE_OK)
	{
		textLog(*db, "initBaseSQL", "DATABASE", "SYSTEM", "FAIL_ERROR-SQLite");
		errString->append("_initBaseSQL-FAIL_ERROR-SQLite");
		return -2;
	}
	int rc = checkTable(*db, LogInfo, errString);
	if(rc > 0)
	{
		textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "OK(WARN):table in an unexpected way");
		errString->append("_initBaseSQL-OK(WARN):table in an unexpected way");
		if (rc != 1)
		{
			int rrc = dropTable(*db, LogInfo, errString);
			if(rrc < 0)
			{
				textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "FAIL_ERROR-dropTable:" + to_string(rrc));
				errString->append("_initBaseSQL-FAIL_ERROR-dropTable:" + to_string(rrc));
				return -3;
			}
		}
		rc = createTable(*db, LogInfo, errString);
		if(rc != 0)
		{
			textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "FAIL_ERROR-createTable:" + to_string(rc));
			errString->append("_initBaseSQL-FAIL_ERROR-createTable:" + to_string(rc));
			return -4;
		}
		Log(*db, "initBaseSQL", "DATABASE", "SYSTEM", "OK(WARN):the table has been overwritten", errString);
		errString->append("_initBaseSQL-OK(WARN):the table has been overwritten");
		return 0;
	}
	if(rc < 0)
	{
		textLog(*db, "initBaseSQL", "TABLE:Log", "SYSTEM", "FAIL_ERROR-checkTable:" + to_string(rc));
		errString->append("_initBaseSQL-FAIL_ERROR:" + to_string(rc));
		return -5;
	}
	Log(*db, "initBaseSQL", "DATABASE", "SYSTEM", "OK", errString);
	errString->append("_initBaseSQL-OK");
	return 0;
}
