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

#ifndef INTERESTINGROUTINES_H
#define INTERESTINGROUTINES_H

using namespace std;

// we'd like to do some extra processing on "interesting routines."
// e.g. for getprocaddr calls, we'd like to capture the function name argument
// and the returned address so we can add that to our symbol table
enum InterestingRoutineType
{
	CREATEPROCESS,
	GETPROCADDR,
	INTERNETOPENURL,
	LOADLIBRARY,
	SHELLEXECUTE,
	URLDOWNLOADTOFILE
};

class InterestingRoutine
{
public:
	InterestingRoutineType type;
	ADDRINT funcaddr;			// lib call address. use as key into symbol dictionary to get more info
	ADDRINT calling_address;	// address of instruction that called this interesting routine
	int section_idx;			// section index the calling address originated from
	int tid;					// id of thread this call was made on
	InterestingRoutine(InterestingRoutineType type) : type{ type } {};
};
typedef InterestingRoutine* InterestingRoutinePtr;

class CreateProcessRoutine : public InterestingRoutine
{
public:
	string arg_lpapplicationname;
	string arg_lpcommandline;
	CreateProcessRoutine() : InterestingRoutine(InterestingRoutineType::CREATEPROCESS) {};
};
typedef CreateProcessRoutine* CreateProcessRoutinePtr;

class GetProcAddrRoutine : public InterestingRoutine
{
public:
	string arg_func_name;		// function name argument to GetProcAddr call
	ADDRINT arg_hmodule;		// key to imageMap entry for GetProcAddr's library
	GetProcAddrRoutine() : InterestingRoutine(InterestingRoutineType::GETPROCADDR) {};
};
typedef GetProcAddrRoutine* GetProcAddrRoutinePtr;

class InternetOpenURLRoutine : public InterestingRoutine
{
public:
	string arg_lpszUrl;		// url argument to InternetOpenURL call
	InternetOpenURLRoutine() : InterestingRoutine(InterestingRoutineType::INTERNETOPENURL) {};
};
typedef InternetOpenURLRoutine* InternetOpenURLRoutinePtr;


class LoadLibraryRoutine : public InterestingRoutine
{
public:
	string arg_lpLibFileName;		// function name argument to LoadLibrary call
	LoadLibraryRoutine() : InterestingRoutine(InterestingRoutineType::LOADLIBRARY) {};
};
typedef LoadLibraryRoutine* LoadLibraryRoutinePtr;

class ShellExecuteRoutine : public InterestingRoutine
{
public:
	string arg_lpoperation;
	string arg_lpfile;
	string arg_lpparameters;
	string arg_lpdirectory;
	ShellExecuteRoutine() : InterestingRoutine(InterestingRoutineType::SHELLEXECUTE) {};
};
typedef ShellExecuteRoutine* ShellExecuteRoutinePtr;

class URLDownloadToFileRoutine : public InterestingRoutine
{
public:
	string arg_szurl;
	string arg_szfilename;
	URLDownloadToFileRoutine() : InterestingRoutine(InterestingRoutineType::URLDOWNLOADTOFILE) {};
};
typedef URLDownloadToFileRoutine* URLDownloadToFileRoutinePtr;


void createprocess_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPCSTR lpApplicationName, WINDOWS::LPSTR lpCommandLine, ADDRINT return_addr, CONTEXT* ctxt);
void createprocess_return_processing(int tid, CreateProcessRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);
void getprocaddress_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::HMODULE hModule, WINDOWS::LPCSTR funcName, ADDRINT return_addr, CONTEXT* ctxt);
void getprocaddress_return_processing(int tid, GetProcAddrRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);
void internetopenurl_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPCSTR lpszUrl, ADDRINT return_addr, CONTEXT* ctxt);
void internetopenurl_return_processing(int tid, InternetOpenURLRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);
void loadlibrary_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPCSTR lpLibFileName, ADDRINT return_addr, CONTEXT* ctxt);
void loadlibrary_return_processing(int tid, LoadLibraryRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);
void shellexecute_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPCSTR lpOperation, WINDOWS::LPCSTR lpFile, WINDOWS::LPCSTR lpParameters, WINDOWS::LPCSTR lpDirectory, ADDRINT return_addr, CONTEXT* ctxt);
void shellexecute_return_processing(int tid, ShellExecuteRoutinePtr state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);
void urldownloadtofile_analysis(ADDRINT inst_addr, THREADID tid, WINDOWS::LPUNKNOWN pCaller, WINDOWS::LPCSTR szURL, WINDOWS::LPCSTR szFileName, ADDRINT return_addr, CONTEXT* ctxt);
void urldownloadtofile_return_processing(int tid, URLDownloadToFileRoutinePtr urldownloadtofile_state_info_ptr, string symbol_name, string source_image, CONTEXT* ctxt);

#endif