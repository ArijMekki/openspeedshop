////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file
 *
 * Definition of the Process class.
 *
 */

#include "Assert.hxx"
#include "Blob.hxx"
#include "Guard.hxx"
#include "MainLoop.hxx"
#include "Process.hxx"
#include "ProcessTable.hxx"
#include "SmartPtr.hxx"
#include "Time.hxx"

#include <dpcl.h>
#include <ltdl.h>
#include <sstream>
#include <stdexcept>
#include <vector>



/** TEMPORARY HACK. */
static void stdoutCB(GCBSysType sys, GCBTagType, GCBObjType, GCBMsgType msg)
{
    char* ptr = (char*)msg;
    for(int i = 0; i < sys.msg_size; ++i, ++ptr)
	fputc(*ptr, stdout);
}



/** TEMPORARY HACK. */
static void stderrCB(GCBSysType sys, GCBTagType, GCBObjType, GCBMsgType msg)
{
    char* ptr = (char*)msg;
    for(int i = 0; i < sys.msg_size; ++i, ++ptr)
	fputc(*ptr, stdout);
}



//
// To avoid requiring fully qualified names to be used everywhere, something
// like "using namespace OpenSpeedShop::Framework;" is usually placed near the
// top of our source files. Here, however, that particular formulation doesn't
// work. DPCL defines a class Process in the global namespace and our typical
// using directive creates a conflict. The solution is to place the definitions
// directly inside the namespace. Then we are able to refer to "Process" and
// get "OpenSpeedShop::Framework::Process" unless we explicitly ask for the
// DPCL version via "::Process".
//
namespace OpenSpeedShop { namespace Framework {



namespace {



    /**
     * Process termination callback.
     *
     * Callback function called by the DPCL main loop when a process terminates.
     * Marks the process as terminated.
     */
    void processTerminationCB(GCBSysType, GCBTagType,
			      GCBObjType, GCBMsgType msg)
    {
	std::string name = std::string(reinterpret_cast<char*>(msg));
	SmartPtr<Process> process;

	// Critical section touching the process table
	{
	    Guard guard_process_table(ProcessTable::TheTable);
	    
	    // Get the process (if any) by its unique name
	    process = ProcessTable::TheTable.getProcessByName(name);
	}
	
	// Go no further if the process no longer exists
	if(process.isNull())
	    return;
	
	// Mark the process as terminated
	process->terminateNow();
    }
    
    
    
    /**
     * State changed callback.
     *
     * Callback function called by the DPCL main loop when acknowledgement of a
     * resume, suspend, or destroy request is received. Completes the state
     * change for the process.
     */
    void stateChangedCB(GCBSysType, GCBTagType tag, GCBObjType, GCBMsgType msg)
    {	
	std::string name = std::string(reinterpret_cast<char*>(tag));
	AisStatus* retval = reinterpret_cast<AisStatus*>(msg);
	SmartPtr<Process> process;
	
	// Check assertions
	Assert(retval != NULL);
	Assert(retval->status() == ASC_success);

	// Critical section touching the process table
	{
	    Guard guard_process_table(ProcessTable::TheTable);
	    
	    // Get the process (if any) by its unique name
	    process = ProcessTable::TheTable.getProcessByName(name);
	}
	
	// Go no further if the process no longer exists
	if(process.isNull())
	    return;
	
	// Complete the state change for the process
	process->completeStateChange();
    }



}



/**
 * Form a process' unique name.
 *
 * Returns the unique name for a process. The name is defined as being the
 * string "[host]:[pid]", where [host] is the name of the host on which the
 * process resides, and [pid] is the identifier of the process on that host.
 * This unique name is used for identifying processes within messages sent
 * between hosts.
 *
 * @param host    Name of the host on which the process resides.
 * @param pid     Process identifier for the process.
 * @return        Unique name of this process.
 */
std::string Process::formUniqueName(const std::string& host, const pid_t& pid)
{
    std::stringstream name;
    name << host << ":" << pid;
    return name.str();
}

    

/**
 * Constructor from process creation.
 *
 * Creates a new process to execute the specified command. The command is
 * created with the same initial environment (standard file descriptors,
 * environment variables, etc.) as when the tool was started. The process
 * is created in a suspended state.
 *
 * @note    An exception of type std::runtime_error is thrown if the thread 
 *          cannot be created for any reason (host doesn't exist, specified
 *          command cannot be executed, etc.)
 *
 * @todo    Should probably try to find a full pathname for argv[0].
 *
 * @todo    The stdout and stderr callbacks don't appear to be working.
 *
 * @param host       Name of host on which to execute the command.
 * @param command    Command to be executed.
 */
Process::Process(const std::string& host, const std::string& command) :
    dm_process(NULL),
    dm_host(host),
    dm_pid(0),
    dm_current_state(Thread::Suspended),
    dm_is_state_changing(false),
    dm_future_state(Thread::Suspended),
    dm_library_name_to_entry()
{
    // Critical section touching the process table
    {
	Guard guard_process_table(ProcessTable::TheTable);

	// Start the DPCL main loop if this is the first process
	if(ProcessTable::TheTable.isEmpty()) {
	    MainLoop::start();
	    
	    // Setup the process termination handler
	    GCBFuncType old_cb_fp;
	    GCBTagType old_cb_tag;	
	    AisStatus retval = 
		Ais_override_default_callback(AIS_PROC_TERMINATE_MSG,
					      processTerminationCB, NULL,
					      &old_cb_fp, &old_cb_tag);
	    Assert(retval.status() == ASC_success);	
	}
    }
    
    // Allocate a new DPCL process handle
    dm_process = new ::Process();
    Assert(dm_process != NULL);
    
    // Extract individual arguments from the command
    std::vector<std::string> args;    
    for(std::string::size_type
	    i = 0, next = 0; i != std::string::npos; i = next) {
	
	// Find the next space character in the command
	next = command.find(' ', i);
	
	// Extract this argument
	args.push_back(
	    command.substr(i, (next == std::string::npos) ? next : next - i)
	    );
	
	// Find the next non-space character in the command
	next = command.find_first_not_of(' ', next);	
	
    }
    
    // Translate the arguments into an argv-style argument list
    const char** argv = new const char*[args.size() + 1];
    for(std::vector<std::string>::size_type i = 0; i < args.size(); ++i)
	argv[i] = args[i].c_str();
    argv[args.size()] = NULL;
    
    // Declare access to the external environment variables
    extern char** environ;
    
    // Ask DPCL to create and attach a process for executing the command
    MainLoop::suspend();    
    AisStatus retval = dm_process->bcreate(dm_host.c_str(),
					   argv[0], argv, ::environ,
					   stdoutCB, NULL, stderrCB, NULL);
    if(retval.status() == ASC_failure) {
	MainLoop::resume();
	throw std::runtime_error("Cannot create process to execute the "
				 "command \"" + command + "\".");
    }
    Assert(retval.status() == ASC_success);
    retval = dm_process->battach();
    Assert(retval.status() == ASC_success);
    MainLoop::resume();
    
    // Destroy argv-style argument list
    delete [] argv;

    // Ask DPCL for the process identifier
    dm_pid = static_cast<pid_t>(dm_process->get_pid());
}
    
    
    
/**
 * Constructor from process attachment.
 *
 * Attaches to an existing process. The process is assumed to be in the running
 * state.
 *
 * @note    An exception of type std::runtime_error is thrown if the thread
 *          cannot be attached for any reason (host or process identifier
 *          doesn't exist, etc.) 
 * 
 * @todo    Explore using asynchronous connect/attach.
 *
 * @param host    Name of the host on which the process resides.
 * @param pid     Process identifier for the process.
 */
Process::Process(const std::string& host, const pid_t& pid) :
    dm_process(),
    dm_host(host),
    dm_pid(pid),
    dm_current_state(Thread::Running),
    dm_is_state_changing(false),
    dm_future_state(Thread::Running),
    dm_library_name_to_entry()
{
    // Critical section touching the process table
    {
	Guard guard_process_table(ProcessTable::TheTable);

	// Start the DPCL main loop if this is the first process
	if(ProcessTable::TheTable.isEmpty()) {
	    MainLoop::start();
	    
	    // Setup the process termination handler
	    GCBFuncType old_cb_fp;
	    GCBTagType old_cb_tag;	
	    AisStatus retval = 
		Ais_override_default_callback(AIS_PROC_TERMINATE_MSG,
					      processTerminationCB, NULL,
					      &old_cb_fp, &old_cb_tag);
	    Assert(retval.status() == ASC_success);	
	}
    }

    // Allocate a new DPCL process handle
    dm_process = new ::Process(dm_host.c_str(), dm_pid);
    Assert(dm_process != NULL);
    
    // Ask DPCL to connect and attach to the process
    MainLoop::suspend();    
    AisStatus retval = dm_process->bconnect();
    if(retval.status() == ASC_failure) {
	MainLoop::resume();
	std::stringstream pid_name;
	pid_name << dm_pid;
	throw std::runtime_error("Cannot connect to PID " + pid_name.str() + 
				 " on host \"" + host + "\".");
    }
    Assert(retval.status() == ASC_success);
    retval = dm_process->battach();
    Assert(retval.status() == ASC_success);
    MainLoop::resume();	
}
    
    
    
/**
 * Destructor.
 *
 * Start the process running if it is currently suspended, disconnect from the
 * process, and destroy our DPCL process handle.
 */
Process::~Process()
{
    // Handle Running --> Disconnected
    if(dm_current_state == Thread::Running) {
	
	// Ask DPCL to disconnect from the process
	MainLoop::suspend();    		
	AisStatus retval = dm_process->disconnect(NULL, NULL);	
	Assert(retval.status() == ASC_success);	
	MainLoop::resume();
	
    }
    
    // Handle Suspended --> Running
    if(dm_current_state == Thread::Suspended) {
	
	// Ask DPCL to resume the process
	MainLoop::suspend();
	AisStatus retval = dm_process->resume(NULL, NULL);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();
	
	// Ask DPCL to disconnect from the process
	MainLoop::suspend();    		
	retval = dm_process->disconnect(NULL, NULL);	
	Assert(retval.status() == ASC_success);	
	MainLoop::resume();
	
    }
    
    // Destroy the actual DPCL process handle
    delete dm_process;
    
    // Critical section touching the process table
    {
	Guard guard_process_table(ProcessTable::TheTable);
	
	// Stop the DPCL main loop if this was the last process
	if(ProcessTable::TheTable.isEmpty())
	    MainLoop::stop();
    }
}

    
    
/**
 * Get our host name.
 *
 * Return the name of the host on which this process is located.
 *
 * @return    Name of host on which this process is located.
 */
std::string Process::getHost() const
{
    // Return the host name to the caller
    return dm_host;
}
    
    
    
/**
 * Get our process identifier.
 *
 * Returns the identifier of this process.
 *
 * @return    Identifier of this process.
 */
pid_t Process::getProcessId() const
{
    // Return the process identifier to the caller
    return dm_pid;
}
    
    
    
/**
 * Get our state.
 *
 * Returns the caller the current state of this process. Since this state
 * changes asynchronously and must be updated across a network, there is a lag
 * between when the actual process' state changes and when that is reflected
 * here.
 *
 * @return    Current state of this process.
 */
Thread::State Process::getState() const
{
    Guard guard_myself(this);

    // Return our current state to the caller
    return dm_current_state;
}
    
    
    
/**
 * Change our state.
 *
 * Changes the current state of this process to the passed value. Used to, for
 * example, suspend a process that was previously running. This function does
 * not wait until the process has actually completed the state change, and
 * calling getState() immediately following changeState() will not reflect the
 * new state until the change has actually completed.
 *
 * @note    Only one in-progress state change is allowed per process at any
 *          given time. For example, if you request that a process be suspended,
 *          you cannot request that it be terminated before the suspension is
 *          completed. An exception of type std::logic_error is thrown when
 *          multiple in-progress changes are requested.
 *
 * @note    Some transitions are disallowed because they do not make sense or
 *          cannot be implemented. For example, a terminated process cannot be
 *          set to a running process. An exception of type std::logic_error is
 *          thrown when such an invalid transition is requested.
 *
 * @param state    Change to this state.
 */
void Process::changeState(const Thread::State& state)
{
    Guard guard_myself(this);

    // Finished if already in the requested state
    if(dm_current_state == state)
	return;
    
    // Finished if already changing to the requested state
    if(dm_is_state_changing && (dm_future_state == state))
	return;
    
    // Disallow multiple, different, in-progress state changes
    if(dm_is_state_changing)
	throw std::logic_error(
	    "Cannot change a process' state when a different state change "
	    "is in-progress but hasn't completed.");
    
    // Disallow Terminated --> [ Running | Suspended ]
    if(dm_current_state == Thread::Terminated)
	throw std::logic_error(
	    "Cannot " + 
	    std::string(((state == Thread::Suspended) ? "suspend" : "run")) + 
	    " a terminated process.");
    
    // Cast the process' name to the proper type for use below
    std::string name = formUniqueName(dm_host, dm_pid);
    void* name_ptr =
	const_cast<void*>(reinterpret_cast<const void*>(name.c_str()));
    
    // Handle Suspended --> Running
    if((dm_current_state == Thread::Suspended) && (state == Thread::Running)) {

	// Ask DPCL to resume the process
	MainLoop::suspend();
	AisStatus retval = dm_process->resume(stateChangedCB, name_ptr);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();
	
	// Indicate this thread is changing to the running state
	dm_is_state_changing = true;
	dm_future_state = Thread::Running;
	
    }
    
    // Handle Running --> Suspended
    if((dm_current_state == Thread::Running) && (state == Thread::Suspended)) {

	// Ask DPCL to suspend the process
	MainLoop::suspend();
	AisStatus retval = dm_process->suspend(stateChangedCB, name_ptr);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();
	
	// Indicate this thread is changing to the suspended state
	dm_is_state_changing = true;
	dm_future_state = Thread::Suspended;	
	
    }
    
    // Handle [Suspended | Running ] --> Terminated
    if(state == Thread::Terminated) {
	
	// Ask DPCL to destroy the process
	MainLoop::suspend();
	AisStatus retval = dm_process->destroy(stateChangedCB, name_ptr);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();
	
	// Indicate this thread is changing to the terminated state
	dm_is_state_changing = true;
	dm_future_state = Thread::Terminated;
	
    }
}
    
    
    
/**
 * Complete a state change.
 *
 * Called by stateChangedCB() when the process completes its in-progress
 * state change. Simply sets the current state to the future state and marks
 * the process as no longer undergoing a state change.
 */
void Process::completeStateChange()
{
    Guard guard_myself(this);

    // Check assertions
    Assert(dm_is_state_changing);
    
    // Set the current state equal to the future state
    dm_current_state = dm_future_state;
    dm_is_state_changing = false;
}
    
    
    
/**
 * Terminate now.
 *
 * Called by processTerminationCB() when the process terminates. Simply sets the
 * current state and future state to terminated.
 */
void Process::terminateNow()
{
    Guard guard_myself(this);

    // Set the current and future state to terminated
    dm_current_state = Thread::Terminated;
    dm_future_state = Thread::Terminated;
}
    
    
    
/**
 * Update address space.
 *
 * Called when a thread within this process is first created or attached, and
 * any time a shared library is loaded or unloaded.  Requests the module list
 * from DPCL and uses that list to update the database containing the passed
 * thread with the current in-memory address space of that thread. Symbol
 * information is created (or existing information reused) for each linked
 * object that is located.
 *
 * @todo    Since multiple calls to processSymbols() can take a good amount
 *          of time, parallelizing those calls represents a good opportunity
 *          to improve performance.
 *
 * @todo    Currently the criteria for reusing symbol information from an
 *          existing linked object is if the path names of the linked objects
 *          are identical. This is inadequate since two hosts might have linked
 *          objects with the same name but different contents. For example, two
 *          different versions of "/lib/ld-linux.so.2". In order to address this
 *          shortcoming, a checksum (or similar) should also be compared.
 *
 * @param thread    Thread within this process to be updated.
 * @param when      Time at which update occured.
 */
void Process::updateAddressSpace(const Thread& thread, const Time& when)
{
    Guard guard_myself(this);
    
    // Define a type pairing a linked object name with its source object list
    typedef std::pair<Path, std::vector<SourceObj> > table_entry_t;    
    
    // Define a type mapping address ranges to this pairing
    typedef std::map<AddressRange, table_entry_t> table_t;
    
    // Table mapping address ranges to linked object names and source objects
    table_t objects;
    
    // Iterate over each module associated with this process
    SourceObj program = dm_process->get_program_object();
    for(int m = 0; m < program.child_count(); ++m) {
	SourceObj module = program.child(m);
	
	// Obtain the name of the module
	std::string module_name = "";
	if(module.module_name_length() > 0) {
	    char buffer[module.module_name_length() + 1];
	    module.module_name(buffer, module.module_name_length() + 1);
	    module_name = buffer;
	}
	
	// Obtain the start and end addresses of the module
	AddressRange range(static_cast<Address>(module.address_start()),
			   static_cast<Address>(module.address_end()));
	
	// Look for an entry in the table for this address range
	table_t::iterator i = objects.find(range);
	
	// Create an entry if one wasn't found
	if(i == objects.end())
	    i = objects.insert(
		std::make_pair(range,
			       std::make_pair(Path(module_name),
					      std::vector<SourceObj>()))
		).first;
	
   	//
	// Otherwise if an entry was found, assume that we are dealing with the
	// executable, which can contain multiple DPCL modules. Use the name of
	// the executable as the linked object name if this is the second module
	// to be added to the entry.
	//
	else if(i->second.second.size() == 1) {
	    
	    // Obtain the name of the program
	    std::string program_name = "";
	    if(program.program_name_length() > 0) {
		char buffer[program.program_name_length() + 1];
		program.program_name(buffer, program.program_name_length() + 1);
		program_name = buffer;
	    }
	    
	    // Use the program name as this entry's linked object name
	    i->second.first = Path(program_name);
	    
	}
	
	// Add the source object to this entry
	i->second.second.push_back(module);
	
    }

    // Begin a transaction on this thread's database
    SmartPtr<Database> database = ThreadSpy(thread).getDatabase();
    BEGIN_TRANSACTION(database);
    
    //
    // Use Time::TheBeginning() as the "time_begin" value of the to-be-created
    // address space entries if there are currently no address space entries
    // for this thread. Otherwise set "time_begin" to be "when".
    //
    Time time_begin = Time::TheBeginning();
    database->prepareStatement(
	"SELECT COUNT(*) FROM AddressSpaces WHERE thread = ?;"
	);
    database->bindArgument(1, ThreadSpy(thread).getEntry());
    while(database->executeStatement())
	if(database->getResultAsInteger(1) > 0)
	    time_begin = when;
    
    // Update time interval of active address space entries for this thread
    database->prepareStatement(
	"UPDATE AddressSpaces SET time_end = ?"
	" WHERE thread = ? AND time_end = ?;"
	);
    database->bindArgument(1, when);
    database->bindArgument(2, ThreadSpy(thread).getEntry());
    database->bindArgument(3, Time::TheEnd());
    while(database->executeStatement());
    
    // Iterate over each linked object entry in the table
    for(table_t::iterator i = objects.begin(); i != objects.end(); ++i) {

	// Is there an existing linked object in the database?
	Optional<int> linked_object;
	database->prepareStatement(
	    "SELECT LinkedObjects.id FROM LinkedObjects JOIN Files"
	    " ON LinkedObjects.file = Files.id WHERE Files.path = ?;"
	    );
	database->bindArgument(1, i->second.first);
	while(database->executeStatement())
	    linked_object = database->getResultAsInteger(1);
	
	// Parse symbol information if it isn't present in the database
	if(!linked_object.hasValue()) {
	    
	    // Create the file entry
	    database->prepareStatement("INSERT INTO Files (path) VALUES (?);");
	    database->bindArgument(1, i->second.first);
	    while(database->executeStatement());
	    int file = database->getLastInsertedUID();
	    
	    // Create the linked object entry
	    database->prepareStatement(
		"INSERT INTO LinkedObjects (addr_begin, addr_end, file)"
		" VALUES (0, ?, ?);"
		);
	    database->bindArgument(1, Address(i->first.getEnd() - 
					      i->first.getBegin()));
	    database->bindArgument(2, file);	    
	    while(database->executeStatement());
	    linked_object = database->getLastInsertedUID();
	    
	    // Process the symbol information
	    processSymbols(thread, linked_object, i->first, i->second.second);
	    
	}
	
	// Create an address space entry for this linked object
	database->prepareStatement(
	    "INSERT INTO AddressSpaces (thread, time_begin, time_end,"
	    " addr_begin, addr_end, linked_object)"
	    "VALUES (?, ?, ?, ?, ?, ?);"
	    );
	database->bindArgument(1, ThreadSpy(thread).getEntry());
	database->bindArgument(2, time_begin);
	database->bindArgument(3, Time::TheEnd());
	database->bindArgument(4, i->first.getBegin());
	database->bindArgument(5, i->first.getEnd());
	database->bindArgument(6, linked_object);
	while(database->executeStatement());	
	
    }
    
    // End the transaction on this thread's database
    END_TRANSACTION(database);    
}



/**
 * Process symbol information.
 *
 * Processes symbol information for the specified executable or shared library.
 * DPCL is queried for the functions, source statements, and call sites that
 * were found within the linked object. This information is filled into the
 * database for the specified thread.
 *
 * @todo    DPCL does not currently provide complete source statement or call
 *          site information. For now we process only the function information.
 *          Source statement and call site processing needs to be added.
 *
 * @param thread           Thread that contains the linked object.
 * @param linked_object    Entry (id) for the linked object.
 * @param range            Address range occupied.
 * @param objects          Source object list.
 */
void Process::processSymbols(const Thread& thread,
			     const int& linked_object,
			     const AddressRange& range,
			     std::vector<SourceObj>& objects)
{
    // Begin a transaction on this thread's database
    SmartPtr<Database> database = ThreadSpy(thread).getDatabase();
    BEGIN_TRANSACTION(database);
    
    // Iterate over each (module) source object
    for(std::vector<SourceObj>::iterator
	    i = objects.begin(); i != objects.end(); ++i) {
	
	// Ask DPCL to expand this (module) source object
	MainLoop::suspend();
	AisStatus retval = i->bexpand(*dm_process);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();
	
	// Iterate over each child of this module
	for(unsigned j = 0; j < i->child_count(); ++j) {
	    SourceObj child = i->child(j);
	    
	    // Is this child a function object?
	    if(child.src_type() == SOT_function) {
		
		// Get the start/end address of the function
		Address start = 
		    Address(child.address_start()) - range.getBegin();
		Address end = Address(child.address_end()) - range.getBegin();
		
		// Get the demangled name of the function
		char name[child.get_demangled_name_length() + 1];
		child.get_demangled_name(name, sizeof(name));
		
		// Create a function entry for this function
		database->prepareStatement(
		    "INSERT INTO Functions"
		    " (linked_object, addr_begin, addr_end, name)"
		    " VALUES (?, ?, ?, ?);"
		    );
		database->bindArgument(1, linked_object);
		database->bindArgument(2, start);
		database->bindArgument(3, end);
		database->bindArgument(4, name);
		while(database->executeStatement());	
		
	    }
	    
	}
	
    }
    
    // End the transaction on the specified database
    END_TRANSACTION(database);
}



/**
 * Load a library.
 *
 * Loads the specified library into this process. Each process maintains a list
 * of loaded libraries and their reference counts. If the specified library is
 * already loaded, its reference count is simply incremented. Otherwise the
 * library is located and loaded into the process.
 *
 * @note    Libraries are located using libltdl and the standard search path
 *          established by the CollectorPluginTable class. An exception of type
 *          std::invalid_arguemnt is thrown if the library can't be located.
 *
 * @param library    Name of library to be loaded.
 */
void Process::loadLibrary(const std::string& library)
{
    Guard guard_myself(this);
    
    // Has this library been previously loaded?
    std::map<std::string, LibraryEntry>::iterator
	i = dm_library_name_to_entry.find(library);
    if(i != dm_library_name_to_entry.end()) {
	
	// Increment the library's reference count
	i->second.references++;
	
    }
    else {
	
	// Can we open this library as a libltdl module?
	lt_dlhandle handle = lt_dlopenext(library.c_str());
	if(handle == NULL)
	    throw std::invalid_argument(
		"Cannot locate the library \"" + library + 
		"\" that was to be loaded."
		);
	
	// Get the full path of the library
	const lt_dlinfo* info = lt_dlgetinfo(handle);
	Assert(info != NULL);
	Path full_path = info->filename;
	
	// Close the module handle
	Assert(lt_dlclose(handle) == 0);

	// Ask DPCL to load the library
	ProbeModule* module = new ProbeModule(full_path.c_str());
	MainLoop::suspend();
	AisStatus retval = dm_process->bload_module(module);
	Assert(retval.status() == ASC_success);
	MainLoop::resume();	
	
	// Create an entry for this library
	LibraryEntry entry;
	entry.name = library;
	entry.path = full_path;
	entry.module = module;
	entry.references = 1;
	entry.functions = std::map<std::string, int>();

	// Iterate over each function in this library
	for(int i = 0; i < module->get_count(); ++i) {
	    
	    // Obtain the function's name
	    unsigned length = module->get_name_length(i);
	    char* buffer = new char[length];
	    module->get_name(i, buffer, length);
	    
	    // Add this function to the list of functions for this library
	    entry.functions.insert(std::make_pair(buffer, i));
	    
	}
		
	// Add this entry to the list of libraries
	dm_library_name_to_entry.insert(std::make_pair(library, entry));
	
    }
}



/**
 * Unload a library.
 *
 * Unloads the specified library from this process. Each process maintains a
 * list of loaded libraries and their reference counts. If the specified library
 * has more than one reference, its reference count is simply decremeneted.
 * Otherwise the library is actually unloaded from the process.
 *
 * @pre    A library must be loaded before it can be unloaded. An exception of
 *         type std::invalid_argument is thrown if the library is not already
 *         loaded into the process.
 *
 * @param library    Name of library to be unloaded.
 */
void Process::unloadLibrary(const std::string& library)
{
    Guard guard_myself(this);

    // Find the entry for this library
    std::map<std::string, LibraryEntry>::iterator
	i = dm_library_name_to_entry.find(library);

    // Check preconditions
    if(i == dm_library_name_to_entry.end())
	throw std::invalid_argument(
	    "Cannot unload the library \"" + library + 
	    "\" that was not previously loaded."
	    );
    
    // Decrement the library's reference count
    i->second.references--;
    
    // Was this the last reference to this library?
    if(i->second.references == 0) {

	// Note: An error of ASC_terminated_pid is silently ignored here because
	//       closing an experiment frequently results in colllectors trying
	//       to unload their runtime libraries after the processes being
	//       monitored have already terminated.
	
	// Ask DPCL to unload the library
	MainLoop::suspend();
	AisStatus retval = dm_process->bunload_module(i->second.module);
	Assert((retval.status() == ASC_success) ||
	       (retval.status() == ASC_terminated_pid));
	MainLoop::resume();
	delete i->second.module;
	
	// Remove this entry from the list of libraries
	dm_library_name_to_entry.erase(i);
	
    }
}



/**
 * Execute a library function.
 *
 * Immediately executes the specified function in this process.
 *
 * @pre    A library must be loaded before a function within it can be executed.
 *         An exception of type std::invalid_argument is thrown if the library
 *         is not already loaded into the process.
 *
 * @pre    The function to be executed must be found in the specified library.
 *         An exception of type std::invalid_argument is thrown if the function
 *         cannot be found within the specified library.
 *
 * @param library     Name of library containing function to be executed.
 * @param function    Name of function to be executed.
 * @param argument    Blob argument to the function.
 */
void Process::execute(const std::string& library,
		      const std::string& function,
		      const Blob& argument)
{    
    Guard guard_myself(this);

    // Find the library's entry
    std::map<std::string, LibraryEntry>::const_iterator
	i = dm_library_name_to_entry.find(library);
    
    // Check preconditions
    if(i == dm_library_name_to_entry.end())
	throw std::invalid_argument(
	    "Cannot execute a function in the library \"" + library +
	    "\" that was not previously loaded."
	    );

    // Find the function's entry within the library
    std::map<std::string, int>::const_iterator
	j = i->second.functions.find(function);
    
    // Check preconditions
    if(j == i->second.functions.end())
	throw std::invalid_argument(
	    "Cannot find the function \"" + function +
	    "\" within the library \"" + library + "\"."
	    );
    
    // Obtain a probe expression reference for the function
    ProbeExp function_exp = i->second.module->get_reference(j->second);

    // Create a probe experssion for the argument (first encoded as a string)
    ProbeExp argument_exp(encodeBlobAsString(argument).c_str());
    
    // Create a probe expression for the function call
    ProbeExp call_exp = function_exp.call(1, &argument_exp);
    
    // Ask DPCL to execute the function in this process    
    MainLoop::suspend();
    AisStatus retval = dm_process->bexecute(call_exp, NULL, NULL);
    Assert(retval.status() == ASC_success);
    MainLoop::resume();    
}



/**
 * Encode a blob as a string.
 *
 * Encodes a blob into a standard (0x00 terminated) C string. This is done by
 * replacing all occurences of 0x00 within the blob with a different, "safe",
 * non-zero byte value. In addition, all previous occurences of that safe value
 * are doubled up. For example, if the safe value is 0x22, the translations
 * ( 0x00 --> 0x22 ) and ( 0x22 --> 0x22 0x22 ) are applied to each byte in the
 * blob. This encoding is easily reversed and imposes minimal overhead.
 *
 * @note    The only reason this encoding is even necessary is that DPCL only
 *          allows passing a restricted subset of types (integers, floats, and
 *          strings) as parameters when calling a library function. We can get
 *          about 95% around this by utilizing blobs containing XDR encodings
 *          of arbitrary data structures. Those blobs, however, contain 0x00
 *          bytes that prevent them from being directly passed to the library
 *          function as a string. This encoding gets us the last 5% of the way.
 *
 * @note    The safe value is chosen to be a non-zero byte that isn't likely
 *          to occur frequently in a typical blob. This minimizes the overhead
 *          imposed by the encoding. Using 0x01, 0x80, 0xFE, or 0xFF, for
 *          example, would be a poor choice because they all occur frequently
 *          in structures containing integer or floating-point values.
 *
 * @param blob    Blob to be encoded.
 * @return        String encoding of the blob.
 */
std::string Process::encodeBlobAsString(const Blob& blob)
{
    std::string encoding;
    
    // Constants used in encoding
    const char ZeroValue = 0x00;
    const char SafeValue = 0xBA;
    
    // Iterate over each byte in the blob
    for(unsigned i = 0; i < blob.getSize(); ++i) {	
	char byte = reinterpret_cast<const char*>(blob.getContents())[i];
	
	// Replace zero values with the safe value
	if(byte == ZeroValue)
	    encoding.append(1, SafeValue);
	
	// Double-up occurences of the safe value
	else if(byte == SafeValue)
	    encoding.append(2, SafeValue);
	
	// Otherwise pass the byte through unchanged
	else
	    encoding.append(1, byte);
	
    }
    
    // Return the encoding to the caller
    return encoding;
}


    
} }  // OpenSpeedShop::Framework
