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
#include <map>
#include <cctype> 

using namespace std;

// --- TRIE DATA STRUCTURE ---
struct TrieNode {
    map<char, TrieNode*> children;
    bool isEndOfWord = false;
};

class Trie {
    TrieNode* root;
 
    map<string, string> originalWords; 

    // Helper function to find all words from a given node
    void dfs(TrieNode* node, string currentPrefix, vector<string>& results, int limit) {
        if (results.size() >= limit) {
            return;
        }
        if (node->isEndOfWord) {
            // Use the map to retrieve the original word with its correct casing
            if (originalWords.count(currentPrefix)) {
                results.push_back(originalWords[currentPrefix]);
            }
        }
        for (const auto& pair : node->children) {
    char key = pair.first;
    TrieNode* val = pair.second;
    dfs(val, currentPrefix + key, results, limit);
    if (results.size() >= limit) {
        return;
    }
}
    }

public:
    Trie() { 
        root = new TrieNode(); 
    }

    // Inserts a word into the trie
    void insert(const string& word) {
        if (word.empty()) return;

        string lowerWord = word;
        transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
        
        // Store the original word if this lowercase version isn't already mapped
        if (originalWords.find(lowerWord) == originalWords.end()) {
            originalWords[lowerWord] = word;
        }

        TrieNode* node = root;
        for (char ch : lowerWord) {
            if (node->children.find(ch) == node->children.end()) {
                node->children[ch] = new TrieNode();
            }
            node = node->children[ch];
        }
        node->isEndOfWord = true;
    }

    // Returns a vector of suggestions for a given prefix
    vector<string> suggest(const string& prefix, int limit) {
        string lowerPrefix = prefix;
        transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

        TrieNode* node = root;
        for (char ch : lowerPrefix) {
            if (node->children.find(ch) == node->children.end()) {
                return {}; // No suggestions found
            }
            node = node->children[ch];
        }

        vector<string> results;
        dfs(node, lowerPrefix, results, limit);
        return results;
    }
};

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

// (NEW) Helper function to load words from a file into the Trie
void loadWordsIntoTrie(const string& filename, Trie& trie) {
    ifstream file(filename);
    if (!file.is_open()) {
        return; // Silently fail if file can't be opened
    }
    string word;
    while (getline(file, word)) {
        trie.insert(trim(word));
    }
    file.close();
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

    // === HANDLE PROFILE DATA REQUEST ===
    if (getQueryParam(queryStr, "get_profile") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        string password = "";
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT password FROM users WHERE username = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "{\"username\":\"" << json_escape(user) << "\",\"password\":\"" << json_escape(password) << "\"}";
        return 0;
    }

    // === HANDLE PASSWORD UPDATE (POST) ===
    if (method == "POST" && getQueryParam(queryStr, "update_password") == "1") {
        cout << "Content-Type: text/plain\r\n\r\n";
        string newPassword;
        char c;
        while(cin.get(c)) { newPassword += c; }

        sqlite3* db;
        int rc = -1;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "UPDATE users SET password = ? WHERE username = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, newPassword.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }

        if (rc == SQLITE_DONE) {
            cout << "Password updated successfully!";
        } else {
            cout << "Failed to update password.";
        }
        return 0;
    }

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

    // === HANDLE SAVING A SEARCH (POST) ===
    if (method == "POST" && getQueryParam(queryStr, "save_search") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        string term_to_save = getQueryParam(queryStr, "term");

        if (user.empty() || term_to_save.empty()) {
            cout << "{\"success\":false,\"error\":\"Missing user or term parameter\"}";
            return 0;
        }

        sqlite3* db;
        sqlite3_open(DB_PATH.c_str(), &db);
        string sql = "INSERT OR IGNORE INTO saved_searches (username, search_term) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, term_to_save.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                 if (sqlite3_changes(db) > 0) {
                    cout << "{\"success\":true,\"message\":\"Search saved successfully\"}";
                 } else {
                    cout << "{\"success\":true,\"message\":\"Search was already saved\"}";
                 }
            } else {
                cout << "{\"success\":false,\"error\":\"Failed to save search\"}";
            }
        } else {
             cout << "{\"success\":false,\"error\":\"SQL preparation failed\"}";
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    // === HANDLE SAVED SEARCHES REQUEST (GET) ===
    if (getQueryParam(queryStr, "get_saved") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        vector<string> jsonRows;
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT id, search_term, timestamp FROM saved_searches WHERE username = ? ORDER BY timestamp DESC;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int id = sqlite3_column_int(stmt, 0);
                    const char* term_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    const char* time_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                    string term = term_cstr ? term_cstr : "";
                    string time = time_cstr ? time_cstr : "";
                    jsonRows.push_back("{\"id\":" + to_string(id) + ",\"search_term\":\"" + json_escape(term) + "\",\"timestamp\":\"" + json_escape(time) + "\"}");
                }
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }
        cout << "[" << join(jsonRows, ",") << "]";
        return 0;
    }

    // === HANDLE DELETE SAVED SEARCH ITEM (POST) ===
    if (method == "POST" && getQueryParam(queryStr, "delete_saved") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        
        string saved_id = getQueryParam(queryStr, "saved_id");
        
        if (saved_id.empty()) {
            cout << "{\"success\":false,\"error\":\"Missing saved_id parameter\"}";
            return 0;
        }

        sqlite3* db;
        int rc = -1;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "DELETE FROM saved_searches WHERE id = ? AND username = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, stoi(saved_id));
                sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }

        if (rc == SQLITE_DONE) {
            cout << "{\"success\":true,\"message\":\"Saved search deleted successfully\"}";
        } else {
            cout << "{\"success\":false,\"error\":\"Failed to delete saved search\"}";
        }
        return 0;
    }

    // === HANDLE DELETE HISTORY ITEM (POST) ===
    if (method == "POST" && getQueryParam(queryStr, "delete_history") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        
        string history_id = getQueryParam(queryStr, "history_id");
        
        if (history_id.empty()) {
            cout << "{\"success\":false,\"error\":\"Missing history_id parameter\"}";
            return 0;
        }

        sqlite3* db;
        int rc = -1;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "DELETE FROM search_history WHERE id = ? AND username = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, stoi(history_id));
                sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
        }

        if (rc == SQLITE_DONE) {
            cout << "{\"success\":true,\"message\":\"History item deleted successfully\"}";
        } else {
            cout << "{\"success\":false,\"error\":\"Failed to delete history item\"}";
        }
        return 0;
    }

    // === HANDLE HISTORY REQUEST (GET) ===
    if (getQueryParam(queryStr, "history") == "1") {
        cout << "Content-Type: application/json\r\n\r\n";
        vector<string> jsonRows;
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string sql = "SELECT id, search_term, timestamp FROM search_history WHERE username = ? ORDER BY timestamp DESC LIMIT 50;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int id = sqlite3_column_int(stmt, 0);
                    const char* term_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    const char* time_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                    string term = term_cstr ? term_cstr : "";
                    string time = time_cstr ? time_cstr : "";
                    jsonRows.push_back("{\"id\":" + to_string(id) + ",\"search_term\":\"" + json_escape(term) + "\",\"timestamp\":\"" + json_escape(time) + "\"}");
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
    
    // === (MODIFIED) HANDLE AUTOCOMPLETE SUGGESTIONS (GET) USING TRIE ===
    if (!query.empty()) {
        cout << "Content-Type: text/plain\r\n\r\n";
        
        string current_filename = "";
        sqlite3* db;
        int suggestions_limit = 10;

        // 1. Get user's suggestion limit and most recent filename from the database
        if (sqlite3_open(DB_PATH.c_str(), &db) == SQLITE_OK) {
            string pref_sql = "SELECT suggestions_count FROM user_preferences WHERE username = ?;";
            sqlite3_stmt* pref_stmt;
            if (sqlite3_prepare_v2(db, pref_sql.c_str(), -1, &pref_stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(pref_stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(pref_stmt) == SQLITE_ROW) { 
                    suggestions_limit = sqlite3_column_int(pref_stmt, 0); 
                }
            }
            sqlite3_finalize(pref_stmt);

            string file_sql = "SELECT filename FROM uploads WHERE username = ? ORDER BY upload_time DESC LIMIT 1;";
            sqlite3_stmt* file_stmt;
            if (sqlite3_prepare_v2(db, file_sql.c_str(), -1, &file_stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(file_stmt, 1, user.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(file_stmt) == SQLITE_ROW) {
                    const char* fname = reinterpret_cast<const char*>(sqlite3_column_text(file_stmt, 0));
                    if (fname) current_filename = string(fname);
                }
            }
            sqlite3_finalize(file_stmt);
            sqlite3_close(db);
        }

        // 2. If a file exists, load it into the Trie and get suggestions
        if (!current_filename.empty()) {
            Trie trie;
            loadWordsIntoTrie("../uploaded/" + current_filename, trie);
            
            vector<string> results = trie.suggest(query, suggestions_limit);

            for(const auto& res : results) {
                cout << " - " << res << "\n";
            }
        }
        
        return 0;
    }

    // Fallback for invalid requests
    cout << "Content-Type: text/plain\r\n\r\n";
    cout << "No valid request parameters provided.";
    return 0;
}