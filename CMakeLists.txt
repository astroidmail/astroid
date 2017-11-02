##
# This MUST be the first line
#
cmake_minimum_required (VERSION 3.9)

##
# don't use obscure build types, but only Debug and Release
#
set (CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
message (STATUS "Available config types: ${CMAKE_CONFIGURATION_TYPES}")

##
# make sure the user deliberately selects a  build type
#
if (NOT CMAKE_BUILD_TYPE)
    message ( WARNING "No CMAKE_BUILD_TYPE value specified, defaulting to Debug." )
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
else ()
    message ( STATUS "The CMake build type is: ${CMAKE_BUILD_TYPE}" )
endif ()

##
# project name
#
project (astroid)

##
# retrieve current project version from git
#
execute_process (
  COMMAND git describe --abbrev=8 --tags --always
  OUTPUT_VARIABLE PROJECT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

##
# cmake -D...=ON|OFF command line feature switches and their default values
#
option (DISABLE_TERMINAL "Disable embedded terminal (default = enabled)" OFF)
option (DISABLE_EMBEDDED_EDITOR "Disable embedded editor (default = enabled)" OFF)

##
# welcome message
#
message (STATUS "Building ${PROJECT_NAME} ${PROJECT_VERSION}")

##
# check for required packages and libraries
#
find_package ( PkgConfig REQUIRED )
pkg_check_modules (GTKMM3     REQUIRED  gtkmm-3.0>=3.10)
pkg_check_modules (GLIBMM2    REQUIRED  glibmm-2.4)
pkg_check_modules (GMIME3     REQUIRED  gmime-3.0>=3.0.0)
pkg_check_modules (WEBKITGTK3 REQUIRED  webkitgtk-3.0)
pkg_check_modules (SASS       REQUIRED  libsass)

find_package ( Threads )

find_package ( Boost REQUIRED
  COMPONENTS
  filesystem
  program_options
  log_setup
  log
  thread
  date_time
  system
  )

if(APPLE)
  # with homebrew, libtettext is keg-only, i.e. not symlinked into
  # /usr/local, and it does not provide any pkg-config infromation of
  # its own; hence we have to locate it manually
  set ( Gettext_LDFLAGS "-L/usr/local/opt/gettext/lib" )
endif(APPLE)
  
find_library ( NOTMUCH_LIB notmuch )
if(NOT NOTMUCH_LIB)
  message (FATAL_ERROR "notmuch library not found")
endif()

include ( CheckSymbolExists )
set ( CMAKE_REQUIRED_LIBRARIES ${NOTMUCH_LIB} )
check_symbol_exists ( notmuch_database_index_file notmuch.h NOTMUCH_INDEX_FILE_API )
if(NOTMUCH_INDEX_FILE_API)
  add_definitions ( -DHAVE_NOTMUCH_INDEX_FILE )
endif()

pkg_check_modules (VTE2 vte-2.91)
if(DISABLE_TERMINAL OR ( NOT VTE2_FOUND ))
  add_definitions ( -DDISABLE_VTE )
  if(NOT VTE2_FOUND)
    message (WARNING "Vte library not found.")
  endif()
  message (STATUS "Disabling embedded terminal.")
endif()

##
# compile flags and options
#

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_compile_options (
  ${GTKMM3_CFLAGS}
  ${GLIBMM2_CFLAGS}
  ${GMIME3_CFLAGS}
  ${WEBKITGTK3_CFLAGS}
  ${VTE2_CFLAGS}
  ${SASS_CFLAGS}
  )

add_compile_options (
  -Wall
  -g
  -Wextra
  )

add_definitions (
  -DDISABLE_PLUGINS
  -DBOOST_LOG_DYN_LINK
  -DDEBUG
  )

set ( GIT_DESC ${PROJECT_VERSION} )
set ( PREFIX ${CMAKE_INSTALL_PREFIX} )
configure_file (
    src/build_config.hh.in
    src/build_config.hh
    )

include (CheckIncludeFile)
set ( CMAKE_REQUIRED_INCLUDES ${SASS_INCLUDE_DIRS} )
check_include_file (sass_context.h SASSCTX_SASS_CONTEXT_H)
check_include_file (sass/context.h SASSCTX_CONTEXT_H)
if( SASSCTX_SASS_CONTEXT_H )
  add_definitions ( -DSASSCTX_SASS_CONTEXT_H )
elseif( SASSCTX_CONTEXT_H )
  add_definitions ( -DSASSCTX_CONTEXT_H )
else()
  message (
    FATAL_ERROR
    "libsass must be installed: could not find header file. You can disable libsass with --disable-libsass, however, that requires a SCSS compiler like 'sassc' which can be specified with --scss-compiler=<path to compiler>."
    )
endif()  

include_directories (
  src/
  src/actions
  src/modes
  src/modes/editor
  src/modes/thread_index
  src/modes/thread_view
  src/plugin
  src/utils
  src/utils/gmime
  )

add_executable(
  astroid
  
  src/account_manager.cc
  src/astroid.cc
  src/chunk.cc
  src/command_bar.cc
  src/compose_message.cc
  src/config.cc
  src/crypto.cc
  src/db.cc
  src/main.cc
  src/main_window.cc
  src/message_thread.cc
  src/poll.cc

  src/modes/edit_message.cc
  src/modes/forward_message.cc
  src/modes/help_mode.cc
  src/modes/keybindings.cc
  src/modes/log_view.cc
  src/modes/mode.cc
  src/modes/paned_mode.cc
  src/modes/raw_message.cc
  src/modes/reply_message.cc
  src/modes/saved_searches.cc

  src/modes/editor/external.cc

  src/modes/thread_index/query_loader.cc
  src/modes/thread_index/thread_index.cc
  src/modes/thread_index/thread_index_list_cell_renderer.cc
  src/modes/thread_index/thread_index_list_view.cc

  src/modes/thread_view/dom_utils.cc
  src/modes/thread_view/theme.cc
  src/modes/thread_view/thread_view.cc
  src/modes/thread_view/web_inspector.cc

  src/actions/action.cc
  src/actions/action_manager.cc
  src/actions/cmdaction.cc
  src/actions/difftag_action.cc
  src/actions/onmessage.cc
  src/actions/tag_action.cc
  src/actions/toggle_action.cc

  src/utils/address.cc
  src/utils/cmd.cc
  src/utils/date_utils.cc
  src/utils/gravatar.cc
  src/utils/resource.cc
  src/utils/ustring_utils.cc
  src/utils/utils.cc
  src/utils/vector_utils.cc

  src/utils/gmime/gmime-compat.cc
  src/utils/gmime/gmime-filter-html-bq.c
  src/utils/gmime/gtrie.c
  src/utils/gmime/url-scanner.c
  )

target_link_libraries (
  astroid
  ${NOTMUCH_LIB}
  ${WEBKITGTK3_LIBRARIES}
  ${GTKMM3_LIBRARIES}
  ${GLIBMM2_LIBRARIES}
  ${GMIME3_LIBRARIES}
  ${VTE2_LIBRARIES}
  ${SASS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )

##
# conditional compilation based on feature switches
#
if(DISABLE_EMBEDDED_EDITOR)
  add_definitions ( -DDISABLE_EMBEDDED )
  message (STATUS "Disabling embedded editor.")
else(DISABLE_EMBEDDED_EDITOR)
  if (APPLE)
    message (WARNING "Embedded editor cannot be built on macOS; disabling embedded editor.")
    add_definitions ( -DDISABLE_EMBEDDED )
  else(APPLE)
    target_sources (
      astroid
      PRIVATE
      src/modes/editor/editor.cc
      src/modes/editor/plugin.cc
      )
  endif(APPLE)
endif(DISABLE_EMBEDDED_EDITOR)
