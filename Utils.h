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

#ifndef TRACEVIZ_UTILS_H
#define TRACEVIZ_UTILS_H

using namespace std;

#include <string>
#include "pin.H"
#include "InterestingRoutines.h"

ADDRINT get_eax_pointer(CONTEXT* ctxt);
bool is_addr_in_main(ADDRINT addr);
string addr_to_image(ADDRINT addr, int* id);
int addr_to_section_idx(ADDRINT addr);
InterestingRoutinePtr get_interesting_routine_state_info(ADDRINT addr, int tid);
void delete_interesting_routine_state_info(ADDRINT addr, int tid);
string sec_type_to_string(SEC_TYPE type);
string get_command_line();
void get_time(UINT64 *now);
string get_filename(string pathplusfname);
string truncate_string(string inputs, int new_length);
void find_and_replace_all(string& data, string replacee, string replacer);
string wstring_to_string(wstring input);

#endif
