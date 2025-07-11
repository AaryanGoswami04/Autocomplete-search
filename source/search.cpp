#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <cctype> // for isspace
using namespace std;

// Trie Node structure
struct TrieNode {
    map<char, TrieNode*> children;
    bool isEndOfWord = false;
};
class Trie {
    TrieNode* root;
    map<string, string> originalWords;  // maps lowercase word -> original word

    void dfs(TrieNode* node, string current, vector<string>& results) {
        if (node->isEndOfWord) {
            if (originalWords.count(current))
                results.push_back(originalWords[current]);  // get original casing
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
        originalWords[lowerWord] = word;  // store original casing

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

    // Remove leading whitespace
    while (start < end && isspace(static_cast<unsigned char>(str[start]))) ++start;

    // Remove trailing whitespace
    while (end > start && isspace(static_cast<unsigned char>(str[end - 1]))) --end;

    return str.substr(start, end - start);
}

void loadWordsFromFile(const string& filename, Trie& trie) {
    ifstream file(filename);
    string word;
    if (!file.is_open()) {
        cout << "Could not open file: " << filename << "\n";
        return;
    }

    while (getline(file, word)) {
    word = trim(word);

    // Remove UTF-8 BOM if present at beginning of the first word
    if (!word.empty() && static_cast<unsigned char>(word[0]) == 0xEF) {
        if (word.size() >= 3 &&
            static_cast<unsigned char>(word[1]) == 0xBB &&
            static_cast<unsigned char>(word[2]) == 0xBF) {
            word = word.substr(3);  // remove BOM
        }
    }

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
    return query.substr(start, end - start);
}
int main() {
    string basePath = "C:/xampp/htdocs/autocomplete-search/";
    ofstream log(basePath + "debug.txt");
    log << "CGI Executed\n";
    log.close();

    string requestMethod = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";

    if (requestMethod == "POST") {
    cout << "Content-type: text/plain\n\n";

    string contentLengthStr = getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0";
    int contentLength = stoi(contentLengthStr);

    string body(contentLength, '\0');
    cin.read(&body[0], contentLength);

    ofstream debug(basePath + "debug.txt", ios::app);
    debug << "=== POST Received ===\n";
    debug << "CONTENT_LENGTH = " << contentLength << "\n";
    debug << "Raw body:\n" << body << "\n";
    debug << "---\n";

    ofstream out(basePath + "words.txt");
    if (!out.is_open()) {
        debug << "❌ Failed to open words.txt for writing.\n";
        debug.close();
        return 0;
    }

    istringstream stream(body);
    string line;
    while (getline(stream, line)) {
        line = trim(line);

        // Remove BOM
        if (!line.empty() && static_cast<unsigned char>(line[0]) == 0xEF &&
            line.size() >= 3 &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }

        line.erase(remove(line.begin(), line.end(), '\0'), line.end());
        if (!line.empty()) {
            out << line << "\n";
        }
    }

    out.close();
    debug << "✅ words.txt written successfully\n";
    debug.close();
    return 0;
}

    // Else, handle GET
    cout << "Content-type: text/plain\n\n";

    string queryStr = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
    string keyword = getQueryParam(queryStr, "query");

    Trie trie;
    loadWordsFromFile("C:/xampp/htdocs/autocomplete-search/words.txt", trie);

    ofstream debug(basePath + "debug.txt", ios::app);
    debug << "🔍 GET query: " << keyword << "\n";

    if (keyword.empty()) {
        cout << "No query provided.";
        debug << "❌ No query string found.\n";
    } else {
        vector<string> results = trie.suggest(keyword);
        if (results.empty()) {
            cout << "No autocomplete suggestions for: " << keyword;
            debug << "❌ No suggestions for: " << keyword << "\n";
        } else {
            for (const string& word : results) {
                cout << " - " << word << "\n";
            }
            debug << "✅ Suggestions served for: " << keyword << "\n";
        }
    }
    debug.close();
    return 0;
}
