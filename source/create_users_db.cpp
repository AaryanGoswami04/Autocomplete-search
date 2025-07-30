#include <iostream>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *errMsg = 0;

    // Open database (creates it if it doesn't exist)
    int rc = sqlite3_open("sqlite/users.db", &db);
    if (rc) {
        return 1;
    }

    // Create users table
    const char *userTableSQL = "CREATE TABLE IF NOT EXISTS users ("
                               "username TEXT PRIMARY KEY,"
                               "password TEXT NOT NULL);";

    rc = sqlite3_exec(db, userTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
    }

    // Create search_history table
    const char *historyTableSQL = "CREATE TABLE IF NOT EXISTS search_history ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "username TEXT NOT NULL,"
                                  "search_term TEXT NOT NULL,"
                                  "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                                  "FOREIGN KEY(username) REFERENCES users(username));";

    rc = sqlite3_exec(db, historyTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
    return 0;
}
