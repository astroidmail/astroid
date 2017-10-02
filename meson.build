project ('astroid', ['cpp', 'c'],
    default_options : [ 'cpp_std=c++11'] )

version = '0.10.1'

c  = meson.get_compiler ('c')
cc = meson.get_compiler ('cpp')

gmime3   = dependency ('gmime-3.0', required : false)
gmime2   = dependency ('gmime-2.6', required : false)

if gmime3.found ()
  gmime = gmime3
elif gmime2.found ()
  gmime = gmime2
else
  error ('neither gmime3 or gmime2 available')
endif

boost   = dependency ('boost',
                        modules : [ 'filesystem', 'system', 'program_options',
                                    'log_setup', 'log', 'thread', 'date_time' ])
add_global_arguments ('-DBOOST_LOG_DYN_LINK', language : 'cpp')

gtk     = dependency ('gtk+-3.0')
gtkmm   = dependency ('gtkmm-3.0')
webkit  = dependency ('webkitgtk-3.0')
peas    = dependency ('libpeas-1.0')
threads = dependency ('threads')

notmuch = cc.find_library ('notmuch', required : true)
if cc.has_function ('notmuch_database_index_file', dependencies : notmuch, prefix : '''#include <notmuch.h>''')
  add_global_arguments ('-DHAVE_NOTMUCH_INDEX_FILE', language: 'cpp')
endif


vte = dependency ('vte-2.91')
if not vte.found () or get_option('disable-terminal')
  add_global_arguments ('-DDISABLE_VTE', language: 'cpp')
  message ('Embedded terminal is disabled.')
endif

if get_option ('disable-embedded-editor')
  add_global_arguments ('-DDISABLE_EMBEDDED', language: 'cpp')
  message ('Embedded editor is disabled.')
endif

libsass = dependency ('libsass')
if c.has_header ('sass_context.h')
  add_global_arguments ('-DSASSCTX_SASS_CONTEXT_H', language : 'cpp')
elif c.has_header  ('sass/context.h')
  add_global_arguments ('-DSASSCTX_CONTEXT_H', language : 'cpp')
endif

if get_option ('buildtype').startswith('debug')
  add_global_arguments ('-DDEBUG', language : 'cpp')
endif

conf_data = configuration_data ()
if get_option ('buildtype').startswith('debug')
  conf_data.set ('git_desc', run_command ('git', ['describe', '--abbrev=8', '--tags', '--always']).stdout().strip('\n'))
else
  conf_data.set ('git_desc', version)
endif
conf_data.set ('prefix', get_option('prefix'))
configure_file (input : 'src/build_config.hh.in',
                output : 'build_config.hh',
                configuration : conf_data)

deps = [ gmime, boost, gtk, gtkmm, webkit, peas, vte, threads, notmuch, libsass ]

src = [
    'src/modes/log_view.cc',
    'src/modes/mode.cc',
    'src/modes/thread_view/thread_view.cc',
    'src/modes/thread_view/web_inspector.cc',
    'src/modes/thread_view/dom_utils.cc',
    'src/modes/thread_view/theme.cc',
    'src/modes/paned_mode.cc',
    'src/modes/thread_index/thread_index_list_cell_renderer.cc',
    'src/modes/thread_index/query_loader.cc',
    'src/modes/thread_index/thread_index.cc',
    'src/modes/thread_index/thread_index_list_view.cc',
    'src/modes/saved_searches.cc',
    'src/modes/edit_message.cc',
    'src/modes/reply_message.cc',
    'src/modes/forward_message.cc',
    'src/modes/editor/editor.cc',
    'src/modes/editor/external.cc',
    'src/modes/editor/plugin.cc',
    'src/modes/keybindings.cc',
    'src/modes/raw_message.cc',
    'src/modes/help_mode.cc',
    'src/actions/tag_action.cc',
    'src/actions/cmdaction.cc',
    'src/actions/onmessage.cc',
    'src/actions/action_manager.cc',
    'src/actions/difftag_action.cc',
    'src/actions/toggle_action.cc',
    'src/actions/action.cc',
    'src/astroid.cc',
    'src/plugin/manager.cc',
    'src/plugin/astroid_activatable.c',
    'src/plugin/thread_view_activatable.c',
    'src/plugin/thread_index_activatable.c',
    'src/db.cc',
    'src/config.cc',
    'src/poll.cc',
    'src/message_thread.cc',
    'src/utils/resource.cc',
    'src/utils/address.cc',
    'src/utils/gmime/gmime-compat.cc',
    'src/utils/gravatar.cc',
    'src/utils/date_utils.cc',
    'src/utils/ustring_utils.cc',
    'src/utils/cmd.cc',
    'src/utils/utils.cc',
    'src/utils/vector_utils.cc',
    'src/utils/gmime/gtrie.c',
    'src/utils/gmime/gmime-filter-html-bq.c',
    'src/utils/gmime/url-scanner.c',
    'src/crypto.cc',
    'src/account_manager.cc',
    'src/chunk.cc',
    'src/command_bar.cc',
    'src/compose_message.cc',
    'src/main_window.cc',
  ]


## Astroid executable
astroid_exe = executable ('astroid', src + ['src/main.cc'], dependencies : deps, include_directories : include_directories ('src') )

#install_data ('ui/thread-view.html', install_dir : join_paths(get_option('datadir'), 'progname'))

## GIR generation (for plugins)
girsource = [
    'src/plugin/astroid_activatable.c',
    'src/plugin/thread_view_activatable.c',
    'src/plugin/thread_index_activatable.c',
    'src/plugin/astroid_activatable.h',
    'src/plugin/thread_view_activatable.h',
    'src/plugin/thread_index_activatable.h',
    'src/plugin/gir_main.c'
    ]
gir_target  = executable ('gir_main', girsource, dependencies : deps, include_directories : include_directories ('src'), install : false)
gnome = import ('gnome')
gir   = gnome.generate_gir (gir_target, sources : girsource, namespace : 'Astroid', nsversion : '0.1', include_directories : include_directories ('src'), dependencies : deps, install : true, includes : ['GObject-2.0', 'GMime-3.0'])

