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

#ifndef CVSGENERATOR_H
#define CVSGENERATOR_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "TraceViz.h"

using namespace std;


/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                        CVSNodeItem: class to hold info about nodes                                             *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class CVSNodeItem;

class CVSNodeItem
{
public:
	int tid;
	int secidx;
	string label;
	CVSNodeItem(int t, int si, string l ) : tid(t), secidx(si), label(l) {};
	// default copy constructor / assignment operator
	friend ostream& operator<<(ostream& out, const CVSNodeItem& n);
};
typedef CVSNodeItem* CVSNodeItemPtr;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                         CVSNodeManager: class to manage nodes                                                  *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class CSVNodeManager
{
public:
	int node_counter;
	map<int, string> tidlist;			// key = tid, value = unused
	map<int, CVSNodeItemPtr> node_map;	// key -> node_counter, value = CVSNodeItemPtr

	CSVNodeManager();
	~CSVNodeManager();
	// default copy constructor / assignment operator. Warning: will do a shallow copy of node_map and placeholder_nodes
	// Warning: class that calls addNode is responsible for heap management of parameter node object
	void addNode(CVSNodeItemPtr n);
	vector<int> getTids();
	vector<CVSNodeItemPtr> getNodes();
	friend ostream& operator<<(ostream& os, const CSVNodeManager& n);
};
typedef CSVNodeManager* CVSNodeManagerPtr;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                  CVSGenerator: class to generate the dot file                                                 *
 *                                                                                                                                *
/*********************************************************************************************************************************/

class CSVGenerator
{
public:
	CVSNodeManagerPtr node_manager;
	string fname;
	ofstream file_output;
	vector<SectionDataPtr> section_info;

	CSVGenerator(const char* fname, const char* comment);
	~CSVGenerator();
	// use default copy constructors and assignment operators. Warning: will do a shallow copy of node_manager.
	void setMainSectionInfo(vector<SectionDataPtr>);
	void addNewImage(int tid, string imgname, int secidx, ADDRINT start_address);
	void addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, string arg = "");
	static string formatDetailsLines(vector<string> input_strings);
	void closeOutputFile();
	friend ostream& operator<<(ostream& out, CSVGenerator& cg);

private:
	void writeCSVData();
};
typedef CSVGenerator* CSVGeneratorPtr;

#endif
