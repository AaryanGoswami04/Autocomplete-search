#include <iostream>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *errMsg = 0;

    // Open database (creates it if it doesn't exist)
    int rc = sqlite3_open("sqlite/users.db", &db);
    if (rc) {
        // It's good practice to print the error to stderr
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    // --- Create users table ---
    const char *userTableSQL = "CREATE TABLE IF NOT EXISTS users ("
                               "username TEXT PRIMARY KEY,"
                               "password TEXT NOT NULL);";

    rc = sqlite3_exec(db, userTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    // --- Create search_history table ---
    const char *historyTableSQL = "CREATE TABLE IF NOT EXISTS search_history ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "username TEXT NOT NULL,"
                                  "search_term TEXT NOT NULL,"
                                  "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                                  "FOREIGN KEY(username) REFERENCES users(username));";

    rc = sqlite3_exec(db, historyTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    // --- Create uploads table ---
    const char *uploadTableSQL = "CREATE TABLE IF NOT EXISTS uploads ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "username TEXT NOT NULL,"
                                 "filename TEXT NOT NULL,"
                                 "upload_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
                                 "FOREIGN KEY(username) REFERENCES users(username));";

    rc = sqlite3_exec(db, uploadTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    // --- Create user_preferences table (NEW) ---
    const char *preferencesTableSQL = "CREATE TABLE IF NOT EXISTS user_preferences ("
                                      "username TEXT PRIMARY KEY,"
                                      "theme TEXT NOT NULL DEFAULT 'light',"
                                      "suggestions_count INTEGER NOT NULL DEFAULT 10,"
                                      "FOREIGN KEY(username) REFERENCES users(username));";

    rc = sqlite3_exec(db, preferencesTableSQL, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        fprintf(stdout, "Tables created successfully\n");
    }

    sqlite3_close(db);
    return 0;
}
