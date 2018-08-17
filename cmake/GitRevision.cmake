# Setting up some revision informations

# Find Git executable
find_package(Git)

# Find git hash and commit time
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE REVISION_ID
    RESULT_VARIABLE GIT_RESULT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(GIT_RESULT)
    set(REVISION_ID "Git repository not found")
    set(REVISION_DATE "\"0000-00-00T00:00:00+00:00\"")
  else()
    execute_process(
      COMMAND ${GIT_EXECUTABLE} show --quiet --date=iso-strict --format="%ad" ${REVISION_ID}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE REVISION_DATE
      RESULT_VARIABLE GIT_RESULT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_RESULT)
      set(REVISION_DATE "\"0000-00-00T00:00:00+00:00\"")
    endif()
  endif()
else()
  set(REVISION_ID "Git not found")
  set(REVISION_DATE "\"0000-00-00T00:00:00+00:00\"")
endif()

string(TIMESTAMP BUILD_DATE "%Y-%m-%dT%H:%M:%S UTC" UTC)

# Generating the revision.h
configure_file(
  "${GIT_REVISION_WORK_DIR}/revision.h.in"
  "${GIT_REVISION_WORK_DIR}/revision.h"
  @ONLY
)