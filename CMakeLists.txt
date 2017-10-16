cmake_minimum_required (VERSION 3.9)

project (astroid)

execute_process (
  COMMAND git describe --abbrev=8 --tags --always
  OUTPUT_VARIABLE PROJECT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

message (
  STATUS
  "Building ${PROJECT_NAME} ${PROJECT_VERSION}"
  )

find_package ( PkgConfig REQUIRED )
pkg_check_modules (GTKMM3     REQUIRED  gtkmm-3.0>=3.10)
pkg_check_modules (GLIBMM2    REQUIRED  glibmm-2.4)
pkg_check_modules (GMIME3     REQUIRED  gmime-3.0>=3.0.0)
pkg_check_modules (WEBKITGTK3 REQUIRED  webkitgtk-3.0)
pkg_check_modules (VTE2       REQUIRED  vte-2.91)
pkg_check_modules (SASS       REQUIRED  libsass)

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

find_library ( NOTMUCH_LIB notmuch )
if(NOT NOTMUCH_LIB)
  message(FATAL_ERROR "notmuch library not found")
endif()

link_directories (
  ${GTKMM3_LIBRARY_DIRS}
  ${GLIBMM2_LIBRARY_DIRS}
  ${GMIME3_LIBRARY_DIRS}
  ${WEBKITGTK3_LIBRARY_DIRS}
  ${VTE2_LIBRARY_DIRS}
  ${SASS_LIBRARY_DIRS}
  )

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
  -pthread
  -g
  -Wextra
  )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_definitions (
  -DDISABLE_EMBEDDED
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

if( EXISTS ${SASS_INCLUDE_DIRS}/sass_context.h )
  add_definitions ( -DSASSCTX_SASS_CONTEXT_H )
elseif( EXISTS ${SASS_INCLUDE_DIRS}/sass/context.h)
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

  src/modes/thread_index/query_loader.cc
  src/modes/thread_index/thread_index.cc
  src/modes/thread_index/thread_index_list_cell_renderer.cc
  src/modes/thread_index/thread_index_list_view.cc

  src/modes/thread_view/dom_utils.cc
  src/modes/thread_view/theme.cc
  src/modes/thread_view/thread_view.cc
  src/modes/thread_view/web_inspector.cc

  src/modes/editor/editor.cc
  src/modes/editor/external.cc
  src/modes/editor/plugin.cc

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

  # src/plugin/astroid_activatable.c
  # src/plugin/gir_main.c
  # src/plugin/thread_index_activatable.c
  # src/plugin/thread_view_activatable.c
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
  )
