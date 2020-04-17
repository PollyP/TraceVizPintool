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

#ifndef TRACEVIZ_H
#define TRACEVIZ_H

using namespace std;
#include "InterestingRoutines.h"

// keep a running list of loaded images
struct image_data_s
{
	string name;
	UINT32 id;
};
typedef struct image_data_s ImageData, * ImageDataPtr;
typedef map<ADDRINT, ImageDataPtr> ImageDataMap;	// key = mapped image low address

// keep a list of sections (segments) associated with the main executable
struct section_data_s
{
	ADDRINT start_addr;
	ADDRINT end_addr;
};
typedef struct section_data_s SectionData, * SectionDataPtr;

// keep a running list of symbols. this is for all loaded images.
struct symbol_s
{
	string undecSymbol;
	string source_image;
	UINT32 source_image_id;
	bool hasCustomInstrumentFunction;
};
typedef struct symbol_s Symbol, * SymbolPtr;
static map<ADDRINT, SymbolPtr> trace_dot_symbols;	// key = mapped symbol address

extern ofstream logfile;

// for some routines, we want to capture extra information. we do that by
// capturing state information at the routine's entry and using it when the
// routine returns to main. this data structure holds the state information
// we need at return time.
typedef map<ADDRINT, vector<InterestingRoutinePtr>> RoutineStateInfo;	// key is the routine's return address
// fixme should probably turn inner vector into a map for efficent lookup reasons
// fixme clunky! turn into a class so i can do a delete by ptr method?

#endif