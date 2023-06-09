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

#define MAX_IDENT_LEN 10
#define MAX_NUM_LEN 10

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

std::unordered_map<std::string, std::string> reverseMapper = {
    {"<", ">="}, {">", "<="}, {":=", "#"}, {"#", "="}, {"<", ">"}, {">", "<"}};

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

struct Quadruple {
  std::string first, second, third, fourth;

  Quadruple(std::string a, std::string b, std::string c, std::string d)
      : first(a), second(b), third(c), fourth(d) {}
};

class PredictTable {
public:
  std::queue<std::string> inputs;
  std::vector<Quadruple *> InterCodes;
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

  Quadruple *buildQuadruple(std::string a, std::string b, std::string c,
                            std::string d) {
    auto t = new Quadruple(a, b, c, d);
    return t;
  }

  bool addToSymbolTable(int idx, int value) {
    auto token = tokens[idx];
    auto name = token.getLexemeString();
    if (token.type != IDENTIFIER)
      throw std::runtime_error("Not Indetifier");
    auto type = tokens[idx - 1].getLexemeString();
    if (existInSymbolTable(name))
      return true;
    symbolTable[name] = {type, value};
    return false;
  }

  bool addToSymbolTable(Token token, int value, const std::string &type) {
    auto name = token.getLexemeString();
    if (token.type != IDENTIFIER)
      throw std::runtime_error("Not Indetifier");
    if (existInSymbolTable(name))
      return true;
    symbolTable[name] = {type, value};
    return false;
  }
  bool existInSymbolTable(const std::string &name) {
    auto e = symbolTable.find(name);
    if (e != symbolTable.end())
      return true;
    return false;
  }

  std::pair<std::string, int> getSymbol(const std::string &name) {
    if (!existInSymbolTable(name))
      throw std::runtime_error("No such symbol" + name);
    return symbolTable[name];
  }

  void analysis(bool verbose) {
    if (inputs.size() == 0)
      throw std::runtime_error("The size of inputs should be greater than 1.");
    int step = 1, idx = 0, tempidx = 1;
    stack.push("#");
    stack.push(start_symbol);

    InterCodes.push_back(buildQuadruple("syss", "_", "_", "_"));
    if (verbose) {
      printSeperater(6);
      std::cout << "步骤" << '\t' << "分析栈"
                << "\t\t"
                << "输入缓存区" << '\t' << "动作" << std::endl;
      printSeperater(6);
    }
    bool is_under_p = false, isError = false;
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
        } else if (name == "c") {
          auto t = tokens[idx + 1];
          InterCodes.push_back(
              buildQuadruple("const", t.getLexemeString(), "_", "_"));
          addToSymbolTable(t, std::stoi(tokens[idx + 3].getLexemeString()),
                           "const");
          idx++;
        } else if (name == "=") {
          auto t = tokens[idx + 1];
          InterCodes.push_back(
              buildQuadruple("=", t.getLexemeString(), "_",
                             tokens[idx - 1].getLexemeString()));
          idx++;
        } else if (name == "v") {
          int a = idx;
          while (1) {
            auto token = tokens[a + 1];
            auto id = token.getLexemeString();
            if (id != "," && id != ";") {
              if (existInSymbolTable(id)) {
                std::cout << "(语义错误,行号:" << token.getLineno() << ")"
                          << std::endl;
                isError = true;
              }
              InterCodes.push_back(buildQuadruple("var", id, "_", "_"));
              addToSymbolTable(token, 0, "var");
            } else if (id == ";")
              break;
            a++;
          }
          idx++;
        } else if (name == "p") {
          auto token = tokens[idx + 1];
          auto id = token.getLexemeString();
          addToSymbolTable(token, -1, "procedure");
          InterCodes.push_back(buildQuadruple("procedure", id, "_", "_"));
          is_under_p = true;
          idx++;
        } else if (name == "i") {
          auto id1 = tokens[idx + 1].getLexemeString();
          auto id2 = tokens[idx + 2].getLexemeString();
          auto id3 = tokens[idx + 3].getLexemeString();
          InterCodes.push_back(
              buildQuadruple("j" + id2, id1, id3,
                             "$" + std::to_string(InterCodes.size() + 2)));
          idx++;
        } else if (name == "x") {
          auto id1 = tokens[idx - 1].getLexemeString();
          auto id2 = tokens[idx + 1].getLexemeString();
          auto id3 = tokens[idx + 2].getLexemeString();
          auto id4 = tokens[idx + 3].getLexemeString();
          InterCodes.push_back(buildQuadruple(id3, id2, id4, id1));
          idx++;
        } else if (name == "e") {
          if (is_under_p) {
            is_under_p = false;
            InterCodes.push_back(buildQuadruple("ret", "_", "_", "_"));
          }
          idx++;
        } else if (name == "y") {
          auto id = tokens[idx + 2].getLexemeString();
          auto type = getSymbol(id).first;
          if (type != "const" && type != "var") {
            std::cout << "(语义错误,行号:" << tokens[idx + 2].getLineno() << ")"
                      << std::endl;
            isError = true;
          }
          InterCodes.push_back(buildQuadruple("read", id, "_", "_"));
          idx++;
        } else if (name == "w") {
          auto id1 = tokens[idx + 1].getLexemeString();
          auto id2 = tokens[idx + 2].getLexemeString();
          auto id3 = tokens[idx + 3].getLexemeString();
          InterCodes.push_back(
              buildQuadruple("j" + id2, id1, id3,
                             "$" + std::to_string(InterCodes.size() + 3)));
          int i = 0, j = 0;
          while (1) {
            auto id = tokens[idx + i + j + 6].getLexemeString();
            if (id == "end")
              break;
            else if (id == "call" || id == "write" || id == "read")
              i++;
            else
              j++;
          }
          InterCodes.push_back(
              buildQuadruple("j" + reverseMapper[id2], id1, id3,
                             "$" + std::to_string(InterCodes.size() + i + 3)));
          idx++;
        } else if (name == "r") {
          auto id1 = tokens[idx + 1].getLexemeString();
          if (!existInSymbolTable(id1)) {
            std::cout << "(语义错误,行号:" << tokens[idx + 1].getLineno() << ")"
                      << std::endl;
            isError = true;
          }
          InterCodes.push_back(buildQuadruple("call", id1, "_", "_"));
          idx++;
          // } else if (name == "*" || name == "/" || name == "+" || name ==
          // "-") {
          //   if (tokens[idx - 2].getLexemeString() != "=" &&
          //       tokens[idx - 2].getLexemeString() != ":=") {
          //     auto id1 = tokens[idx - 1].getLexemeString();
          //     auto id2 = tokens[idx + 1].getLexemeString();
          //     InterCodes.push_back(
          //         buildQuadruple(name, id1, id2, "T" +
          //         std::to_string(tempidx)));
          //     new_temp = true;
          //     tempidx++;
          //   }
          //   idx++;
        } else if (name == "z") {
          auto t = tokens[idx + 3].getLexemeString();
          if (t == "*" || t == "/" || t == "+" || t == "-") {
            auto id1 = tokens[idx + 2].getLexemeString();
            auto id2 = tokens[idx + 4].getLexemeString();
            if (!existInSymbolTable(id2)) {
              std::cout << "(语义错误,行号:" << tokens[idx + 4].getLineno()
                        << ")" << std::endl;
              isError = true;
            }
            InterCodes.push_back(
                buildQuadruple(t, id1, id2, "T" + std::to_string(tempidx)));
            InterCodes.push_back(buildQuadruple(
                "write", "T" + std::to_string(tempidx), "_", "_"));
            tempidx++;
          } else
            InterCodes.push_back(buildQuadruple(
                "write", tokens[idx + 2].getLexemeString(), "_", "_"));
          idx++;
        } else {
          idx++;
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
    InterCodes.push_back(buildQuadruple("syse", "_", "_", "_"));
    if (!isError) {
      std::cout << "语义正确" << std::endl;
      displayInterCodes();
      displaySymbolTable();
    }
  }

  int inputsSize() { return inputs.size(); }

  void setTokens(std::vector<Token> &t) { tokens = t; }

  void displayInterCodes() {
    std::cout << "中间代码:" << std::endl;
    int idx = 1;
    for (auto i : InterCodes) {
      std::cout << "(" << idx << ")(" << i->first << "," << i->second << ","
                << i->third << "," << i->fourth << ")\n";
      idx++;
    }
  }

  void displaySymbolTable() {
    std::cout << "符号表:" << std::endl;
    std::unordered_map<std::string, std::pair<std::string, int>> temp;
    for (auto it = symbolTable.begin(); it != symbolTable.end(); it++)
      temp[it->first] = it->second;
    for (auto it = temp.begin(); it != temp.end(); it++) {
      std::cout << it->second.first << " " << it->first << " ";
      if (it->second.second > -1)
        std::cout << it->second.second;
      std::cout << std::endl;
    }
  }

private:
  std::stack<std::string> stack;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      table;
  std::unordered_map<std::string, int> terminals;
  std::string start_symbol;
  std::unordered_map<std::string, std::pair<std::string, int>> symbolTable;
  std::vector<Token> tokens;
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
  a.setTokens(token_list);

  std::string inp, tar;
  for (auto i : token_list) {
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
  // std::cout << inp;
  a.setInputs(inp);
  // a.diplayTable();
  // G.printSets("select");
  // G.printSets("follow");
  a.analysis(false);
  // a.analysis(false);
  return 0;
}
