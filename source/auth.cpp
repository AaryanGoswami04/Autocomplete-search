#include <iostream>
#include <string>
#include <cstdlib>
#include <sqlite3.h>
#include <algorithm>
#include <cctype>

using namespace std;

void printHeader() {
    cout << "Content-type: text/html\r\n\r\n";
}

// URL decode function to handle special characters in POST data
string urlDecode(const string& str) {
    string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            decoded += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

string getValue(const string &data, const string &key) {
    size_t start = data.find(key + "=");
    if (start == string::npos) return "";
    start += key.length() + 1;
    size_t end = data.find("&", start);
    if (end == string::npos) end = data.length();
    return urlDecode(data.substr(start, end - start));
}

void insertSampleUser(sqlite3 *db) {
    const char* sql_check = "SELECT COUNT(*) FROM users;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 0) {
            sqlite3_finalize(stmt);
            const char* insert_sql = "INSERT INTO users (username, password) VALUES ('admin', 'admin123');";
            char *errMsg = nullptr;
            if (sqlite3_exec(db, insert_sql, 0, 0, &errMsg) != SQLITE_OK) {
                cerr << "Failed to insert sample user: " << errMsg << endl;
                sqlite3_free(errMsg);
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }
}

bool userExists(sqlite3 *db, const string& username) {
    const char* sql = "SELECT COUNT(*) FROM users WHERE username=?;";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    return exists;
}

bool registerUser(sqlite3 *db, const string& username, const string& password) {
    if (userExists(db, username)) {
        return false;
    }
    
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool loginUser(sqlite3 *db, const string& username, const string& password) {
    const char* sql = "SELECT * FROM users WHERE username=? AND password=?;";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return success;
}

void printModernResponse(const string& title, const string& message, const string& type, const string& redirectUrl = "") {
    cout << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" << title << R"(</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .response-container {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
            padding: 40px;
            text-align: center;
            max-width: 400px;
            width: 100%;
        }
        .icon {
            font-size: 4rem;
            margin-bottom: 20px;
        }
        .success { color: #48bb78; }
        .error { color: #f56565; }
        h1 {
            color: #2d3748;
            font-size: 1.8rem;
            margin-bottom: 15px;
        }
        p {
            color: #718096;
            font-size: 1.1rem;
            margin-bottom: 25px;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            text-decoration: none;
            display: inline-block;
            transition: transform 0.3s ease;
        }
        .btn:hover {
            transform: translateY(-2px);
        }
        .countdown {
            color: #667eea;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="response-container">
        <div class="icon )" << type << R"(">
            )" << (type == "success" ? "✓" : "✗") << R"(
        </div>
        <h1>)" << title << R"(</h1>
        <p>)" << message << R"(</p>
        <a href="/autocomplete-search/htdocs/login.html" class="btn">Back to Login</a>
    </div>
)";

    if (!redirectUrl.empty()) {
        cout << R"(
    <script>
        let countdown = 3;
        const countdownElement = document.querySelector('.countdown');
        if (countdownElement) {
            const timer = setInterval(() => {
                countdownElement.textContent = countdown;
                countdown--;
                if (countdown < 0) {
                    clearInterval(timer);
                    window.location.href = ')" << redirectUrl << R"(';
                }
            }, 1000);
        } else {
            setTimeout(() => {
                window.location.href = ')" << redirectUrl << R"(';
            }, 3000);
        }
    </script>
)";
    }

    cout << R"(
</body>
</html>
)";
}

int main() {
    printHeader();

    char *lenstr = getenv("CONTENT_LENGTH");
    if (!lenstr) {
        printModernResponse("Error", "No data received.", "error");
        return 0;
    }

    int len = atoi(lenstr);
    string postData(len, 0);
    cin.read(&postData[0], len);

    string action = getValue(postData, "action");
    string username = getValue(postData, "username");
    string password = getValue(postData, "password");
    string confirmPassword = getValue(postData, "confirm_password");

    // Basic input validation
    if (username.empty() || password.empty()) {
        printModernResponse("Error", "Username and password are required.", "error");
        return 0;
    }

    if (username.length() < 3 || password.length() < 3) {
        printModernResponse("Error", "Username and password must be at least 3 characters long.", "error");
        return 0;
    }

    // Open database
    sqlite3 *db;
    int rc = sqlite3_open("../sqlite/users.db", &db);

    if (rc) {
        printModernResponse("Database Error", "Cannot connect to database.", "error");
        return 1;
    }

    // Create table if it doesn't exist
    const char* create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    char *errMsg = nullptr;
    if (sqlite3_exec(db, create_table_sql, 0, 0, &errMsg) != SQLITE_OK) {
        printModernResponse("Database Error", "Failed to create users table.", "error");
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    // Insert sample user if DB is empty
    insertSampleUser(db);

    if (action == "register") {
        // Registration logic
        if (password != confirmPassword) {
            printModernResponse("Registration Failed", "Passwords do not match.", "error");
            sqlite3_close(db);
            return 0;
        }

        if (registerUser(db, username, password)) {
            printModernResponse("Registration Successful", 
                              "Your account has been created successfully! You can now log in.", 
                              "success");
        } else {
            printModernResponse("Registration Failed", 
                              "Username already exists or registration failed. Please try a different username.", 
                              "error");
        }
    } else if (action == "login") {
        // Login logic
       if (loginUser(db, username, password)) {
    printModernResponse("Login Successful", 
                      "Welcome back, " + username + "! Redirecting to main page...", 
                      "success",
                      "/autocomplete-search/htdocs/index.html?user=" + username); // Add username parameter
} else {
    printModernResponse("Login Failed", 
                      "Invalid username or password. Please try again.", 
                      "error");
}
    } else {
        printModernResponse("Error", "Invalid action specified.", "error");
    }

    sqlite3_close(db);
    return 0;
}