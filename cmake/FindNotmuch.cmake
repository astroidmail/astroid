# - Try to find Notmuch
# Once done, this will define
#
#  Notmuch_FOUND          - system has Notmuch
#  Notmuch_VERSION        - the version of the Notmuch shared library
#  Notmuch_INCLUDE_DIRS   - the Notmuch include directories
#  Notmuch_LIBRARIES      - link these to use Notmuch
#  Notmuch_GMIME_VERSION  - the GMime version notmuch was linked against

include (LibFindMacros)

mark_as_advanced (
  Notmuch_INCLUDE_DIR
  Notmuch_LIBRARY
  _notmuch_version_file
  _notmuch_version_contents
  _notmuch_version_major
  _notmuch_version_minor
  _notmuch_version_micro
  _notmuch_prerequisites
  )

# find include directory
find_path (Notmuch_INCLUDE_DIR notmuch.h)
set (Notmuch_PROCESS_INCLUDES Notmuch_INCLUDE_DIR)

# find library diretcory
find_library (Notmuch_LIBRARY notmuch)
set (Notmuch_PROCESS_LIBS Notmuch_LIBRARY)

# version information
# (unfortunately cannott use libfind_version_header, because notmuch.h
# defines separate macros for major, minor, and micro versions, and
# without the double quotes)
set (Notmuch_VERSION "unknown"
  CACHE INTERNAL "The version of the Notmuch shared library which was detected")
set (_notmuch_version_file "${Notmuch_INCLUDE_DIR}/notmuch.h")
if (EXISTS "${_notmuch_version_file}")
  file (STRINGS "${_notmuch_version_file}" _notmuch_version_contents
    REGEX "#define[ \t]+(LIBNOTMUCH_(MAJOR|MINOR|MICRO)_VERSION)[ \t]+[0-9]+")
  if (_notmuch_version_contents)
    string (REGEX REPLACE
      ".*#define[ \t]+LIBNOTMUCH_MAJOR_VERSION[ \t]+([0-9]+).*"
      "\\1" _notmuch_version_major ${_notmuch_version_contents})
    string (REGEX REPLACE
      ".*#define[ \t]+LIBNOTMUCH_MINOR_VERSION[ \t]+([0-9]+).*"
      "\\1" _notmuch_version_minor ${_notmuch_version_contents})
    string (REGEX REPLACE
      ".*#define[ \t]+LIBNOTMUCH_MICRO_VERSION[ \t]+([0-9]+).*"
      "\\1" _notmuch_version_micro ${_notmuch_version_contents})
    set (Notmuch_VERSION
      "${_notmuch_version_major}.${_notmuch_version_minor}.${_notmuch_version_micro}")
  else()
    message(WARNING "[ FindNotmuch.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
      "Failed to parse version number, please report this as a bug.")
  endif()
  unset (_notmuch_version_contents)
endif()

# set common output variables
libfind_process (Notmuch)	# will set Notmuch_FOUND, Notmuch_INCLUDE_DIRS and Notmuch_LIBRARIES

# notmuch_database_index_file() API presence
include (CheckSymbolExists)
set (CMAKE_REQUIRED_INCLUDES ${Notmuch_INCLUDE_DIR})
set (CMAKE_REQUIRED_LIBRARIES ${Notmuch_LIBRARY})

# GMime version notmuch was linked against
include (GetPrerequisites)
GET_PREREQUISITES(${Notmuch_LIBRARY} _notmuch_prerequisites 0 0 "" "")
set (Notmuch_GMIME_VERSION  "unknown")
if (_notmuch_prerequisites)
  foreach (_nm_prereq ${_notmuch_prerequisites})
    if (_nm_prereq MATCHES
      "^(.*/)?${CMAKE_SHARED_LIBRARY_PREFIX}gmime[-\\.]([0-9]+\\.[0-9]+)(\\${CMAKE_SHARED_LIBRARY_SUFFIX})?(\\.[0-9]+)(\\${CMAKE_SHARED_LIBRARY_SUFFIX})?$"
    )
      set (Notmuch_GMIME_VERSION "${CMAKE_MATCH_2}${CMAKE_MATCH_4}")
      message (STATUS "Notmuch was built against GMime ${Notmuch_GMIME_VERSION}")
    endif ()
  endforeach (_nm_prereq)
else()
  message(WARNING "[ FindNotmuch.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
    "Failed to determine libnotmuch prerequisites, please report this as a bug.")
endif()
unset (_notmuch_prerequisites)
if (Notmuch_GMIME_VERSION EQUAL "unknown")
  message(WARNING "[ FindNotmuch.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
    "Failed to determine needed libgmime version number, please report this as a bug.")
endif ()
