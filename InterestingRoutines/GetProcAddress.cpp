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
getprocaddress_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::HMODULE hModule, WINDOWS::LPCSTR funcName, ADDRINT return_addr, CONTEXT* ctxt)
{

	// funcName could be a pointer to a string, or it could be an ordinal
	//// fixme: how to detect if funcName is in heap or stack?
	ADDRINT p = (ADDRINT)funcName;
	string newfuncName = "unknown";
	PIN_LockClient();
	IMG img = IMG_FindByAddress(p);
	PIN_UnlockClient();
	if (!IMG_Valid(img) )
	{
		logfile << "not mapped" << endl;
		// funcName is not in a mapped memory region. must be an ordinal
		std::stringstream out;
		out << "Ordinal_" << p;
		newfuncName = out.str();
	}
	else
	{
		newfuncName = funcName;
	}

	if (is_addr_in_main(return_addr))
	{
		logfile << "[getprocaddr analysis] instruction addr: " << hex << showbase << inst_addr << " hModule: " << hModule << " function name: " << newfuncName << " tid: " << tid << " return address: " << hex << showbase << return_addr << endl;
		// this is a getprocessaddr() getting called from the main executable

		// save the return address and arguments
		if (get_interesting_routine_state_info(return_addr, tid) != NULL)
		{
			logfile << "getprocaddr return address " << hex << showbase << return_addr << " already exists?!?" << endl;
			InterestingRoutinePtr x = get_interesting_routine_state_info(return_addr, tid);
			logfile << "get_interesting_routine_state_info: match: " << x << endl;
			return;
		}
		GetProcAddrRoutinePtr routine_info_ptr = new GetProcAddrRoutine();
		assert(routine_info_ptr != NULL);
		routine_info_ptr->tid = tid;
		routine_info_ptr->funcaddr = inst_addr;
		routine_info_ptr->arg_func_name = (string)newfuncName;
		routine_info_ptr->arg_hmodule = (ADDRINT)hModule;
		routine_info_ptr->calling_address = last_main_instr;
		interesting_routines_state_info[return_addr].push_back(routine_info_ptr);
	}
}

void
getprocaddress_return_processing(int tid, GetProcAddrRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt)
{

	// get the FARPROC addr returned by getprocaddr
	ADDRINT ret_funcaddr = get_eax_pointer(ctxt);

	// look up the previously saved function name and image info
	string arg_funcname = state_info_ptr->arg_func_name;
	ADDRINT arg_hmodule = state_info_ptr->arg_hmodule;
	ADDRINT funcaddr = state_info_ptr->funcaddr;
	ADDRINT calling_address = state_info_ptr->calling_address;
	int section_idx = state_info_ptr->section_idx;

	logfile << "hModule = " << arg_hmodule << endl;

	string image_name = "ImageNameUnknown";
	UINT32 image_id = -1;
	if (image_map.count(arg_hmodule) > 0)
	{
		ImageDataPtr image_data_ptr = image_map[arg_hmodule];
		image_name = image_data_ptr->name;
		image_id = image_data_ptr->id;
	}

	// build the details lines
	vector<string> inputs;
	stringstream line1;
	string trunc_arg_funcname = truncate_string(arg_funcname, max_symbol_len);
	line1 << "args: " << trunc_arg_funcname;
	inputs.push_back(line1.str());
	stringstream line2;
	find_and_replace_all(image_name, "\\", "\\\\");
	string trunc_image_name = truncate_string(image_name, max_imgname_len);
	line2 << trunc_image_name;
	inputs.push_back(line2.str());
	stringstream line3;
	line3 << "returned " << hex << showbase << ret_funcaddr;
	inputs.push_back(line3.str());
	string details = DotGenerator2::formatDetailsLines(inputs);

	// log all this info
	logfile << "[interesting_routines_processing] " << details << endl;
	logfile << "; branch/call to library routine (via interesting routines) libcall: " << symbol_name << " tid: " << tid << details << endl;

	dotgen->addNewLibCall(tid, symbol_name, source_image, section_idx, funcaddr, StringFromAddrint(calling_address), details);

}

