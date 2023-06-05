#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
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

inline bool isNonterminal(const char name) { return std::isupper(name); }

inline std::string ToEpsilon(const std::string var) {
  return var == Epsilon ? "ε" : var;
}

template <typename T> inline void getFullName(T &iterator, std::string &name) {
  iterator++;
  if (*iterator == '\'') {
    name += "'";
    iterator++;
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
      for (auto prod : node.productions) {
        for (auto s_it = prod.begin(); s_it != prod.end();) {
          std::string name({*s_it});
          getFullName<std::string::iterator>(s_it, name);
          if (name == target_name) {
            if (node.name == target_name)
              continue;
            std::set<std::string> temp;
            if (s_it == prod.end() || *s_it == ' ') {
              temp = getFollowSet(getTargetNonterminal(node.name));
              concat(follows, temp);
            } else if (isNonterminal(*s_it)) {
              temp = getFirstSet(getTargetNonterminal(node.name));
              if (temp.count(Epsilon)) {
                filterEspilon(temp);
                concat(follows, temp);
                temp = getFollowSet(getTargetNonterminal(node.name));
              }
              concat(follows, temp);
            } else {
              follows.insert({*s_it});
            }
          }
        }
      }
    }
    followCache[target_name] = follows;
    return follows;
  };
  std::set<std::string> getSelectSet(Nonterminal node) {
    auto cache = selectCache.find(node.name);
    if (cache != selectCache.end())
      return cache->second;

    std::set<std::string> selects;
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
    std::cout << target << " set:\n";
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

private:
  void _printSets(std::unordered_map<std::string, std::set<std::string>> inp) {
    for (auto it = inp.begin(); it != inp.end(); it++) {
      _printSet(it->first, it->second);
    }
  }
  void _printSet(const std::string name, const std::set<std::string> &s) {
    std::cout << name << ": { ";
    for (const auto &element : s) {
      std::cout << ToEpsilon(element) << ", ";
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
  std::unordered_map<std::string, std::set<std::string>> firstCache,
      followCache, selectCache;
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
  GrammarInputType inputs = {{"S", {"a", "/", "(T)"}}, {"T", {"T,S", "S"}}};
  std::string start_symbol = "S";
  Grammar G = buildGrammar(start_symbol, inputs);
  std::cout << G;
  G.eliminateLeftRecursion();
  std::cout << G;
  G.getFirstSet(G.getTargetNonterminal("S"));
  G.getFirstSet(G.getTargetNonterminal("T'"));
  G.getFirstSet(G.getTargetNonterminal("T"));
  G.printSets("first");

  G.getFollowSet(G.getTargetNonterminal("S"));
  G.getFollowSet(G.getTargetNonterminal("T'"));
  G.getFollowSet(G.getTargetNonterminal("T"));
  G.printSets("follow");
  return 0;
}
