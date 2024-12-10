#ifndef BASESQL_H
#define BASESQL_H

#include <iostream>
#include <sqlite3.h>
#include <string>
#include <ctime>
#include <vector>
#include <initializer_list>

using namespace std;

struct column {
	string name;
	string type;
	column(const string& name, const string& type);
};

struct tableInfo {
	string name;
	vector<column> columns;
	tableInfo(const string& tableName, const initializer_list<column>& cols);
	tableInfo(const string& tableName, const vector<column>& cols);
};

void textLog(sqlite3 *db, string eventName, string object, string subject, string eventStatus);

int Log(sqlite3 *db, string eventName, string object, string subject, string eventStatus, string *errString);

int checkTable(sqlite3 *db, string tableName, vector<column> columns, string *errString);

int checkTable(sqlite3 *db, tableInfo table, string *errString);

int initBaseSQL(sqlite3 **db, string databaseName, string *errString);

#endif
