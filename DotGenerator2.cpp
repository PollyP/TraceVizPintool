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

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "DotGenerator2.h"

using namespace std;

extern ofstream logfile;
extern string get_filename(string pathplusfname);
extern string truncate_string(string inputs, int new_length);;
extern void find_and_replace_all(string& data, string replacee, string replacer);

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                          Section2Color: map section indexes to colors                                          *
 *                                                                                                                                *
/*********************************************************************************************************************************/

Section2ColorPtr Section2Color::inst = NULL;
const string Section2Color::colors[] = { "yellow", "pink", "lightblue", "orange", "green",  "tan", };

Section2Color *Section2Color::getInstance()
{
	if (Section2Color::inst == NULL)
	{
		Section2Color::inst = new Section2Color();
	}
	return Section2Color::inst;
}

string Section2Color::getColor(int section_idx)
{
	int idx = section_idx % Section2Color::colors->length();
	return Section2Color::inst->colors[idx];
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                         NodeItem: class to hold info about nodes                                               *
 *                                                                                                                                *
/*********************************************************************************************************************************/

ostream& operator<<(ostream& os, const NodeItem& n)
{
	os <<  " nodeid: " << n.nodeid << " label " << n.label << " color: " << n.color ;
	return os;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                           NodeManager: class to manage nodes                                                   *
 *                                                                                                                                *
/*********************************************************************************************************************************/

NodeManager::NodeManager()
{
	this->node_counter = 0;
}

NodeManager::~NodeManager()
{
	this->node_counter = 0;

	// clean up the heap from all the internally-created (ie placeholder) nodes
	// note: the class that added nodes via addNode() is responsible for reclaiming that heap.
	for (auto n : this->placeholder_nodes)
	{
		delete n;
	}
}

vector<int> NodeManager::getTids()
{
	vector<int> ret;
	for (map<int, string>::iterator it = this->tidlist.begin(); it != this->tidlist.end(); ++it)
	{
		ret.push_back(it->first);
	}
	sort(ret.begin(), ret.end());
	return ret;
}

void NodeManager::addNode(NodeItemPtr n)
{
	this->tidlist[n->tid] = "";
	this->node_map[this->node_counter] = n;
	this->node_counter++;
	//fprintf(stderr, "node counter = %d\n", this->node_counter);
}

vector<NodeItemPtr> NodeManager::getNodes()
{
	vector<NodeItemPtr> ret;

	for (pair<int, NodeItemPtr> element : this->node_map)
	{
		NodeItemPtr n = element.second;
		ret.push_back(n);
	}
	return ret;
}

vector<NodeItemPtr> NodeManager::getNodesForTid(int tid)
{
	vector<NodeItemPtr> ret;

	for (pair<int, NodeItemPtr> element : this->node_map)
	{
		int node_idx = element.first;
		NodeItemPtr n = element.second;

		// does this node come from this tid?
		if (n->tid != tid)
		{
			// no, generate a placeholder node first
			n = this->getPlaceholderNode(tid, node_idx);
		}
		ret.push_back(n);
	}
	return ret;
}

vector<pair<NodeItemPtr,NodeItemPtr>> NodeManager::getNodesThatJumpTids()
{
	vector<pair<NodeItemPtr,NodeItemPtr>> ret;
	for (pair<int, NodeItemPtr> kv : this->node_map)
	{
		int i = kv.first;
		NodeItemPtr n = kv.second;
		if (this->node_map.find(i + 1) != this->node_map.end())
		{
			NodeItemPtr nextnode = this->node_map[i + 1];
			if (n->tid != nextnode->tid)
			{
				pair<NodeItemPtr, NodeItemPtr> apair = { n, nextnode };
				ret.push_back(apair);
			}
		}

	}
	return ret;
}

NodeItemPtr NodeManager::getPlaceholderNode(int tid, int node_idx)
{
	// build unique node id
	stringstream ss;
	ss << "cluster_" << tid << "_node_" << node_idx;

	// create the nodeitem
	// heap management is in class dtor
	NodeItemPtr ret = new NodeItem(tid, ss.str(), "placeholder", "red");
	ret->isplaceholder = true;

	// add the placeholder node on a list so we can reclaim the heap
	this->placeholder_nodes.push_back(ret);

	return ret;
}

ostream& operator<<(ostream& os, NodeManager& nm)
{
	os << "nm state:\n";
	os << "\tnode counter: " << nm.node_counter << "\n";
	vector<int> tids = nm.getTids();
	os << "\ttid count = " << tids.size() << "\n";
	for (auto t : tids)
	{
		os << "\ttid = " << t << "\n";
		vector<NodeItemPtr> nodes = nm.getNodesForTid(t);
		for (auto n : nodes)
		{
			os << "\t\t" << *n << "\n";
		}
	}
	return os;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                  DotGenerator2: class to generate the dot file                                                 *
 *                                                                                                                                *
/*********************************************************************************************************************************/

DotGenerator2::DotGenerator2(const char * fname, const char *comment)
{
	this->fname = fname;
	this->file_output.open(this->fname.c_str(), ios::out | ios::trunc);
	if (this->file_output.is_open() != true)
	{
		logfile << "could not open " << fname << endl;
		return;
	}

	this->node_manager = new NodeManager();
	assert(this->node_manager != NULL);

	this->sections2colors = Section2Color::getInstance();

	this->file_output << "digraph {" << endl;
	this->file_output << "\t# " << comment << endl;
	this->file_output << "\tlabel=\"" << comment << "\";" << endl;
	this->file_output << "\tcompound=true;" << endl;
	this->file_output << "\tedge[style=\"invis\"];" << endl;
	this->file_output << "\tnode [shape=rectangle, style=filled, height=1.5, width=6.0, fixedsize=true, margin=.25];        # units=inches" << endl;
	this->file_output << endl;
}

DotGenerator2::~DotGenerator2()
{
	// finish writing output file
	this->closeOutputFile();

	// clean up heap'ed memory
	vector<NodeItemPtr> nodes = this->node_manager->getNodes();
	for (auto n : nodes)
	{
		delete n;
	}

	// clean up heap'ed memory, part two
	delete this->node_manager;
}

void DotGenerator2::addNewImage(int tid, string imgname, int secidx, ADDRINT start_address )
{
	// build a unique node id
	string node_id = this->buildNodeId(tid);

	// build a label for the node
	string filename = get_filename(imgname);
	find_and_replace_all(imgname, "\\", "\\\\");
	string trunc_imgname = truncate_string(imgname, max_imgname_len);
	stringstream ss2;
	ss2 << "image load: mapped " << filename << " to " << hex << showbase << start_address << "\\lfull path: " << trunc_imgname << "\\l";
	string label = ss2.str();

	// turn this into a node and add it to our nodemanager
	NodeItemPtr n = new NodeItem(tid, node_id, label, "lightgray");
	assert(n != NULL);
	this->addImageLoadNode(n);
}

void DotGenerator2::addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, string details)
{
	// if there's a lot of text, make the node larger
	bool needsLargeNode = false;
	if (details != "")
	{
		needsLargeNode = true;
	}

	// build a unique node id
	string node_id = this->buildNodeId(tid);

	// build a label for the node
	string filename = get_filename(imgname);
	find_and_replace_all(imgname, "\\", "\\\\");
	string trunc_imgname = truncate_string(imgname, max_imgname_len);
	string trunc_symbol = truncate_string(symbol, max_symbol_len);
	stringstream ss2;
	ss2 << "library call from " << calling_address << " \\l" << trunc_symbol << " (" << hex << showbase << addr << ", " << filename << ") \\l" << "full path: " << trunc_imgname << " \\l";
	ss2 << details;
	string label = ss2.str();

	// map the section idx to a background color
	string color = this->sections2colors->getColor(secidx);

	// turn this into a node and add it to our nodemanager
	NodeItemPtr n = new NodeItem(tid, node_id, label, color);
	assert(n != NULL);
	if (needsLargeNode)
	{
		this->addLargeLibCallNode(n);
	}
	else
	{
		this->addLibCallNode(n);
	}
}

string DotGenerator2::formatDetailsLines(vector<string> input_strings)
{
	stringstream ss;
	for (auto is : input_strings)
	{
		ss << is << "\\l";
	}
	return ss.str();
}

void DotGenerator2::closeOutputFile()
{

	if (this->file_output.is_open())
	{
		this->buildClusters();

		this->file_output << "}" << endl;
		this->file_output.close();
	}
}

void DotGenerator2::buildClusters()
{
	// for every thread we saw ...
	vector<int> tids = this->node_manager->getTids();
	for (const auto tid : tids)
	{
		// get the nodes associated with that thread
		vector<NodeItemPtr> nodes = this->node_manager->getNodesForTid(tid);
		if (nodes.size() == 0)
		{
			continue;
		}

		// turn the nodes into a cluster

		// step 1. build cluster header
		stringstream ss;
		ss << "cluster_" << tid;
		string cluster_id = ss.str();

		this->file_output << endl;
		this->file_output << "\tsubgraph " << cluster_id << " {" << endl;
		this->file_output << "\t\t# " << cluster_id << endl;
		this->file_output << "\t\tlabel=\"thread #" << tid << "\"" << endl;

		// step 2. xdot doesn't give you a way to line up the nodes
		// as a grid, so I create placeholder nodes to get things to
		// line up. yes, it's a horrible hack. :( anyway, i need to first
		// define the placeholder nodes in the cluster.
		this->file_output << "\t\t# placeholder nodes" << endl;
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (nodes[i]->isplaceholder)
			{
				this->file_output << "\t\tnode [label=\"" << nodes[i]->label << "\", style=invis, fillcolor=\"" << nodes[i]->color << "\", height=1.25] " << nodes[i]->nodeid << ";" << endl;
			}
		}

		// step 3. link up the nodes in this cluster.
		stringstream ss2;
		ss2 << "\t\t" << nodes.front()->nodeid;
		for ( int i = 1 ; i < (int)nodes.size() ; i++ )
		{
			ss2 << " -> " << nodes[i]->nodeid << " ";
		}
		
		// step 4. build cluster tail
		this->file_output << ss2.str() << ";"  << endl;
		this->file_output << "\t} # subgraph for " << cluster_id << endl;
	}

	// is the application multithreaded? If so make the various threads line us nicely in the output
	// by building links for each node in thread x that is followed by a node in thread y
	vector<pair<NodeItemPtr,NodeItemPtr>> jumppairs = this->node_manager->getNodesThatJumpTids();
	if ( jumppairs.size() > 0 )
	{
		this->file_output << "\n\n\t# thread jumps" << endl;
		for (const auto jumppair : jumppairs)
		{
			// xdot builds really strange diagrams when you have descendent clusters linking back to ancestor clusters. leave them out.
			//if (true)
			if (jumppair.first->nodeid < jumppair.second->nodeid)
			{
				this->file_output << "\t" << jumppair.first->nodeid << " -> " << jumppair.second->nodeid << ";" << endl;
			}
		}
	}
}

// build a unique node id
string DotGenerator2::buildNodeId(int tid)
{
	stringstream ss;
	ss << "cluster_" << tid << "_node_" << this->node_manager->node_counter;
	return ss.str();
}

// write this node to file output and add it to the node manager for later clustering/linking.
void DotGenerator2::addImageLoadNode(NodeItemPtr n)
{
	this->file_output << "\tnode [label=\"" << n->label << "\", style=\"filled, rounded\", fillcolor=\"" << n->color << "\", height=0.75] " << n->nodeid << ";" << endl;
	this->node_manager->addNode(n);
}

// write this node to file output and add it to the node manager for later clustering/linking.
void DotGenerator2::addLargeLibCallNode(NodeItemPtr n)
{
	this->file_output << "\tnode [label=\"" << n->label << "\", style=filled, fillcolor=\"" << n->color << "\", height=1.25] " << n->nodeid << ";" << endl;
	this->node_manager->addNode(n);
}

// write this node to file output and add it to the node manager for later clustering/linking.
void DotGenerator2::addLibCallNode(NodeItemPtr n)
{
	this->file_output << "\tnode [label=\"" << n->label << "\", style=filled, fillcolor=\"" << n->color << "\", height=1.0] " << n->nodeid << ";" << endl;
	this->node_manager->addNode(n);
}

// dump the state to an ostream
inline ostream& operator<<(ostream& os, const DotGenerator2& dg)
{
	os << "dg state:\n";
	os << "\tfname: " << dg.fname;
	os << "\tnm: " << *(dg.node_manager);
	return os;
}
