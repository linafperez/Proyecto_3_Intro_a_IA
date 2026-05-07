#ifndef __BAYESIANNETWORK_H__
#define __BAYESIANNETWORK_H__

#include "Node.h"

class BayesianNetwork {

    protected:
        std::vector<Node> vertices;
        bool** edges;

    public:
        BayesianNetwork();
        BayesianNetwork(std::vector<Node> &nvertices, bool** nedges);
        ~BayesianNetwork();

        std::vector<Node>& getVertices();
        bool** getEdges();
        void setVertices(std::vector<Node>& nvertices);
        bool setEdges(bool** nedges);
        
        // Vertex operations
        int numberOfVertices();
        bool insertVertex(Node data);
        int searchVertex(Node data) const;
        bool deleteVertex(Node data);
		Node getVertex(std::string name);
		int getRoot() const;
        std::vector<Node> getPredecessors(Node node) const;

        // Edge operations
        int numberOfEdges();
        bool insertEdge(Node source, Node destination);
        bool searchEdge(Node source, Node destination); 
        bool deleteEdge(Node source, Node destination);

		void printFromVertex(std::ostream& os, int index, std::vector<bool>& visited, std::string prefix) const;
		friend std::ostream& operator<<(std::ostream& os, const BayesianNetwork& BN);


};

std::ostream& operator<<(std::ostream& os, const BayesianNetwork& BN);

#endif