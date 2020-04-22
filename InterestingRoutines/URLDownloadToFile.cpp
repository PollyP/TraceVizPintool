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

#include <string>

#include "pin.H"

using namespace std;

namespace WINDOWS
{
#include <Windows.h>
}

#include "../Utils.h"
#include "InterestingRoutines.h"
#include "../TraceViz.h"
#include "../OutputManager.h"

extern ImageDataMap image_map;
extern RoutineStateInfo interesting_routines_state_info;
extern OutputManagerPtr output_manager;
extern ADDRINT last_main_instr;
extern vector<SectionDataPtr> section_data;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                routine analysis functions                                                      *
 *                                                                                                                                *
/*********************************************************************************************************************************/

void
urldownloadtofile_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPUNKNOWN pCaller, WINDOWS::LPCSTR szURL, WINDOWS::LPCSTR szFileName, ADDRINT return_addr, CONTEXT* ctxt)
{
	logfile << "[URLDownloadToFile analysis] " << hex << showbase << inst_addr << " szURL " << szURL << " szFileName " << szFileName << " return address " << hex << showbase << return_addr << endl;
	if (is_addr_in_main(return_addr))
	{
		// this is a URLDownloadToFile() getting called from the main executable

		// save the return address and arguments
		if (get_interesting_routine_state_info(return_addr, tid) != NULL)
		{
			logfile << "URLDownloadToFile return address " << hex << showbase << return_addr << "already exists?!?" << endl;
			return;
		}
		URLDownloadToFileRoutinePtr routine_info_ptr = new URLDownloadToFileRoutine();
		assert(routine_info_ptr != NULL);
		routine_info_ptr->tid = tid;
		routine_info_ptr->funcaddr = inst_addr;
		routine_info_ptr->arg_szurl = (string)szURL;
		routine_info_ptr->arg_szfilename = (ADDRINT)szFileName;
		routine_info_ptr->calling_address = last_main_instr;
		interesting_routines_state_info[return_addr].push_back(routine_info_ptr);
	}
}

void
urldownloadtofile_return_processing(int tid, URLDownloadToFileRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt)
{

	// get the hresult returned by urldownloadtofile
	UINT32 hResult = (UINT32)get_eax_pointer(ctxt);

	// look up the previously saved function name and image info
	string arg_szurl = state_info_ptr->arg_szurl;
	string arg_szfilename = state_info_ptr->arg_szfilename;
	ADDRINT funcaddr = state_info_ptr->funcaddr;
	ADDRINT calling_address = state_info_ptr->calling_address;
	int section_idx = state_info_ptr->section_idx;

	// build the details lines
	vector<string> inputs;
	stringstream line1;
	//string trunc_arg_szurl = truncate_string(arg_szurl, max_symbol_len);
	line1 << "args: " << arg_szurl;
	inputs.push_back(line1.str());
	stringstream line2;
	find_and_replace_all(arg_szfilename, "\\", "\\\\");
	//string trunc_arg_lpfile = truncate_string(arg_szfilename, max_imgname_len);
	line2 << arg_szfilename;
	inputs.push_back(line2.str());
	stringstream line3;
	line3 << "returned " << hex << showbase << hResult;
	inputs.push_back(line3.str());

	// log all this info
	logfile << "[interesting_routines_processing] " << line1 << " " << line2 << " " << line3 << endl;
	logfile << "; branch/call to library routine (via interesting routines) libcall: " << symbol_name << " tid: " << tid << " " << line1 << " " << line2 << " " << line3 << endl;

	output_manager->addNewLibCall(tid, symbol_name, source_image, section_idx, funcaddr, StringFromAddrint(calling_address), inputs);
}

