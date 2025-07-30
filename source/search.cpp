#include <iostream>
#include <cstdlib>
#include <ctime>
extern "C" {
    #define new new_
    #include "sqlite3.h"
    #undef new
}
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <cctype>
using namespace std;

struct TrieNode {
    map<char, TrieNode*> children;
    bool isEndOfWord = false;
};

class Trie {
    TrieNode* root;
    map<string, string> originalWords;

    void dfs(TrieNode* node, string current, vector<string>& results) {
        if (node->isEndOfWord) {
            if (originalWords.count(current))
                results.push_back(originalWords[current]);
            else
                results.push_back(current);
        }
        for (auto& pair : node->children) {
            char ch = pair.first;
            TrieNode* child = pair.second;
            dfs(child, current + ch, results);
        }
    }

public:
    Trie() { root = new TrieNode(); }

    void insert(const string& word) {
        string lowerWord = word;
        transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
        originalWords[lowerWord] = word;

        TrieNode* node = root;
        for (char ch : lowerWord) {
            if (!node->children[ch])
                node->children[ch] = new TrieNode();
            node = node->children[ch];
        }
        node->isEndOfWord = true;
    }

    vector<string> suggest(const string& prefix) {
        string lowerPrefix = prefix;
        transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

        TrieNode* node = root;
        for (char ch : lowerPrefix) {
            if (!node->children.count(ch)) return {};
            node = node->children[ch];
        }

        vector<string> results;
        dfs(node, lowerPrefix, results);
        return results;
    }
};

string trim(const string& str) {
    size_t start = 0, end = str.size();
    while (start < end && isspace(static_cast<unsigned char>(str[start]))) ++start;
    while (end > start && isspace(static_cast<unsigned char>(str[end - 1]))) --end;
    return str.substr(start, end - start);
}

// URL decode helper
static string urlDecode(const string& s) {
    string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int v = 0;
            std::istringstream is(s.substr(i + 1, 2));
            is >> std::hex >> v;
            out.push_back(static_cast<char>(v));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

// Ensure user exists in database (create if doesn't exist)
// Ensure user exists in database (create if doesn't exist)
void ensureUserExists(sqlite3* db, const string& username) {
    const char* checkUserSQL = "SELECT username FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, checkUserSQL, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            
            const char* insertUserSQL = "INSERT OR IGNORE INTO users (username, password) VALUES (?, 'default')";
            if (sqlite3_prepare_v2(db, insertUserSQL, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
            }
        }
        sqlite3_finalize(stmt);
    }
}

void handleSearchHistory(const string& username, const string& basePath) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    
    string dbPath = basePath + "sqlite/users.db";
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        cout << "Content-Type: application/json\r\n\r\n";
        cout << "{\"error\": \"Database connection failed: " << sqlite3_errmsg(db) << "\"}\n";
        return;
    }
    
    const char* sql = "SELECT search_term, datetime(timestamp, 'localtime') as formatted_timestamp FROM search_history "
                      "WHERE username = ? "
                      "ORDER BY timestamp DESC "
                      "LIMIT 20";
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cout << "Content-Type: application/json\r\n\r\n";
        cout << "{\"error\": \"SQL prepare failed: " << sqlite3_errmsg(db) << "\"}\n";
        sqlite3_close(db);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    cout << "Content-Type: application/json\r\n\r\n";
    cout << "[\n";
    
    bool first = true;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (!first) {
            cout << ",\n";
        }
        first = false;
        
        const char* search_term = (const char*)sqlite3_column_text(stmt, 0);
        const char* timestamp = (const char*)sqlite3_column_text(stmt, 1);
        
        cout << "  {\n";
        cout << "    \"search_term\": \"" << (search_term ? search_term : "") << "\",\n";
        cout << "    \"timestamp\": \"" << (timestamp ? timestamp : "") << "\"\n";
        cout << "  }";
    }
    
    cout << "\n]\n";
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void loadWordsFromFile(const string& filename, Trie& trie) {
    ifstream file(filename);
    string word;
    if (!file.is_open()) {
        return;
    }

    bool first = true;
    while (getline(file, word)) {
        word = trim(word);

        if (first && !word.empty() &&
            static_cast<unsigned char>(word[0]) == 0xEF &&
            word.size() >= 3 &&
            static_cast<unsigned char>(word[1]) == 0xBB &&
            static_cast<unsigned char>(word[2]) == 0xBF) {
            word = word.substr(3);
        }
        first = false;

        if (!word.empty() && word.back() == '\r') word.pop_back();

        if (!word.empty())
            trie.insert(word);
    }
    file.close();
}

string getQueryParam(const string& query, const string& key) {
    size_t pos = query.find(key + "=");
    if (pos == string::npos) return "";
    size_t start = pos + key.size() + 1;
    size_t end = query.find('&', start);
    string raw = query.substr(start, end - start);
    return urlDecode(raw);
}

int main() {
    string basePath = "C:/xampp/htdocs/autocomplete-search/";
 
    string requestMethod = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";

    if (requestMethod == "POST") {
        cout << "Content-type: text/plain\r\n\r\n";

        string contentLengthStr = getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0";
        int contentLength = stoi(contentLengthStr);

        string body(contentLength, '\0');
        cin.read(&body[0], contentLength);

        ofstream out(basePath + "words.txt");
        istringstream stream(body);
        string line;
        bool first = true;
        while (getline(stream, line)) {
            line = trim(line);

            if (first && !line.empty() &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                line.size() >= 3 &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            first = false;

            line.erase(remove(line.begin(), line.end(), '\0'), line.end());
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (!line.empty()) {
                out << line << "\n";
            }
        }

        out.close();
        cout << "File uploaded successfully\n";
        return 0;
    }

    string queryStr = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
    
    if (queryStr.find("history=1") != string::npos) {
        string username = getQueryParam(queryStr, "user");
        if (username.empty()) username = "guest";
        handleSearchHistory(username, basePath);
        return 0;
    }
    
    cout << "Content-type: text/plain\r\n\r\n";
    
    string keyword = getQueryParam(queryStr, "query");
    string username = getQueryParam(queryStr, "user");
    string logFlag = getQueryParam(queryStr, "log");

    if (username.empty()) username = "guest";
    if (!keyword.empty() && keyword.back() == '\r') keyword.pop_back();

    Trie trie;
    loadWordsFromFile(basePath + "words.txt", trie);

    if (keyword.empty()) {
        cout << "No query provided.";
    } else {
        vector<string> results = trie.suggest(keyword);
        if (results.empty()) {
            cout << "No autocomplete suggestions for: " << keyword;
        } else {
            for (const string& word : results) {
                cout << " - " << word << "\n";
            }
        }

        if (logFlag == "1" && keyword.size() >= 1) {
            sqlite3* db;
            string dbPath = basePath + "sqlite/users.db";
            int rc = sqlite3_open(dbPath.c_str(), &db);
            if (rc == SQLITE_OK) {
                sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
                ensureUserExists(db, username);
                const char* insertSQL = "INSERT INTO search_history (username, search_term) VALUES (?, ?);";
                sqlite3_stmt* stmt;
                if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmt, 2, keyword.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(db);
            }
        }
    }

    return 0;
}
