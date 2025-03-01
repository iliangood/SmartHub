#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>
#include "SQL/BaseSQL.h"

#include "SQL/TgSQL.h"

using namespace std;

#define ADD_USER_MIN_PRIVILEGE 100
#define MOD_USER_MIN_PRIVILEGE 100
#define DELETE_USER_MIN_PRIVILEGE 100

int getAddUserMinPrivilege()
{
	return ADD_USER_MIN_PRIVILEGE;
}

int getModUserMinPrivilege()
{
	return MOD_USER_MIN_PRIVILEGE;
}

int getDeleteUserMinPrivilege()
{
	return DELETE_USER_MIN_PRIVILEGE;
}

int userCount(sqlite3 * db, string userID, string *errString)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE userID = ?", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "userCount", userID, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_userCount-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -1;
	}
	rc = sqlite3_bind_text(res, 1, userID.c_str(), -1, SQLITE_STATIC);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "userCount", userID, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_userCount-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -2;
	}
	rc = sqlite3_step(res);
	if(rc == SQLITE_ROW) //OK
	{
		Log(db, "userCount", userID, "", "OK", errString);
		errString->append("_userCount-OK");
		int result = sqlite3_column_int(res,0);
		sqlite3_finalize(res);
		return result;
	}
	const char *errmsg = sqlite3_errmsg(db); //TODO
	Log(db, "userCount", userID, "", "FAIL_ERROR-SQLite:" + string(errmsg), errString);
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
		Log(db, "getUserPrivilege", object, "", "FAIL:there are a few users with that userID", errString);
		errString->append("_getUserPrivilege-FAIL:there are a few users with that userID");
		return -2;
	}
	else if(count < 0) //OK
	{
		Log(db, "getUserPrivilege", object, "", "FAIL_ERROR-userCount:" + to_string(count), errString);
		errString->append("_getUserPrivilege-FAIL_ERROR-userCount:" + to_string(count));
		return -3;
	}
	int rc = sqlite3_prepare_v2(db, "SELECT userID, privilege FROM users WHERE userID = ?", -1, &res, 0);
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
		Log(db, "modUser", object, subject, "FAIL:there are a few users with that userID", errString);
		errString->append("_modUser-FAIL:there are a few users with that userID");
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
	int rc = sqlite3_prepare_v2(db, "UPDATE users SET privilege = ? WHERE userID = ?", -1, &res, 0);
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

int addUser(sqlite3 *db, string object, string subject, int privilege, string *errString)
{
	int countSubject = userCount(db, subject, errString);
	if(countSubject < 0)
	{
		Log(db, "addUser", object, subject, "FAIL_ERROR-userCount:" + to_string(countSubject), errString);
		errString->append("_addUser-FAIL_ERROR-userCount:" + to_string(countSubject));
		return -1;
	}
	if(countSubject == 0)
	{
		Log(db, "addUser", object, subject, "FAIL:user is not exist", errString);
		errString->append("_addUser-FAIL:user is not exist");
		return -2;
	}
	if(countSubject > 1)
	{
		Log(db, "addUser", object, subject, "FAIL:there are a few users with that userID", errString);
		errString->append("_addUser-FAIL:there are a few users with that userID");
		return -3;
	}
	int subjectPrivilege = getUserPrivilege(db, subject, errString);
	if(subjectPrivilege < 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege), errString);
		errString->append("_addUser-FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege));
		return -4;
	}
	if(subjectPrivilege < getAddUserMinPrivilege() || subjectPrivilege < privilege) //OK
	{
		Log(db, "addUser", object, subject, "FAIL:the user does not have enough privileges", errString);
		errString->append("_addUser-FAIL:the user does not have enough privileges");
		return -5;
	}
	int count = userCount(db, object, errString);
	if(count > 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL:this userID is already in use", errString);
		errString->append("_addUser-FAIL:this userID is already in use");
		return -6;
	}
	else if(count < 0) //OK
	{
		Log(db, "addUser", object, subject, "FAIL_ERROR-userCount:" + to_string(count), errString);
		errString->append("_addUser-FAIL_ERROR-userCount:" + to_string(count));
		return -6;
	}
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db,"INSERT INTO users(userID, privilege) VALUES(?, ?)", -1, &res, 0);
	if(rc != SQLITE_OK) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR:SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -7;
	}
	if(!((sqlite3_bind_text(res, 1, object.c_str(), -1, SQLITE_STATIC) == SQLITE_OK) &&
	(sqlite3_bind_int(res, 2, privilege) == SQLITE_OK))) //OK
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -8;
	}
	rc = sqlite3_step(res);
	if(rc != SQLITE_DONE) //TODO
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "addUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString);
		errString->append("_addUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -9;
	}
	Log(db, "addUser", object, subject, "OK", errString); //OK
	errString->append("_addUser-OK");
	sqlite3_finalize(res);
	return 0;
}

int deleteUser(sqlite3 *db, string object, string subject, string *errString)
{
	int countSubject = userCount(db, subject, errString);
	if(countSubject < 0)
	{
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-userCount:" + to_string(countSubject), errString);
		errString->append("_deleteUser-FAIL_ERROR-userCount:" + to_string(countSubject));
		return -1;
	}
	if(countSubject == 0)
	{
		Log(db, "deleteUser", object, subject, "FAIL:user is not exist", errString);
		errString->append("_deleteUser-FAIL:user is not exist");
		return -2;
	}
	if(countSubject > 1)
	{
		Log(db, "deleteUser", object, subject, "FAIL:there are a few users with that userID", errString);
		errString->append("_deleteUser-FAIL:there are a few users with that userID");
		return -3;
	}
	int countObject = userCount(db, object, errString);
	if(countObject < 0)
	{
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-userCount:" + to_string(countSubject), errString);
		errString->append("_deleteUser-FAIL_ERROR-userCount:" + to_string(countSubject));
		return -4;
	}
	if(countObject == 0)
	{
		Log(db, "deleteUser", object, subject, "FAIL:object is not exist", errString);
		errString->append("_deleteUser-FAIL:object is not exist");
		return -5;
	}
	int objectPrivilege = getUserPrivilege(db, object, errString);
	if(objectPrivilege < 0) //OK
	{
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-getUserPrivilege:" + to_string(objectPrivilege), errString);
		errString->append("_deleteUser-FAIL_ERROR-userPrivilege:" + to_string(objectPrivilege));
		return -6;
	}
	int subjectPrivilege = getUserPrivilege(db, subject, errString);
	if(subjectPrivilege < 0) //OK
	{
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege), errString);
		errString->append("_deleteUser-FAIL_ERROR-getUserPrivilege:" + to_string(subjectPrivilege));
		return -7;
	}
	if(subjectPrivilege < getDeleteUserMinPrivilege() || subjectPrivilege < objectPrivilege) //OK
	{
		Log(db, "deleteUser", object, subject, "FAIL:the user does not have enough privileges", errString);
		errString->append("_deleteUser-FAIL:the user does not have enough privileges");
		return -6;
	}
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db,"DELETE FROM users WHERE userID = ?", -1, &res, 0);
	if(!(sqlite3_bind_text(res, 1, object.c_str(), -1, SQLITE_STATIC) == SQLITE_OK))
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString); //OK
		errString->append("_deleteUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -9;
	}
	sqlite3_step(res);
	if(rc != SQLITE_OK)
	{
		const char *errmsg = sqlite3_errmsg(db);
		Log(db, "deleteUser", object, subject, "FAIL_ERROR-SQLite:" + string(errmsg), errString); //OK
		errString->append("_deleteUser-FAIL_ERROR-SQLite:" + string(errmsg));
		sqlite3_finalize(res);
		return -10;
	}
	Log(db, "deleteUser", object, subject, "OK", errString); //OK
	errString->append("_deleteUser-OK");
	sqlite3_finalize(res);
	return 0;
}

tableInfo usersInfo("users", {column("id", "INTEGER"), column("userID", "TEXT"), column("privilege", "INTEGER")});

int initTgSQL(sqlite3 *db, string *errString)
{
	int rc = checkTable(db, usersInfo, errString);
	if(rc > 0)
	{
		textLog(db, "initTgSQL", "TABLE:users", "SYSTEM", "FAIL:table in an unexpected way");
		errString->append("_initTgSQL-FAIL:table in an unexpected way");
		return 1;
	}
	if(rc < 0)
	{
		textLog(db, "initTgSQL", "TABLE:users", "SYSTEM", "FAIL_ERROR-ckeckTable:" + to_string(rc));
		errString->append("_initTgSQLFAIL_ERROR-ckeckTable:" + to_string(rc));
		return -1;
	}
	Log(db, "initTgSQL", "DATABASE", "SYSTEM", "OK", errString);
	errString->append("_initTgSQL-OK");
	return 0;
}

/*int main()
{
	sqlite3 *db;
	char *err_msg = 0;
	int rc;
	string errString;
	if(initBaseSQL(&db, "db.db", &errString) != 0)
	{
		cout << errString << endl;
		sqlite3_close(db);
		return 1;
	}
	if(initTgSQL(db, &errString) != 0)
	{
		cout << errString << endl;
		sqlite3_close(db);
		return 2;
	}
	string creator, userID;
	int privilege;
	cin>>creator>>userID;
	rc = deleteUser(db, userID ,creator, &errString);
	cout << errString << endl;
	printf("code:%d\n", rc);
	sqlite3_close(db);
	return 0;
}*/
