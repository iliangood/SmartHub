#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>
#include "BaseSQL.h"

using namespace std;

#define ADD_USER_MIN_PRIVILEGE 100
#define MOD_USER_MIN_PRIVILEGE 100
FILE* file;

int getAddUserMinPrivilege()
{
	return ADD_USER_MIN_PRIVILEGE;
}

int getModUserMinPrivilege()
{
	return MOD_USER_MIN_PRIVILEGE;
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

int addUser(sqlite3 *db, string object, string subject, int privilege, string *errString) //TODO check subcject exist
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
	if(initBaseSQL(&db, "db.db", &errString) == 0)
	{
		string creator, username;
		int privilege;
		cin>>creator>>username>>privilege;
		addUser(db, username ,creator, privilege, &errString);
	}
	cout << errString << endl;
	sqlite3_close(db);
	return 0;
}
