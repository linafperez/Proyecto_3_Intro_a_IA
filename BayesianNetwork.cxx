#include "BayesianNetwork.h"

BayesianNetwork::BayesianNetwork(){
    vertices.clear();
    edges = nullptr;
}

BayesianNetwork::BayesianNetwork(std::vector<Node> &nvertices, bool** nedges){
    vertices = nvertices; 

    // Since the graph is an ordered pair, it is important to verify that the set of vertices exists in order to create the set of edges
    // It is assumed that if the size of the list is different from the size of the vector, the adjacency matrix does not contain the same vertices as the vector
    if(!vertices.empty()){
        std::size_t n = vertices.size();
        edges = new bool*[n];
        
        for (std::size_t i = 0; i < n; ++i) {
            edges[i] = new bool[n];
            for (std::size_t j = 0; j < n; ++j) {
                edges[i][j] = nedges[i][j];
            }
        }
    }else{
        edges = nullptr;
    }
}

BayesianNetwork::~BayesianNetwork() {
    if (edges != nullptr) {
        std::size_t n = vertices.size();
        for (std::size_t i = 0; i < n; ++i) {
            delete[] edges[i];
        }
        delete[] edges;
        edges = nullptr;
    }

    vertices.clear();
}

std::vector<Node>& BayesianNetwork::getVertices(){
    return vertices;
}

bool** BayesianNetwork::getEdges(){
    return edges;
}

void BayesianNetwork::setVertices(std::vector<Node>& nvertices){
    if (edges != nullptr) {
        std::size_t n = vertices.size();
        for (std::size_t i = 0; i < n; ++i) {
            delete[] edges[i];
        }
        delete[] edges;
        edges = nullptr;
    }

    vertices = nvertices;
}

bool BayesianNetwork::setEdges(bool** nedges) {
    if (vertices.empty()) return false;

    std::size_t n = vertices.size();

    // Free previous matrix if it exists
    if (edges != nullptr) {
        for (std::size_t i = 0; i < n; ++i) {
            delete[] edges[i];
        }
        delete[] edges;
    }

    // Allocate new matrix
    edges = new bool*[n];
    for (std::size_t i = 0; i < n; ++i) {
        edges[i] = new bool[n];
        for (std::size_t j = 0; j < n; ++j) {
            edges[i][j] = nedges[i][j];
        }
    }

    return true;
}

// Vertex operations
int BayesianNetwork::numberOfVertices(){
    return vertices.size();
}

bool BayesianNetwork::insertVertex(Node data){
    if(searchVertex(data) != -1) return false;

    std::size_t n_old = vertices.size();
    vertices.push_back(data);
    std::size_t n_new = vertices.size();

    bool** newEdges = new bool*[n_new];
    for (std::size_t i = 0; i < n_new; ++i) {
        newEdges[i] = new bool[n_new];
    }

    // Copy the old values if there was a previous matrix
    if (edges != nullptr && n_old > 0) {
        for (std::size_t i = 0; i < n_old; ++i) {
            for (std::size_t j = 0; j < n_old; ++j) {
                newEdges[i][j] = edges[i][j];
            }
        }
    }

    // Initialize the new row and the new column
    for (std::size_t i = 0; i < n_new; ++i) {
        newEdges[i][n_new - 1] = false;       
        newEdges[n_new - 1][i] = false;       
    }

    // Free the old matrix
    if (edges != nullptr && n_old > 0) {
        for (std::size_t i = 0; i < n_old; ++i) {
            delete[] edges[i];
        }
        delete[] edges;
    }

    // Update edges
    edges = newEdges;

    return true;    
}

int BayesianNetwork::searchVertex(Node data) const{
    for(std::size_t i = 0; i < vertices.size(); i++){
        if(vertices[i] == data) return (int)i;
    }   

    return -1; 
}

bool BayesianNetwork::deleteVertex(Node data){
    int index = searchVertex(data);

    if(index == -1) return false;

    std::size_t n_old = vertices.size();
    std::size_t n_new = n_old - 1;

    bool** newEdges = nullptr;

    if (n_new > 0) {
        newEdges = new bool*[n_new];
        for (std::size_t i = 0; i < n_new; ++i) {
            newEdges[i] = new bool[n_new];
        }
    }

    if (edges != nullptr && n_new > 0) {
        for (std::size_t i = 0, ni = 0; i < n_old; ++i) {
            if (i == index) continue;       // Skip deleted row

            for (std::size_t j = 0, nj = 0; j < n_old; ++j) {
                if (j == index) continue;   // Skip deleted column

                newEdges[ni][nj] = edges[i][j];
                nj++;
            }
            ni++;
        }
    }

    if (edges != nullptr) {
        for (std::size_t i = 0; i < n_old; ++i) {
            delete[] edges[i];
        }
        delete[] edges;
    }

    edges = newEdges;

    std::vector<Node>::iterator itv = vertices.begin() + index;
    vertices.erase(itv);

    return true;
}

Node BayesianNetwork::getVertex(std::string name){
    for(std::size_t i = 0; i < vertices.size(); i++){
        if(vertices[i].getName() == name) return vertices[i];
    }  

    return Node();
}

int BayesianNetwork::getRoot() const{
    for(std::size_t i = 0; i < vertices.size(); i++){
        if(getPredecessors(vertices[i]).empty()){
            return i;
        }
    }

    return -1;
}

std::vector<Node> BayesianNetwork::getPredecessors(Node node) const{
    int index = searchVertex(node);

    std::vector<Node> predecessors;

    if(edges == nullptr || index == -1) return predecessors;

    for(std::size_t j = 0; j < vertices.size(); j++){
        if(edges[j][index] == true){
            predecessors.push_back(vertices[j]);
        }
    }

    return predecessors;
}

// Edge operations
int BayesianNetwork::numberOfEdges(){
    if (edges == nullptr) return 0;
    int numberEdges = 0;
    
    for (std::size_t i = 0; i < vertices.size(); ++i){
        for (std::size_t j = 0; j < vertices.size(); ++j){
            if(edges[i][j] == true) numberEdges++;
        }
    }

    return numberEdges;
}

bool BayesianNetwork::insertEdge(Node source, Node destination){
    int sourceIndex = searchVertex(source);
    int destinationIndex = searchVertex(destination);

    if((edges == nullptr) || (sourceIndex == -1) || (destinationIndex == -1)) return false;

    edges[sourceIndex][destinationIndex] = true;
     
    return true;
}

bool BayesianNetwork::searchEdge(Node source, Node destination){
    int sourceIndex = searchVertex(source);
    int destinationIndex = searchVertex(destination);

    if((edges == nullptr) || (sourceIndex == -1) || (destinationIndex == -1)) return false;

    return edges[sourceIndex][destinationIndex];
}

bool BayesianNetwork::deleteEdge(Node source, Node destination){
    int sourceIndex = searchVertex(source);
    int destinationIndex = searchVertex(destination);

    if((edges == nullptr) || (sourceIndex == -1) || (destinationIndex == -1)) return false;

    edges[sourceIndex][destinationIndex] = false;

    return true;
}

static void printIndentedNode(std::ostream& os, const Node& node, std::string prefix){
    std::ostringstream buffer;
    buffer << node;

    std::istringstream input(buffer.str());
    std::string line;

    while(std::getline(input, line)){
        os << prefix << line << std::endl;
    }
}

void BayesianNetwork::printFromVertex(std::ostream& os, int index, std::vector<bool>& visited, std::string prefix) const{
    if(edges == nullptr) return;
    if(index < 0 || index >= (int)vertices.size()) return;

    printIndentedNode(os, vertices[index], prefix);

    visited[index] = true;

    std::vector<int> children;

    for(std::size_t j = 0; j < vertices.size(); j++){
        if(edges[index][j] == true){
            children.push_back((int)j);
        }
    }

    for(std::size_t i = 0; i < children.size(); i++){

        int childIndex = children[i];
        bool isLast = (i == children.size() - 1);

        std::string connector;
        std::string nextPrefix;

        if(isLast){
            connector = "└──> ";
            nextPrefix = prefix + "    ";
        }else{
            connector = "├──> ";
            nextPrefix = prefix + "│   ";
        }

        os << prefix << connector << vertices[childIndex].getName() << std::endl;

        printFromVertex(os, childIndex, visited, nextPrefix);
    }
}

std::ostream& operator<<(std::ostream& os, const BayesianNetwork& BN){
    if(BN.vertices.empty()){
        os << "Empty Bayesian Network";
        return os;
    }

    if(BN.edges == nullptr){
        os << "Bayesian Network without edges";
        return os;
    }

    int rootIndex = BN.getRoot();

    if(rootIndex == -1){
        os << "The Bayesian Network has no root";
        return os;
    }

    std::vector<bool> visited(BN.vertices.size(), false);

    os << "Bayesian Network" << std::endl << std::endl;

    os << BN.vertices[rootIndex].getName() << std::endl;
    BN.printFromVertex(os, rootIndex, visited, "");

    for(std::size_t i = 0; i < BN.vertices.size(); i++){
        if(!visited[i]){
            os << std::endl;
            os << "Disconnected component" << std::endl << std::endl;
            BN.printFromVertex(os, (int)i, visited, "");
        }
    }

    return os;
}