////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

#include "ToolAPI.hxx"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
 
using namespace OpenSpeedShop::Framework;

int main(int argc, char* argv[])
{
    // Display usage information when necessary
    if(argc < 2) {
        std::cout << "Usage: "
                  << ((argc > 0) ? argv[0] : "???")
                  << " <a.out> <args>" << std::endl;
	return 1;
    }
    
    // Build a command string from the passed arguments
    std::string command;
    for(int i = 1; i < argc; ++i) {
        if(i > 1)
            command += " ";
        command += argv[i];
    }
    
    try {
	
	// Create and open an experiment
	std::string name = std::string(argv[1]) + ".openss";
	Experiment::create(name);
	Experiment experiment(name);
	
	// Create a process for the command in the suspended state
	Thread thread = experiment.createProcess(command);

	// Create the example collector and set its sampling rate
	Collector collector = experiment.createCollector("example");
	collector.setParameterValue("sampling_rate", (unsigned)10);

	// Attach this process to the collector and start collecting data
	collector.attachThread(thread);
	collector.startCollecting();
	
	// Resume all threads and wait for them to terminate
	experiment.getThreads().changeState(Thread::Running);
	while(!experiment.getThreads().areAllState(Thread::Terminated))
	    sleep(1);
	
	// TODO: metric evaluation

    }
    catch(const std::exception& error) {
	std::cerr
	    << std::endl
	    << "Error: "
	    << (((error.what() == NULL) || (strlen(error.what()) == 0)) ?
		"Unknown runtime error." : error.what()) << std::endl
	    << std::endl;
	return 1;
    }

    // Indicate success to the shell
    return 0;
}
