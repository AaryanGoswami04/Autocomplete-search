#include <iostream>
#include <cstdlib>
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

// --- URL decode helper (fixes %0D etc.) ---
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

void loadWordsFromFile(const string& filename, Trie& trie) {
    ifstream file(filename);
    string word;
    if (!file.is_open()) {
        cout << "Could not open file: " << filename << "\n";
        return;
    }

    bool first = true;
    while (getline(file, word)) {
        word = trim(word);

        // Remove UTF-8 BOM only on first line if present
        if (first && !word.empty() &&
            static_cast<unsigned char>(word[0]) == 0xEF &&
            word.size() >= 3 &&
            static_cast<unsigned char>(word[1]) == 0xBB &&
            static_cast<unsigned char>(word[2]) == 0xBF) {
            word = word.substr(3);
        }
        first = false;

        // Remove stray CR (carriage return) if present
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
    return urlDecode(raw); // decode percent-encoded
}

int main() {
    string basePath = "C:/xampp/htdocs/autocomplete-search/";
    ofstream log(basePath + "debug.txt");
    log << "CGI Executed\n";
    log.close();

    string requestMethod = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";

    // ------------------ POST: upload words file ------------------
    if (requestMethod == "POST") {
        cout << "Content-type: text/plain\n\n";

        string contentLengthStr = getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0";
        int contentLength = stoi(contentLengthStr);

        string body(contentLength, '\0');
        cin.read(&body[0], contentLength);

        ofstream debugLog(basePath + "debug.txt", ios::app);
        debugLog << "=== POST Received ===\n";
        debugLog << "CONTENT_LENGTH = " << contentLength << "\n";
        debugLog << "Raw body:\n" << body << "\n";
        debugLog << "---\n";

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

            // Remove embedded NUL and trailing \r
            line.erase(remove(line.begin(), line.end(), '\0'), line.end());
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (!line.empty()) {
                out << line << "\n";
            }
        }

        out.close();
        debugLog.close();
        return 0;
    }

    // ------------------ GET: suggestions / click logging ------------------
    cout << "Content-type: text/plain\n\n";

    string queryStr = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
    string keyword  = getQueryParam(queryStr, "query");
    string username = getQueryParam(queryStr, "user");
    string logFlag  = getQueryParam(queryStr, "log"); // "1": click log; "0" or empty: typing/suggestions only

    if (username.empty()) username = "guest";
    if (!keyword.empty() && keyword.back() == '\r') keyword.pop_back(); // strip stray CR if any

    ofstream debugLog(basePath + "debug.txt", ios::app);
    debugLog << "=== Search Request Debug ===\n";
    debugLog << "Full QUERY_STRING: [" << queryStr << "]\n";
    debugLog << "Extracted keyword: [" << keyword << "]\n";
    debugLog << "Extracted username: [" << username << "]\n";
    debugLog << "Extracted log flag: [" << logFlag << "]\n";
    debugLog << "---\n";

    Trie trie;
    loadWordsFromFile(basePath + "words.txt", trie);

    if (keyword.empty()) {
        cout << "No query provided.";
        debugLog << "❌ No query string found.\n";
    } else {
        // Always return suggestions for the typed query
        vector<string> results = trie.suggest(keyword);
        if (results.empty()) {
            cout << "No autocomplete suggestions for: " << keyword;
        } else {
            for (const string& word : results) {
                cout << " - " << word << "\n";
            }
        }

        // Insert only when log=1 (clicked suggestion)
        if (logFlag == "1") {
            if (keyword.size() >= 1) {
                sqlite3* db;
                string dbPath = basePath + "sqlite/users.db";
                int rc = sqlite3_open(dbPath.c_str(), &db);
                if (rc != SQLITE_OK) {
                    debugLog << "❌ Failed to open DB: " << sqlite3_errmsg(db) << "\n";
                } else {
                    // Enforce foreign keys (nice to have)
                    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
                    debugLog << "✅ Opened DB successfully (log=1)\n";

                    const char* insertSQL = "INSERT INTO search_history (username, search_term) VALUES (?, ?);";
                    sqlite3_stmt* stmt;
                    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 2, keyword.c_str(), -1, SQLITE_TRANSIENT);
                        if (sqlite3_step(stmt) != SQLITE_DONE) {
                            debugLog << "❌ Failed to insert query: " << sqlite3_errmsg(db) << "\n";
                        } else {
                            debugLog << "✅ Click query inserted: user=" << username
                                     << " term=[" << keyword << "]\n";
                        }
                        sqlite3_finalize(stmt);
                    } else {
                        debugLog << "❌ Prepare failed: " << sqlite3_errmsg(db) << "\n";
                    }
                    sqlite3_close(db);
                }
            } else {
                debugLog << "ℹ️ Skipped insert (too short): [" << keyword << "]\n";
            }
        } else {
            debugLog << "ℹ️ log=0 (typing) → not inserting into DB\n";
        }
    }

    debugLog.close();
    return 0;
}
