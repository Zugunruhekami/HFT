#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <set>
#include <limits>

using namespace std;

typedef pair<unsigned int, pair<unsigned int, unsigned int>> edge;
typedef list<edge> edgesList;
typedef list<pair<unsigned int, unsigned int>> edgeIndexList;


class Graph
{
private:
	edgesList edges;
	edgeIndexList solutionEdges;
	unsigned int verticeCount;

public:
	explicit Graph(unsigned int count) : verticeCount(count) {}

	void addEdge(unsigned int i, unsigned int j, unsigned int weight)
	{
		edges.push_back(make_pair(weight, make_pair(i, j)));
	}

	unsigned int getVerticeCount() const
	{
		return verticeCount;
	}

	const edgesList& getEdges() const
	{
		return edges;
	}

	void sortEdges()
	{
		edges.sort();
	}

};

class Kruskal {
private:
	Graph* graph;
	vector<unsigned int> elements;
	vector<unsigned int> parent;

	void createSet(unsigned int x) {
		elements[x] = 0;
		parent[x] = x;
	}

	unsigned int findSet(unsigned int x) {
		if (x != parent[x]) parent[x] = findSet(parent[x]);
		return parent[x];
	}

	void mergeSet(unsigned int x, unsigned int y) {
		if (elements[x] > elements[y]) parent[y] = x;
		else parent[x] = y;
		if (elements[x] == elements[y]) ++elements[y];
	}

public:
	explicit Kruskal(Graph* inGraph) : graph(inGraph) {}
	unsigned int Execute(edgeIndexList& solutionEdges) {
		elements.resize(graph->getVerticeCount());
		parent.resize(graph->getVerticeCount());
		unsigned int cost = 0;
		unsigned int remaining = graph->getVerticeCount() - 1;
		for (unsigned int i = 0; i < graph->getVerticeCount(); ++i)
		{
			createSet(i);
		}
		graph->sortEdges();
		for (auto& edge : graph->getEdges()) {
			unsigned int u = findSet(edge.second.first);
			unsigned int v = findSet(edge.second.second);
			if (u == v)
				continue;
			solutionEdges.push_back(make_pair(u, v));
			mergeSet(u, v);
			cost += edge.first;
			if (!--remaining)
				break;
		}

		if (remaining)
			return std::numeric_limits<unsigned int>::infinity();
		return cost;
	}
};

class DataCenter
{
private: 
	unsigned int dataCenterCount;
	unsigned int** dataCenterDistance;
public:
	DataCenter() {
		/* distance.txt
		matrix of distance between data centers including distance inside of each data center
		*/
		std::ifstream inputDistance("distance.txt");
		unsigned int weight;
		inputDistance >> dataCenterCount;
		dataCenterDistance = new unsigned int*[dataCenterCount];
		for (unsigned int i = 0; i < dataCenterCount; ++i) {
			dataCenterDistance[i] = new unsigned int[dataCenterCount];
			for (unsigned int j = 0; j <= i; ++j) {
				inputDistance >> weight;
				dataCenterDistance[i][j] = dataCenterDistance[j][i] = weight;
				//cout << i << "," << j << "=" << weight << endl;
			}
		}
		inputDistance.close();
	}

	unsigned int distance(unsigned int i, unsigned int j) const
	{
		return dataCenterDistance[i][j];
	}

	~DataCenter()
	{
		for (unsigned int i = 0; i < dataCenterCount; i++)
			delete dataCenterDistance[i];
		delete[] dataCenterDistance;
	}

};

int main() {
	vector<unsigned int> mappingOfIndexes; //current index -> new index
	vector<unsigned int> dataCenterMapping; //machine id -> data center id
	set<unsigned int> ignoreVertice;
	unsigned int verticeCount;

	/* machines.txt:
	first line: number of vertices
	others: <load factor> <data center id>
	*/
	std::ifstream inputMachines("machines.txt");
	inputMachines >> verticeCount;
	unsigned int loadFactor;
	unsigned int dataCenterId;
	for (unsigned int i = 0; i < verticeCount; ++i)
	{
		inputMachines >> loadFactor;
		if (loadFactor < 10)
			mappingOfIndexes.push_back(i);
		else
			ignoreVertice.insert(i);
		inputMachines >> dataCenterId;
		dataCenterMapping.push_back(dataCenterId);
	}
	inputMachines.close();

	DataCenter dataCenter;

	//create graph of machines
	unsigned int ignoredIs = 0, ignoredJs = 0;
	const unsigned int newVerticeCount = verticeCount - ignoreVertice.size();
	Graph graph(newVerticeCount);
	unsigned int weight;
	for (unsigned int i = 1; i < verticeCount; ++i) {
		if (ignoreVertice.find(i) != ignoreVertice.end())
		{
			ignoredIs++;
			continue; //loadFactor >=10 so ignore
		}
		for (unsigned int j = 0; j < i; ++j) {
			weight = dataCenter.distance(dataCenterMapping[i],dataCenterMapping[j]);
			if (ignoreVertice.find(j) != ignoreVertice.end())
			{
				ignoredJs++;
				continue; //loadFactor >=10 so ignore
			}
			graph.addEdge(i - ignoredIs, j - ignoredJs, weight);
		}
	}

	edgeIndexList solutionEdges;
	Kruskal kruskal(&graph);
	cout << "Cost of minimum spanning tree: " << kruskal.Execute(solutionEdges) << endl;
	cout << "Following machine connections (edges) should be chosen (by index): " << endl;
	for (auto& edge : solutionEdges)
	{
		cout << mappingOfIndexes[edge.first] << "," << mappingOfIndexes[edge.second] << "\t";
	}

	return 0;
}