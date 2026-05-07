#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>

#include "BayesianNetwork.h"

std::vector<std::string> tokenizer(std::string line);
bool createBayesianNetwork(BayesianNetwork& BN, std::string filename);
void showPredecessorsNode(BayesianNetwork& BN, Node node);

int main(int argc, char * argv[]){

    if (argc != 3){
        std::cerr << "Wrong Parameters" << std::endl;
        std::cerr << "Usage: " << argv[0];
        std::cerr << " BayesianNetworkFile conditionalProbabilityTableFile";
        std::cerr << std::endl;

        return EXIT_FAILURE;
    }

    std::string BNFile = argv[1];
    std::string CPTFile = argv[2];

    BayesianNetwork BN;

    createBayesianNetwork(BN,BNFile);
    std::cout << BN << std::endl;
    
    std::cout << std::endl <<"Predecessors of each variable:" << std::endl;
    for(std::size_t i = 0; i < BN.getVertices().size(); i++){
        showPredecessorsNode(BN, BN.getVertices()[i]);
    }

    return EXIT_SUCCESS;
}

std::vector<std::string> tokenizer(std::string line){
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while(ss >> token){
        tokens.push_back(token);
    }

    return tokens;
}

bool createBayesianNetwork(BayesianNetwork& BN, std::string filename){

    std::ifstream file(filename, std::ios::in);

    if(!file.is_open()){
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    int numNodes;
    file >> numNodes;

    for(int i = 0; i < numNodes; i++){
        
        Node node;

        std::string word;
        file >> word;

        node.setName(word);

        int numStates;
        file >> numStates;

        for(int j = 0; j < numStates; j++){
            file >> word;
            node.insertState(word);
        }

        BN.insertVertex(node);
    }

    std::string line;

    while(std::getline(file, line)){

        if(line.empty()) continue;

        std::vector<std::string> tokens = tokenizer(line);

        if(tokens.empty()) continue;

        Node source = BN.getVertex(tokens[0]);

        if(source.isEmpty()){
            std::cerr << "Source node not found: " << tokens[0] << std::endl;
            continue;
        }
        
        for(std::size_t i = 1; i < tokens.size(); i++){

            Node destination = BN.getVertex(tokens[i]);

            if(destination.isEmpty()){
                std::cerr << "Destination node not found: " << tokens[i] << std::endl;
                continue;
            }

            BN.insertEdge(source, destination);
        }
    }

    file.close();

    return true;
}

void showPredecessorsNode(BayesianNetwork& BN, Node node){
    std::vector<Node> predecessors = BN.getPredecessors(node);

    std::cout<< node.getName() << ": ";
    if(predecessors.empty()){
        std::cout<< "none";
    }else{
        for(std::size_t i = 0; i < predecessors.size(); i++){
            std::cout<< predecessors[i].getName() << " ";
        }
    }

    std::cout<< std::endl;
}