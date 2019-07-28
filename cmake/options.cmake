option(CMOPT_DEBUG         "Include additional debug-code in core" OFF)
option(CMOPT_WARNINGS      "Show all warnings during compile"      OFF)
option(CMOPT_PCH           "Use precompiled headers"               ON)

# TODO: options that should be checked/created:
#option(CLI                  "With CLI"                              ON)
#option(RA                   "With Remote Access"                    OFF)
#option(SQL                  "Copy SQL files"                        OFF)
#option(TOOLS                "Build tools"                           OFF)

message("")
message(STATUS
  "This script builds the Auth server that can be used with CMaNGOS servers.
  Options that can be used in order to configure the process:
    CMAKE_INSTALL_PREFIX    Path where the server should be installed to
    CMOPT_PCH               Use precompiled headers
    CMOPT_DEBUG             Include additional debug-code in core
    CMOPT_WARNINGS          Show all warnings during compile

  To set an option simply type -D<OPTION>=<VALUE> after 'cmake <srcs>'.
  Also, you can specify the generator with -G. see 'cmake --help' for more details
  For example:
    Build auth server
    cmake -DCMAKE_INSTALL_PREFIX=../opt/cmangos .."
)
message("")
