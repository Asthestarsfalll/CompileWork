#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define Epsilon " "
#define Invalid "<INVALID>"
#define UNKNOWN -1
#define TRUE 1
#define FALSE 0
#define KEEP -2

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

inline bool isNonterminal(const std::string name) {
  return std::isupper(name[0]);
}

template <typename T> inline void concat(std::set<T> &v1, std::set<T> &v2) {
  v1.insert(v2.begin(), v2.end());
}

inline void filterEspilon(std::set<std::string> &v) { v.erase(Epsilon); }

inline bool isNonterminal(const char &name) { return std::isupper(name); }

inline std::string ToEpsilon(const std::string var) {
  return var == Epsilon ? "ε" : var;
}

inline void printSeperater(int size) {
  for (int i = 0; i < size; i++) {
    std::cout << "========";
  }
  std::cout << std::endl;
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
  Grammar(std::string start_symbol, std::vector<Nonterminal> in_nonterminals,
          std::vector<Terminal> in_terminals)
      : start_symbol(start_symbol) {
    for (Nonterminal var : in_nonterminals) {
      nonterminals[var.name] = var;
    }
    for (Terminal var : in_terminals) {
      terminals[var.name] = var;
    }
    analysisIsGenerallyToEmpty();
  };

  Grammar(std::string start_symbol, std::vector<Nonterminal> in_nonterminals,
          std::unordered_map<std::string, Terminal> in_terminals)
      : start_symbol(start_symbol) {
    for (Nonterminal var : in_nonterminals) {
      nonterminals[var.name] = var;
    }
    terminals = in_terminals;
    analysisIsGenerallyToEmpty();
  };

  friend std::ostream &operator<<(std::ostream &os, const Grammar &self) {
    os << "G[" << self.start_symbol << "]: \n";
    for (auto it = self.nonterminals.begin(); it != self.nonterminals.end();
         it++) {
      os << it->second;
    }
    return os;
  }

  bool hasNonterminal(std::string name) {
    auto it = nonterminals.find(name);
    if (it != nonterminals.end())
      return true;
    return false;
  }

  std::set<std::string> getTargetFirstSet(std::string name) {
    return getFirstSet(getTargetNonterminal(name));
  }

  std::set<std::string> getTargetFollowSet(std::string name) {
    return getFollowSet(getTargetNonterminal(name));
  }

  std::set<std::string> getTargetSelectFollowSet(std::string name,
                                                 std::string production) {
    auto node = getTargetNonterminal(name);
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

  void getAllSelectFollowSet() {
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      for (auto s_it = it->second.productions.begin();
           s_it != it->second.productions.end(); s_it++) {
        getTargetSelectFollowSet(it->second.name, *s_it);
      }
    }
  }

  friend void buildPredcitTable(Grammar &G, TableType &table) {
    for (auto it = G.nonterminals.begin(); it != G.nonterminals.end(); it++) {
      auto node = it->second;
      for (auto s_it = node.productions.begin(); s_it != node.productions.end();
           s_it++) {
        std::set<std::string> temp =
            G.getTargetSelectFollowSet(node.name, *s_it);
        for (auto s = temp.begin(); s != temp.end(); s++) {
          auto t = table[node.name].find(*s);
          if (t != table[node.name].end())
            throw std::runtime_error("Not LL(1) Grammar");
          table[node.name][*s] = *s_it;
        }
      }
    }
  }
  std::unordered_map<std::string, Terminal> getTerminal() { return terminals; }

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
          Nonterminal n = getTargetNonterminal(str);
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
      Nonterminal node = it->second;
      if (followLocks[target_name].count(node.name))
        continue;
      for (auto prod : node.productions) {
        for (auto s_it = prod.begin(); s_it != prod.end();) {
          std::string name({*s_it});
          getFullName<std::string::iterator>(s_it, name);
          if (name == target_name) {
            if (node.name == target_name)
              continue;
            std::set<std::string> temp;
            if (s_it == prod.end() || *s_it == ' ') {
              followLocks[target_name].insert(node.name);
              temp = getFollowSet(getTargetNonterminal(node.name));
              followLocks[target_name].erase(node.name);
              concat(follows, temp);
            } else {
              std::string next_name = std::string({*s_it});
              getFullName<std::string::iterator>(s_it, next_name);
              if (isNonterminal(next_name)) {
                temp = getFirstSet(getTargetNonterminal({next_name}));
                if (temp.count(Epsilon)) {
                  filterEspilon(temp);
                  concat(follows, temp);
                  followLocks[target_name].insert(node.name);
                  temp = getFollowSet(getTargetNonterminal(node.name));
                  followLocks[target_name].erase(node.name);
                }
                concat(follows, temp);
              } else {
                follows.insert(next_name);
              }
            }
          }
        }
      }
    }
    followCache[target_name] = follows;
    return follows;
  };
  std::set<std::string> getSelectSet(Nonterminal node, std::string production) {
    auto cache = selectCache.find(node.name + " --> " + production);
    if (cache != selectCache.end())
      return cache->second;

    bool generallyToEmpty = true;
    std::set<std::string> selects;
    for (auto s_it = production.begin(); s_it != production.end();) {
      if (isNonterminal(*s_it)) {
        auto n = getTargetNonterminal({*s_it});
        auto temp = getFirstSet(n);
        s_it++;
        if (temp.count(Epsilon)) {
          filterEspilon(temp);
          concat(selects, temp);
        } else {
          generallyToEmpty = false;
          concat(selects, temp);
          break;
        }
      }
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
    std::unordered_map<std::string, Nonterminal> new_nonterminals;
    for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
      const std::string name = it->first;
      const std::string new_name = name + "'";
      std::vector<std::string> target, others;
      for (std::string str : it->second.productions) {
        if (str.compare(0, 1, name) == 0) {
          target.push_back(str);
        } else {
          others.push_back(str);
        }
      }
      Nonterminal new_node(new_name), old(name);
      for (auto n : target) {
        new_node.addProduction(n.substr(1) + new_name);
      }
      if (target.size() > 0)
        for (auto n : others) {
          old.addProduction(n + new_name);
        }
      else
        for (auto n : others) {
          old.addProduction(n);
        }
      new_nonterminals[name] = old;
      if (new_node.productions.size() > 0) {
        new_node.addProduction(Epsilon);
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

  Nonterminal getTargetNonterminal(const std::string name) {
    auto it = nonterminals.find(name);
    if (it == nonterminals.end())
      throw std::runtime_error("Unexpected key for nonterminals: " + name);
    return it->second;
  };

  void printSets(std::string target) {
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

  SetType getSet(std::string target) {
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
  void _printSet(const std::string name, const std::set<std::string> &s) {
    std::cout << name << (name.size() == 2 ? "" : " ") << ": { ";
    for (const auto &element : s) {
      std::cout << ToEpsilon(element) << " ";
    }
    std::cout << "}\n";
  }

  std::vector<Nonterminal> getFilteredNodes(std::vector<std::string> targets) {
    std::vector<Nonterminal> new_list;
    for (auto var : targets) {
      std::string c = {var[0]};
      if (var[1] == '\'')
        c += "'";
      auto non = getTargetNonterminal(c);
      if (!non.generallyEmpty)
        new_list.push_back(non);
    }
    return new_list;
  }

  int analysis(Nonterminal node) {
    bool contain_unknown = false;
    if (!HASEPSILON)
      return FALSE;
    auto target_nodes = getFilteredNodes(node.getUnknownNonterminal());
    if (target_nodes.size() == 0)
      return KEEP;
    for (auto non : target_nodes) {
      if (non.generallyEmpty == UNKNOWN) {
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
        it->second.generallyEmpty = res;
    }
  }

  std::unordered_map<std::string, Nonterminal> nonterminals;
  std::unordered_map<std::string, Terminal> terminals;
  SetType firstCache, followCache, selectCache;
  SetType followLocks;
};

class PredictTable {
public:
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

  void setInputs(const std::string str) {
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
    if (verbose) {
      printSeperater(6);
      std::cout << "步骤" << '\t' << "分析栈" << "\t\t" << "输入缓存区" << '\t'
                << "动作" << std::endl;
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
      }
      step++;
    }
  }

private:
  template <typename T> void printStack(std::stack<T> t) {
    std::vector<T> temp;
    while (!t.empty()) {
      temp.push_back(t.top());
      t.pop();
    }
    for (int i = temp.size() - 1; i >= 0; i--) {
      std::cout << temp[i];
    }
  }

  template <typename T> void printQueue(std::queue<T> q) {
    while (!q.empty()) {
      std::cout << q.front();
      q.pop();
    }
  }
  std::stack<std::string> stack;
  std::queue<std::string> inputs;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      table;
  std::unordered_map<std::string, int> terminals;
  std::string start_symbol;
};

Grammar buildGrammar(std::string start_symbol, GrammarInputType nonterminals) {
  std::vector<Nonterminal> targets;
  std::unordered_map<std::string, Terminal> terms;
  for (auto it = nonterminals.begin(); it != nonterminals.end(); it++) {
    Nonterminal temp(it->first);
    for (std::string prod : it->second) {
      for (auto s_it = prod.begin(); s_it != prod.end(); s_it++) {
        char c = *s_it;
        if (!isNonterminal(c))
          terms[{c}] = Terminal({c});
      }
      temp.addProduction(prod);
    }
    targets.push_back(temp);
  }
  return Grammar(start_symbol, targets, terms);
}

int main(int argc, char *argv[]) {
  // GrammarInputType inputs = {{"S", {"a", "/", "(T)"}}, {"T", {"T,S",
  // "S"}}}; std::string start_symbol = "S";
  GrammarInputType inputs = {{"E", {"TE'"}},
                             {"E'", {"+E", " "}},
                             {"T", {"FT'"}},
                             {"T'", {"T", " "}},
                             {"F", {"PF'"}},
                             {"F'", {"*F'", " "}},
                             {"P", {"(E)", "a", "b", "/"}}};
  std::string start_symbol = "E";
  // GrammarInputType inputs = {
  //     {"S", {"a", "\\", "(T)"}}, {"T", {"ST'"}}, {"T'", {",ST'", Epsilon}}};
  // std::string start_symbol = "S";
  Grammar G = buildGrammar(start_symbol, inputs);
  std::cout << G;
  G.eliminateLeftRecursion();
  std::cout << G;
  // G.getFirstSet(G.getTargetNonterminal("S"));
  // G.getFirstSet(G.getTargetNonterminal("T'"));
  // G.getFirstSet(G.getTargetNonterminal("T"));
  // G.printSets("first");

  // G.getFollowSet(G.getTargetNonterminal("S"));
  // G.getFollowSet(G.getTargetNonterminal("T'"));
  // G.getFollowSet(G.getTargetNonterminal("T"));
  // G.printSets("follow");

  G.getTargetFirstSet("E");
  G.getTargetFirstSet("E'");
  G.getTargetFirstSet("T");
  G.getTargetFirstSet("T'");
  G.getTargetFirstSet("F");
  G.getTargetFirstSet("F'");
  G.getTargetFirstSet("P");
  G.printSets("first");

  G.getTargetFollowSet("E");
  G.getTargetFollowSet("E'");
  G.getTargetFollowSet("T");
  G.getTargetFollowSet("T'");
  G.getTargetFollowSet("F");
  G.getTargetFollowSet("F'");
  G.getTargetFollowSet("P");
  G.printSets("follow");

  G.getAllSelectFollowSet();
  // auto a = PredictTable(G);

  G.printSets("select");
  // a.diplayTable();
  // a.setInputs("(a,a)");
  // a.analysis(true);
  return 0;
}
