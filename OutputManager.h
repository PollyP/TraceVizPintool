
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

#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "CSVGenerator.h"
#include "DotGenerator2.h"
#include "TraceViz.h"

using namespace std;

class OutputManager;

class OutputManager
{
public:
	// singleton class
	static OutputManager* getInstance();
	void initialize(string csv_output_fname, string dot_output_fname, string app_command_line_and_ts);
	void setMainSectionInfo(vector<SectionDataPtr>);
	void addNewImage(int tid, string imgname, int secidx, ADDRINT start_address);
	void addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, vector<string> details);
	void closeOutputFile();
	friend ostream& operator<<(ostream& out, const OutputManager& om);
private:
	static OutputManager* inst;
	CSVGeneratorPtr csvgen;
	DotGeneratorPtr dotgen;
	OutputManager();
	~OutputManager();
};

typedef OutputManager* OutputManagerPtr;

#endif