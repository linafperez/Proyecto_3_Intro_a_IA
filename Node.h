#ifndef __NODE_H__
#define __NODE_H__

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Node {

    protected:
        std::string name;
        std::vector<std::string> states;
        std::vector<std::vector<std::string>> conditionalProbabilityTable;

    public:
        Node();
        Node(std::string &nName, std::vector<std::vector<std::string>> &nCPT);
        ~Node();

        const std::string& getName() const;
        std::vector<std::string>& getStates();
        std::vector<std::vector<std::string>>& getConditionalProbabilityTable();
        void setName(std::string &nName);
        bool setStates(std::vector<std::string> &nStates);
        bool setConditionalProbabilityTable(std::vector<std::vector<std::string>> &nCPT);

        bool isEmpty() const;

        // State operations
        int numberOfStates();
        bool insertState(std::string state);
        int searchState(std::string state);
        bool deleteState(std::string state);

        bool operator==(const Node& other) const;
        friend std::ostream& operator<<(std::ostream& os, const Node& node);
        static std::string repeatString(std::string text, std::size_t times);

};



#endif