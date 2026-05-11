#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <numeric>
#include <iomanip>
#include <algorithm>

#include "BayesianNetwork.h"


std::vector<std::string> tokenizer(std::string line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (ss >> token)
        tokens.push_back(token);
    return tokens;
}


// Build the BN graph from file 

bool createBayesianNetwork(BayesianNetwork& BN, std::string filename) {
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }
    int numNodes;
    file >> numNodes;
    for (int i = 0; i < numNodes; i++) {
        Node node;
        std::string word;
        file >> word;
        node.setName(word);
        int numStates;
        file >> numStates;
        for (int j = 0; j < numStates; j++) {
            file >> word;
            node.insertState(word);
        }
        BN.insertVertex(node);
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> tokens = tokenizer(line);
        if (tokens.empty()) continue;
        Node source = BN.getVertex(tokens[0]);
        if (source.isEmpty()) { std::cerr << "Source node not found: " << tokens[0] << std::endl; continue; }
        for (std::size_t i = 1; i < tokens.size(); i++) {
            Node destination = BN.getVertex(tokens[i]);
            if (destination.isEmpty()) { std::cerr << "Destination node not found: " << tokens[i] << std::endl; continue; }
            BN.insertEdge(source, destination);
        }
    }
    file.close();
    return true;
}


// Show predecessors 
void showPredecessorsNode(BayesianNetwork& BN, Node node) {
    std::vector<Node> predecessors = BN.getPredecessors(node);
    std::cout << node.getName() << ": ";
    if (predecessors.empty()) std::cout << "none";
    else for (std::size_t i = 0; i < predecessors.size(); i++) std::cout << predecessors[i].getName() << " ";
    std::cout << std::endl;
}


// Cartesian product of parent states

std::vector<std::vector<std::pair<std::string,std::string>>>
cartesianProduct(const std::vector<Node>& parents) {
    std::vector<std::vector<std::pair<std::string,std::string>>> result = {{}};
    for (const Node& p : parents) {
        std::vector<std::vector<std::pair<std::string,std::string>>> extended;
        for (auto& existing : result) {
            for (const std::string& s : const_cast<Node&>(p).getStates()) {
                auto copy = existing;
                copy.push_back({p.getName(), s});
                extended.push_back(copy);
            }
        }
        result = extended;
    }
    return result;
}


bool loadCPTs(BayesianNetwork& BN, std::string filename) {
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Error opening CPT file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string currentNode = "";
    std::vector<std::vector<std::string>> cptRows;

    auto flushNode = [&]() {
        if (currentNode.empty() || cptRows.empty()) return;
        int idx = BN.searchVertex(BN.getVertex(currentNode));
        if (idx == -1) { std::cerr << "CPT: node not found: " << currentNode << std::endl; return; }

        Node& node = BN.getVertices()[idx];
        std::vector<Node> parents = BN.getPredecessors(node);
        int numStates  = node.numberOfStates();

        // Expected rows = product of all parent state counts (1 if no parents)
        int expectedRows = 1;
        for (const Node& p : parents) expectedRows *= const_cast<Node&>(p).numberOfStates();

        bool ok = true;

        // Build all expected parent combinations to report missing rows by name
        auto combos = cartesianProduct(parents);

        if ((int)cptRows.size() != expectedRows) {
            std::cerr << "  [CPT WARNING] '" << currentNode << "': expected " << expectedRows
                      << " row(s) but got " << cptRows.size() << "." << std::endl;
            // Report which parent combinations are missing
            for (std::size_t r = cptRows.size(); r < combos.size(); r++) {
                std::cerr << "    Missing row for: ";
                for (std::size_t k = 0; k < combos[r].size(); k++) {
                    if (k) std::cerr << ", ";
                    std::cerr << combos[r][k].first << "=" << combos[r][k].second;
                }
                std::cerr << std::endl;
            }
            ok = false;
        }

        for (std::size_t r = 0; r < cptRows.size(); r++) {
            // Label this row with its parent combination if possible
            std::string rowLabel = "row " + std::to_string(r+1);
            if (r < combos.size() && !combos[r].empty()) {
                rowLabel = "";
                for (std::size_t k = 0; k < combos[r].size(); k++) {
                    if (k) rowLabel += ", ";
                    rowLabel += combos[r][k].first + "=" + combos[r][k].second;
                }
            }

            if ((int)cptRows[r].size() != numStates) {
                std::cerr << "  [CPT WARNING] '" << currentNode << "' (" << rowLabel << ")"
                          << ": expected " << numStates << " value(s) but got "
                          << cptRows[r].size() << "." << std::endl;
                ok = false;
            } else {
                double sum = 0.0;
                bool numErr = false;
                for (const std::string& v : cptRows[r]) {
                    try { sum += std::stod(v); }
                    catch (...) {
                        std::cerr << "  [CPT WARNING] '" << currentNode << "' (" << rowLabel << ")"
                                  << ": '" << v << "' is not a valid number." << std::endl;
                        ok = false; numErr = true;
                    }
                }
                if (!numErr && std::abs(sum - 1.0) > 1e-6) {
                    std::cerr << "  [CPT WARNING] '" << currentNode << "' (" << rowLabel << ")"
                              << ": probabilities sum to " << std::fixed << std::setprecision(6)
                              << sum << " (expected 1.0)." << std::endl;
                    ok = false;
                }
            }
        }

        if (!ok)
            std::cerr << "  [CPT WARNING] '" << currentNode
                      << "' loaded with errors — inference may be incorrect." << std::endl;

        BN.getVertices()[idx].setConditionalProbabilityTable(cptRows);
    };

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> tokens = tokenizer(line);
        if (tokens.empty()) continue;

        bool isHeader = !tokens[0].empty() &&
                        (std::isalpha((unsigned char)tokens[0][0]) || tokens[0][0] == '_');

        if (isHeader) {
            flushNode();
            currentNode = tokens[0];
            cptRows.clear();
        } else {
            cptRows.push_back(tokens);
        }
    }
    flushNode();
    file.close();
    return true;
}


// Display CPT for one node
void displayNodeCPT(BayesianNetwork& BN, const std::string& nodeName) {
    int idx = BN.searchVertex(BN.getVertex(nodeName));
    if (idx == -1) return;
    Node& n = BN.getVertices()[idx];
    std::vector<Node> parents = BN.getPredecessors(n);
    std::vector<std::vector<std::string>>& cpt = n.getConditionalProbabilityTable();
    std::vector<std::string>& states = n.getStates();

    // Build header
    std::cout << "\n  ┌── CPT: " << nodeName;
    if (!parents.empty()) {
        std::cout << " | ";
        for (std::size_t i = 0; i < parents.size(); i++) {
            if (i) std::cout << ", ";
            std::cout << parents[i].getName();
        }
    }
    std::cout << " ──────────────────────────" << std::endl;

    // Column headers
    std::cout << "  │  ";
    if (!parents.empty()) {
        for (const Node& p : parents)
            std::cout << std::left << std::setw(14) << p.getName();
    } else {
        std::cout << std::left << std::setw(14) << "(prior)";
    }
    for (const std::string& s : states)
        std::cout << std::left << std::setw(10) << ("P("+s+")");
    std::cout << std::endl;
    std::cout << "  │  " << std::string(14*(parents.empty()?1:(int)parents.size()) + 10*(int)states.size(), '-') << std::endl;

    if (cpt.empty()) {
        std::cout << "  │  (no data loaded)" << std::endl;
    } else {
        auto combos = cartesianProduct(parents);
        for (std::size_t row = 0; row < cpt.size(); row++) {
            std::cout << "  │  ";
            if (!parents.empty() && row < combos.size()) {
                for (auto& kv : combos[row])
                    std::cout << std::left << std::setw(14) << kv.second;
            } else {
                std::cout << std::left << std::setw(14) << "";
            }
            for (const std::string& val : cpt[row])
                std::cout << std::left << std::setw(10) << val;
            std::cout << std::endl;
        }
    }
    std::cout << "  └──────────────────────────────────────────────────────" << std::endl;
}

void displayAllCPTs(BayesianNetwork& BN) {
    std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║      CONDITIONAL PROBABILITY TABLES  (CPTs)         ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
    for (std::size_t i = 0; i < BN.getVertices().size(); i++)
        displayNodeCPT(BN, BN.getVertices()[i].getName());
}


// P(node=stateValue | current assignment of parents)
double getProbability(BayesianNetwork& BN,
                      const std::string& nodeName,
                      const std::string& stateValue,
                      const std::map<std::string,std::string>& assignment) {
    int idx = BN.searchVertex(BN.getVertex(nodeName));
    if (idx == -1) return 0.0;
    Node& n = BN.getVertices()[idx];
    std::vector<Node> parents = BN.getPredecessors(n);
    std::vector<std::vector<std::string>>& cpt = n.getConditionalProbabilityTable();

    int stateIdx = n.searchState(stateValue);
    if (stateIdx == -1 || cpt.empty()) return 0.0;

    auto combos = cartesianProduct(parents);
    int rowIdx = 0;
    for (std::size_t r = 0; r < combos.size(); r++) {
        bool match = true;
        for (auto& kv : combos[r]) {
            auto it = assignment.find(kv.first);
            if (it == assignment.end() || it->second != kv.second) { match = false; break; }
        }
        if (match) { rowIdx = (int)r; break; }
    }

    if (rowIdx >= (int)cpt.size() || stateIdx >= (int)cpt[rowIdx].size()) return 0.0;
    return std::stod(cpt[rowIdx][stateIdx]);
}


// Topological order (BFS from root)
std::vector<std::string> topologicalOrder(BayesianNetwork& BN) {
    std::vector<std::string> order;
    int n = BN.numberOfVertices();
    std::vector<bool> visited(n, false);
    std::vector<int> queue;
    int root = BN.getRoot();
    if (root == -1) return order;
    queue.push_back(root);
    while (!queue.empty()) {
        int cur = queue.front(); queue.erase(queue.begin());
        if (visited[cur]) continue;
        visited[cur] = true;
        order.push_back(BN.getVertices()[cur].getName());
        for (int j = 0; j < n; j++)
            if (BN.getEdges()[cur][j]) queue.push_back(j);
    }
    for (int i = 0; i < n; i++)
        if (!visited[i]) order.push_back(BN.getVertices()[i].getName());
    return order;
}


// Enumerate-all: recursive core of enumeration inference algorithm
double enumerateAll(BayesianNetwork& BN,
                    const std::vector<std::string>& vars,
                    std::map<std::string,std::string>& assignment,
                    bool trace, int depth) {

    // Base case: all variables processed → joint contribution is 1.0
    // (every factor has already been multiplied as we descended)
    if (vars.empty()) {
        if (trace)
            std::cout << std::string(depth*2,' ') << "→ leaf = 1.0" << std::endl;
        return 1.0;
    }

    std::string Y = vars[0];
    std::vector<std::string> rest(vars.begin()+1, vars.end());

    // Y is fixed (evidence or query variable): multiply its CPT factor now
    if (assignment.count(Y)) {
        double p = getProbability(BN, Y, assignment[Y], assignment);
        if (trace)
            std::cout << std::string(depth*2,' ')
                      << "P(" << Y << "=" << assignment[Y] << "|pa) = "
                      << std::fixed << std::setprecision(4) << p
                      << "  [fixed]" << std::endl;
        return p * enumerateAll(BN, rest, assignment, trace, depth+1);
    }

    // Y is hidden: sum over all its states, multiplying each CPT factor
    int idx = BN.searchVertex(BN.getVertex(Y));
    std::vector<std::string>& yStates = BN.getVertices()[idx].getStates();
    double sum = 0.0;
    for (const std::string& yVal : yStates) {
        assignment[Y] = yVal;
        double p = getProbability(BN, Y, yVal, assignment);
        if (trace)
            std::cout << std::string(depth*2,' ')
                      << "∑ P(" << Y << "=" << yVal << "|pa) = "
                      << std::fixed << std::setprecision(4) << p << std::endl;
        sum += p * enumerateAll(BN, rest, assignment, trace, depth+1);
        assignment.erase(Y);
    }
    return sum;
}


// Enumeration-Ask: full inference with trace and normalisation

std::map<std::string,double> enumerationAsk(
        BayesianNetwork& BN,
        const std::string& queryVar,
        const std::map<std::string,std::string>& evidence,
        bool trace = false) {

    std::vector<std::string> vars = topologicalOrder(BN);

    int idx = BN.searchVertex(BN.getVertex(queryVar));
    std::vector<std::string>& qStates = BN.getVertices()[idx].getStates();

    // Header
    std::cout << "\n╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  INFERENCE BY ENUMERATION                           ║" << std::endl;
    std::cout << "║  Query variable : " << std::left << std::setw(34) << queryVar << "║" << std::endl;
    if (evidence.empty()) {
        std::cout << "║  Evidence       : (none)" << std::setw(29) << " " << "║" << std::endl;
    } else {
        for (auto& kv : evidence) {
            std::string ev = kv.first + " = " + kv.second;
            std::cout << "║  Evidence       : " << std::left << std::setw(34) << ev << "║" << std::endl;
        }
    }
    std::cout << "╠══════════════════════════════════════════════════════╣" << std::endl;

    std::map<std::string,double> dist;

    for (const std::string& qs : qStates) {
        std::map<std::string,std::string> assignment = evidence;
        assignment[queryVar] = qs;
        if (trace) std::cout << "\n  ── Enumerating: " << queryVar << "=" << qs << " ──" << std::endl;
        double val = enumerateAll(BN, vars, assignment, trace, 1);
        dist[qs] = val;
        std::cout << "║  Raw  P(" << queryVar << "=" << std::left << std::setw(10) << qs
                  << " |e) = " << std::right << std::fixed << std::setprecision(8) << val
                  << "  ║" << std::endl;
    }

    // Normalise
    double total = 0.0;
    for (auto& kv : dist) total += kv.second;
    std::cout << "╠══════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  α (norm. constant) = " << std::fixed << std::setprecision(8)
              << std::setw(12) << total << "                  ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════╣" << std::endl;

    if (total <= 0.0) {
        std::cout << "║  [ERROR] α = 0: the CPT file is incomplete or      ║" << std::endl;
        std::cout << "║  inconsistent for this combination of evidence.    ║" << std::endl;
        std::cout << "║  Check that all required rows exist in the CPT.   ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
        return dist;
    }

    for (auto& kv : dist) {
        kv.second = kv.second / total;
        std::cout << "║  P(" << queryVar << "=" << std::left << std::setw(12) << kv.first
                  << " |e) = " << std::right << std::fixed << std::setprecision(6)
                  << std::setw(10) << kv.second << "             ║" << std::endl;
    }
    std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;

    return dist;
}


// MAIN

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " BayesianNetworkFile CPTFile" << std::endl;
        return EXIT_FAILURE;
    }

    // ── STEP 1: Load graph structure
    std::cout << "══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  STEP 1 – Loading BN structure: " << argv[1] << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;
    BayesianNetwork BN;
    createBayesianNetwork(BN, argv[1]);
    std::cout << BN << std::endl;
    std::cout << "\nPredecessors of each variable:" << std::endl;
    for (std::size_t i = 0; i < BN.getVertices().size(); i++)
        showPredecessorsNode(BN, BN.getVertices()[i]);

    // ── STEP 2: Load CPTs
    std::cout << "\n══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  STEP 2 – Loading CPTs: " << argv[2] << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;
    if (!loadCPTs(BN, argv[2])) return EXIT_FAILURE;
    std::cout << "  CPTs loaded successfully." << std::endl;

    // ── STEP 3: Visualise CPTs 
    std::cout << "\n══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  STEP 3 – Visualising CPTs" << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;
    displayAllCPTs(BN);

    // ── STEP 4: Interactive inference loop
    std::cout << "\n══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  STEP 4 – Inference by Enumeration (interactive)" << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  Format:  P(QueryVar | Var1=val1, Var2=val2)" << std::endl;
    std::cout << "  No evidence: P(QueryVar)" << std::endl;
    std::cout << "  Add 'trace' at the end to see step-by-step detail." << std::endl;
    std::cout << "  Type 'exit' to quit." << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;

    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) break;

        // Trim whitespace
        while (!line.empty() && std::isspace((unsigned char)line.front())) line.erase(line.begin());
        while (!line.empty() && std::isspace((unsigned char)line.back()))  line.pop_back();

        if (line.empty()) continue;
        if (line == "exit" || line == "quit") break;

        // Detect optional 'trace' flag at the end
        bool trace = false;
        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.size() >= 5 && lower.substr(lower.size() - 5) == "trace") {
            trace = true;
            line = line.substr(0, line.size() - 5);
            while (!line.empty() && std::isspace((unsigned char)line.back())) line.pop_back();
        }

        // ── Parse P(QueryVar | Var1=val1, Var2=val2)
        // Find opening and closing parentheses
        std::size_t pOpen  = line.find('(');
        std::size_t pClose = line.rfind(')');
        if (pOpen == std::string::npos || pClose == std::string::npos || pClose < pOpen) {
            std::cout << "  [!] Bad format. Expected: P(QueryVar | Var=val, ...)" << std::endl;
            continue;
        }

        std::string inner = line.substr(pOpen + 1, pClose - pOpen - 1);

        // Split on '|' to separate query variable from evidence
        std::string queryVar;
        std::map<std::string, std::string> evidence;

        std::size_t pipe = inner.find('|');
        if (pipe == std::string::npos) {
            // No evidence: P(QueryVar)
            queryVar = inner;
            while (!queryVar.empty() && std::isspace((unsigned char)queryVar.front())) queryVar.erase(queryVar.begin());
            while (!queryVar.empty() && std::isspace((unsigned char)queryVar.back()))  queryVar.pop_back();
        } else {
            // With evidence: P(QueryVar | Var1=val1, Var2=val2)
            queryVar = inner.substr(0, pipe);
            while (!queryVar.empty() && std::isspace((unsigned char)queryVar.front())) queryVar.erase(queryVar.begin());
            while (!queryVar.empty() && std::isspace((unsigned char)queryVar.back()))  queryVar.pop_back();

            std::string evStr = inner.substr(pipe + 1);
            // Split evidence on commas
            std::istringstream evStream(evStr);
            std::string token;
            bool parseError = false;
            while (std::getline(evStream, token, ',')) {
                // Trim token
                while (!token.empty() && std::isspace((unsigned char)token.front())) token.erase(token.begin());
                while (!token.empty() && std::isspace((unsigned char)token.back()))  token.pop_back();
                if (token.empty()) continue;

                // Split on '='
                std::size_t eq = token.find('=');
                if (eq == std::string::npos) {
                    std::cout << "  [!] Evidence must be in 'Var=value' format. Got: '" << token << "'" << std::endl;
                    parseError = true;
                    break;
                }
                std::string eVar = token.substr(0, eq);
                std::string eVal = token.substr(eq + 1);
                while (!eVar.empty() && std::isspace((unsigned char)eVar.back()))   eVar.pop_back();
                while (!eVal.empty() && std::isspace((unsigned char)eVal.front()))  eVal.erase(eVal.begin());
                evidence[eVar] = eVal;
            }
            if (parseError) continue;
        }

        // ── Validate query variable exists in the network
        if (BN.getVertex(queryVar).isEmpty()) {
            std::cout << "  [!] Variable '" << queryVar << "' not found in the network." << std::endl;
            std::cout << "  Available variables: ";
            for (std::size_t i = 0; i < BN.getVertices().size(); i++) {
                if (i) std::cout << ", ";
                std::cout << BN.getVertices()[i].getName();
            }
            std::cout << std::endl;
            continue;
        }

        // ── Validate each evidence variable and its value
        bool evError = false;
        for (auto& kv : evidence) {
            Node evNode = BN.getVertex(kv.first);
            if (evNode.isEmpty()) {
                std::cout << "  [!] Evidence variable '" << kv.first << "' not found in the network." << std::endl;
                evError = true; break;
            }
            int idx = BN.searchVertex(evNode);
            if (BN.getVertices()[idx].searchState(kv.second) == -1) {
                std::cout << "  [!] State '" << kv.second << "' is not valid for variable '" << kv.first << "'." << std::endl;
                std::cout << "  Valid states: ";
                auto& st = BN.getVertices()[idx].getStates();
                for (std::size_t i = 0; i < st.size(); i++) { if (i) std::cout << ", "; std::cout << st[i]; }
                std::cout << std::endl;
                evError = true; break;
            }
            if (kv.first == queryVar) {
                std::cout << "  [!] Query variable '" << queryVar << "' cannot also be evidence." << std::endl;
                evError = true; break;
            }
        }
        if (evError) continue;

        // ── Run inference
        enumerationAsk(BN, queryVar, evidence, trace);
    }

    std::cout << "\n  Goodbye." << std::endl;
    return EXIT_SUCCESS;
}
