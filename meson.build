project ('astroid', ['cpp', 'c'],
    default_options : [ 'cpp_std=c++14'] )

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
threads = dependency ('threads')

notmuch = cc.find_library ('notmuch', required : true)
if cc.has_function ('notmuch_database_index_file', dependencies : notmuch, prefix : '''#include <notmuch.h>''')
  add_global_arguments ('-DHAVE_NOTMUCH_INDEX_FILE', language: 'cpp')
endif

deps = [ gmime, boost, gtk, gtkmm, webkit, threads, notmuch ]

vte = dependency ('vte-2.91')
if not vte.found () or get_option('disable-terminal')
  add_global_arguments ('-DDISABLE_VTE', language: 'cpp')
  message ('Embedded terminal is disabled.')
else
  deps += vte
endif

if get_option ('disable-embedded-editor')
  add_global_arguments ('-DDISABLE_EMBEDDED', language: 'cpp')
endif


if get_option ('disable-libsass')
  sassc = find_program(get_option('scss-compiler'))
  add_global_arguments ('-DDISABLE_LIBSASS', language : 'cpp')
  css = custom_target ('css',
      output : 'thread-view.css',
      input :  'ui/thread-view.scss',
      command : [sassc, '@INPUT@', '@OUTPUT@' ],
      build_by_default : true,
      install : true,
      install_dir : 'ui')
else
  libsass = dependency ('libsass', required : true)
  deps += libsass

  if c.has_header ('sass_context.h')
    add_global_arguments ('-DSASSCTX_SASS_CONTEXT_H', language : 'cpp')
  elif c.has_header  ('sass/context.h')
    add_global_arguments ('-DSASSCTX_CONTEXT_H', language : 'cpp')
  endif
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

plugin_src = [
    'src/plugin/manager.cc',
    'src/plugin/astroid_activatable.c',
    'src/plugin/thread_view_activatable.c',
    'src/plugin/thread_index_activatable.c',
    ]

if get_option ('disable-plugins')
  add_global_arguments ('-DDISABLE_PLUGINS', language: 'cpp')
else
  peas = dependency ('libpeas-1.0')
  deps += peas
  src  += plugin_src
endif

## Astroid executable
astroid_exe = executable ('astroid', src + ['src/main.cc'], dependencies : deps, include_directories : include_directories ('src'), install : true, build_by_default : true )

## GIR generation (for plugins)
if not get_option ('disable-plugins')
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
endif

## Installaing data files
ui_src = [
    'ui/thread-view.html',
    'ui/edit-message.glade',
    'ui/no-mail.png'
    ]

if not get_option ('disable-libsass')
  ui_src += 'ui/thread-view.scss'
endif

install_data (ui_src, install_dir : join_paths(get_option('datadir'), 'astroid', 'ui'))

icons = [
  'ui/icons/horizontal_color.png',
  'ui/icons/horizontal_color.svg',
  'ui/icons/horizontal_white.png',
  'ui/icons/horizontal_white.svg',
  'ui/icons/icon_color.png',
  'ui/icons/icon_color.svg',
  'ui/icons/icon_white.png',
  'ui/icons/icon_white.svg',
  'ui/icons/LICENSE',
  'ui/icons/vertical_color.png',
  'ui/icons/vertical_color.svg',
  'ui/icons/vertical_white.png',
  'ui/icons/vertical_white.svg',
  ]

install_data (icons, install_dir : join_paths(get_option('datadir'), 'astroid', 'ui', 'icons'))

install_data ('ui/icons/astroid.png', install_dir : join_paths(get_option('datadir'), 'icons', 'hicolor', '512x512', 'apps'))
install_data ('ui/icons/astroid.svg', install_dir : join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps'))

install_data ('ui/astroid.desktop', install_dir : join_paths(get_option('datadir'), 'applications'))

## Tests
if not get_option('disable-tests')
  test_setup = custom_target ('test_setup',
      command : [ find_program('tests/test_setup.sh').path(), '@INPUT@', '@OUTPUT@' ],
      input : 'tests',
      output : 'tests',
      install : false)

  test_env = environment ()
  test_env.set ('NOTMUCH_CONFIG', join_paths(meson.build_root (), 'tests', 'mail', 'test_config'))
  test_env.set ('GNUPGHOME', join_paths(meson.build_root (), 'tests', 'test_home', 'gnupg'))

  src_o = astroid_exe.extract_objects (src)

  boost_test = dependency ('boost', modules : [ 'unit_test_framework' ])
  test_deps = deps
  test_deps += boost_test

  tests = [ [ 'generic test',   'test_generic',         [ 'tests/test_generic.cc' ]],
            [ 'compose testing', 'test_composed_message',     [ 'tests/test_composed_message.cc' ]],
            [ 'non existant file', 'test_non_existant_file',  [ 'tests/test_non_existant_file.cc' ]],
            [ 'open db',        'test_open_db',         [ 'tests/test_open_db.cc' ]],
            [ 'convert error',  'test_convert_error',   [ 'tests/test_convert_error.cc' ]],
            [ 'no newline msg', 'test_no_newline_msg',  [ 'tests/test_no_newline_msg.cc' ]],
            [ 'mime message',   'test_mime_message',    [ 'tests/test_mime_message.cc' ]],
            [ 'theme',          'test_theme',           [ 'tests/test_theme.cc' ]],
            [ 'keybindings',    'test_keybindings',     [ 'tests/test_keybindings.cc' ]],
            [ 'bad content-id', 'test_bad_content_id',  [ 'tests/test_bad_content_id.cc' ]],
            [ 'notmuch',        'test_notmuch',         [ 'tests/test_notmuch.cc' ]],
            [ 'notmuch (standalone)', 'test_notmuch_standalone', [ 'tests/test_notmuch_standalone.cc' ]],
            [ 'address',        'test_address',         [ 'tests/test_address.cc' ]],
            [ 'dates',          'test_dates',           [ 'tests/test_dates.cc' ]],
            [ 'crypto',         'test_crypto',          [ 'tests/test_crypto.cc' ]],
          ]

  foreach t : tests
    test (t[0],
        executable (t[1], t[2], objects : src_o, include_directories : include_directories ('src'), dependencies : test_deps, link_depends : test_setup, build_by_default : false),
        env : test_env, workdir : meson.build_root())
  endforeach
endif

