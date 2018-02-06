##
# This MUST be the first line
#
cmake_minimum_required (VERSION 3.9)

##
# project name
#
project (astroid)

##
# don't use obscure build types, but only Debug and Release
#
set (CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
message (STATUS "Available config types: ${CMAKE_CONFIGURATION_TYPES}")

##
# generate a compile_commands.json file (needed for ninja)
#
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
# where supporting modules live
#
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

##
# version number
#
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  # debug build - retrieve current project version from git
  execute_process (
    COMMAND git describe --abbrev=8 --tags --always
    OUTPUT_VARIABLE PROJECT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
  # release build - use hard-coded version string
  set ( PROJECT_VERSION "0.11.0" )
endif ()

##
# plugin API namespace and version
#
if(NOT DISABLE_PLUGINS)
  set (PLUGIN_NAMESPACE    "Astroid")
  set (PLUGIN_API_VERSION  "0.1")
endif()

##
# cmake -D...=ON|OFF command line feature switches and their default values
#
option (DISABLE_TERMINAL         "Disable embedded terminal (default = enabled)"  OFF)
option (DISABLE_EMBEDDED_EDITOR  "Disable embedded editor (default = enabled)"    OFF)
option (DISABLE_PLUGINS          "Disable plugins (default = enabled)"            OFF)
option (ENABLE_PROFILING         "Enable profiling (default = disabled)"          OFF)
option (DISABLE_LIBSASS          "Disable libsass, requires a sassc compiler (default = enabled)" OFF)

set (SCSS_COMPILER "sassc" CACHE STRING "SCSS compiler to use when libsass is disabled (default = sassc)")

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

pkg_check_modules (GMIME                gmime-3.0>=3.0.0)
set (GMIME_VER 3.0)
if (NOT GMIME_FOUND)
  # fall back to gmime 2
  pkg_check_modules (GMIME              gmime-2.6>=2.6.0)
  set (GMIME_VER 2.6)
  if (NOT GMIME_FOUND)
    message (
      FATAL_ERROR
      "GMime 3 or GMime 2.6 required"
      )
  endif()
endif()

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

find_path ( NOTMUCH_INCLUDE_DIR notmuch.h )
find_library ( NOTMUCH_LIBRARY notmuch )
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set NOTMUCH_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(notmuch  DEFAULT_MSG
                                  NOTMUCH_LIBRARY NOTMUCH_INCLUDE_DIR)
if(NOT NOTMUCH_FOUND)
  message (FATAL_ERROR "notmuch library not found")
endif()
mark_as_advanced( NOTMUCH_INCLUDE_DIR NOTMUCH_LIBRARY )
set( NOTMUCH_LIBRARIES ${NOTMUCH_LIBRARY} )
set( NOTMUCH_INCLUDE_DIRS ${NOTMUCH_INCLUDE_DIR} )

include ( CheckSymbolExists )
set ( CMAKE_REQUIRED_INCLUDES ${NOTMUCH_INCLUDE_DIRS} )
set ( CMAKE_REQUIRED_LIBRARIES ${NOTMUCH_LIBRARIES} )
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

include_directories (
  ${GTKMM3_INCLUDE_DIRS}
  ${GLIBMM2_INCLUDE_DIRS}
  ${GMIME_INCLUDE_DIRS}
  ${WEBKITGTK3_INCLUDE_DIRS}
  ${VTE2_INCLUDE_DIRS}
  ${SASS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  )

add_compile_options (
  ${GTKMM3_CFLAGS}
  ${GLIBMM2_CFLAGS}
  ${GMIME_CFLAGS}
  ${WEBKITGTK3_CFLAGS}
  ${VTE2_CFLAGS}
  ${SASS_CFLAGS}
  )

#
# default options
#
add_compile_options (
  -Wall
  )
add_definitions (
  -DBOOST_LOG_DYN_LINK
  )
#
# debug builds
#
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
add_compile_options (
  -Wextra
  -g
  )
add_definitions (
  -DDEBUG
  )
endif ()
#
# release builds
#
if (CMAKE_BUILD_TYPE STREQUAL "Release")
add_compile_options (
  -O2
  )
endif()

##
# Check if notmuch is compiled against the same GMime library
#
include(CheckCXXSourceRuns)
unset ( CMAKE_REQUIRED_FLAGS )
unset ( CMAKE_REQUIRED_INCLUDES )
unset ( CMAKE_REQUIRED_LIBRARIES )
# try_run will get confused and pass flags to cmake instead if they are connected with ";"
string ( REPLACE ";" "," NEW_CFLAGS "${GMIME_CFLAGS}" )
list ( APPEND CMAKE_REQUIRED_FLAGS "${NEW_CFLAGS}" )
list ( APPEND CMAKE_REQUIRED_INCLUDES  ${GMIME_INCLUDE_DIRS} ${NOTMUCH_INCLUDE_DIRS} )
list ( APPEND CMAKE_REQUIRED_LIBRARIES ${NOTMUCH_LIBRARIES} ${GMIME_LDFLAGS} )

CHECK_CXX_SOURCE_RUNS(
  "
  # include <notmuch.h>
  # include <gmime/gmime.h>

  # include <iostream>

  /* test if we are linked against the same GMime version as notmuch */

  # if (GMIME_MAJOR_VERSION < 3)
  # define g_mime_init() g_mime_init(0)
  # define internet_address_list_parse(f,str) internet_address_list_parse_string(str)
  # define internet_address_list_to_string(ia,f,encode) internet_address_list_to_string(ia,encode)
  # endif

  int main () {
    g_mime_init ();

    const char * a = \"Test <hey@asdf.da>\";
    InternetAddressList * ia = internet_address_list_parse (NULL, a);

    const char * addrs = internet_address_list_to_string (ia, NULL, false);

    std::cout << \"address: \" << addrs << std::endl;

    return 0;
  }
  "

  GMIME_MATCHES)

if (NOT GMIME_MATCHES)
  message (
    FATAL_ERROR
    "You must compile and link against the same GMime version (e.g. 2.6 or 3) with both notmuch and astroid."
    )
endif()

if (ENABLE_PROFILING)
  message (STATUS "Enabling profiling.")
  add_compile_options ( -pg )
  set (CMAKE_EXE_LINKER_FLAGS "-pg")
endif()

set ( GIT_DESC ${PROJECT_VERSION} )
set ( PREFIX ${CMAKE_INSTALL_PREFIX} )
configure_file (
    src/build_config.hh.in
    src/build_config.hh
    )

include (CheckIncludeFile)
if(NOT DISABLE_LIBSASS)
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
      "libsass must be installed: could not find header file. You can disable libsass with -DDISABLE_LIBSASS=ON, however, that requires a SCSS compiler like 'sassc' which can be specified with -DSCSS_COMPILER=<path to compiler>."
      )
  endif()
else ()
  add_definitions ( -DDISABLE_LIBSASS )
  # the css target is defined after astroid target has been defined
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
  ${CMAKE_BINARY_DIR}/src	# needed to find the generated build_config.hh
  )

add_library(
  hypocycloid

  src/account_manager.cc
  src/astroid.cc
  src/chunk.cc
  src/command_bar.cc
  src/compose_message.cc
  src/config.cc
  src/crypto.cc
  src/db.cc
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

add_executable (
  astroid

  src/main.cc
  )

target_link_libraries (
  astroid

  hypocycloid
  ${NOTMUCH_LIBRARIES}
  ${WEBKITGTK3_LDFLAGS}
  ${GTKMM3_LDFLAGS}
  ${GLIBMM2_LDFLAGS}
  ${GMIME_LDFLAGS}
  ${VTE2_LDFLAGS}
  ${SASS_LDFLAGS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Gettext_LDFLAGS}
  )

##
# conditional compilation based on feature switches
#

##
# pre-compiled style sheets (instead of dynamic at run-time)
#
if(DISABLE_LIBSASS)
  message (STATUS "Generating thread-view.css")
  add_custom_command ( TARGET astroid
    COMMAND ${SCSS_COMPILER} ${CMAKE_SOURCE_DIR}/ui/thread-view.scss ${CMAKE_BINARY_DIR}/thread-view.css )
endif()

##
# embedded editor
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
      hypocycloid
      PRIVATE
      src/modes/editor/editor.cc
      src/modes/editor/plugin.cc
      )
  endif(APPLE)
endif(DISABLE_EMBEDDED_EDITOR)

##
# plugins
#
if(DISABLE_PLUGINS)
  add_definitions ( -DDISABLE_PLUGINS )
  message (STATUS "Disabling plugins.")
else()
  if(APPLE)
    # with homebrew, libffi is keg-only, i.e. not symlinked into
    # /usr/local; hence we have to tell pkg-config where to look for
    # it
    list ( APPEND CMAKE_PREFIX_PATH /usr/local/opt/libffi )
  endif()
  include (FindGObjectIntrospection)   # this performs a pkg_check_modules()
  pkg_check_modules (PEAS libpeas-1.0>=1.0.0)
  if(INTROSPECTION_FOUND AND PEAS_FOUND)
    #
    # GObject introspection
    #
    include (GObjectIntrospectionMacros)
    set (GIR_NAME "${PLUGIN_NAMESPACE}-${PLUGIN_API_VERSION}.gir")
    set (${GIR_NAME}_NAMESPACE      ${PLUGIN_NAMESPACE})
    set (${GIR_NAME}_VERSION        ${PLUGIN_API_VERSION})
    set (${GIR_NAME}_INCLUDEPKGS    GObject-2.0 GMime-${GMIME_VER})
    set (${GIR_NAME}_PROGRAM        ${CMAKE_CURRENT_BINARY_DIR}/gir_main)
    set (${GIR_NAME}_FILES
      src/plugin/astroid_activatable.c
      src/plugin/thread_view_activatable.c
      src/plugin/thread_index_activatable.c
      src/plugin/astroid_activatable.h
      src/plugin/thread_view_activatable.h
      src/plugin/thread_index_activatable.h
      src/plugin/gir_main.c
      )
    list ( APPEND INTROSPECTION_GIRS ${GIR_NAME} )
    set (INTROSPECTION_SCANNER_ARGS --warn-all)
    gir_add_introspections (INTROSPECTION_GIRS)   # magic happens here
    #
    # additional source files for astroid (plugin framework)
    #
    target_sources (
      hypocycloid
      PRIVATE
      src/plugin/astroid_activatable.c
      src/plugin/manager.cc
      src/plugin/thread_index_activatable.c
      src/plugin/thread_view_activatable.c
      )
    #
    # link astroid against libpeas
    #
    include_directories ( ${PEAS_INCLUDE_DIRS} )
    target_link_libraries (
      astroid
      ${PEAS_LDFLAGS}
      )
    #
    # build gir_main executable for introspection
    #
    add_executable (
      gir_main
      ${${GIR_NAME}_FILES}
      )
    target_link_libraries (
      gir_main
      ${WEBKITGTK3_LDFLAGS}
      ${GTKMM3_LDFLAGS}
      ${PEAS_LDFLAGS}
      ${INTROSPECTION_LIBS}
      ${Gettext_LDFAGS}
      )
    #
    # depenedecies for proper build order
    #
    add_dependencies ( gir-typelibs gir_main )
    add_dependencies ( astroid  gir-typelibs )
  else(INTROSPECTION_FOUND AND PEAS_FOUND)
    add_definitions ( -DDISABLE_PLUGINS )
    message (WARNING "Required packages not found; disabling plugins.")
  endif(INTROSPECTION_FOUND AND PEAS_FOUND)
endif(DISABLE_PLUGINS)

##
# Installing stuff
#
install ( TARGETS astroid
  RUNTIME DESTINATION bin )

# icons are installed in three locations
install (DIRECTORY ui/icons
  DESTINATION share/astroid/ui
  )
include (InstallSymbolicLink)
install_symlink (../../../../astroid/ui/icons/icon_color.png
  ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/512x512/apps/astroid.png
  )
install_symlink (../../../../astroid/ui/icons/icon_color.svg
  ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/app/astroid.svg
  )

install (FILES ui/astroid.desktop
  DESTINATION share/applications
  )

install (FILES ui/thread-view.html ui/edit-message.glade ui/no-mail.png
  DESTINATION share/astroid/ui
  )

if(DISABLE_LIBSASS)
  install (FILES ${CMAKE_BINARY_DIR}/thread-view.css
    DESTINATION share/astroid/ui )
else()
  install (FILES ui/thread-view.scss
    DESTINATION share/astroid/ui)
endif()

##
# Tests
#
enable_testing()
add_subdirectory (tests)

