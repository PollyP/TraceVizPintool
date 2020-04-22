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

#include "CSVGenerator.h"

using namespace std;

const string quote_str = R"(")";
const string escaped_quote_str = R"(\")";

extern ofstream logfile;
extern string get_filename(string pathplusfname);
//extern string truncate_string(string inputs, int new_length);;
extern void find_and_replace_all(string& data, string replacee, string replacer);

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                       CSVNodeItem: class to hold info about nodes                                              *
 *                                                                                                                                *
/*********************************************************************************************************************************/

ostream& operator<<(ostream& os, const CVSNodeItem& n)
{
	os << "tid: " << n.tid << " section index: " << n.secidx << " label " << n.label;
	return os;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                           CSVNodeManager: class to manage nodes                                                   *
 *                                                                                                                                *
/*********************************************************************************************************************************/

CSVNodeManager::CSVNodeManager()
{
	this->node_counter = 0;
}

CSVNodeManager::~CSVNodeManager()
{
	this->node_counter = 0;
}

vector<int> CSVNodeManager::getTids()
{
	vector<int> ret;
	for (map<int, string>::iterator it = this->tidlist.begin(); it != this->tidlist.end(); ++it)
	{
		ret.push_back(it->first);
	}
	sort(ret.begin(), ret.end());
	return ret;
}

void CSVNodeManager::addNode(CVSNodeItemPtr n)
{
	this->tidlist[n->tid] = "";
	this->node_map[this->node_counter] = n;
	this->node_counter++;
}

vector<CVSNodeItemPtr> CSVNodeManager::getNodes()
{
	vector<CVSNodeItemPtr> ret;

	for (pair<int, CVSNodeItemPtr> element : this->node_map)
	{
		CVSNodeItemPtr n = element.second;
		ret.push_back(n);
	}
	return ret;
}

ostream& operator<<(ostream& os, CSVNodeManager& nm)
{
	os << "nm state:\n";
	os << "\tnode counter: " << nm.node_counter << "\n";
	vector<int> tids = nm.getTids();
	os << "\ttid count = " << tids.size() << "\n";
	for (auto t : tids)
	{
		os << "\t\t" << t << "\n";
	}
	vector<CVSNodeItemPtr> nodes = nm.getNodes();
	for (auto n : nodes)
	{
		os << "\t\t" << *n << "\n";
	}
	return os;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                  CSVGenerator: class to generate the dot file                                                 *
 *                                                                                                                                *
/*********************************************************************************************************************************/

CSVGenerator::CSVGenerator(const char* fname, const char* comment)
{
	this->fname = fname;
	this->file_output.open(this->fname.c_str(), ios::out | ios::trunc);
	if (this->file_output.is_open() != true)
	{
		logfile << "could not open " << fname << endl;
		return;
	}

	this->node_manager = new CSVNodeManager();
	assert(this->node_manager != NULL);

	this->file_output << "# " << comment << endl;
	this->file_output << endl;
}

CSVGenerator::~CSVGenerator()
{
	// finish writing output file
	this->closeOutputFile();

	// clean up heap'ed memory
	vector<CVSNodeItemPtr> nodes = this->node_manager->getNodes();
	for (auto n : nodes)
	{
		delete n;
	}

	// clean up heap'ed memory, part two
	delete this->node_manager;
}

void CSVGenerator::setMainSectionInfo(vector<SectionDataPtr> sv)
{
	this->section_info = sv;
}

void CSVGenerator::addNewImage(int tid, string imgname, int secidx, ADDRINT start_address)
{
	// build a label for the node
	string filename = get_filename(imgname);
	find_and_replace_all(imgname, "\\", "\\\\");
	stringstream ss2;
	ss2 << "image load: mapped " << filename << " to " << hex << showbase << start_address << "\\nfull path: " << imgname << "\\n";
	string label = ss2.str();

	// turn this into a node and add it to our nodemanager
	CVSNodeItemPtr n = new CVSNodeItem(tid, secidx, label);
	assert(n != NULL);
	this->node_manager->addNode(n);
}

void CSVGenerator::addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, string details)
{
	// build a label for the node
	string filename = get_filename(imgname);
	find_and_replace_all(imgname, "\\", "\\\\");
	stringstream ss2;
	ss2 << "library call from " << calling_address << " \\n" << symbol << " (" << hex << showbase << addr << ", " << filename << ") \\n" << "full path: " << imgname << " \\n";
	ss2 << details;
	string label = ss2.str();

	// turn this into a node and add it to our nodemanager
	CVSNodeItemPtr n = new CVSNodeItem(tid, secidx, label);
	assert(n != NULL);
	this->node_manager->addNode(n);
}

string CSVGenerator::formatDetailsLines(vector<string> input_strings)
{
	stringstream ss;
	for (auto is : input_strings)
	{
		ss << is << "\\n";
	}
	return ss.str();
}

void CSVGenerator::writeCSVData()
{
	vector<int> tids = this->node_manager->getTids();
	int tidlen = (int)tids.size();
	int cols_per_tid = (int)this->section_info.size();
	int cols_per_row = tidlen * cols_per_tid;

	// write the header
	for (int i = 0; i < tidlen; i++)
	{
		for (int j = 0; j < cols_per_tid; j++)
		{
			stringstream ss;
			ss << quote_str << "thread #" << i;
			if (j < (int)this->section_info.size() && this->section_info[j]->name != "")
			{
				ss << " sec #" << j << " (" << this->section_info[j]->name << ")";
			}
			else
			{
				ss << " sec #" << j;
			}
			ss << quote_str << ",";
			this->file_output << ss.str();
		}
	}
	this->file_output << endl;

	// write the data
	vector<CVSNodeItemPtr> nodes = this->node_manager->getNodes();
	for (const auto n : nodes)
	{
		int tid = n->tid;
		int secidx = n->secidx;
		for (int i = 0; i < tidlen; i++)
		{
			for (int j = 0; j < cols_per_tid; j++)
			{
				if (tids[i] == tid && j == secidx)
				{
					string label = n->label;
					find_and_replace_all(label, quote_str, escaped_quote_str);
					this->file_output << "\"" << label << "\",";
				}
				else
				{
					this->file_output << ",";
				}
			}
			this->file_output << endl;
		}
	}

	// write out the section details
	this->file_output << endl;
	this->file_output << "main section details" << endl;
	this->file_output << "index,name,start addr,end_addr" << endl;
	for ( int i = 0 ; i < (int)this->section_info.size() ; i++ )
	{
		SectionDataPtr s = this->section_info[i];
		string name = s->name;
		find_and_replace_all(name, quote_str, escaped_quote_str);
		this->file_output << dec <<  i << "," << quote_str << s->name << quote_str << "," << hex << showbase << s->start_addr << ","  << hex << showbase << s->end_addr << endl;
	}
}

void CSVGenerator::closeOutputFile()
{

	if (this->file_output.is_open())
	{
		this->writeCSVData();
		this->file_output.close();
	}
}

// dump the state to an ostream
inline ostream& operator<<(ostream& os, const CSVGenerator& cg)
{
	os << "cg state:\n";
	os << "\tfname: " << cg.fname;
	os << "\tnm: " << *(cg.node_manager);
	return os;
}
