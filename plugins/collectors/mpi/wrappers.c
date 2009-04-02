/*******************************************************************************
** Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
**
** This library is free software; you can redistribute it and/or modify it under
** the terms of the GNU Lesser General Public License as published by the Free
** Software Foundation; either version 2.1 of the License, or (at your option)
** any later version.
**
** This library is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
** details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

/** @file
 *
 * Special-Purpose MPI Wrapper Functions
 *
 * Most of the MPI wrapper functions are generated automagically via the custom
 * `mkwrapper' tool and its associated template file `mkwrapper.template'. The
 * following functions cannot be generated via that mechanism because they have
 * specialized implementations that don't fit the template. 
 */

#include "RuntimeAPI.h"
#include "blobs.h"
#include "runtime.h"

#include <mpi.h>

#if defined (OPENSS_OFFLINE)
int MPI_Init(int * arg1, char *** arg2)
#else
int mpi_PMPI_Init(int * arg1, char *** arg2)
#endif
{
    int retval, datatype_size;
    mpi_event event;

    bool_t dotrace = mpi_do_trace("MPI_Init");

    if (dotrace) {
	mpi_start_event(&event);
	event.start_time = OpenSS_GetTime();
    }

    /* Call the real MPI function */
    retval = PMPI_Init(arg1, arg2);

    if (dotrace) {
	event.stop_time = OpenSS_GetTime();

	/* Record event */
	mpi_record_event(&event, OpenSS_GetAddressOfFunction(PMPI_Init));
    }

    /* Return the real MPI function's return value to the caller */
    return retval;
}
