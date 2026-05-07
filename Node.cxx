#include "Node.h"

Node::Node(){
    name = "";
    conditionalProbabilityTable.clear();
}

Node::Node(std::string &nName, std::vector<std::vector<std::string>> &nCPT){
    name = nName;
    conditionalProbabilityTable = nCPT;
}

Node::~Node(){
    name = "";
    conditionalProbabilityTable.clear();
}

const std::string& Node::getName() const{
    return name;
}

std::vector<std::string>& Node::getStates(){
    return states;
}

std::vector<std::vector<std::string>>& Node::getConditionalProbabilityTable(){
    return conditionalProbabilityTable;
}

void Node::setName(std::string &nName){
    name = nName;
}

bool Node::setStates(std::vector<std::string> &nStates){
    if(nStates.empty()) return false;

    states = nStates;

    return true;
}

bool Node::setConditionalProbabilityTable(std::vector<std::vector<std::string>> &nCPT){
    if(nCPT.empty()) return false;

    conditionalProbabilityTable = nCPT;

    return true;
}

bool Node::isEmpty() const{
    return name == "";
}

// State operations
int Node::numberOfStates(){
    return states.size();
}

bool Node::insertState(std::string state){
    if(searchState(state) != -1) return false;

    states.push_back(state);

    return true;
}

int Node::searchState(std::string state){
    for(std::size_t i = 0; i < states.size(); i++){
        if(states[i] == state) return (int)i;
    }

    return -1;
}

bool Node::deleteState(std::string state){
    int index = searchState(state);

    if(index == -1) return false;

    std::vector<std::string>::iterator it = states.begin() + index;
    states.erase(it);

    return true;
}

bool Node::operator==(const Node& other) const{
    return name == other.name;
}

std::string Node::repeatString(std::string text, std::size_t times){
    std::string result = "";

    for(std::size_t i = 0; i < times; i++){
        result += text;
    }

    return result;
}

std::ostream& operator<<(std::ostream& os, const Node& node){

    std::string statesText = "{";

    for(std::size_t i = 0; i < node.states.size(); i++){
        statesText += node.states[i];

        if(i != node.states.size() - 1){
            statesText += ", ";
        }
    }

    statesText += "}";

    std::size_t contentWidth = node.name.size();

    if(statesText.size() > contentWidth){
        contentWidth = statesText.size();
    }

    std::size_t innerWidth = contentWidth + 4;

    std::size_t namePaddingLeft = (innerWidth - node.name.size()) / 2;
    std::size_t namePaddingRight = innerWidth - node.name.size() - namePaddingLeft;

    std::size_t statesPaddingLeft = (innerWidth - statesText.size()) / 2;
    std::size_t statesPaddingRight = innerWidth - statesText.size() - statesPaddingLeft;

    os << "╭" << Node::repeatString("─", innerWidth) << "╮" << std::endl;

    os << "│";
    os << std::string(namePaddingLeft, ' ');
    os << node.name;
    os << std::string(namePaddingRight, ' ');
    os << "│" << std::endl;

    os << "│";
    os << std::string(statesPaddingLeft, ' ');
    os << statesText;
    os << std::string(statesPaddingRight, ' ');
    os << "│" << std::endl;

    os << "╰" << Node::repeatString("─", innerWidth) << "╯";

    return os;
}