################################################################################
# Copyright (c) 2013-2014 Krell Institute. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA
################################################################################

include(FindPackageHandleStandardArgs)

set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")


# This project does not use Libtool directly but still uses ltdl for
# plug-in loading.
find_library(LTDL_LIBRARY_SHARED NAMES ltdl
    HINTS $ENV{LTDL_ROOT}
    HINTS ${LTDL_ROOT}
    PATHS /usr /usr/local
    PATH_SUFFIXES lib lib64
    )

include(CheckIncludeFileCXX)

find_path(LTDL_INCLUDE_DIR
    NAMES ltdl.h
    PATHS /usr /usr/local
    HINTS $ENV{LTDL_ROOT}
    HINTS ${LTDL_ROOT}
    PATH_SUFFIXES include
    )

message(STATUS "LTDL LTDL_H_FOUND: " ${LTDL_H_FOUND})
message(STATUS "LTDL LTDL_LIBRARY_SHARED: " ${LTDL_LIBRARY_SHARED})
message(STATUS "LTDL LTDL_INCLUDE_DIR: " ${LTDL_INCLUDE_DIR})
message(STATUS "LTDL LTDL_FOUND: " ${LTDL_FOUND})

mark_as_advanced(
            LTDL_INCLUDE_DIR
	    LTDL_LIBRARY_SHARED
            )
