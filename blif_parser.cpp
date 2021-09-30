#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>
#include <set>
#include <string>
using namespace std;

class Node {
public:
  Node(string node, bool negate) {
    this->_node = node;
    this->_negate = negate;
  }

  void addPredecessor(string s) {
    this->_prodecessor.insert(s);
  }

  void addSuccessor(string s) {
    this->_successor.insert(s);
  }

  string node() const {
    return this->_node;
  }

  bool negate() const {
    return this-> _negate;
  }

  string nodeWithNegate() const {
    return this->_node + (this->_negate ? "'" : "");
  }

  string getProdecessor() {
    if (!this->_prodecessor.size()) {
      return "-";
    }

    return accumulate(next(this->_prodecessor.begin()), this->_prodecessor.end(), move(*this->_prodecessor.begin()), this->_joinFunction);
  }

  string getSuccessor() {
    if (!this->_successor.size()) {
      return "-";
    }

    return accumulate(next(begin(this->_successor)), end(this->_successor), move(*this->_successor.begin()), this->_joinFunction);
  }

  friend ostream& operator<<(ostream& os, Node& rhs) {
    os << "predecessor: ";
    os << rhs.getProdecessor();

    os << endl << "successor: ";
    os << rhs.getSuccessor();
    os << endl;
    return os;
  }

private:
  string _node;
  bool _negate;
  set<string> _prodecessor;
  set<string> _successor;
  function<string(string, string)> _joinFunction = [] (const string& a, const string& b) {
    return a + ", " + move(b);
  };
};

class Terms {
public:
  Terms(vector<Node> terms, bool negate) {
    this->_terms = terms;
    this->_negate = negate;
  }

  vector<Node> getTerms() {
    return this->_terms;
  }

  string toString() {
    string terms = accumulate(next(begin(this->_terms)), end(this->_terms), this->_terms.begin()->nodeWithNegate(),
      [] (const string& s, const Node& i) {
        return s + " " + i.nodeWithNegate();
      });
    
    if (this->_negate) {
      terms = "(" + terms + ")'";
    }

    return terms;
  }
private:
  vector<Node> _terms;
  bool _negate;
};

class BFunction {
public:
  BFunction(string output) {
    this->_output = output;
  }

  string name() {
    return _output;
  }

  void addInputs(Terms inputs) {
    this->_inputs.push_back(inputs);
  }
  
  vector<Terms> getInputs() {
    return _inputs;
  }

  friend ostream& operator<<(ostream& os, BFunction& rhs) {
    if (!rhs._inputs.size()) {
      return os;
    }

    os << rhs.name() << " = ";
    os << accumulate(next(begin(rhs._inputs)), end(rhs._inputs), rhs._inputs.begin()->toString(),
      [&] (string& s, Terms& i) {
        return s + " + " + i.toString();
      });
  
    os << endl;
    return os;
  }

private:
  string _output;
  vector<Terms> _inputs;
};

class Blif {
public:
  Blif(string filename) {
    this->readFile(filename);
    this->processNodes();
  }

  void outputFile(string filename) {
    ofstream f(filename);

    f << "Node function:" << endl;
    for (auto& i: this->_functions) {
      f << i;
    }
    f << "END";
  }

  vector<Node> getAllNodes() {
    return this->_allNodes;
  }

private:
  string _model;
  vector<string> _inputNodes;
  vector<string> _outputNodes;
  vector<Node> _allNodes;
  vector<BFunction> _functions;

  string token(string s, string delim) {
    if (s.find(delim) == string::npos) {
      return s;
    }

    return s.substr(s.find(delim) + delim.length(), s.length());
  }

  vector<string> split(string s) {
    stringstream ss(s);
    string v;
    vector<string> res;

    while (ss >> v) {
      res.push_back(v);
    }

    return res;
  }

  vector<string> splitChar(string s) {
    stringstream ss(s);
    char v;
    vector<string> res;

    while (ss >> v) {
      res.push_back(string{v});
    }

    return res;
  }

  void pushAllNodes(string s) {
    if (find_if(begin(this->_allNodes), end(this->_allNodes), [=] (Node& n) { return n.node() == s; }) == end(this->_allNodes)) {
      this->_allNodes.push_back(Node(s, false)); // all nodes stores only relation information, negate can be ignored
    }
  }

  void readFile(string filename) {
    ifstream f(filename);
    string line; 

    // model name
    do {
      getline(f, line);
    } while (line.find(".model ") == string::npos);
    string model = token(line, ".model ");
    this->_model = model;

    // inputs
    bool cont = false;
    do {
      getline(f, line);
      string inputs = token(line, ".inputs ");
      vector<string> tokened = split(inputs);
      if (tokened.back() == "\\") {
        tokened.pop_back();
        cont = true;
      } else {
        cont = false;
      }

      tokened.pop_back();
      this->_inputNodes.insert(this->_inputNodes.end(), make_move_iterator(begin(tokened)), make_move_iterator(end(tokened)));
    } while (cont);

    // outputs
    cont = false;
    do {
      getline(f, line);
      string outputs = token(line, ".outputs ");
      vector<string> tokened = split(outputs);
      if (tokened.back() == "\\") {
        tokened.pop_back();
        cont = true;
      } else {
        cont = false;
      }

      this->_outputNodes.insert(this->_outputNodes.end(), make_move_iterator(begin(tokened)), make_move_iterator(end(tokened)));
    } while (cont);

    for (auto& i: _inputNodes) {
      this->_functions.push_back(BFunction(i)); 
      this->pushAllNodes(i);
    }
    for (auto& i: _outputNodes) {
      this->_functions.push_back(BFunction(i)); 
      this->pushAllNodes(i);
    }

    // all boolean functions
    while (getline(f, line)) {
      if (line.find(".end") != string::npos) {
        break;
      }

      // read node names
      vector<string> names;
      cont = false;
      do {
        vector<string> tokened = split(token(line, ".names "));
        if (tokened.back() == "\\") {
          tokened.pop_back();
          cont = true;
          getline(f, line);
        } else {
          cont = false;
        }

        names.insert(names.end(), make_move_iterator(begin(tokened)), make_move_iterator(end(tokened)));
      } while (cont);

      // read boolean functions
      // until next dot keyword
      while (f.peek() != '.') {
        getline(f, line);
        vector<string> function = splitChar(line);
        vector<Node> inputs;
        for (size_t i = 0; i < function.size() - 1; ++i) {
          if (function[i] == "-") {
            continue;
          }

          this->pushAllNodes(names[i]);
          inputs.push_back(Node(names[i], function[i] == "0" ? true : false));
        }

        Terms terms(inputs, (function.back() == "0" ? true : false));

        auto node = find_if(this->_functions.begin(), this->_functions.end(), [=] (BFunction& i) {
          return names.back() == i.name();
        });

        if (node == this->_functions.end()) {
          this->pushAllNodes(names.back());
          this->_functions.push_back(BFunction(names.back()));
          this->_functions.back().addInputs(terms);
        } else {
          node->addInputs(terms);
        }
      }
    }
  
    f.close();
  }

  void setRelation(string input, string output) {
    auto predecessorNode = find_if(begin(this->_allNodes), end(this->_allNodes), [=] (Node& i) {
      return i.node() == input;
    });

    auto successorNode = find_if(begin(this->_allNodes), end(this->_allNodes), [=] (Node& i) {
      return i.node() == output;
    });

    predecessorNode->addSuccessor(output);
    successorNode->addPredecessor(input);
  }

  void processNodes() {
    for (auto& function: this->_functions) {
      auto output = function.name();
      for (auto& inputs: function.getInputs()) {
        for (auto& input: inputs.getTerms()) {
          setRelation(input.node(), output);
        }
      }
    }
  }
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cout << "usage: ./blif_parser sample01.blif" << endl;
    return 1;
  }

  string inputFilename = argv[1];

  Blif blif(inputFilename);
  blif.outputFile("function.out");

  string input;
  vector<Node> allNodes = blif.getAllNodes();
  do {
    cout << "Please input a node: ";
    cin >> input;
    if (input == "0") {
      return 0;
    }


    auto n = find_if(begin(allNodes), end(allNodes), [=] (Node& i) {
      return i.node() == input;
    });

    if (n == end(allNodes)) {
      cout << "node " << input << " does not exist" << endl;
      continue;
    }

    cout << *n;
  } while (true);

  return 0;
}
