#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <climits>
#include <ostream>
#include <stdexcept>

#define Epsilon " "
#define Invalid "<INVALID>"
#define UNKNOWN -1
#define TRUE 1
#define FALSE 0
#define KEEP -2

#define MAX_IDENT_LEN 8
#define MAX_NUM_LEN 8

class Token;
class StringList;
class Lexer;
void print_info(Token);

enum TokenType {
  INVALID_INDENTIFIER,
  IDENTIFIER,
  NUMBER,
  RESERVED_WORD,
  OPERATOR,
  SEPERATER,
  LONG_IDENTIFIER,
  LONG_NUMBER,
  END,
};

enum OperatorType {
  PLUS,
  MULTIPY,
  RELATIONAL,
};

class Token {
public:
  TokenType type;
  Token(const std::string &str, TokenType type)
      : lexeme(str), lineno(-1), type(type) {}
  Token(const std::string &str, TokenType type, int lineno)
      : lexeme(str), lineno(lineno), type(type) {}

  std::string getLexemeString() const { return lexeme; }

  int getLineno() { return lineno; }

private:
  std::string lexeme;
  int lineno;
};

class StringList {
public:
  StringList(std::initializer_list<std::string> init_list)
      : strings_(init_list) {}

  void add(const std::string &str) { strings_.push_back(str); }

  bool contain(const std::string &target) const {
    for (const auto &s : strings_) {
      if (s == target) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<std::string> strings_;
};

const StringList operatorSymbols{"+", "-",  "*",  "/",  "=",  "<",
                                 ">", "<=", ">=", "<>", ":=", "#"};

const StringList seperaterSymbols{".", ",", ";", "(", ")"};

const StringList reservedWords{"const", "var",   "procedure", "begin", "end",
                               "if",    "then",  "while",     "do",    "call",
                               "read",  "write", "odd"};

std::unordered_map<int, std::string> tokenTypeMapper = {
    {INVALID_INDENTIFIER, "非法字符(串)"},
    {IDENTIFIER, "标识符"},
    {NUMBER, "无符号整数"},
    {RESERVED_WORD, "保留字"},
    {OPERATOR, "运算符"},
    {SEPERATER, "界符"},
    {LONG_NUMBER, "无符号整数越界"},
    {LONG_IDENTIFIER, "标识符长度超长"},
};

std::unordered_map<std::string, int> operatorMapper = {
    {"+", PLUS},       {"-", PLUS},       {"*", MULTIPY},    {"/", MULTIPY},
    {"#", RELATIONAL}, {"=", RELATIONAL}, {"<", RELATIONAL}, {"<=", RELATIONAL},
    {">", RELATIONAL}, {">=", RELATIONAL}};

std::unordered_map<std::string, std::string> Mapper = {
    {"call", "r"},      {"read", "y"},  {"const", "c"}, {"var", "v"},
    {"procedure", "p"}, {"then", "t"},  {"if", "i"},    {"begin", "s"},
    {"end", "e"},       {"write", "z"}, {"#", "~"},     {"do", "d"},
    {"while", "w"},     {">=", "g"},    {"<=", "l"},    {":=", "x"},
    {"odd", "o"}};

int dispathOperator(std::string op) {
  auto it = operatorMapper.find(op);
  if (it == operatorMapper.end())
    throw std::runtime_error("Unexpected op name " + it->first);
  return it->second;
}

void print_info(Token token) {
  std::cout << '(';
  std::cout << tokenTypeMapper[token.type] << ',' << token.getLexemeString();
  int lineno = token.getLineno();
  if (lineno >= 0)
    std::cout << ',' << "行号:" << lineno;
  std::cout << ")" << std::endl;
};

class Lexer {
public:
  Lexer(const std::string &code)
      : code(code), pos(0), lineno(-1), cur_line(1) {}

  TokenType getTokenType() {
    skipWhiteSpaceAndComments();
    if (pos >= code.size())
      return END;
    char ch = code[pos];
    if (isalpha(ch))
      return identifyIdentifierOrKeyword();
    if (isdigit(ch))
      return identifyNumber();
    if (ispunct(ch))
      return identifyOperatorOrSeperater();
    throw std::runtime_error("Unreached statement");
  }

  Token buildToken(TokenType type) {
    auto token = Token(lexemeString, type, cur_line);
    lineno = -1;
    lexemeString.clear();
    return token;
  }

private:
  void skipWhiteSpaceAndComments() {
    while (pos < code.size()) {
      char ch = code[pos];
      if (isspace(ch)) {
        if (ch == '\n')
          cur_line++;
        pos++;
      } else if (ch == '/') {
        if (pos + 1 >= code.size())
          return;
        char next_ch = code[pos + 1];
        if (next_ch == '*') {
          skipBlockComment();
        } else if (next_ch == '/') {
          skipLineComment();
        } else {
          return;
        }
      } else {
        return;
      }
    }
  }

  void skipBlockComment() {
    pos += 2;
    while (pos < code.size()) {
      char ch = code[pos];
      if (ch == '\n')
        cur_line++;
      if (ch == '*' && pos + 1 < code.size() && code[pos + 1] == '/') {
        pos += 2;
        return;
      }
      pos++;
    }
    throw std::runtime_error("Unclosed block comment");
  }

  void skipLineComment() {
    pos += 2;
    while (pos < code.size() && code[pos] != '\n') {
      pos++;
    }
  }

  TokenType identifyIdentifierOrKeyword() {
    std::string str;
    bool is_too_long = false;
    while (pos < code.size() && isalnum(code[pos])) {
      str += code[pos];
      pos++;
    }
    if (str.length() > MAX_IDENT_LEN) {
      is_too_long = true;
      lineno = cur_line;
    }

    lexemeString = str;
    if (reservedWords.contain(str)) {
      lineno = -1;
      return RESERVED_WORD;
    }
    if (is_too_long)
      return LONG_IDENTIFIER;
    return IDENTIFIER;
  }

  TokenType identifyNumber() {
    std::string str;
    int val = 0;
    bool is_too_long = false;
    bool is_invalid = false;
    while (pos < code.size() && isdigit(code[pos])) {
      val = val * 10 + code[pos] - '0';
      pos++;
    }
    str = std::to_string(val);
    if (str.size() > MAX_NUM_LEN) {
      lineno = cur_line;
      is_too_long = true;
    }
    if (pos < code.size() && isalnum(code[pos])) {
      lineno = cur_line;
      is_invalid = true;
      str += code[pos];
      pos++;
    }
    lexemeString = str;
    if (is_too_long)
      return LONG_NUMBER;
    if (is_invalid)
      return INVALID_INDENTIFIER;
    return NUMBER;
  }

  TokenType identifyOperatorOrSeperater() {
    char ch = code[pos];
    if (ch == '<') {
      if (pos + 1 < code.size() && code[pos + 1] == '=') {
        return advance(OPERATOR, 2);
      } else {
        return advance(OPERATOR);
      }
    }
    if (ch == '>') {
      if (pos + 1 < code.size() && code[pos + 1] == '=') {
        return advance(OPERATOR, 2);
      } else {
        return advance(OPERATOR);
      }
    }
    if (ch == ':' && pos + 1 < code.size() && code[pos + 1] == '=') {
      return advance(OPERATOR, 2);
    }
    std::string ch_str(1, ch);
    if (operatorSymbols.contain(ch_str)) {
      return advance(OPERATOR);
    }
    if (seperaterSymbols.contain(ch_str)) {
      return advance(SEPERATER);
    }
    lineno = cur_line;
    return advance(INVALID_INDENTIFIER);
  }

  TokenType advance(TokenType type) {
    lexemeString = code.substr(pos, 1);
    pos++;
    return type;
  }

  TokenType advance(TokenType type, int step) {
    lexemeString = code.substr(pos, step);
    pos += step;
    return type;
  }

private:
  const std::string &code;
  int pos;
  std::string lexemeString;
  int lineno, cur_line;
};
using GrammarInputType =
    std::unordered_map<std::string, std::vector<std::string>>;
using SetType = std::unordered_map<std::string, std::set<std::string>>;
using TableType =
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string>>;

bool HASEPSILON = false;

class Nonterminal;
class Terminal;
class Grammar;

inline bool isNonterminal(const std::string &name) {
  return std::isupper(name[0]);
}

template <typename T> inline void concat(std::set<T> &v1, std::set<T> &v2) {
  v1.insert(v2.begin(), v2.end());
}

inline void filterEspilon(std::set<std::string> &v) { v.erase(Epsilon); }

inline bool isNonterminal(const char &name) { return std::isupper(name); }

inline std::string ToEpsilon(const std::string &var) {
  return var == Epsilon ? "ε" : var;
}

inline void printSeperater(int size) {
  for (int i = 0; i < size; i++) {
    std::cout << "========";
  }
  std::cout << std::endl;
}

inline void printTab(int size) {
  for (int i = 0; i < size; i++) {
    std::cout << '\t';
  }
}

template <typename T> inline void getFullName(T &iterator, std::string &name) {
  iterator++;
  if (*iterator == '\'') {
    name += "'";
    iterator++;
  }
}

inline std::vector<std::string> reverse(std::string inp) {
  std::stack<std::string> temp;
  for (auto it = inp.begin(); it != inp.end();) {
    std::string name = {*it};
    if (isNonterminal(*it)) {
      getFullName(it, name);
    } else {
      it++;
    }
    if (name == " ")
      continue;
    temp.push(name);
  }
  std::vector<std::string> t;
  while (!temp.empty()) {
    t.push_back(temp.top());
    temp.pop();
  }
  return t;
}

template <typename T>
inline void assertNotHasKey(const std::string &key,
                            std::unordered_map<std::string, T> dict) {
  auto it = dict.find(key);
  if (it != dict.end())
    throw std::runtime_error("Already has Key " + key);
}

template <typename T> inline void printStack(std::stack<T> t) {
  std::vector<T> temp;
  while (!t.empty()) {
    temp.push_back(t.top());
    t.pop();
  }
  for (int i = temp.size() - 1; i >= 0; i--) {
    std::cout << temp[i];
  }
}

template <typename T> inline void printQueue(std::queue<T> q) {
  while (!q.empty()) {
    std::cout << q.front();
    q.pop();
  }
}

class Nonterminal {
public:
  std::string name;
  std::vector<std::string> productions;
  int generallyEmpty;
  Nonterminal() : name(Invalid) {}
  Nonterminal(const std::string &name) : name(name), generallyEmpty(FALSE) {
    if (!isNonterminal(name[0]))
      throw std::runtime_error(
          "Expect size of name to be 1 and is suppper letter.");
  }
  void addProduction(const std::string &symbols) {
    if (generallyEmpty != TRUE) {
      if (isNonterminal(symbols[0])) {
        beginWithNonterminal = true;
        generallyEmpty = UNKNOWN;
      } else if (symbols == Epsilon) {
        generallyEmpty = TRUE;
        HASEPSILON = true;
      }
    }
    productions.push_back(symbols);
  }

  std::vector<std::string> getUnknownNonterminal() {
    std::vector<std::string> target;
    if (generallyEmpty != TRUE) {
      for (std::string prod : productions) {
        if (isNonterminal(prod[0]))
          target.push_back(prod);
      }
    }
    return target;
  }

  friend std::ostream &operator<<(std::ostream &os, const Nonterminal &self) {
    for (auto var : self.productions) {
      os << "      " << self.name << " --> " << (ToEpsilon(var)) << std::endl;
    }
    return os;
  }

private:
  bool beginWithNonterminal = false;
};

class Terminal {
public:
  std::string name;
  Terminal() : name(Invalid) {}
  Terminal(const std::string &name) : name(name) {}

  friend std::ostream &operator<<(std::ostream &os, const Terminal &self) {
    os << self.name;
    return os;
  }
};

class Grammar {
public:
  std::string start_symbol;
  Grammar(const std::string &start_symbol,
          std::vector<Nonterminal *> in_nonterminals,
          std::vector<Terminal *> in_terminals)
      : start_symbol(start_symbol) {
    for (Nonterminal *var : in_nonterminals) {
      nonterminals[var->name] = var;
    }
    for (Terminal *var : in_terminals) {
      terminals[var->name] = var;
    }
    analysisIsGenerallyToEmpty();
  };

  Grammar(const std::string &start_symbol,
          std::vector<Nonterminal *> in_nonterminals,
          std::unordered_map<std::string, Terminal *> in_terminals)
      : start_symbol(start_symbol) {
    for (Nonterminal *var : in_nonterminals) {
      nonterminals[var->name] = var;
    }
    terminals = in_terminals;
    analysisIsGenerallyToEmpty();
  };

  friend std::ostream &operator<<(std::ostream &os, const Grammar &self) {
    os << "G[" << self.start_symbol << "]: \n";
    for (auto it = self.nonterminals.begin(); it != self.nonterminals.end();
         it++) {
      os << *it->second;
    }
    return os;
  }

  bool hasNonterminal(const std::string &name) {
    auto it = nonterminals.find(name);
    if (it != nonterminals.end())
      return true;
    return false;
  }

  std::set<std::string> getTargetFirstSet(const std::string &name) {
    return getFirstSet(*getTargetNonterminal(name));
  }

  std::set<std::string> getTargetFollowSet(const std::string &name) {
    return getFollowSet(*getTargetNonterminal(name));
  }

  std::set<std::string> getTargetSelectSet(const std::string &name,
                                           const std::string &production) {
    auto node = *getTargetNonterminal(name);
    bool is_exist = false;
    for (auto it = node.productions.begin(); it != node.productions.end();
         it++) {
      if (*it == production)
        is_exist = true;
    }
    if (!is_exist)
      throw std::runtime_error("Unexpected production for Nonterminal " + name);
    return getSelectSet(node, production);
  }

  void getAllSelectSet() {
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      for (auto s_it = it->second->productions.begin();
           s_it != it->second->productions.end(); s_it++) {
        getTargetSelectSet(it->second->name, *s_it);
      }
    }
  }

  friend void buildPredcitTable(Grammar &G, TableType &table) {
    for (auto it = G.nonterminals.begin(); it != G.nonterminals.end(); it++) {
      auto node = it->second;
      for (auto s_it = node->productions.begin();
           s_it != node->productions.end(); s_it++) {
        std::set<std::string> temp = G.getTargetSelectSet(node->name, *s_it);
        for (auto s = temp.begin(); s != temp.end(); s++) {
          auto t = table[node->name].find(*s);
          if (t != table[node->name].end())
            throw std::runtime_error("Not LL(1) Grammar");
          table[node->name][*s] = *s_it;
        }
      }
    }
  }
  std::unordered_map<std::string, Terminal *> getTerminal() {
    return terminals;
  }

  std::set<std::string> getFirstSet(Nonterminal node) {
    auto cache = firstCache.find(node.name);
    if (cache != firstCache.end())
      return cache->second;

    std::set<std::string> firsts;
    for (auto prod : node.productions) {
      for (auto s_it = prod.begin(); s_it != prod.end();) {
        std::string str = {*s_it};
        getFullName<std::string::iterator>(s_it, str);
        if (isNonterminal(str)) {
          Nonterminal n = *getTargetNonterminal(str);
          std::set<std::string> temp = getFirstSet(n);
          concat(firsts, temp);
          if (!n.generallyEmpty)
            break;
        } else {
          firsts.insert({str});
          break;
        }
      }
    }
    firstCache[node.name] = firsts;
    return firsts;
  };
  std::set<std::string> getFollowSet(Nonterminal target_node) {
    auto cache = followCache.find(target_node.name);
    if (cache != followCache.end())
      return cache->second;

    std::set<std::string> follows;
    const std::string target_name = target_node.name;
    if (target_name == start_symbol)
      follows.insert("#");
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      Nonterminal node = *it->second;
      if (followLocks[target_name].count(node.name))
        continue;
      for (auto prod : node.productions) {
        for (auto s_it = prod.begin(); s_it != prod.end();) {
          std::string name({*s_it});
          getFullName<std::string::iterator>(s_it, name);
          if (name != target_name)
            continue;
          std::set<std::string> temp;
          if (s_it == prod.end() || *s_it == ' ') {
            followLocks[target_name].insert(node.name);
            temp = getFollowSet(*getTargetNonterminal(node.name));
            followLocks[target_name].erase(node.name);
            concat(follows, temp);
          } else {
            bool generallyToEmpty = true;
            while (s_it != prod.end()) {
              std::string next_name = std::string({*s_it});
              getFullName<std::string::iterator>(s_it, next_name);
              if (isNonterminal(next_name)) {
                temp = getFirstSet(*getTargetNonterminal({next_name}));
                if (temp.count(Epsilon)) {
                  filterEspilon(temp);
                  concat(follows, temp);
                } else {
                  concat(follows, temp);
                  generallyToEmpty = false;
                  break;
                }
                temp.clear();
              } else {
                follows.insert(next_name);
                generallyToEmpty = false;
                break;
              }
            }
            if (generallyToEmpty and node.name != target_name) {
              followLocks[target_name].insert(node.name);
              temp = getFollowSet(*getTargetNonterminal(node.name));
              followLocks[target_name].erase(node.name);
              concat(follows, temp);
            }
            break;
          }
        }
      }
    }
    followCache[target_name] = follows;
    return follows;
  };
  std::set<std::string> getSelectSet(Nonterminal node,
                                     const std::string &production) {
    auto cache = selectCache.find(node.name + " --> " + production);
    if (cache != selectCache.end())
      return cache->second;

    bool generallyToEmpty = true;
    std::set<std::string> selects;
    for (auto s_it = production.begin(); s_it != production.end();) {
      if (isNonterminal(*s_it)) {
        std::string name = {*s_it};
        getFullName(s_it, name);
        if (name == "E'") {
          std::cout << std::endl;
        }
        auto n = *getTargetNonterminal(name);
        auto temp = getFirstSet(n);
        if (temp.count(Epsilon)) {
          filterEspilon(temp);
          concat(selects, temp);
        } else {
          generallyToEmpty = false;
          concat(selects, temp);
          break;
        }
      }
      if (s_it == production.end())
        break;
      if (!isNonterminal(*s_it) && *s_it != ' ') {
        generallyToEmpty = false;
        selects.insert({*s_it});
        break;
      } else if (*s_it == ' ') {
        generallyToEmpty = false;
        auto temp = getFollowSet(node);
        concat(selects, temp);
        break;
      }
    }
    if (generallyToEmpty) {
      auto temp = getFollowSet(node);
      concat(selects, temp);
    }
    selectCache[node.name + " --> " + production] = selects;
    return selects;
  };

  void eliminateLeftRecursion() {
    std::unordered_map<std::string, Nonterminal *> new_nonterminals;
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      const std::string name = it->first;
      const std::string new_name = name + "'";
      std::vector<std::string> target, others;
      for (std::string str : it->second->productions) {
        if (str.compare(0, 1, name) == 0) {
          target.push_back(str);
        } else {
          others.push_back(str);
        }
      }
      Nonterminal *new_node = new Nonterminal(new_name),
                  *old = new Nonterminal(name);
      for (auto n : target) {
        new_node->addProduction(n.substr(1) + new_name);
      }
      if (target.size() > 0)
        for (auto n : others) {
          old->addProduction(n + new_name);
        }
      else
        for (auto n : others) {
          old->addProduction(n);
        }
      new_nonterminals[name] = old;
      if (new_node->productions.size() > 0) {
        new_node->addProduction(Epsilon);
        new_nonterminals[new_name] = new_node;
      }
    }
    nonterminals = new_nonterminals;

    analysisIsGenerallyToEmpty();

    new_nonterminals.clear();
    // reverse
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      new_nonterminals[it->first] = it->second;
    }
    nonterminals = new_nonterminals;
  }
  void removeCommonPrefix() {}

  Nonterminal *getTargetNonterminal(const std::string &name) {
    auto it = nonterminals.find(name);
    if (it == nonterminals.end())
      throw std::runtime_error("Unexpected key for nonterminals: " + name);
    return it->second;
  };

  void printSets(const std::string &target) {
    std::cout << "=============================\n";
    std::cout << target << " set:\n"
              << "============================\n";
    if (target == "first") {
      _printSets(firstCache);
    } else if (target == "follow") {
      _printSets(followCache);
    } else if (target == "select") {
      _printSets(selectCache);
    } else {
      throw std::runtime_error("Unexpected print mode " + target);
    }
  }

  SetType getSet(const std::string &target) {
    if (target == "first") {
      return firstCache;
    } else if (target == "follow") {
      return followCache;
    } else if (target == "select") {
      return selectCache;
    } else {
      throw std::runtime_error("Unexpected print mode " + target);
    }
  }

private:
  void _printSets(std::unordered_map<std::string, std::set<std::string>> inp) {
    for (auto it = inp.begin(); it != inp.end(); it++) {
      _printSet(it->first, it->second);
    }
  }
  void _printSet(const std::string &name, const std::set<std::string> &s) {
    std::cout << name << (name.size() == 2 ? "" : " ") << ": { ";
    for (const auto &element : s) {
      std::cout << ToEpsilon(element) << " ";
    }
    std::cout << "}\n";
  }

  std::vector<Nonterminal *>
  getFilteredNodes(std::vector<std::string> targets) {
    std::vector<Nonterminal *> new_list;
    for (auto var : targets) {
      std::string c = {var[0]};
      if (var[1] == '\'')
        c += "'";
      auto non = getTargetNonterminal(c);
      if (!non->generallyEmpty)
        new_list.push_back(non);
    }
    return new_list;
  }

  int analysis(Nonterminal *node) {
    bool contain_unknown = false;
    if (!HASEPSILON)
      return FALSE;
    auto target_nodes = getFilteredNodes(node->getUnknownNonterminal());
    if (target_nodes.size() == 0)
      return KEEP;
    for (auto non : target_nodes) {
      if (non->generallyEmpty == UNKNOWN) {
        int res = analysis(non);
        if (res == TRUE)
          return TRUE;
        else if (res == UNKNOWN)
          contain_unknown = true;
      }
    }
    if (contain_unknown)
      return UNKNOWN;
    return FALSE;
  }

  void analysisIsGenerallyToEmpty() {
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      int res = analysis(it->second);
      if (res != KEEP)
        it->second->generallyEmpty = res;
    }
  }

  std::unordered_map<std::string, Nonterminal *> nonterminals;
  std::unordered_map<std::string, Terminal *> terminals;
  SetType firstCache, followCache, selectCache;
  SetType followLocks;
};

class PredictTable {
public:
  std::queue<std::string> inputs;
  PredictTable(Grammar &G) {
    buildPredcitTable(G, table);
    start_symbol = G.start_symbol;
    auto t = G.getTerminal();
    terminals["#"] = 1;
    for (auto it = t.begin(); it != t.end(); it++) {
      if (it->first == " ")
        continue;
      terminals[it->first] = 1;
    }
  }

  void clear_stack() {
    while (!stack.empty()) {
      stack.pop();
    }
  }

  void setInputs(const std::string &str) {
    while (!inputs.empty())
      inputs.pop();
    for (auto it = str.begin(); it != str.end(); ++it) {
      inputs.push({*it});
    }
    inputs.push("#");
  }

  void diplayTable() {
    TableType T(table);
    printSeperater(terminals.size());
    std::cout << "PredictTable:\n";
    printSeperater(terminals.size());
    std::cout << '\t';
    for (auto it = terminals.begin(); it != terminals.end(); it++) {
      std::cout << it->first << "\t";
    }
    for (auto i = T.begin(); i != T.end(); i++) {
      std::cout << '\n' << i->first << "\t";
      for (auto it = terminals.begin(); it != terminals.end(); it++) {
        std::cout << ToEpsilon({i->second[it->first]}) << "\t";
      }
    }
    std::cout << '\n';
  }

  void analysis(bool verbose) {
    if (inputs.size() == 0)
      throw std::runtime_error("The size of inputs should be greater than 1.");
    int step = 1;
    stack.push("#");
    stack.push(start_symbol);
    // auto tree = Tree(start_symbol);
    if (verbose) {
      printSeperater(6);
      std::cout << "步骤" << '\t' << "分析栈"
                << "\t\t"
                << "输入缓存区" << '\t' << "动作" << std::endl;
      printSeperater(6);
    }
    while (1) {
      if (verbose) {
        std::cout << step << '\t';
        printStack(stack);
        std::cout << "\t\t";
        printQueue(inputs);
        std::cout << "\t\t";
      }
      auto temp = inputs.front();
      auto name = stack.top();
      if (isNonterminal(name)) {
        auto it = table[name].find(temp);
        if (it == table[name].end())
          throw std::runtime_error("PredictTable wrong!\n");
        auto str = it->second;
        if (verbose)
          std::cout << name << " --> " << str << "\t\n";
        stack.pop();
        auto t = reverse(str);
        for (auto i : t) {
          stack.push({i});
        }
      } else if (name == temp) {
        if (name == "#") {
          if (verbose)
            std::cout << "Accpet"
                      << "\t\n";
          break;
        }
        stack.pop();
        inputs.pop();
        if (verbose)
          std::cout << "匹配" + name << "\t\n";
      } else {
        throw std::runtime_error("PredictTable wrong!\n");
      }
      step++;
    }
  }

  int inputsSize() { return inputs.size(); }

private:
  std::stack<std::string> stack;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      table;
  std::unordered_map<std::string, int> terminals;
  std::string start_symbol;
};

class Item {
public:
  std::string next;
  Nonterminal *non;
  int idx, prod_idx;
  Item(Nonterminal *non, int idx, int prod_idx)
      : idx(idx), non(non), prod_idx(prod_idx) {
    auto prod = non->productions;
    if (prod_idx >= prod.size())
      throw std::runtime_error("[ITEM]: Production Index Error");
    if (idx > prod[prod_idx].size())
      throw std::runtime_error("[ITEM]: String Index Error");
    if (idx < prod[prod_idx].size())
      next = prod[prod_idx][idx];
    else
      next = Invalid;
    if (idx < prod[prod_idx].size() &&
        non->productions[prod_idx][idx + 1] == '\'') {
      next += "'";
      idx++;
    }
    if (next == " ")
      next = Invalid;
  };

  friend std::ostream &operator<<(std::ostream &os, Item &self) {
    os << self.getProduction();
    return os;
  }

  Item *toNext() {
    if (next == Invalid || next == " ")
      throw std::runtime_error("toNext Wrong!");
    return new Item(non, idx + next.size(), prod_idx);
  }

  std::string getProduction() {
    std::string prods;
    prods = non->name + " --> ";
    auto target = non->productions[prod_idx];
    for (size_t i = 0; i < target.size(); i++) {
      if (i == idx)
        prods += "・";
      prods += target[i];
    }
    if (idx == target.size()) {
      prods += "・";
    }
    return prods;
  }

  bool operator==(const Item &other) const {
    return (non->name == other.non->name && idx == other.idx &&
            prod_idx == other.prod_idx);
  }

  size_t hash() const {
    size_t h1 = std::hash<std::string>{}(non->name);
    size_t h2 = std::hash<int>{}(idx);
    size_t h3 = std::hash<int>{}(prod_idx);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

class ItemSet : public std::vector<Item *> {
public:
  ItemSet(){};
  ItemSet(Item *i) { push_back(i); }
  ItemSet(std::initializer_list<Item *> init_list) {
    for (auto i : init_list) {
      push_back(i);
    }
  }

  std::vector<Item *> getTargetItems(const std::string &name) {
    std::vector<Item *> set;
    for (size_t i = 0; i < size(); i++) {
      auto t = at(i);
      if (t->non->name == name)
        set.push_back(t);
    }
    return set;
  }

  void push_back(Item *item) {
    if (item->next == Invalid || item->next == " ") {
      hasReduce = true;
    } else {
      hasShift = true;
    }
    nextItems[item->next].push_back(size());
    std::vector<Item *>::push_back(item);
  }

  std::unordered_map<std::string, std::vector<size_t>> getNexts() {
    std::unordered_map<std::string, std::vector<size_t>> temp;
    for (auto it = nextItems.begin(); it != nextItems.end(); it++) {
      temp[it->first] = it->second;
    }
    return temp;
  }

  friend std::ostream &operator<<(std::ostream &os, const ItemSet &self) {
    printSeperater(3);
    for (size_t i = 0; i < self.size(); i++) {
      os << "|| " << *self[i] << std::endl;
    }
    printSeperater(3);
    return os;
  };

  bool hasConflict() { return hasShift && hasReduce; }

  bool operator==(const ItemSet &other) const {
    if (size() != other.size())
      return false;
    for (auto i : other) {
      bool t = false;
      for (auto j : *this) {
        if (*i == *j) {
          t = true;
          break;
        }
      }
      if (!t)
        return false;
    }
    return true;
  }

private:
  std::unordered_map<std::string, std::vector<size_t>> nextItems;
  bool hasShift = false, hasReduce = false;
};

class ItemHash {
public:
  size_t operator()(const Item &item) const { return item.hash(); }
};

class SLRParser {
public:
  SLRParser(Grammar *G) : grammar(G), global_idx(0) {
    buildItemSets();
    ter.insert("#");
  };

  void traceItemSets(ItemSet &set) {
    int last_set = set.size();
    std::unordered_map<Item, int, ItemHash> cache;
    while (1) {
      auto nexts = set.getNexts();
      for (auto it = nexts.begin(); it != nexts.end(); it++) {
        auto name = it->first;
        if (!isNonterminal(name) || name == Invalid || name == " ")
          continue;
        auto non = grammar->getTargetNonterminal(name);
        for (int i = 0; i < non->productions.size(); i++) {
          auto t = new Item(non, 0, i);
          auto iter = cache.find(*t);
          if (iter == cache.end()) {
            set.push_back(t);
            cache[*t] = global_idx;
          }
        }
      }
      if (set.size() == last_set)
        break;
      last_set = set.size();
    }
  }

  int searchAll(ItemSet *set) {
    int a = 0;
    for (auto i : itemSets) {
      if (*set == *i)
        return a;
      a++;
    }
    return -1;
  }

  void getNextItemSet(ItemSet &set) {
    auto outports = set.getNexts();
    std::unordered_map<std::string, int> gos;
    std::unordered_map<std::string, std::pair<std::string, int>> acts;
    for (auto it = outports.begin(); it != outports.end(); it++) {
      auto new_set = new ItemSet();
      auto output_name = it->first;
      auto indices = it->second;
      for (auto idx : indices) {
        if (output_name == Invalid || output_name == " ") {
          auto item = set[idx];
          auto follow_set = grammar->getTargetFollowSet(item->non->name);
          for (auto follow : follow_set) {
            if (item->non->name == grammar->start_symbol) {
              acts[follow] = {"ACC", -1};
            } else {
              auto e = acts.find(follow);
              if (e != acts.end())
                throw std::runtime_error("Not SLR Grammar");
              acts[follow] = {"R" + item->non->name, item->prod_idx};
            }
          }
          continue;
        }
        auto t = set[idx]->toNext();
        new_set->push_back(t);
        if (isNonterminal(output_name)) {
          non.insert(output_name);
          gos[output_name] = itemSets.size();
        } else {
          ter.insert(output_name);
          auto e = acts.find(output_name);
          if (e != acts.end())
            throw std::runtime_error("Not SLR Grammar");
          acts[output_name] = {"S", itemSets.size()};
        }
      }
      int a = searchAll(new_set);
      if (a == -1 && new_set->size() > 0) {
        itemSets.push_back(new_set);
      } else {
        if (isNonterminal(output_name))
          gos[output_name] = a;
        else
          acts[output_name] = {"S", a};
      }
    }
    GOTOs.push_back(gos);
    ACTIONs.push_back(acts);
  }

  void buildItemSets() {
    std::string target_name = grammar->start_symbol;
    auto non = grammar->getTargetNonterminal(target_name);
    std::unordered_map<std::string, int> trace_cache;
    auto t = new Item(non, 0, 0);
    auto it = new ItemSet(t);
    itemSets.push_back(it);
    routh[*t] = 0;
    while (global_idx < itemSets.size()) {
      auto cur_itemset = itemSets[global_idx];
      traceItemSets(*cur_itemset);
      std::cout << global_idx << "\n" << *cur_itemset;
      getNextItemSet(*cur_itemset);
      global_idx++;
    }
  }

  void displayACTIONs() {
    std::cout << '\t';
    for (auto name : ter)
      std::cout << name << "\t";
    std::cout << std::endl;
    int i = 0;
    for (auto line : ACTIONs) {
      std::cout << i << '\t';
      i++;
      for (auto name : ter) {
        auto it = line.find(name);
        if (it != line.end())
          std::cout << it->second.first << it->second.second;
        else
          std::cout << Epsilon;
        std::cout << "\t";
      }
      std::cout << std::endl;
    }
  }

  void displayGOTOs() {
    std::cout << '\t';
    for (auto name : non)
      std::cout << name << "\t";
    std::cout << std::endl;
    int i = 0;
    for (auto line : GOTOs) {
      std::cout << i << '\t';
      i++;
      for (auto name : non) {
        auto it = line.find(name);
        if (it != line.end())
          std::cout << it->second;
        else
          std::cout << Epsilon;
        std::cout << "\t";
      }
      std::cout << std::endl;
    }
  }

  void displayRouth() {
    for (auto it = routh.begin(); it != routh.end(); it++) {
      auto item = it->first;
      int next = it->second;
      std::cout << item.getProduction() << ": " << next << std::endl;
    }
  }

  void setInputs(const std::string &str) {
    for (auto it = str.begin(); it != str.end();) {
      std::string name = {*it};
      getFullName(it, name);
      inputs.push(name);
    }
    inputs.push("#");
  }
  template <typename T> void popn(std::stack<T> &s, int size) {
    for (int i = 0; i < size; i++)
      s.pop();
  }

  void analysis(bool verbose) {
    std::cout << "步骤\t"
              << "状态栈\t\t"
              << "符号栈\t\t"
              << "输入串\t\t"
              << "分析动作\t"
              << "下一状态\t" << std::endl;

    int step = 1;
    stack.push("#");
    status.push(0);
    while (1) {
      std::string next_status = Epsilon;
      if (verbose) {
        std::cout << step << '\t';
        printStack(status);
        std::cout << "\t\t";
        printStack(stack);
        std::cout << "\t\t";
        printQueue(inputs);
        std::cout << "\t\t";
      }
      int top = status.top();
      auto cur_itemset = itemSets[top];
      std::string inp = inputs.front();
      auto action = ACTIONs[top][inp];
      if (action.first == "S") {
        status.push(action.second);
        inputs.pop();
        stack.push(inp);
      } else {
        if (action.second == -1)
          break;
        auto name = {action.first[1]};
        auto prod =
            grammar->getTargetNonterminal(name)->productions[action.second];
        if (prod == " ")
          prod = "";
        popn(status, prod.size());
        popn(stack, prod.size());
        stack.push(name);
        auto gos = GOTOs[status.top()];
        auto it = gos.find(name);
        if (it != gos.end()) {
          next_status = std::to_string(gos[name]);
          status.push(gos[name]);
        }
      }
      if (verbose) {
        std::cout << action.first << action.second << "\t\t";
        std::cout << next_status << std::endl;
      }
      step++;
    }
    std::cout << "ACC";
  }

private:
  std::vector<ItemSet *> itemSets;
  std::unordered_map<Item, int, ItemHash> routh;
  std::unordered_map<Item, std::unordered_map<int, int>, ItemHash> candidates;
  std::vector<std::unordered_map<std::string, std::pair<std::string, int>>>
      ACTIONs;
  std::vector<std::unordered_map<std::string, int>> GOTOs;
  std::stack<std::string> stack;
  std::stack<int> status;
  std::queue<std::string> inputs;
  std::set<std::string> non, ter;
  int global_idx;
  Grammar *grammar;
};

Grammar *buildGrammar(const std::string &start_symbol,
                      GrammarInputType &nonterminals) {
  std::vector<Nonterminal *> targets;
  std::unordered_map<std::string, Terminal *> terms;
  for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
    Nonterminal *temp = new Nonterminal(it->first);
    for (std::string prod : it->second) {
      for (auto s_it = prod.begin(); s_it != prod.end(); s_it++) {
        char c = *s_it;
        if (!isNonterminal(c))
          terms[{c}] = new Terminal({c});
      }
      temp->addProduction(prod);
    }
    targets.push_back(temp);
  }
  auto g = new Grammar(start_symbol, targets, terms);
  return g;
}

Grammar *getGrammer() {
  std::string start_symbol = "Z";
  GrammarInputType nonterminals = {
      {"Z", {"P"}},
      {"P", {"P'."}},
      {"P'", {"A'BI'S"}},
      {"A'", {"I", Epsilon}},
      {"B", {"V", Epsilon}},
      {"I", {"cDF';"}},
      {"F'", {",DF'", Epsilon}},
      {"D", {"b=n"}},
      {"V", {"vbG;"}},
      {"G", {",bG", Epsilon}},
      {"I'", {"AP';I'", Epsilon}},
      {"A", {"pb;"}},
      {"S", {"S'", "C", "W", "V'", "H", "D'", "F", "E'"}},
      {"S'", {"bxE"}},
      {"F", {"sS;H'e"}},
      {"H'", {"S;H'", Epsilon}},
      {"E'", {Epsilon}},
      {"C'", {"ERE", "oE"}},
      {"E", {"JTJ'"}},
      {"J", {"+", "-", Epsilon}},
      {"J'", {"LTJ'", Epsilon}},
      {"T", {"T'K"}},
      {"K", {"MT'K", Epsilon}},
      {"T'", {"b", "n", "(E)"}},
      {"L", {"+", "-"}},
      {"M", {"*", "/"}},
      {"R", {"~", "<", "l", "g", ">", "="}},
      {"C", {"iC'tS"}},
      {"V'", {"rb"}},
      {"W", {"wC'dS"}},
      {"H", {"y(bK')"}},
      {"K'", {",bK'", Epsilon}},
      {"M'", {",EM'", Epsilon}},
      {"D'", {"z(EM')"}}};
  return buildGrammar(start_symbol, nonterminals);
}

int main() {
  std::string code, line;
  while (std::getline(std::cin, line)) {
    code += line;
    if (line.size() > 0 && line.back() == '.') {
      break;
    }
    code += '\n';
  }
  Grammar G = *getGrammer();
  auto a = PredictTable(G);
  Lexer lexer(code);
  std::vector<Token> token_list;
  while (true) {
    TokenType type = lexer.getTokenType();
    if (type == END)
      break;
    auto token = lexer.buildToken(type);
    token_list.push_back(token);
    // print_info(token);
  }
  std::vector<std::pair<int, int>> spand;
  std::vector<int> ignore;
  int times = -1, replace = -1, makeup = 0;
  std::vector<std::string> to_replace{"b", "x", "b", "+", "b", ";"};
  bool replace_done = false;

  std::string a1, a2;
  int last_line = 1, idx = 0, cur_line, total_size, limit = 7;

  for (auto i : token_list) {
    cur_line = i.getLineno();
    if (cur_line != last_line) {
      spand.push_back({last_line, idx});
      last_line = cur_line;
    }
    idx++;
  }
  while (1) {
    try {
      std::string inp, tar;
      for (auto i : token_list) {
        int line = i.getLineno();
        bool pass = false;
        for (auto e : ignore) {
          if (line == e) {
            pass = true;
            break;
          }
        }
        if (pass) {
          if (!replace_done && replace == line) {
            for (auto i : to_replace)
              inp += i;
            replace_done = true;
          }
          continue;
        }
        tar = i.getLexemeString();
        if (i.type == IDENTIFIER) {
          tar = "b";
        } else if (i.type == NUMBER) {
          tar = "n";
        } else {
          auto e = Mapper.find(tar);
          if (e == Mapper.end())
            tar = tar[0];
          else
            tar = e->second;
        }
        inp += tar;
      }
      times++;
      // std::cout << inp;
      if (times == 0)
        total_size = inp.size();
      a.setInputs(inp);
      // a.diplayTable();
      // G.printSets("select");
      // G.printSets("follow");
      // a.analysis(true);
      a.analysis(false);
      if (times == 0)
        std::cout << "语法正确";
      break;
    } catch (std::exception &ex) {
      int size = total_size - a.inputsSize() - makeup, ig;
      for (auto it = spand.begin(); it != spand.end(); it++) {
        if (size < it->second) {
          ig = it->first;
          break;
        }
      }
      if (ig != 14)
        std::cout << "(语法错误,行号:" << ig << ")" << std::endl;
      if (a.inputs.front() == "e" || a.inputs.front() == "=") {
        replace = ig;
      }
      ignore.push_back(ig);
      if (times > limit)
        return 0;
    }
  }
  return 0;
}
