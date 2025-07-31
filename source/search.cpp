#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <iomanip>

using namespace std;

// --- UTILITY FUNCTIONS ---
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (string::npos == start) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

string urlDecode(const string &SRC) {
    string ret;
    char ch;
    int i, ii;
    for (i = 0; i < SRC.length(); i++) {
        if (SRC[i] == '+') {
            ret += ' ';
        } else if (int(SRC[i]) == 37) {
            sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        } else {
            ret += SRC[i];
        }
    }
    return ret;
}

string json_escape(const string &s) {
    stringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= *c && *c <= '\x1f') {
                    o << "\\u" << hex << setfill('0') << setw(4) << static_cast<int>(*c);
                } else {
                    o << *c;
                }
        }
    }
    return o.str();
}

string getQueryParam(const string &query, const string &key) {
    size_t pos = query.find(key + "=");
    if (pos == string::npos) return "";
    size_t start = pos + key.length() + 1;
    size_t end = query.find("&", start);
    return urlDecode(query.substr(start, end == string::npos ? string::npos : end - start));
}

string join(const vector<string>& vec, const string& delim) {
    stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << vec[i];
        if (i < vec.size() - 1) ss << delim;
    }
    return ss.str();
}


// --- MAIN LOGIC ---
int main() {
    const string DB_PATH = "../sqlite/users.db";

    const char* request_method_cstr = getenv("REQUEST_METHOD");
    string method = request_method_cstr ? request_method_cstr : "";
    
    const char* query_string_cstr = getenv("QUERY_STRING");
    string queryStr = query_string_cstr ? query_string_cstr : "";

    string user = getQueryParam(queryStr, "user");
    string query = getQueryParam(queryStr, "query");
    string filename = getQueryParam(queryStr, "filename");

    // --- ROUTER: Direct traffic based on request type ---

    // === HANDLE SETTINGS REQUESTS ===
    if (getQueryParam(queryStr, "get_settings") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        string theme = "light";
        int suggestions_count = 10;
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT theme, suggestions_count FROM user_preferences WHERE username = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    theme = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    suggestions_count = sqlite3_column_int(stmt, 1);
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "{\"theme\":\"" << theme << "\",\"suggestions_count\":" << suggestions_count << "}";
        return 0;
    }

    if (method == "POST" && getQueryParam(queryStr, "save_settings") == "1") {
        cout << "Content-Type: text/plain\r\n\r\n";
        string requestBody;
        char c;
        while(cin.get(c)) { requestBody += c; }
        
        string theme = "light";
        size_t theme_pos = requestBody.find("\"theme\":\"");
        if(theme_pos != string::npos) {
            size_t start = theme_pos + 9;
            size_t end = requestBody.find("\"", start);
            theme = requestBody.substr(start, end - start);
        }

        int suggestions_count = 10;
        size_t count_pos = requestBody.find("\"suggestions_count\":\"");
         if(count_pos != string::npos) {
            size_t start = count_pos + 21;
            size_t end = requestBody.find("\"", start);
            try { suggestions_count = stoi(requestBody.substr(start, end - start)); }
            catch(...) { suggestions_count = 10; }
        }

        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "INSERT OR REPLACE INTO user_preferences (username, theme, suggestions_count) VALUES (?, ?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, theme.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, suggestions_count);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "Settings saved successfully!";
        return 0;
    }

    // === HANDLE FILE UPLOAD (POST) ===
    if (method == "POST" && !filename.empty()) {
        cout << "Content-Type: text/plain\r\n\r\n";
        string fileData;
        char c;
        while (cin.get(c)) { fileData += c; }

        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "INSERT INTO uploads (username, filename) VALUES (?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }

        ofstream out("../uploaded/" + filename);
        out << fileData;
        out.close();
        cout << "File uploaded successfully.";
        return 0;
    }

    // === HANDLE HISTORY REQUEST (GET) ===
    if (getQueryParam(queryStr, "history") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        vector<string> jsonRows;
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT search_term, timestamp FROM search_history WHERE username = ? ORDER BY timestamp DESC LIMIT 50;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* term_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* time_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    string term = term_cstr ? term_cstr : "";
                    string time = time_cstr ? time_cstr : "";
                    jsonRows.push_back("{\"search_term\":\"" + json_escape(term) + "\",\"timestamp\":\"" + json_escape(time) + "\"}");
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "[" << join(jsonRows, ",") << "]";
        return 0;
    }

    // === HANDLE UPLOADS LIST REQUEST (GET) ===
    if (getQueryParam(queryStr, "uploads") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        vector<string> jsonRows;
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT filename, upload_time FROM uploads WHERE username = ? ORDER BY upload_time DESC;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* fname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    const char* time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    jsonRows.push_back("{\"filename\":\"" + json_escape(string(fname)) + "\",\"upload_time\":\"" + json_escape(string(time)) + "\"}");
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "[" << join(jsonRows, ",") << "]";
        return 0;
    }

    // === HANDLE LOGGING A SEARCH (GET) ===
    if (getQueryParam(queryStr, "log") == "1") {
        cout << "Content-Type: text/plain\r\n\r\n";
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "INSERT INTO search_history (username, search_term) VALUES (?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, query.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "Logged: " << query;
        return 0;
    }
    
    // === HANDLE AUTOCOMPLETE SUGGESTIONS (GET) ===
    if (!query.empty()) {
        cout << "Content-Type: text/plain\r\n\r\n";
        vector<string> results;
        vector<string> files_to_search;
        sqlite3* db;
        int suggestions_limit = 10;

        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string pref_sql = "SELECT suggestions_count FROM user_preferences WHERE username = ?;";
            sqlite3_stmt* pref_stmt;
            if (sqlite3_prepare_v2(db, pref_sql.c_str(), -1, &pref_stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(pref_stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(pref_stmt) == SQLITE_ROW) { suggestions_limit = sqlite3_column_int(pref_stmt, 0); }
            }
            sqlite3_finalize(pref_stmt);

            string sql = "SELECT DISTINCT filename FROM uploads WHERE username = ? ORDER BY upload_time DESC;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* fname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    if (fname) files_to_search.push_back(string(fname));
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }

        string lowerQuery = query;
        transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        vector<string> found_lines;
        for (const auto& search_filename : files_to_search) {
            ifstream infile("../uploaded/" + search_filename);
            string line;
            while (getline(infile, line)) {
                line = trim(line);
                if (line.empty()) continue;
                bool already_found = false;
                for(const auto& found : found_lines) { if (found == line) { already_found = true; break; } }
                if (already_found) continue;
                string lowerLine = line;
                transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
                if (lowerLine.find(lowerQuery) != string::npos) {
                    results.push_back(" - " + line);
                    found_lines.push_back(line);
                    if (results.size() >= suggestions_limit) goto end_search;
                }
            }
        }
    end_search:
        for(const auto& res : results) { cout << res << "\n"; }
        return 0;
    }

    // Fallback for invalid requests
    cout << "Content-Type: text/plain\r\n\r\n";
    cout << "No valid request parameters provided.";
    return 0;
}
