#
# RDPShare: Windows Desktop Sharing remote control interface
#
# Copyright 2016 Simul Piscator
#
# RDPShare is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RDPShare is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RDPShare.  If not, see <http://www.gnu.org/licenses/>.
#
cmake_minimum_required( VERSION 3.0 )
cmake_policy( VERSION 3.0 )

project( RDPShare VERSION 0.1.0 )

add_definitions( -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE )
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
		)
   if(${flag_var} MATCHES "/MD")
      string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif()
endforeach()

add_executable( RDPShare WIN32
	RDPShare.cpp
	Guids.cpp
	Sharer.cpp
	Sharer.h
	RDPSessionEvents.h
	Utils.cpp
	Utils.h
	RDPFile.cpp
	RDPFile.h
	Server.cpp
	Server.h
)
target_link_libraries( RDPShare ws2_32 shlwapi )
