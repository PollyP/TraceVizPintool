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

#include <ctime>
#include <string>
#include <vector>

#include "pin.H"

namespace WINDOWS
{
#include <Windows.h>
}

#include "TraceViz.h"

using namespace std;

extern ofstream logfile;
extern vector<SectionDataPtr> section_data;
extern RoutineStateInfo interesting_routines_state_info;

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                      utilities                                                                 *
 *                                                                                                                                *
/*********************************************************************************************************************************/


ADDRINT
get_eax_pointer(CONTEXT* ctxt)
{
	return (ADDRINT)PIN_GetContextReg(ctxt, REG_EAX);
}

bool
is_addr_in_main(ADDRINT addr)
{
	bool ret = false;
	PIN_LockClient();
	IMG img = IMG_FindByAddress(addr);
	if (IMG_Valid(img))
	{
		ret = IMG_IsMainExecutable(img);
	}
	PIN_UnlockClient();
	return ret;
}

string
addr_to_image(ADDRINT addr, int* id)
{
	string ret = "";
	PIN_LockClient();
	IMG img = IMG_FindByAddress(addr);
	if (IMG_Valid(img))
	{
		ret = IMG_Name(img);
		if (id)
		{
			*id = IMG_Id(img);
		}
	}
	PIN_UnlockClient();
	return ret;
}

int
addr_to_section_idx(ADDRINT addr)
{
	vector<SectionDataPtr>::iterator it;
	for (it = section_data.begin(); it != section_data.end(); ++it)
	{
		SectionDataPtr s = *it;
		if (addr >= s->start_addr && addr < s->end_addr)
		{
			int section_idx = distance(section_data.begin(), it);
			return section_idx;
		}
	}
	return -1;
}

InterestingRoutinePtr get_interesting_routine_state_info(ADDRINT addr, int tid)
{
	if (interesting_routines_state_info.count(addr) > 0)
	{
		vector<InterestingRoutinePtr> addr_vector = interesting_routines_state_info[addr];
		for (int i = 0; i < (int)addr_vector.size(); i++)
		{
			if (addr_vector[i]->tid == tid)
			{
				return addr_vector[i];
			}
		}
	}
	return NULL;
}

// just removes entry from vector; does not reclaim heap from actual element
void delete_interesting_routine_state_info(ADDRINT addr, int tid)
{
	if (interesting_routines_state_info.count(addr) > 0)
	{
		vector<InterestingRoutinePtr> addr_vector = interesting_routines_state_info[addr];
		for (int i = 0; i < (int)addr_vector.size(); i++)
		{
			if (addr_vector[i]->tid == tid)
			{
				addr_vector.erase(addr_vector.begin() + i);
				interesting_routines_state_info[addr] = addr_vector;
				return;
			}
		}
	}
	return;
}

string
sec_type_to_string(SEC_TYPE type)
{
	if (type == SEC_TYPE_BSS)
	{
		return "bss";
	}
	else if (type == SEC_TYPE_COMMENT)
	{
		return "comment";
	}
	else if (type == SEC_TYPE_DATA)
	{
		return "data";
	}
	else if (type == SEC_TYPE_DEBUG)
	{
		return "debug";
	}
	else if (type == SEC_TYPE_DYNAMIC)
	{
		return "dynamic";
	}
	else if (type == SEC_TYPE_DYNREL)
	{
		return "dynrel";
	}
	else if (type == SEC_TYPE_DYNSTR)
	{
		return "dynstr";
	}
	else if (type == SEC_TYPE_DYNSYM)
	{
		return "dynsym";
	}
	else if (type == SEC_TYPE_EXEC)
	{
		return "exec";
	}
	else if (type == SEC_TYPE_GOT)
	{
		return "got";
	}
	else if (type == SEC_TYPE_HASH)
	{
		return "hash";
	}
	else if (type == SEC_TYPE_LOOS)
	{
		return "loos";
	}
	else if (type == SEC_TYPE_USER)
	{
		return "user";
	}
	// fixme
	return "need to map all the sec type enums";
}

string
get_command_line()
{
	stringstream ss;
	NATIVE_PID pid;
	OS_GetPid(&pid);
	if (true)
		// fixme return code?
	//if (ret != OS_RETURN_CODE_PROCESS_QUERY_FAILED)
	{
		USIZE argc;
		char** argv;
		USIZE bufsize;
		OS_GetCommandLine(pid, &argc, &argv, &bufsize);
		ss << "Application commandline: ";
		for (uint i = 0; i < argc; i++)
		{
			ss << argv[i] << " ";
		}
		OS_FreeMemory(NATIVE_PID_CURRENT, argv, bufsize);
		return ss.str();
	}
}

void get_time(UINT64 *now)
{
	OS_Time(now);
}

string get_filename(string pathplusfname)
{
	// just use the filename part of the image name
	// fixme remove trailing slashes if any
	size_t found = pathplusfname.find_last_of("/\\");
	string fname = pathplusfname.substr(found + 1);
	stringstream ss;
	ss << fname;
	return ss.str();
}


string
truncate_string(string inputs, int new_length)
{
	if (new_length >= (int)inputs.length())
	{
		return inputs;
	}
	string trunc_text = "[...]";
	string ret = "";
	int new_length_minus_trunc = new_length - trunc_text.length();
	if (new_length_minus_trunc <= 0)
	{
		return ret;
	}
	int new_index = inputs.length() - new_length_minus_trunc;
	string new_inputs = inputs.substr(new_index);
	ret = trunc_text + new_inputs;
	return ret;
}

void
find_and_replace_all(std::string& data, std::string replacee, std::string replacer)
{
	size_t pos = data.find(replacee);
	while (pos != std::string::npos)
	{
		data.replace(pos, replacee.size(), replacer);
		pos = data.find(replacee, pos + replacer.size());
	}
}

string
wstring_to_string(wstring input)
{
	// the lowest of the low rent converters
	string str(input.begin(), input.end());
	return str;
}


