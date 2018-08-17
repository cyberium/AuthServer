# output generic information about the core
message(STATUS "CMaNGOS-Core revision : ${REVISION_ID}")
message(STATUS "Revision time stamp   : ${REVISION_DATE}")

# Show infomation about the options selected during configuration

# if(CLI)
#   message(STATUS "Build with CLI        : Yes (default)")
#   add_definitions(-DENABLE_CLI)
# else()
#   message(STATUS "Build with CLI        : No")
# endif()

# if(RA)
#   message(STATUS "* Build with RA       : Yes")
#   add_definitions(-DENABLE_RA)
# else(RA)
#   message(STATUS "* Build with RA       : No  (default)")
# endif(RA)

if(CMOPT_PCH)
  message(STATUS "Use PCH               : Yes (default)")
else()
  message(STATUS "Use PCH               : No")
endif()

if(CMOPT_DEBUG)
  message(STATUS "Build in debug-mode   : Yes")
else()
  message(STATUS "Build in debug-mode   : No  (default)")
endif()

if(CMOPT_WARNINGS)
  message(STATUS "Show all warnings     : Yes (default)")
else()
  message(STATUS "Show all warnings     : No")
endif()

# if(SQL)
#   message(STATUS "Install SQL-files     : Yes")
# else()
#   message(STATUS "Install SQL-files     : No  (default)")
# endif()

# if(TOOLS)
#   message(STATUS "Build map/vmap tools  : Yes")
# else()
#   message(STATUS "Build map/vmap tools  : No  (default)")
# endif()

message("")
