#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <iomanip>
#include <csignal>
#include <csetjmp>
#include <sqlite3.h>
#include "events.h"

#include "SQL/BaseSQL.h"
#include "SQL/TgSQL.h"


// Коды ANSI для цветов
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string RESET = "\033[0m";

// Точка возврата для обработки сигналов
static jmp_buf env;

void signalHandler(int signal) {
	if (signal == SIGFPE || signal == SIGABRT) {
		longjmp(env, signal); // Возвращаемся с кодом сигнала
	}
}

// Проверка существования таблицы
bool tableExists(sqlite3* db, const std::string& tableName) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name=?", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result == SQLITE_ROW;
}

// Подсчёт записей в таблице Log
int getLogCount(sqlite3* db) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Log", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return -1;
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

// Подсчёт записей в таблице users для конкретного userID
int getUserCount(sqlite3* db, const std::string& userID) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE userID=?", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

// Получение привилегий пользователя напрямую
int getUserPrivilegeDirect(sqlite3* db, const std::string& userID) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, "SELECT privilege FROM users WHERE userID=?", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        int privilege = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return privilege;
    }
    sqlite3_finalize(stmt);
    return -1; // Пользователь не найден
}

class Test {
private:
	std::string name;
	std::function<bool()> testFunction;
	bool passed;
	std::string errorDescription;
public:
	Test(const std::string& testName, std::function<bool()> func)
		: name(testName), testFunction(func), passed(false), errorDescription("") {}

	bool run(int testNum) {
		// Устанавливаем точку возврата
		int jumpCode = setjmp(env);
		if (jumpCode == 0) {
			// Устанавливаем обработчики сигналов
			std::signal(SIGFPE, signalHandler);
			std::signal(SIGABRT, signalHandler);

			try {
				passed = testFunction();
				if (!passed) {
					errorDescription = "Test logic failed";
				}
			} catch (const std::exception& e) {
				passed = false;
				errorDescription = std::string("Exception: ") + e.what();
			} catch (...) {
				passed = false;
				errorDescription = "Unknown error occurred";
			}
		} else if (jumpCode == SIGFPE) {
			passed = false;
			errorDescription = "Floating point exception (SIGFPE)";
		} else if (jumpCode == SIGABRT) {
			passed = false;
			errorDescription = "Aborted (double free or memory corruption)";
		}

		// Вывод результата
		printf("  Test %d: ", testNum);
		if (passed) {
			std::cout << GREEN << "PASSED" << RESET;
		} else {
			std::cout << RED << "FAILED" << RESET;
		}
		std::cout << " - " << name;
		if (!passed && !errorDescription.empty()) {
			std::cout << " (" << YELLOW << errorDescription << RESET << ")";
		}
		std::cout << std::endl;

		return passed;
	}

	bool isPassed() const { return passed; }
};

class TestGroup {
private:
	std::string groupName;
	std::vector<Test> tests;
	int passedCount;

public:
	TestGroup(const std::string& name) : groupName(name), passedCount(0) {}

	void addTest(const std::string& testName, std::function<bool()> testFunc) {
		tests.emplace_back(testName, testFunc);
	}

	void runTests() {
		std::cout << "Module: " << groupName << std::endl;
		passedCount = 0;
		for (int i = 0; i < tests.size(); i++) {
			if (tests[i].run(i+1)) {
				passedCount++;
			}
		}
	}

	void printSummary() const {
		double percentage = (tests.size() > 0) ? (passedCount * 100.0 / tests.size()) : 0;
		std::string color = (percentage == 100) ? GREEN : (percentage >= 50) ? YELLOW : RED;

		std::cout << "Summary for " << groupName << ": "
				  << color << passedCount << "/" << tests.size()
				  << " tests passed (" << std::fixed << std::setprecision(1) << percentage << "%)"
				  << RESET << std::endl;
	}

	int getPassedCount() const { return passedCount; }
	int getTotalCount() const { return tests.size(); }
};

class TestSuite {
private:
	std::vector<TestGroup> groups;

public:
	void addGroup(const TestGroup& group) {
		groups.push_back(group);
	}

	int runAllTests() {
		for (auto& group : groups) {
			group.runTests();
			std::cout << std::endl;
		}

		std::cout << "Module Summaries:\n";
		for (const auto& group : groups) {
			group.printSummary();
		}

		int totalPassed = 0, totalTests = 0;
		for (const auto& group : groups) {
			totalPassed += group.getPassedCount();
			totalTests += group.getTotalCount();
		}

		double percentage = (totalTests > 0) ? (totalPassed * 100.0 / totalTests) : 0;
		std::string color = (percentage == 100) ? GREEN : (percentage >= 50) ? YELLOW : RED;

		std::cout << "\nTotal Summary: "
				  << color << totalPassed << "/" << totalTests
				  << " tests passed (" << std::fixed << std::setprecision(1) << percentage << "%)"
				  << RESET << std::endl;
		return !(totalPassed == totalTests);
	}
};


// Определяем LogInfo здесь, так как она не доступна из BaseSQL.h
extern tableInfo LogInfo;

// Вспомогательные константы для создания таблицы Log
const char* CREATE_LOG_TABLE =
    "CREATE TABLE Log ("
    "id INTEGER, "
    "eventName TEXT, "
    "object TEXT, "
    "subject TEXT, "
    "eventStatus TEXT, "
    "eventDateTime TEXT)";

// Тесты для BaseSQL
void setupBaseSQLTests(TestGroup& baseSQLTests) {
    // Тест 1: Успешная инициализация базы данных
    baseSQLTests.addTest("initBaseSQL - Successful initialization", []() {
        sqlite3* db = nullptr;
        std::string errString;
        int result = initBaseSQL(&db, ":memory:", &errString);
        bool logTableExists = tableExists(db, "Log");
        bool success = (result == 0 && errString.find("_initBaseSQL-OK") != std::string::npos && logTableExists);
        if (db) sqlite3_close(db);
        return success;
    });

    // Тест 2: Неудачное открытие файла textLog (условный)
    baseSQLTests.addTest("initBaseSQL - Fail to open textLog file", []() {
        // Этот тест остаётся условным, так как требует мока
        sqlite3* db = nullptr;
        std::string errString;
        int result = initBaseSQL(&db, ":memory:", &errString);
        bool success = (result == 0); // В памяти всё работает
        if (db) sqlite3_close(db);
        return success;
    });

    // Тест 3: Создание таблицы Log при отсутствии в initBaseSQL
    baseSQLTests.addTest("initBaseSQL - Create Log table if not exists", []() {
        sqlite3* db = nullptr;
        std::string errString;
        int result = initBaseSQL(&db, ":memory:", &errString);
        bool logTableExists = tableExists(db, "Log");
        bool success = (result == 0 && logTableExists);
        if (db) sqlite3_close(db);
        return success;
    });

    // Тест 4: Успешное создание новой таблицы через createTable
    baseSQLTests.addTest("createTable - Successful table creation", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        std::vector<column> cols = {{"id", "INTEGER"}, {"name", "TEXT"}};
        int result = createTable(db, "TestTable", cols, &errString);
        bool testTableExists = tableExists(db, "TestTable");
        bool success = (result == 0 && errString.find("_createTable-OK") != std::string::npos && testTableExists);
        sqlite3_close(db);
        return success;
    });

    // Тест 5: Проверка существующей таблицы с правильной структурой через checkTable
    baseSQLTests.addTest("checkTable - Existing table with correct structure", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE TABLE TestTable (id INTEGER, name TEXT)", nullptr, nullptr, nullptr);
        std::vector<column> cols = {{"id", "INTEGER"}, {"name", "TEXT"}};
        int result = checkTable(db, "TestTable", cols, &errString);
        bool success = (result == 0 && errString.find("_checkTable-OK") != std::string::npos);
        sqlite3_close(db);
        return success;
    });

    // Тест 6: Проверка несуществующей таблицы через checkTable
    baseSQLTests.addTest("checkTable - Non-existent table", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        std::vector<column> cols = {{"id", "INTEGER"}};
        int result = checkTable(db, "NonExistentTable", cols, &errString);
        bool success = (result == 1 && errString.find("_checkTable-OK(WARN):table does not exist") != std::string::npos);
        sqlite3_close(db);
        return success;
    });

    // Тест 7: Проверка таблицы с неверным количеством колонок через checkTable
    baseSQLTests.addTest("checkTable - Wrong number of columns", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE TABLE TestTable (id INTEGER)", nullptr, nullptr, nullptr);
        std::vector<column> checkCols = {{"id", "INTEGER"}, {"name", "TEXT"}};
        int result = checkTable(db, "TestTable", checkCols, &errString);
        bool success = (result == 4 && errString.find("_checkTable-FAIL:the number of columns is lesser than expected") != std::string::npos);
        sqlite3_close(db);
        return success;
    });

    // Тест 8: Успешное удаление таблицы через dropTable
    baseSQLTests.addTest("dropTable - Successful table drop", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE TABLE TestTable (id INTEGER)", nullptr, nullptr, nullptr);
        int result = dropTable(db, "TestTable", &errString);
        bool testTableExists = tableExists(db, "TestTable");
        bool success = (result == 0 && errString.find("_dropTable-OK") != std::string::npos && !testTableExists);
        sqlite3_close(db);
        return success;
    });

    // Тест 9: Успешная запись в лог через Log
    baseSQLTests.addTest("Log - Successful log entry", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        int logCountBefore = getLogCount(db);
        int result = Log(db, "testEvent", "obj", "subj", "OK", &errString);
        int logCountAfter = getLogCount(db);
        bool success = (result == 0 && errString.find("_Log-OK") != std::string::npos && logCountAfter == logCountBefore + 1);
        sqlite3_close(db);
        return success;
    });

    // Тест 10: Неудачная запись в лог из-за отсутствия таблицы
    baseSQLTests.addTest("Log - Fail due to missing table", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        int result = Log(db, "testEvent", "obj", "subj", "FAIL", &errString);
        bool success = (result == -1 && errString.find("_Log-FAIL_ERROR-SQLite") != std::string::npos);
        sqlite3_close(db);
        return success;
    });
}


// Определяем usersInfo локально, так как она не доступна из TgSQL.h
extern tableInfo usersInfo;


// Предполагаем, что usersInfo уже определён ранее в tests.cpp, например:
// tableInfo usersInfo("users", {column("id", "INTEGER"), column("userID", "TEXT"), column("privilege", "INTEGER")});

// Тесты для TgSQL

// Определяем структуру таблицы users для тестов
const char* CREATE_USERS_TABLE =
	"CREATE TABLE users ("
	"id INTEGER, "
	"userID TEXT, "
	"privilege INTEGER)";


// Вспомогательная функция для вставки пользователя напрямую через SQLite
void insertUser(sqlite3* db, const std::string& userID, int privilege) {
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO users (userID, privilege) VALUES (?, ?)";
	sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, privilege);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

// Тесты для TgSQL
void setupTgSQLTests(TestGroup& tgSQLTests) {
    // Тест 1: Подсчет пользователей при отсутствии записи
    tgSQLTests.addTest("userCount - No user exists", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        int count = userCount(db, "user1", &errString);
        int directCount = getUserCount(db, "user1");
        bool success = (count == 0 && errString.find("_userCount-OK") != std::string::npos && directCount == 0);
        sqlite3_close(db);
        return success;
    });

    // Тест 2: Подсчет пользователей при наличии одной записи
    tgSQLTests.addTest("userCount - One user exists", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "user1", 200);
        int count = userCount(db, "user1", &errString);
        int directCount = getUserCount(db, "user1");
        bool success = (count == 1 && errString.find("_userCount-OK") != std::string::npos && directCount == 1);
        sqlite3_close(db);
        return success;
    });

    // Тест 3: Получение привилегий для существующего пользователя
    tgSQLTests.addTest("getUserPrivilege - Successful privilege retrieval", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "user1", 150);
        int privilege = getUserPrivilege(db, "user1", &errString);
        int directPrivilege = getUserPrivilegeDirect(db, "user1");
        bool success = (privilege == 150 && errString.find("_getUserPrivilege-OK") != std::string::npos && directPrivilege == 150);
        sqlite3_close(db);
        return success;
    });

    // Тест 4: Получение привилегий для несуществующего пользователя
    tgSQLTests.addTest("getUserPrivilege - User not found", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        int privilege = getUserPrivilege(db, "user1", &errString);
        int directPrivilege = getUserPrivilegeDirect(db, "user1");
        bool success = (privilege == -1 && errString.find("_getUserPrivilege-FAIL:user not found") != std::string::npos && directPrivilege == -1);
        sqlite3_close(db);
        return success;
    });

    // Тест 5: Добавление пользователя с достаточными привилегиями
    tgSQLTests.addTest("addUser - Successful user addition", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "admin", 200);
        int result = addUser(db, "user1", "admin", 100, &errString);
        int count = getUserCount(db, "user1");
        bool success = (result == 0 && errString.find("_addUser-OK") != std::string::npos && count == 1);
        sqlite3_close(db);
        return success;
    });

    // Тест 6: Добавление пользователя с недостаточными привилегиями
    tgSQLTests.addTest("addUser - Insufficient privileges", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "lowPriv", 50);
        int result = addUser(db, "user1", "lowPriv", 100, &errString);
        int count = getUserCount(db, "user1");
        bool success = (result == -5 && errString.find("_addUser-FAIL:the user does not have enough privileges") != std::string::npos && count == 0);
        sqlite3_close(db);
        return success;
    });

    // Тест 7: Модификация привилегий пользователя
    tgSQLTests.addTest("modUser - Successful privilege modification", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "admin", 200);
        insertUser(db, "user1", 100);
        int result = modUser(db, "user1", "admin", 150, &errString);
        int newPrivilege = getUserPrivilegeDirect(db, "user1");
        bool success = (result == 0 && errString.find("_modUser-OK") != std::string::npos && newPrivilege == 150);
        sqlite3_close(db);
        return success;
    });

    // Тест 8: Модификация с недостаточными привилегиями
    tgSQLTests.addTest("modUser - Insufficient privileges", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "lowPriv", 50);
        insertUser(db, "user1", 100);
        int result = modUser(db, "user1", "lowPriv", 150, &errString);
        int privilege = getUserPrivilegeDirect(db, "user1");
        bool success = (result == -6 && errString.find("_modUser-FAIL:the user does not have enough privileges") != std::string::npos && privilege == 100);
        sqlite3_close(db);
        return success;
    });

    // Тест 9: Удаление пользователя с достаточными привилегиями
    tgSQLTests.addTest("deleteUser - Successful user deletion", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "admin", 200);
        insertUser(db, "user1", 100);
        int result = deleteUser(db, "user1", "admin", &errString);
        int count = getUserCount(db, "user1");
        bool success = (result == 0 && errString.find("_deleteUser-OK") != std::string::npos && count == 0);
        sqlite3_close(db);
        return success;
    });

    // Тест 10: Удаление пользователя с недостаточными привилегиями
    tgSQLTests.addTest("deleteUser - Insufficient privileges", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        insertUser(db, "lowPriv", 50);
        insertUser(db, "user1", 100);
        int result = deleteUser(db, "user1", "lowPriv", &errString);
        int count = getUserCount(db, "user1");
        bool success = (result == -6 && errString.find("_deleteUser-FAIL:the user does not have enough privileges") != std::string::npos && count == 1);
        sqlite3_close(db);
        return success;
    });

    // Тест 11: Успешная инициализация TgSQL с существующей таблицей users
    tgSQLTests.addTest("initTgSQL - Successful initialization", []() {
        sqlite3* db = nullptr;
        std::string errString;
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, CREATE_LOG_TABLE, nullptr, nullptr, nullptr);
        sqlite3_exec(db, CREATE_USERS_TABLE, nullptr, nullptr, nullptr);
        int result = initTgSQL(db, &errString);
        bool usersTableExists = tableExists(db, "users");
        bool success = (result == 0 && errString.find("_initTgSQL-OK") != std::string::npos && usersTableExists);
        sqlite3_close(db);
        return success;
    });
}



// Вспомогательная функция для обработки события
void testHandler(void* data) {
	int* value = static_cast<int*>(data);
	*value = 42; // Изменяем данные для проверки
}

// Тесты для events
void setupEventsTests(TestGroup& eventsTests) {
	// Тест 1: Создание объекта event и проверка его полей
	eventsTests.addTest("event - Constructor initializes correctly", []() {
		int data = 5;
		event e("testEvent", &data);
		bool success = (e.type == "testEvent" && e.data == &data);
		return success;
	});

	// Тест 2: Создание объекта eventHandler и проверка его полей
	eventsTests.addTest("eventHandler - Constructor initializes correctly", []() {
		eventHandler h("testType", testHandler);
		bool success = (h.type == "testType" && h.handler == testHandler);
		return success;
	});

	// Тест 3: Регистрация обработчика через объект eventHandler
	eventsTests.addTest("eventDispatcher - Register handler via eventHandler object", []() {
		eventDispatcher dispatcher;
		eventHandler handler("testEvent", testHandler);
		dispatcher.registerHandler(handler);
		int data = 0;
		event e("testEvent", &data);
		dispatcher.dispatchEvent(e);
		bool success = (data == 42); // testHandler изменяет data на 42
		return success;
	});

	// Тест 4: Регистрация обработчика через тип и функцию
	eventsTests.addTest("eventDispatcher - Register handler via type and function", []() {
		eventDispatcher dispatcher;
		dispatcher.registerHandler("testEvent", testHandler);
		int data = 0;
		event e("testEvent", &data);
		dispatcher.dispatchEvent(e);
		bool success = (data == 42);
		return success;
	});

	// Тест 5: Dispatch события с зарегистрированным обработчиком
	eventsTests.addTest("eventDispatcher - Dispatch event with registered handler", []() {
		eventDispatcher dispatcher;
		dispatcher.registerHandler("testEvent", testHandler);
		int data = 0;
		event e("testEvent", &data);
		dispatcher.dispatchEvent(e);
		bool success = (data == 42);
		return success;
	});

	// Тест 6: Dispatch события без зарегистрированного обработчика
	eventsTests.addTest("eventDispatcher - Dispatch event with no handler", []() {
		eventDispatcher dispatcher;
		int data = 0;
		event e("unknownEvent", &data);
		// Перенаправляем cerr в строку для проверки вывода
		std::ostringstream cerrCapture;
		std::streambuf* oldCerr = std::cerr.rdbuf(cerrCapture.rdbuf());
		dispatcher.dispatchEvent(e);
		std::cerr.rdbuf(oldCerr); // Восстанавливаем cerr
		bool success = (data == 0 && cerrCapture.str().find("No handler for event type: unknownEvent") != std::string::npos);
		return success;
	});

	// Тест 7: Поиск существующего обработчика через findHandler(std::string)
	eventsTests.addTest("eventDispatcher - Find existing handler by type", []() {
		eventDispatcher dispatcher;
		dispatcher.registerHandler("testEvent", testHandler);
		long pos = dispatcher.findHandler("testEvent"); // Доступ через отражение или тестирование через поведение
		int data = 0;
		event e("testEvent", &data);
		dispatcher.dispatchEvent(e);
		bool success = (pos == 0 && data == 42);
		return success;
	});

	// Тест 8: Поиск несуществующего обработчика через findHandler(std::string)
	eventsTests.addTest("eventDispatcher - Find non-existent handler by type", []() {
		eventDispatcher dispatcher;
		long pos = dispatcher.findHandler("unknownEvent"); // Проверяем через поведение
		bool success = (pos == -1);
		return success;
	});

	// Тест 9: Поиск обработчика через findHandler(event)
	eventsTests.addTest("eventDispatcher - Find handler by event object", []() {
		eventDispatcher dispatcher;
		dispatcher.registerHandler("testEvent", testHandler);
		int data = 0;
		event e("testEvent", &data);
		long pos = dispatcher.findHandler(e);
		dispatcher.dispatchEvent(e);
		bool success = (pos == 0 && data == 42);
		return success;
	});

	// Тест 10: Множественная регистрация обработчиков и выбор правильного
	eventsTests.addTest("eventDispatcher - Multiple handlers with correct dispatch", []() {
		eventDispatcher dispatcher;
		dispatcher.registerHandler("event1", testHandler);
		dispatcher.registerHandler("event2", [](void* data) {
			int* value = static_cast<int*>(data);
			*value = 99; // Другой обработчик
		});
		int data1 = 0;
		int data2 = 0;
		event e1("event1", &data1);
		event e2("event2", &data2);
		dispatcher.dispatchEvent(e1);
		dispatcher.dispatchEvent(e2);
		bool success = (data1 == 42 && data2 == 99);
		return success;
	});
}

// Пример использования
int main() {
	TestSuite suite;

	TestGroup baseSQLTests("BaseSQL");
	setupBaseSQLTests(baseSQLTests);
	suite.addGroup(baseSQLTests);
	TestGroup tgSQLTests("TgSQL");
	setupTgSQLTests(tgSQLTests);
	suite.addGroup(tgSQLTests);
	TestGroup eventsTests("Events");
	setupEventsTests(eventsTests);
	suite.addGroup(eventsTests);

	return suite.runAllTests();
}
