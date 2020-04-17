/***
	Copyright 2020 P.S.Powledge

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this softwareand associated documentation files(the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions :

	The above copyright noticeand this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
***/

#ifndef DOTGENERATOR2_H
#define DOTGENERATOR2_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

const int max_imgname_len = 60;
const int max_symbol_len = 40;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                          Section2Color: map section indexes to colors                                          *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class Section2Color;

class Section2Color
{
public:
	static Section2Color* getInstance();
	string getColor(int sectionId);
private:
	static Section2Color* inst;
	static const string colors[];
	Section2Color() {};
};
typedef Section2Color* Section2ColorPtr;


/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                         NodeItem: class to hold info about nodes                                               *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class NodeItem;

class NodeItem
{
public:
	int tid;
	string nodeid;
	string label;
	string color;
	bool isplaceholder;
	NodeItem(int t, string nn, string l, string c) : tid(t), nodeid(nn), label(l), color(c), isplaceholder(false) {};
	// default copy constructor / assignment operator
	friend ostream& operator<<(ostream& out, const NodeItem& n);
};
typedef NodeItem* NodeItemPtr;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                           NodeManager: class to manage nodes                                                   *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class NodeManager
{
public:
	int node_counter;
	map<int, string> tidlist;
	map<int, NodeItemPtr> node_map; // key -> node_counter, value = NodeItemPtr
	vector<NodeItemPtr> placeholder_nodes;

	NodeManager();
	~NodeManager();
	// default copy constructor / assignment operator. Warning: will do a shallow copy of node_map and placeholder_nodes
	// Warning: class that calls addNode is responsible for heap management of parameter node object
	void addNode(NodeItemPtr n);
	vector<int> getTids();
	vector<NodeItemPtr> getNodes();
	vector<NodeItemPtr> getNodesForTid(int tid);
	vector<pair<NodeItemPtr, NodeItemPtr>> getNodesThatJumpTids();
	friend ostream& operator<<(ostream& os, NodeManager& n);
private:
	NodeItemPtr getPlaceholderNode(int tid, int node_idx);
};
typedef NodeManager* NodeManagerPtr;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                  DotGenerator2: class to generate the dot file                                                 *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class DotGenerator2
{
public:
	NodeManagerPtr node_manager;
	string fname;
	ofstream file_output;
	Section2Color* sections2colors;

	DotGenerator2(const char *fname, const char *comment);
	~DotGenerator2();
	// use default copy constructors and assignment operators. Warning: will do a shallow copy of node_manager.
	void addNewImage(int tid, string imgname, int secidx, ADDRINT start_address);
	void addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, string arg = "");
	static string formatDetailsLines(vector<string> input_strings);
	void closeOutputFile();
	friend ostream& operator<<(ostream& out, const DotGenerator2& dg);

private:
	string buildNodeId(int tid);
	void addImageLoadNode(NodeItemPtr n);
	void addLargeLibCallNode(NodeItemPtr n);
	void addLibCallNode(NodeItemPtr n);
	void buildClusters();
};
typedef DotGenerator2* DotGenerator2Ptr;

#endif
