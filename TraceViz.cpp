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
#include <fstream>
#include <map>
#include <set>
#include <string>

#include <vector>

#include "pin.H"

namespace WINDOWS
{
#include <Windows.h>
}

#ifdef main
#undef main
#endif

#include "InterestingRoutines.h"
#include "TraceViz.h"
#include "DotGenerator2.h"
#include "Utils.h"

using namespace std;

// test options
KNOB<string> KnobLogFile(KNOB_MODE_WRITEONCE, "pintool", "report_filename", "traceviz.log", "Specify tool output file name");
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "trace_dot_filename", "traceviz.gv", "Specify dot output file name");
KNOB<UINT32> KnobInactivityTimeout(KNOB_MODE_WRITEONCE, "pintool", "inactivity_timeout", "0", "Exit if no instructions are executed for inactivity_timeout seconds. The default value of 0 disables the inactivity timeout.");

// data structures to hold info about instructions that have been instrumented
struct instrumented_struct
{
	string dis;
	string source_image;
	int image_id;
};
typedef struct instrumented_struct Instrumented_s;
map<ADDRINT, Instrumented_s> instrumented_instructions;

// data structure to hold information about loaded images
ImageDataMap image_map;

// this data structure holds information gleaned from the entry of "interesting routines"
// the key is the return address, and when we hit it we use the entry information to
// print information about the call.
RoutineStateInfo interesting_routines_state_info;

void inactivity_monitor_thread(void* v);
void create_inactivity_monitor_thread();
bool instrument_interesting_routines(string undecfname, RTN rtn);
void process_interesting_routine_returns(int tid, ADDRINT inst_addr, CONTEXT* ctxt);
void branchorcall_instruction_analysis(ADDRINT inst_ptr, THREADID tid, ADDRINT target_addr, CONTEXT* ctxt);

DotGenerator2 *dotgen;
vector<SectionDataPtr> section_data;
string app_command_line_and_ts;
string dot_output_fname = "";
ADDRINT last_main_instr = 0x0;
int last_section_idx = -1;
UINT64 instruction_counter = 0;
UINT64 image_load_counter = 0;
int inactivity_timeout = 0;

// xdot does not know what to do with lots of clusters (which is how threads are represented). so drop threads > max_threads to keep the output diagram readable
const int max_threads = 10;
set<int> dropped_tids;

// logging file
ofstream logfile;

// FIXME: interesting functions processing needs to handle wide api versions
// fixme interesting return processing does not work with multiple threads


/*********************************************************************************************************************************
 *
 * Instrumentation and analysis is happening at several different intercepts:
 *
 *  - For every image that is loaded (either at startup or dynamically), we log it and add its symbols
 *    to our symbol dictionary, with the symbol's address as the key. If the symbol is "interesting"
 *    there is some special processing associated with that symbol and we set a flag in the dictionary.
 *    The relevant routines are log_image_load() and find_routines().
 *
 *  - We instrument every branch and call instrument coming from the main image. In the analysis
 *    routine, we filter out the not-taken branches and then look up the target addresses in the
 *    symbol dictionary we created at image-load time. If found AND the symbol does *not* belong to
 *    an "interesting" function with custom processing, we report this call to the DotGenerator class,
 *    which manages to creation of the GraphViz output file. This gets instrumented in the
 *    instrument_instructions() routine.
 *
 *  - So what about special processing of "interesting" functions? Those are library calls where we want
 *    to gather extra call-specific information, like specific arguments and return values. We do that
 *    by looking for interesting functions when image symbol tables are parsed; when we find them we
 *    instrument the routine entry. When the routine entry analysis routine is triggered, we capture
 *    call state information and store it in interesting_routines_state_info. The key is the return
 *    address for the routine. See instrument_interesting_routines() and process_interesting_routine_returns().
 *
 *  - We also instrument every instruction coming from the main image. In the analysis routine, we do
 *    a couple of things:
 *    - First we see if we are at one of the saved return addresses set by the "interesting" routines.
 *      If we are we finish processing the routine (e.g., get the return code) and report the library
 *      call data to the DotGenerator class for rendering. This happens in interesting_routines_processing().
 *    - We dump the disassembly to the log.
 *    This happens in instrument_instructions().
 *
 *    There are a few other routines too. In get_main_section_data(), we instrument image loads
 *    to grab section (segment) data from the main executable. This comes in handy for tracking section hops,
 *    which are color coded in the dot output. We trap out of memory exceptions and log it and die if we get
 *    one. And we have a fini routine that finishes writing the dot output file when the application exits,
 *    the inactivity timeout is triggered, or if the user ^Cs out.
 *
 *********************************************************************************************************************************/



/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                        utility functions                                                       *
 *                                                                                                                                *
/*********************************************************************************************************************************/

void
init_traceviz()
{
	if (inactivity_timeout > 0)
	{
		create_inactivity_monitor_thread();
	}

	dotgen = new DotGenerator2(dot_output_fname.c_str(), app_command_line_and_ts.c_str() );
}

static void create_inactivity_monitor_thread() {
	if (PIN_SpawnInternalThread(inactivity_monitor_thread, NULL, 0, NULL) == INVALID_THREADID)
	{
		logfile << "ERROR: cannot spawn inactivity monitor thread. Exiting Process" << endl;
		PIN_ExitProcess(101);
	}
}

void inactivity_monitor_thread(void* v) {
	UINT64 last_instruction_counter = -1;
	UINT64 last_image_load_counter = -1;
	int timeout = inactivity_timeout * 1000;

	while (true)
	{
		PIN_Yield();
		PIN_Sleep(timeout);
		if (last_instruction_counter == instruction_counter && last_image_load_counter == image_load_counter) break;
		last_instruction_counter = instruction_counter;
		last_image_load_counter = image_load_counter;
	}
	logfile << "Inactivity timeout exceeded. Exiting application." << endl;
	PIN_ExitApplication(0);
}

bool should_drop(int tid)
{
	return false;
	if (tid > max_threads)
	{
		if (dropped_tids.find(tid) != dropped_tids.end())
		{
			// already in dropped_tid set
			return true;
		}
		dropped_tids.insert(tid);
		logfile << "dropping instruction flow on thread " << tid << endl;
		return true;
	}
	return false;
}

void out_of_memory_handler(size_t sz, void* arg)
{
	fprintf(stderr, "Failed to allocate dynamic memory: Out of memory!\n");
	exit(3);
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                 image instrumentation functions                                                *
 *                                                                                                                                *
/*********************************************************************************************************************************/

void
find_routines(IMG img, void* v)
{
	if (!IMG_Valid(img))
	{
		logfile << "find_routines: image is not valid" << endl;
		return;
	}

	logfile << "\n[Find_routines Instrumentation]\nLoading " << IMG_Name(img) << ", Image id = " << IMG_Id(img) << endl;
	for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	{
		string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
		logfile << "sym name " << SYM_Name(sym) << " undec: " << undFuncName << endl;
		RTN rtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
		if (RTN_Valid(rtn))
		{
			logfile << "loaded " << undFuncName << " at " << hex << showbase << RTN_Address(rtn) << endl;
			bool isInstrumented = instrument_interesting_routines(undFuncName, rtn);
			SymbolPtr s_ptr = new Symbol();
			s_ptr->undecSymbol = undFuncName;
			s_ptr->source_image = IMG_Name(img);
			s_ptr->source_image_id = IMG_Id(img);
			s_ptr->hasCustomInstrumentFunction = isInstrumented;
			trace_dot_symbols[RTN_Address(rtn)] = s_ptr;
		}
		else
		{
			logfile << "invalid routine ?!? " << undFuncName << endl;
		}
	}
	logfile << "Finished looking for routines in this image" << endl;
}

void get_main_section_data(IMG img, void *v)
{
	// if this is the main image, save section information
	if (IMG_Id(img) == 0x01)
	{
		logfile << "section data for main executable:" << endl;
		for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) )
		{
			string sec_type = sec_type_to_string(SEC_Type(sec));
			SectionDataPtr s = new SectionData();
			s->start_addr = SEC_Address(sec);
			s->end_addr = SEC_Address(sec) + SEC_Size(sec);
			section_data.push_back(s);

			logfile << "Name: " << SEC_Name(sec) << " type: " << sec_type << " start address: " << s->start_addr << " end address: " << s->end_addr << endl;
			// FIXME what about sections that are added post image load?
		}
		logfile << "End of main sections" << endl;
	}
}

void
log_image_loads(IMG img, void* v)
{
	// note: it is possible for image to be split up. in that case highaddress means last byte of the text segment
	logfile << "\n[Log_image_loads]\nLoading " << IMG_Name(img) << endl;
	logfile << "\tImage id = " << IMG_Id(img) << " start address: " << hex << showbase << IMG_LowAddress(img) << " end address: " << IMG_HighAddress(img) << hex << showbase << endl;
	// FIXME capture unloads and reclaim heap when cleaning up this list?

	// store image info in our image_map
	ImageDataPtr image_data_ptr = new ImageData();
	image_data_ptr->name = IMG_Name(img);
	image_data_ptr->id = IMG_Id(img);
	ADDRINT key = IMG_LowAddress(img);
	image_map[key] = image_data_ptr;

	// pass the image load data to the dot generator
	dotgen->addNewImage(0, image_data_ptr->name, 0, key);
	image_load_counter++;
	logfile << "\nend log_image_loads" << endl;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                              instrument routines of interest                                                   *
 *                                                                                                                                *
/*********************************************************************************************************************************/

bool
instrument_interesting_routines(string undecfname, RTN rtn)
{
	// fixme : do i need to match on the dll too?
	logfile << "dot_instrument_routines: fname = " << undecfname << endl;

	if (undecfname == "CreateProcess" || undecfname == "CreateProcessA")
	{
	RTN_Open(rtn);
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)createprocess_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
	RTN_Close(rtn);
	return true;
	}
	else if (undecfname == "GetProcAddress" || undecfname == "GetProcAddressA" )
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)getprocaddress_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
		RTN_Close(rtn);
		return true;
	}
	else if (undecfname == "InternetOpenUrl" || undecfname == "InternetOpenUrlA")
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)internetopenurl_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
		RTN_Close(rtn);
		return true;
	}
	else if (undecfname == "LoadLibrary" || undecfname == "LoadLibraryA" || undecfname == "LoadLibraryExA")
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)loadlibrary_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
		RTN_Close(rtn);
		return true;
	}
	else if (undecfname == "ShellExecute" || undecfname == "ShellExecuteA")
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)shellexecute_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_FUNCARG_ENTRYPOINT_VALUE, 3, IARG_FUNCARG_ENTRYPOINT_VALUE, 4, IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
		RTN_Close(rtn);
		return true;
	}
	else if (undecfname == "URLDownloadToFileA" || undecfname == "URLDownloadToCacheFileA" )
	{
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)urldownloadtofile_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_FUNCARG_ENTRYPOINT_VALUE, 2,  IARG_RETURN_IP, IARG_CONTEXT, IARG_END);
		RTN_Close(rtn);
		return true;
	}
	return false;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                        process the interesting routines when they return                                       *
 *                                                                                                                                *
/*********************************************************************************************************************************/

static void
process_interesting_routine_returns(int tid, ADDRINT inst_addr, CONTEXT* ctxt)
{

	if (should_drop(tid) )
	{
		return;
	}

	// is this a return from an interesting routine?
	InterestingRoutinePtr state_info_ptr = get_interesting_routine_state_info(inst_addr, tid);
	if (state_info_ptr == NULL)
	{
		// nope
		return;
	}

	// yes, we are at the return address of a "interesting routines" call
	logfile << "process_interesting_routine_returns " << hex << showbase << inst_addr << " tid: " << tid << " stored tid: " << state_info_ptr->tid << endl;
	SymbolPtr s_ptr = NULL;
	if (trace_dot_symbols.count(state_info_ptr->funcaddr) > 0)
	{
		s_ptr = trace_dot_symbols[state_info_ptr->funcaddr];
		//logfile << "got symbol info" << endl;
	}
	else
	{
		logfile << "could not find symbol?" << endl;
		return;
	}

	state_info_ptr->section_idx = addr_to_section_idx(inst_addr);

	if (state_info_ptr->type == InterestingRoutineType::CREATEPROCESS)
	{
		CreateProcessRoutinePtr createprocess_state_info_ptr = (CreateProcessRoutinePtr)state_info_ptr;
		createprocess_return_processing(tid,createprocess_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}
	else if (state_info_ptr->type == InterestingRoutineType::GETPROCADDR)
	{
		GetProcAddrRoutinePtr getprocaddr_state_info_ptr = (GetProcAddrRoutinePtr)state_info_ptr;
		getprocaddress_return_processing(tid,getprocaddr_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}
	else if (state_info_ptr->type == InterestingRoutineType::INTERNETOPENURL)
	{
		InternetOpenURLRoutinePtr internetopenurl_state_info_ptr = (InternetOpenURLRoutinePtr)state_info_ptr;
		internetopenurl_return_processing(tid, internetopenurl_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}
	else if (state_info_ptr->type == InterestingRoutineType::LOADLIBRARY)
	{
		LoadLibraryRoutinePtr loadlibrary_state_info_ptr = (LoadLibraryRoutinePtr)state_info_ptr;
		loadlibrary_return_processing(tid, loadlibrary_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}
	else if (state_info_ptr->type == InterestingRoutineType::SHELLEXECUTE)
	{
		ShellExecuteRoutinePtr shellexecute_state_info_ptr = (ShellExecuteRoutinePtr)state_info_ptr;
		shellexecute_return_processing(tid,shellexecute_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}
	else if (state_info_ptr->type == InterestingRoutineType::URLDOWNLOADTOFILE)
	{
		URLDownloadToFileRoutinePtr urldownloadtofile_state_info_ptr = (URLDownloadToFileRoutinePtr)state_info_ptr;
		urldownloadtofile_return_processing(tid, urldownloadtofile_state_info_ptr, s_ptr->undecSymbol, s_ptr->source_image, ctxt);
	}

	// we don't need to process instructions at this address any more, so remove from interesting_routines_state_info
	delete_interesting_routine_state_info(inst_addr, tid);
	delete state_info_ptr;
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                            instruction-level analysis functions                                                *
 *                                                                                                                                *
/*********************************************************************************************************************************/

static void
instruction_analysis(ADDRINT inst_addr, THREADID tid, CONTEXT* ctxt)
{

	if (should_drop(tid))
	{
		return;
	}

	//logfile << "setting last main instr to " << hex << showbase << inst_ptr << endl;
	last_main_instr = inst_addr;
	instruction_counter++;

	// if this is the return address of an routine neededing special processing, handle it
	process_interesting_routine_returns(tid, inst_addr, ctxt);

	// retrieve stored information about this instruction address
	string dis = instrumented_instructions[inst_addr].dis;
	string imgname = instrumented_instructions[inst_addr].source_image;
	int imgid = instrumented_instructions[inst_addr].image_id;

	// find out which section in the main image holds the address
	int new_last_section_idx = addr_to_section_idx(last_main_instr);
	if (new_last_section_idx != last_section_idx)
	{
		logfile << "; moved to new section" << endl;
		// we have moved into a new section of the main image
		last_section_idx = new_last_section_idx;
	}

	// is this a defined symbol? if so use that symbol in the log output
	string s = "";
	if (trace_dot_symbols.find(inst_addr) != trace_dot_symbols.end())
	{
		SymbolPtr symbolinfo = trace_dot_symbols[inst_addr];
		s = symbolinfo->undecSymbol;
	}

	// log the instruction address, thread id, section index, symbol (if found), and disassembly
	logfile << "[Analysis] " << hex << showbase << inst_addr << "\t" << tid << "\t" << last_section_idx << "\t" << s << "\t" << dis << endl;
}

static void
cond_instruction_analysis(ADDRINT inst_addr, THREADID tid, bool branch_taken, ADDRINT target_addr, CONTEXT* ctxt)
{
	if (branch_taken)
	{
		branchorcall_instruction_analysis(inst_addr, tid, target_addr, ctxt);
	}
}

static void
uncond_instruction_analysis(ADDRINT inst_addr, THREADID tid, ADDRINT target_addr, CONTEXT* ctxt)
{
	branchorcall_instruction_analysis(inst_addr, tid, target_addr, ctxt);
}

static void
branchorcall_instruction_analysis(ADDRINT inst_addr, THREADID tid, ADDRINT target_addr, CONTEXT* ctxt)
{
	if (should_drop(tid))
	{
		return;
	}

	//logfile << "branchorcall_instruction_analysis inst addr: " << inst_addr << " target addr: " << target_addr << " tid:" << tid << endl;

	// is inst_addr in main and the target_addr in an external lib?
	if (is_addr_in_main(inst_addr) && !is_addr_in_main(target_addr))
	{
		// yes! see if we can find a symbol
		string sym = "UnknownSymbol";
		string src_img = "UnknownImage";
		if (trace_dot_symbols.count(target_addr) > 0)
		{
			// yes! this has a symbol!
			SymbolPtr s = trace_dot_symbols[target_addr];
			// if this has custom processing, just drop it. it will get
			// triggered when we do the plain vanilla instruction analysis.
			if (s->hasCustomInstrumentFunction)
			{
				return;
			}
			sym = s->undecSymbol;
			src_img = s->source_image;
		}
		else
		{
			// no symbol? at least find the image
			PIN_LockClient();
			IMG img = IMG_FindByAddress(target_addr);
			PIN_UnlockClient();
			if (IMG_Valid(img))
			{
				src_img = IMG_Name(img);
			}
		}

		// log the branch/call symbol, target image, target address and thread id
		int section_idx = addr_to_section_idx(inst_addr);
		logfile << "; branch/call to library routine " << sym << " in " << src_img << " address: " << target_addr << " tid: " << tid << endl;


		// add a new node to the dot output
		dotgen->addNewLibCall(tid, sym, src_img, section_idx, target_addr, StringFromAddrint(last_main_instr),"");
	}
}


/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                        instruction-level instrumentation functions                                             *
 *                                                                                                                                *
/*********************************************************************************************************************************/

void
instrument_instructions(INS ins, void*)
{
	// get the image name and id in which this instruction resides
	int imgid = 0;
	string imgname = addr_to_image(INS_Address(ins), &imgid);
	//DEBUGHIGH logfile << "trace: " << hex << showbase << INS_Address(ins) << " " << imgname << " " << INS_Disassemble(ins) << endl;

	// if we have not seen this instruction before ...
	if (instrumented_instructions.find(INS_Address(ins)) == instrumented_instructions.end())
	{
		// ... and it is in the main executable ...
		if (imgid == 0x1)
		{
			// instrument it, and cache information about the instruction for later lookup in the analysis routines
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)instruction_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_CONTEXT, IARG_CALL_ORDER, CALL_ORDER_FIRST, IARG_END);
			Instrumented_s is = { INS_Disassemble(ins), imgname, imgid };
			instrumented_instructions[INS_Address(ins)] = is;
		}

		// ... or if it is a conditional control flow instruction ...
		if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
		{
			// ... instrument it
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)cond_instruction_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR, IARG_CONTEXT, IARG_END);
		}
		// ... or if it is an unconditional control flow instruction ...
		else if (INS_IsControlFlow(ins))
		{
			// ... instrument it
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)uncond_instruction_analysis, IARG_INST_PTR, IARG_THREAD_ID, IARG_BRANCH_TARGET_ADDR, IARG_CONTEXT, IARG_END);
		}
	}
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                          fini function                                                         *
 *                                                                                                                                *
/*********************************************************************************************************************************/
void fini(int, void*)
{
	logfile << "Exiting application" << endl;
	dotgen->closeOutputFile();
}

/**********************************************************************************************************************************
 *                                                                                                                                *
 *                                                             main                                                               *
 *                                                                                                                                *
/*********************************************************************************************************************************/

static void
usage()
{
	fprintf(stderr, "%s\n", KNOB_BASE::StringKnobSummary().c_str());
}

int
main(int argc, char* argv[])
{
	PIN_InitSymbols();

	if (PIN_Init(argc, argv) != 0)
	{
		fprintf(stderr, "PIN_Init failed\n");
		usage();
		return 1;
	}

	// get the inactivity timeout value
	inactivity_timeout = KnobInactivityTimeout.Value();

	// get the name of the dot output file
	dot_output_fname = KnobOutputFile.Value();

	// open the log file
	logfile.open(KnobLogFile.Value().c_str(), ios::out | ios::trunc);
	if (!logfile.is_open())
	{
		fprintf(stderr, "could not open %s\n", KnobLogFile.Value().c_str());
		return -1;
	}

	fprintf(stderr, "log file at: %s\n", KnobLogFile.Value().c_str());

	// log basic test info
	stringstream comment;
	UINT64 now;
	get_time(&now);
	comment << get_command_line() << " timestamp: " << now;
	app_command_line_and_ts = comment.str();
	logfile << app_command_line_and_ts;
	logfile << "dot output file at: " << dot_output_fname << endl;
	
	// instrument all the things
	init_traceviz();
	PIN_AddOutOfMemoryFunction(out_of_memory_handler, NULL);
	IMG_AddInstrumentFunction(log_image_loads, 0);
	IMG_AddInstrumentFunction(find_routines, 0);
	IMG_AddInstrumentFunction(get_main_section_data, 0);
	INS_AddInstrumentFunction(instrument_instructions, NULL);
	PIN_AddFiniFunction(fini, 0);

	// run the application
	PIN_StartProgram();

	return 1;
}

