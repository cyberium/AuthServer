#
# This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

set(LIBRARY_NAME Framework)

# Find all the input files
FILE(GLOB_RECURSE LIBRARY_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp" "*.hpp")

if(WIN32)
  RemoveFilesFromPath("BackgroundTask/Daemon/" LIBRARY_SRCS)
else()
  RemoveFilesFromPath("BackgroundTask/Service/" LIBRARY_SRCS)
endif()

# Create visual studio groups using folder structure
foreach(source ${LIBRARY_SRCS})
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

add_library(${LIBRARY_NAME} STATIC
  ${LIBRARY_SRCS}
)

# include external dependencies without warnings
target_include_directories(${LIBRARY_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC "${CMAKE_SOURCE_DIR}/dep/HeaderOnlyLibs"
)
 
target_include_directories(${LIBRARY_NAME} SYSTEM
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${OPENSSL_INCLUDE_DIR}
  PUBLIC ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(${LIBRARY_NAME}
  PUBLIC ${OPENSSL_LIBRARIES}
  PUBLIC ${MYSQL_LIBRARY}
)
