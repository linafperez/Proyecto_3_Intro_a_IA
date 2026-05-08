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
    if (vars.empty()) {
        double prob = 1.0;
        for (auto& kv : assignment) {
            double p = getProbability(BN, kv.first, kv.second, assignment);
            prob *= p;
            if (trace)
                std::cout << std::string(depth*2,' ')
                          << "P(" << kv.first << "=" << kv.second << "|pa) = "
                          << std::fixed << std::setprecision(4) << p << std::endl;
        }
        if (trace)
            std::cout << std::string(depth*2,' ')
                      << "→ joint = " << std::fixed << std::setprecision(8) << prob << std::endl;
        return prob;
    }

    std::string Y = vars[0];
    std::vector<std::string> rest(vars.begin()+1, vars.end());

    // Already assigned (evidence or query)
    if (assignment.count(Y)) {
        if (trace)
            std::cout << std::string(depth*2,' ')
                      << Y << " fixed = " << assignment[Y] << std::endl;
        return enumerateAll(BN, rest, assignment, trace, depth);
    }

    // Sum over unobserved hidden variable
    int idx = BN.searchVertex(BN.getVertex(Y));
    std::vector<std::string>& yStates = BN.getVertices()[idx].getStates();
    double sum = 0.0;
    for (const std::string& yVal : yStates) {
        if (trace)
            std::cout << std::string(depth*2,' ')
                      << "∑ " << Y << "=" << yVal << std::endl;
        assignment[Y] = yVal;
        sum += enumerateAll(BN, rest, assignment, trace, depth+1);
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
    for (auto& kv : dist) {
        kv.second = (total > 0) ? kv.second / total : 0.0;
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

    // ── STEP 4: Inference by enumeration 
    std::cout << "\n══════════════════════════════════════════════════════" << std::endl;
    std::cout << "  STEP 4 – Inference by Enumeration" << std::endl;
    std::cout << "══════════════════════════════════════════════════════" << std::endl;

    // Query 1: P(Appointment | Train=delayed)
    {
        std::map<std::string,std::string> ev;
        ev["Train"] = "delayed";
        enumerationAsk(BN, "Appointment", ev, false);
    }

    // Query 2: P(Train | Rain=heavy, Maintenance=yes) — with trace
    {
        std::map<std::string,std::string> ev;
        ev["Rain"]        = "heavy";
        ev["Maintenance"] = "yes";
        enumerationAsk(BN, "Train", ev, true);
    }

    // Query 3: P(Appointment) — prior (no evidence)
    {
        std::map<std::string,std::string> ev;
        enumerationAsk(BN, "Appointment", ev, false);
    }

    // Query 4: P(Rain | Appointment=miss) — backwards reasoning
    {
        std::map<std::string,std::string> ev;
        ev["Appointment"] = "miss";
        enumerationAsk(BN, "Rain", ev, false);
    }

    return EXIT_SUCCESS;
}
