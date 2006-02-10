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
 * Declaration of the Queries namespace.
 *
 */

#ifndef _OpenSpeedShop_Queries_
#define _OpenSpeedShop_Queries_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ToolAPI.hxx"

#include <functional>
#include <map>
#include <string>
#include <vector>



namespace OpenSpeedShop {

    /**
     * Enclosing namespace for the <em>Open|SpeedShop</em> query library.
     *
     * Namespace containing procedural functions for performing higher-level
     * queries on experiments generated by the <em>Open|SpeedShop</em> framework
     * library. There is nothing "magical" about these functions. They use the
     * same API that is available directly to every tool. The intention is
     * simply to promote re-use by providing some of the more commonly used 
     * queries.
     */
    namespace Queries
    {

	void GetSourceObjects(const Framework::Thread&,
                              std::set<Framework::LinkedObject>&);
        void GetSourceObjects(const Framework::Thread&,
                              std::set<Framework::Function>&);
        void GetSourceObjects(const Framework::Thread&,
                              std::set<Framework::Statement>&);

	/**
	 * Strict weak ordering predicate for linked objects.
	 *
	 * Defines a strict weak ordering predicate for linked objects that
	 * works properly even when the two linked objects are in different
	 * experiment databases. This predicate is slower than the less-than
	 * operator for linked objects, but is useful when comparing across
	 * experiments.
	 */
	struct CompareLinkedObjects :
	    std::binary_function<const Framework::LinkedObject&,
				 const Framework::LinkedObject&,
				 bool>
	{
	    bool operator()(const Framework::LinkedObject&,
			    const Framework::LinkedObject&) const;
	};

	/**
	 * Strict weak ordering predicate for functions.
	 *
	 * Defines a strict weak ordering predicate for functions that works
	 * properly even when the two functions are in different experiment
	 * databases. This predicate is slower than the less-than operator
	 * for functions, but is useful when comparing across experiments.
	 */
	struct CompareFunctions :
	    std::binary_function<const Framework::Function&,
				 const Framework::Function&,
				 bool>
	{
	    bool operator()(const Framework::Function&,
			    const Framework::Function&) const;
	};

	/**
	 * Strict weak ordering predicate for statements.
	 *
	 * Defines a strict weak ordering predicate for statements that works
	 * properly even when the two statements are in different experiment
	 * databases. This predicate is slower than the less-than operator
	 * for statements, but is useful when comparing across experiments.
	 */
	struct CompareStatements :
	    std::binary_function<const Framework::Statement&,
				 const Framework::Statement&,
				 bool>
	{
	    bool operator()(const Framework::Statement&,
			    const Framework::Statement&) const;
	};

	template <typename TS, typename TM>
	void GetMetricInThread(
	    const Framework::Collector&,
	    const std::string&,
	    const Framework::TimeInterval&,
	    const Framework::Thread&,
	    const std::set<TS >&,
	    Framework::SmartPtr<std::map<TS, TM > >&
	    );

	template <typename TS, typename TM>
	void GetMetricOfAllInThread(
	    const Framework::Collector&,
	    const std::string&,
	    const Framework::TimeInterval&,
	    const Framework::Thread&,
	    Framework::SmartPtr<std::map<TS, TM > >&
	    );

	template <typename TM>
	void GetMetricByStatementOfFileInThread(
	    const Framework::Collector&,
	    const std::string&,
	    const Framework::TimeInterval&,
	    const Framework::Thread&,
	    const Framework::Path&,
	    Framework::SmartPtr<std::map<int, TM > >&
	    );

    }

}



template <typename T>
void operator+=(
    std::map<OpenSpeedShop::Framework::StackTrace, std::vector<T > >&,
    const std::map<OpenSpeedShop::Framework::StackTrace, std::vector<T > >&
    );



#include "AdditionAssignment.txx"
#include "GetMetricInThread.txx"
#include "GetMetricOfAllInThread.txx"
#include "GetMetricByStatement.txx"



#endif
