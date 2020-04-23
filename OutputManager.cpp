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

#include "OutputManager.h"

using namespace std;

extern ofstream logfile;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                     OutputManager: manage DotGenerator and CSVGenerator classes                                *
 *                                                                                                                                *
/*********************************************************************************************************************************/

OutputManager* OutputManager::inst = NULL;

OutputManager* OutputManager::getInstance()
{
	if (OutputManager::inst == NULL)
	{
		OutputManager::inst = new OutputManager();
	}
	return OutputManager::inst;
}

OutputManager::OutputManager()
{
}

OutputManager::~OutputManager()
{
	if (this->csvgen) delete this->csvgen;
	if (this->dotgen) delete this->dotgen;
}

void OutputManager::initialize(string csv_output_fname, string dot_output_fname, string app_command_line_and_ts)
{
	this->csvgen = new CSVGenerator(csv_output_fname.c_str(), app_command_line_and_ts.c_str());
	assert(this->csvgen != NULL);
	this->dotgen = new DotGenerator(dot_output_fname.c_str(), app_command_line_and_ts.c_str());
	assert(this->dotgen != NULL);
}

void OutputManager::setMainSectionInfo(vector<SectionDataPtr> section_info)
{
	if (this->csvgen) this->csvgen->setMainSectionInfo(section_info);
}

void OutputManager::addNewImage(int tid, string imgname, int secidx, ADDRINT start_address)
{
	if (this->csvgen) this->csvgen->addNewImage(tid, imgname, secidx, start_address);
	if (this->dotgen) this->dotgen->addNewImage(tid, imgname, secidx, start_address);
}

void OutputManager::addNewLibCall(int tid, string symbol, string imgname, int secidx, ADDRINT addr, string calling_address, vector<string> details)
{
	string csvdetails = "";
	string dotdetails = "";
	if (details.size() > 0)
	{
		csvdetails = CSVGenerator::formatDetailsLines(details);
		dotdetails = DotGenerator::formatDetailsLines(details);
	}

	if (this->csvgen) this->csvgen->addNewLibCall(tid, symbol, imgname, secidx, addr, calling_address, csvdetails);
	if (this->dotgen) this->dotgen->addNewLibCall(tid, symbol, imgname, secidx, addr, calling_address, dotdetails);
}

void OutputManager::closeOutputFile()
{
	if (this->csvgen) this->csvgen->closeOutputFile();
	if (this->dotgen) this->dotgen->closeOutputFile();
}

ostream& operator<<(ostream& os, const OutputManager& om)
{
	os << "output manager state:\n";
	os << "\tcsv: " << om.csvgen << "\n";
	os << "\tcsv: " << om.dotgen << "\n";
	return os;
}