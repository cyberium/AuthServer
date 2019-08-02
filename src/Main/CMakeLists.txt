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

set(EXECUTABLE_NAME ${CMANGOS_BINARY_REALMD_NAME})

FILE(GLOB EXECUTABLE_SRCS "*.h" "*.cpp")

if(WIN32)
  # add resource file to windows build
  set(EXECUTABLE_SRCS ${EXECUTABLE_SRCS} AuthServer.rc)
endif()

add_executable(${EXECUTABLE_NAME}
  ${EXECUTABLE_SRCS}
)

target_link_libraries(${EXECUTABLE_NAME}
  PRIVATE Framework
  PRIVATE RealmListMgr
  PRIVATE ${OPENSSL_CRYPTO_LIBRARY}
  PRIVATE ${MYSQL_LIBRARY}
  
)

if(WIN32)
  if(MINGW)
    target_link_libraries(${EXECUTABLE_NAME}
      wsock32
      ws2_32
    )
  endif()

  # Define OutDir to source/bin/(platform)_(configuaration) folder.
  set_target_properties(${EXECUTABLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${DEV_BIN_DIR}")
  set_target_properties(${EXECUTABLE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DEV_BIN_DIR}")
  set_target_properties(${EXECUTABLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${DEV_BIN_DIR}")
  #set_target_properties(${EXECUTABLE_NAME} PROPERTIES PROJECT_LABEL "AuthServer")

  # Add mysql lib to dev folder
  string(REGEX REPLACE "/" "\\\\" LibFolder ${MYSQL_LIBRARY_DIR})
  add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND copy \"${LibFolder}\\libmysql.dll\" \"$(OutDir)\")

  # Find dll libs of openssl
  find_file(OPENSSL_CRYPTO_DLL NAME libcrypto-1_1-x64.dll HINTS ${OPENSSL_INCLUDE_DIR}/..)
  find_file(OPENSSL_SSL_DLL NAME libssl-1_1-x64.dll HINTS ${OPENSSL_INCLUDE_DIR}/..)
    
  # Add openssl lib to dev folder
  string(REGEX REPLACE "/" "\\\\" OPENSSL_CRYPTO_DLL ${OPENSSL_CRYPTO_DLL})
  add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND copy \"${OPENSSL_CRYPTO_DLL}\" \"$(OutDir)\")
    
  # Add conf file to dev folder
  add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${CMAKE_CURRENT_SOURCE_DIR}/AuthServer.conf.dist.in\" \"$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/AuthServer.conf.dist\")
endif()

# Forcing the revision generation each build
add_custom_command(TARGET ${EXECUTABLE_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND}
    -DGIT_REVISION_WORK_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    -P "${CMAKE_SOURCE_DIR}/cmake/GitRevision.cmake"
)

set(EXECUTABLE_LINK_FLAGS "")

if(UNIX)
  set(EXECUTABLE_LINK_FLAGS "-pthread ${EXECUTABLE_LINK_FLAGS}")
endif()

if(APPLE)
  set(EXECUTABLE_LINK_FLAGS "-framework Carbon ${EXECUTABLE_LINK_FLAGS}")
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS
  "${EXECUTABLE_LINK_FLAGS}"
)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION ${BIN_DIR})
install(FILES authserver.conf.dist.in DESTINATION ${CONF_DIR} RENAME authserver.conf.dist)

if(MSVC)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/\${BUILD_TYPE}/${EXECUTABLE_NAME}.pdb DESTINATION ${BIN_DIR} CONFIGURATIONS Debug)
endif()
