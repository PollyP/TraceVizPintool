
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

#include <stdio.h>
#include <stdlib.h>
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
#include "../DotGenerator2.h"

extern ImageDataMap image_map;
extern RoutineStateInfo interesting_routines_state_info;
extern DotGenerator2 *dotgen;
extern ADDRINT last_main_instr;
extern vector<SectionDataPtr> section_data;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                routine analysis functions                                                      *
 *                                                                                                                                *
/*********************************************************************************************************************************/
void

createprocess_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPCSTR lpApplicationName, WINDOWS::LPSTR lpCommandLine, ADDRINT return_addr, CONTEXT* ctxt)
{
	if (is_addr_in_main(return_addr))
	{
		logfile << "[createprocess analysis] instruction addr: " << hex << showbase << inst_addr << " lpApplicationName: " << lpApplicationName << " lpCommandLine: " << lpCommandLine << " return address: " << hex << showbase << return_addr << endl;

		// save the return address and parameters
		if (get_interesting_routine_state_info(return_addr,tid) != NULL )
		{
			logfile << "createprocess return address " << hex << showbase << return_addr << "already exists?!?" << endl;
			return;
		}
		CreateProcessRoutinePtr routine_info_ptr = new CreateProcessRoutine();
		assert(routine_info_ptr != NULL);
		routine_info_ptr->tid = tid;
		routine_info_ptr->funcaddr = inst_addr;
		routine_info_ptr->arg_lpapplicationname = (string)lpApplicationName;
		routine_info_ptr->arg_lpcommandline = (ADDRINT)lpCommandLine;
		routine_info_ptr->calling_address = last_main_instr;
		interesting_routines_state_info[return_addr].push_back(routine_info_ptr);
	}
}

void
createprocess_return_processing(int tid, CreateProcessRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt)
{

	// get the boolean return code for createprocess
	ADDRINT ret_code = get_eax_pointer(ctxt);

	// look up the previously saved function name and image info
	string arg_applicationname = state_info_ptr->arg_lpapplicationname;
	string arg_lpcommandline = state_info_ptr->arg_lpcommandline;
	ADDRINT funcaddr = state_info_ptr->funcaddr;
	ADDRINT calling_address = state_info_ptr->calling_address;
	int section_idx = state_info_ptr->section_idx;

	// build the details lines
	vector<string> inputs;
	stringstream line1;
	string trunc_arg_applicationname = truncate_string(arg_applicationname, max_symbol_len);
	line1 << "args: " << trunc_arg_applicationname;
	inputs.push_back(line1.str());
	stringstream line2;
	find_and_replace_all(arg_lpcommandline, "\\", "\\\\");
	string trunc_arg_lpcommandline = truncate_string(arg_lpcommandline, max_imgname_len);
	line2 << trunc_arg_lpcommandline;
	inputs.push_back(line2.str());
	stringstream line3;
	line3 << "returned " << hex << showbase << ret_code;
	inputs.push_back(line3.str());
	string details = DotGenerator2::formatDetailsLines(inputs);

	// log all this info
	logfile << "[interesting_routines_processing] " << details << endl;
	logfile << "; branch/call to library routine (via interesting routines) libcall: " << symbol_name << " tid: " << tid << details << endl;

	dotgen->addNewLibCall(tid, symbol_name, source_image, section_idx, funcaddr, StringFromAddrint(calling_address), details);
}

