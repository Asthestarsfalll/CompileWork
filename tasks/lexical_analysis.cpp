#include <cctype>
#include <climits>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define MAX_IDENT_LEN 8
#define MAX_NUM_LEN 8

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

const StringList opreratorSymbols{"+", "-",  "*",  "/",  "=",  "<",
                                  ">", "<=", ">=", "<>", ":=", "#"};

const StringList seperateeSymbols{".", ",", ";", "(", ")"};

const StringList reservedWords{"const", "var",  "procedure", "begin", "end",
                               "if",    "then", "else",      "while", "do",
                               "call",  "read", "write",     "odd"};

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
    auto token = Token(lexemeString, type, lineno);
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
    if (opreratorSymbols.contain(ch_str)) {
      return advance(OPERATOR);
    }
    if (seperateeSymbols.contain(ch_str)) {
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

int main() {
  std::string code, line;
  while (std::getline(std::cin, line)) {
    code += line;
    if (line == "end.") {
      break;
    }
    code += '\n';
  }

  Lexer lexer(code);
  while (true) {
    TokenType type = lexer.getTokenType();
    if (type == END)
      break;
    auto token = lexer.buildToken(type);
    print_info(token);
  }
  return 0;
}
