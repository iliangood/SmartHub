#if !defined TG_SQL_H
#define TG_SQL_H
#include "SQL/BaseSQL.h"

int userCount(sqlite3 * db, string userID, string *errString);

int getUserPrivilege(sqlite3 *db, string object, string *errString);

int modUser(sqlite3 *db, string object, string subject, int newPrivilege, string *errString);

int addUser(sqlite3 *db, string object, string subject, int privilege, string *errString);

int deleteUser(sqlite3 *db, string object, string subject, string *errString);

int initTgSQL(sqlite3 *db, string *errString);
#endif
