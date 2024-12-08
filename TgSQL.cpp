#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>

using namespace std;

#define ADD_USER_MIN_PRIVILEGE 100
#define MOD_USER_MIN_PRIVILEGE 100
FILE* file;

struct column
{
	string name;
	string type;
	column(const string& name, const string& type) : name(name), type(type) {}
};

class tableInfo {
public:
	string name;
	vector<column> columns;
	tableInfo(const string& tableName, const initializer_list<column>& cols) : name(tableName), columns(cols) {}
	tableInfo(const string& tableName, const vector<column>& cols) : name(tableName), columns(cols) {}
};
//id INTEGER PRIMARY KEY, eventName TEXT, object TEXT, subject TEXT, eventStatus TEXT, eventDateTime TEXT
tableInfo LogInfo("Log", {column("id", "INTEGER"), column("eventName", "TEXT"), column("object", "TEXT"), column("subject", "TEXT"), column("eventStatus", "TEXT"), column("eventDateTime", "TEXT")});

int getAddUserMinPrivilege()
{
	return ADD_USER_MIN_PRIVILEGE;
}

int getModUserMinPrivilege()
{
	return MOD_USER_MIN_PRIVILEGE;
}

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

int userCount(sqlite3 * db, string userName, string *errString)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE name = ?", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "userCount", userName, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_userCount-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -1;
	}
	rc = sqlite3_bind_text(res, 1, userName.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "userCount", userName, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_userCount-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -2;
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_ROW) //OK
	{
		Log(db, "userCount", userName, "", "OK", errString);
		errString->append("_userCount-OK");
		int result = sqlite3_column_int(res,0);
		sqlite3_finalize(res);
		return result;
	}
	const char *errmsg = sqlite3_errmsg(db); //TODO
	Log(db, "userCount", userName, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
	errString->append("_userCount-FAIL_ERROR-SQLite:" + string(errmsg));
	sqlite3_finalize(res);
	return -3;
}

int getUserPrivilege(sqlite3 *db, string object, string *errString)
{
	sqlite3_stmt *res;
	int privilege;
	int count = userCount(db, object, errString);
	if(count == 0) //OK
	{
		Log(db, "getUserPrivilege", object, "", "FAIL:user not found", errString);
		errString->append("_getUserPrivilege-FAIL:user not found");
		return -1;
	}
	else if(count > 1) //OK
	{
		Log(db, "getUserPrivilege", object, "", "FAIL:there are a few users with that name", errString);
		errString->append("_getUserPrivilege-FAIL:there are a few users with that name");
		return -2;
	}
	else if(count < 0) //OK
	{
		Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-userCount:" + to_string(count), errString);
		errString->append("_getUserPrivilege-FAIL_ERROR-userCount:" + to_string(count));
		return -3;
	}
	int rc = sqlite3_prepare_v2(db, "SELECT name, privilege FROM users WHERE name = ?", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_getUserPrivilege-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -4;
	}
	rc = sqlite3_bind_text(res, 1, object.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_getUserPrivilege-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -5;
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_ROW) //OK
	{
		privilege = sqlite3_column_int(res, 1);
	}
	else //TODO
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_getUserPrivilege-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -6;
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_DONE) //OK
	{
		Log(db, "getUserPrivilege", object, "", "OK", errString);
		errString->append("_getUserPrivilege-OK");
		sqlite3_finalize(res);
		return privilege;
	}
	const char *errmsg = sqlite3_errmsg(db); //TODO
	Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
	errString->append("_getUserPrivilege-FAIL_ERROR-SQLite:" + string(errmsg));
	sqlite3_finalize(res);
	return -7;
}

int modUser(sqlite3 *db, string object, string subject, int newPrivilege, string *errString)
{
	int subjectPrivilege = getUserPrivilege(db, subject, errString);
	if(subjectPrivilege < 0) //OK
	{
		Log(db, "modUser", object, subject, "FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege), errString);
		errString->append("_modUser-FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege));
		return -1;
	}
	int count = userCount(db, object, errString);
	if(count < 0) //OK
	{
		Log(db, "modUser", object, subject, "FAIL_ERROR-userCount:" + to_string(count), errString);
		errString->append("_modUser-FAIL_ERROR-userCount:" + to_string(count));
		return -2;
	}
	if(count == 0) //OK
	{
		Log(db, "modUser", object, subject, "FAIL:user not found" + to_string(subjectPrivilege), errString);
		errString->append("_modUser-FAIL:user not found");
		return -3;
	}
	if(count > 1) //OK
	{
		Log(db, "modUser", object, subject, "FAIL:there are a few users with that name", errString);
		errString->append("_modUser-FAIL:there are a few users with that name");
		return -4;
	}
	int objectPrivilege = getUserPrivilege(db, object, errString);
	if(objectPrivilege < 0) //OK
	{
		Log(db, "modUser", object, subject, "FAIL_ERROR-getUserPrivilege:" + to_string(objectPrivilege), errString);
		errString->append("_mod-FAIL_ERROR-userPrivilege:" + to_string(objectPrivilege));
		return -5;
	}
	if(objectPrivilege >= subjectPrivilege && object != subject || subjectPrivilege < getModUserMinPrivilege() || subjectPrivilege < newPrivilege) //OK
	{
		Log(db, "modUser", object, subject, "FAIL:the user does not have enough privileges", errString);
		errString->append("_modUser-FAIL:the user does not have enough privileges");
		return -6;
	}
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "UPDATE users SET privilege = ? WHERE name = ?", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "modUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_modUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -7;
	}
	if(!(sqlite3_bind_int(res, 1, newPrivilege) == SQLITE_OK &&
	sqlite3_bind_text(res, 2, object.c_str(), -1, SQLITE_STATIC) == SQLITE_OK)) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "modUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_modUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -8;
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_DONE) //OK
	{
		Log(db, "modUser", object, subject, "OK", errString);
		errString->append("_modUser-OK");
		sqlite3_finalize(res);
		return 0;
	}
	const char *errmsg = sqlite3_errmsg(db); //OK
	Log(db, "modUser", object, subject, "FAIL_ERROR:SQLite:" + string(errmsg), errString);
	errString->append("_modUser-FAIL_ERROR-SQLite:" + string(errmsg));
	sqlite3_finalize(res);
	return -9;
}
//name TEXT, privilege INTEGER

int addUser(sqlite3 *db, string object, string subject, int privilege, string *errString)
{
	int subjectPrivilege = getUserPrivilege(db, subject, errString);
	if(subjectPrivilege < 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL_ERROR:getUserPrivilege:" + to_string(subjectPrivilege), errString);
		errString->append("_addUser-FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege));
		return -1;
	}
	if(subjectPrivilege < getAddUserMinPrivilege() || subjectPrivilege < privilege) //OK
	{
		Log(db, "addUser", object, subject, "FAIL:the user does not have enough privileges", errString);
		errString->append("_addUser-FAIL:the user does not have enough privileges");
		return -2;
	}
	int count = userCount(db, object, errString);
	if(count > 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL:this username is already in use", errString);
		errString->append("_addUser-FAIL:this username is already in use");
		return -3;
	}
	else if(count < 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL_ERROR-userCount:" + to_string(count), errString);
		errString->append("_addUser-FAIL_ERROR-userCount:" + to_string(count));
		return -4;
	}
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db,"INSERT INTO users(name, privilege) VALUES(?, ?)", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR:SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -5;
	}
	if(!((sqlite3_bind_text(res, 1, object.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_int(res, 2, privilege) == SQLITE_OK))) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -6;
	}
	rc = sqlite3_step(res);
	if(rc != SQLITE_DONE) //TODO
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -7;
	}
	Log(db, "addUser", object, subject, "OK", errString); //OK
	errString->append("_addUser-OK");
	sqlite3_finalize(res);
	return 0;
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
	Log(db, "checkTable", "TABLE:"+ tableName, "SYSTEM", "FAIL:SQLite error:" + string(errmsg), errString);
	errString->append("_checkTable-SQLiteError:" + string(errmsg));
	sqlite3_finalize(res);
	return -8; //TODO
}

int checkTable(sqlite3 *db, tableInfo table, string *errString)
{
	return checkTable(db, table.name, table.columns, errString);
}

int init(sqlite3 *db)
{
	return 0;
}
//eventName, object, subject, eventStatus, eventDateTime

int main()
{
	sqlite3 *db;
	char *err_msg = 0;
	int rc;
	string errString;
	rc = sqlite3_open("db.db", &db);
	if(rc == SQLITE_OK)
	{
		cout << checkTable(db, LogInfo, &errString) << endl;
	}
	cout << errString << endl;
	sqlite3_close(db);
	return 0;
}
