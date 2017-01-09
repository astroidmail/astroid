import os, sys
import SCons
from subprocess import *

def getGitDesc():
  return Popen('git describe --abbrev=8 --tags --always', stdout=PIPE, shell=True).stdout.read ().strip ()

env = Environment (ENV=os.environ)

AddOption ("--release", action="store", dest="release", default="git", help="Make a release (default: git describe output)")
AddOption ("--enable-debug", action="store", dest="debug", default=None, help="Enable the -g flag for debugging (default: true when release is git)")
AddOption ("--prefix", action="store", dest="prefix", default = '/usr/local', help="Directory to install astroid under")

AddOption ("--profiler", action="store_true", dest="profile", default = False,
    help = "Compile with profiling support (-pg)")

AddOption ("--disable-libsass", action='store_true', dest='disable_libsass',
    default = False, help = "Disable libsass and the dependency on libsass, requires a scss compiler")
AddOption ('--scss-compiler', action='store', dest='scss_compiler',
    default = 'sassc', help = 'SCSS compiler to use when not using libsass')

AddOption ("--disable-plugins", action = 'store_true', dest = 'disable_plugins',
    default = False, help = "Disable plugins")

disable_libsass = GetOption ("disable_libsass")
scss = GetOption ('scss_compiler')
profile = GetOption ('profile')

disable_plugins = GetOption ("disable_plugins")

prefix = GetOption ("prefix")

release = GetOption("release")
if release != "git":
  GIT_DESC = release
  print "building release: " + release
  in_release = True
else:
  GIT_DESC = getGitDesc ()
  print "building version " + GIT_DESC + " (git).."
  in_release = False

debug = GetOption("debug")
if debug == None:
  debug = (release == "git")

else:
  debug = (debug.lower () == 'yes' or debug.lower () == 'true')

print "debug flag enabled: " + str(debug)

if 'clean_test' in COMMAND_LINE_TARGETS:
  print "cleaning out tests.."
  for fn in os.listdir('./test/'):
    if '.passed' in fn:
      print "delting: " + fn
      os.remove (os.path.join ('./test', fn))

  for fn in os.listdir('./test/mail/'):
    if '.passed' in fn or '.setup' in fn:
      print "delting: " + fn
      os.remove (os.path.join ('./test/mail/', fn))

  # clean notmuch dir
  print "cleaning out notmuch dir.."
  import shutil
  if os.path.exists ("./test/mail/test_mail/.notmuch"):
    shutil.rmtree ("./test/mail/test_mail/.notmuch")

  # remove gpg
  print "cleaning out gnupg dir.."
  if os.path.exists ("./test/test_home/gnupg"):
    shutil.rmtree ("./test/test_home/gnupg")

  exit ()

# Verbose / Non-verbose output{{{
colors = {}
colors['cyan']   = '\033[96m'
colors['purple'] = '\033[95m'
colors['blue']   = '\033[94m'
colors['green']  = '\033[92m'
colors['yellow'] = '\033[93m'
colors['red']    = '\033[91m'
colors['end']    = '\033[0m'

#If the output is not a terminal, remove the colors
if not sys.stdout.isatty():
   for key, value in colors.iteritems():
      colors[key] = ''

compile_source_message = '%scompiling %s==> %s$SOURCE%s' % \
   (colors['blue'], colors['purple'], colors['yellow'], colors['end'])

compile_shared_source_message = '%scompiling shared %s==> %s$SOURCE%s' % \
   (colors['blue'], colors['purple'], colors['yellow'], colors['end'])

link_program_message = '%slinking Program %s==> %s$TARGET%s' % \
   (colors['red'], colors['purple'], colors['yellow'], colors['end'])

link_library_message = '%slinking Static Library %s==> %s$TARGET%s' % \
   (colors['red'], colors['purple'], colors['yellow'], colors['end'])

ranlib_library_message = '%sranlib Library %s==> %s$TARGET%s' % \
   (colors['red'], colors['purple'], colors['yellow'], colors['end'])

link_shared_library_message = '%slinking Shared Library %s==> %s$TARGET%s' % \
   (colors['red'], colors['purple'], colors['yellow'], colors['end'])

java_compile_source_message = '%scompiling %s==> %s$SOURCE%s' % \
   (colors['blue'], colors['purple'], colors['yellow'], colors['end'])

java_library_message = '%screating Java Archive %s==> %s$TARGET%s' % \
   (colors['red'], colors['purple'], colors['yellow'], colors['end'])

AddOption("--verbose",action="store_true", dest="verbose_flag",default=False,help="verbose output")
if not GetOption("verbose_flag"):
  env["CXXCOMSTR"] = compile_source_message,
  env["CCCOMSTR"] = compile_source_message,
  env["SHCCCOMSTR"] = compile_shared_source_message,
  env["SHCXXCOMSTR"] = compile_shared_source_message,
  env["ARCOMSTR"] = link_library_message,
  env["RANLIBCOMSTR"] = ranlib_library_message,
  env["SHLINKCOMSTR"] = link_shared_library_message,
  env["LINKCOMSTR"] = link_program_message,
  env["JARCOMSTR"] = java_library_message,
  env["JAVACCOMSTR"] = java_compile_source_message,

# Verbose }}}

# set up compiler and linker from env variables
if os.environ.has_key('CC'):
  env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
  env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CXX'):
  env['CXX'] = os.environ['CXX']
if os.environ.has_key('CXXFLAGS'):
  env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if os.environ.has_key('CPPFLAGS'):
  env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CPPFLAGS'])
if os.environ.has_key('LDFLAGS'):
  env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

def CheckPKGConfig(context, version):
  context.Message( 'Checking for pkg-config... ' )
  ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
  context.Result( ret )
  return ret

def CheckPKG(context, name):
  context.Message( 'Checking for %s... ' % name )
  ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
  context.Result( ret )
  return ret

nm_db_get_revision_test_src = """
# include <notmuch.h>

int main (int argc, char ** argv)
{
  notmuch_database_t * nm_db;
  const char * uuid;
  notmuch_database_get_revision (nm_db, &uuid);

  return 0;
}
"""

nm_query_threads_st = """
# include <notmuch.h>

int main (int argc, char ** argv)
{
  notmuch_query_t * q;
  notmuch_threads_t * t;
  notmuch_status_t st;

  st = notmuch_query_search_threads_st (q, &t);

  return 0;
}
"""

nm_query_count_threads_st = """
# include <notmuch.h>

int main (int argc, char ** argv)
{
  notmuch_query_t * q;
  unsigned int c;
  notmuch_status_t st;

  st = notmuch_query_count_threads_st (q, &c);

  return 0;
}
"""

# http://www.scons.org/doc/1.2.0/HTML/scons-user/x4076.html
def check_notmuch (ctx, title, src):
  ctx.Message ("Checking for C function %s.." % title)
  result = ctx.TryCompile (src, '.cpp')
  ctx.Result (result)
  return result

conf = Configure(env, custom_tests = { 'CheckPKGConfig' : CheckPKGConfig,
                                       'CheckPKG' : CheckPKG,
                                       'CheckNotmuch' : check_notmuch})


if not conf.CheckPKGConfig('0.15.0'):
  print 'pkg-config >= 0.15.0 not found.'
  Exit(1)

if not conf.CheckPKG('gtkmm-3.0 >= 3.10'):
  print 'gtkmm-3.0 >= 3.10 not found.'
  Exit(1)

if not conf.CheckPKG('glibmm-2.4'):
  print "glibmm-2.4 not found."
  Exit (1)

if not conf.CheckPKG('gmime-2.6 >= 2.6.18'):
  print "gmime-2.6 not found."
  Exit (1)

if not conf.CheckPKG('webkitgtk-3.0'):
  print "webkitgtk not found."
  Exit (1)

if not disable_libsass:
  if conf.CheckLibWithHeader ('libsass', 'sass_context.h', 'c'):
    env.AppendUnique (CPPFLAGS = [ '-DSASSCTX_SASS_CONTEXT_H' ])
  elif conf.CheckLibWithHeader ('libsass', 'sass/context.h', 'c'):
    env.AppendUnique (CPPFLAGS = [ '-DSASSCTX_CONTEXT_H' ])
  else:
    print "libsass must be installed: could not find header file. you can disable libsass with --disable-libsass, however, that requires a SCSS compiler like 'sassc'."
    Exit (1)

else:

  print "warning: libsass is disabled, will generate SCSS at build time using: '%s'.." % scss
  env.AppendUnique (CPPFLAGS = [ '-DDISABLE_LIBSASS' ])

  scssbld = Builder (action = '%s $SOURCE $TARGET' % scss)
  env.Append (BUILDERS = { 'Css': scssbld })

  css = env.Css ('ui/thread-view.css', 'ui/thread-view.scss')

if disable_plugins:
  print "warning: plugins are disabled."
  env.AppendUnique (CPPFLAGS = [ '-DDISABLE_PLUGINS' ])

else:
  if not conf.CheckPKG('gobject-introspection-1.0'):
    print 'gobject-introspection-1.0 not found.'
    Exit (1)
  else:
    env.ParseConfig ('pkg-config --libs --cflags gobject-introspection-1.0')

  if not conf.CheckPKG('libpeas-1.0'):
    print 'libpeas-1.0 not found.'
    Exit (1)
  else:
    env.ParseConfig ('pkg-config --libs --cflags libpeas-1.0')

if not conf.CheckLibWithHeader ('notmuch', 'notmuch.h', 'c'):
  print "notmuch does not seem to be installed."
  Exit (1)

if conf.CheckNotmuch ('notmuch_database_get_revision',
                      nm_db_get_revision_test_src):
  have_get_rev = True
  env.AppendUnique (CPPFLAGS = [ '-DHAVE_NOTMUCH_GET_REV' ])
else:
  have_get_rev = False
  print "notmuch_database_get_revision() not available. A recent notmuch with lastmod capabilities will result in easier updating of new threads and smoother polls."

if conf.CheckNotmuch ('notmuch_query_search_threads_st',
                      nm_query_threads_st):
  env.AppendUnique (CPPFLAGS = [ '-DHAVE_QUERY_THREADS_ST' ])
else:
  print "notmuch_query_*_st status versions are not available, some error checking is not possible - and tests will fail. consider upgrading notmuch to a version equal or later than 0.21."

if conf.CheckNotmuch ('notmuch_query_count_threads_st',
                      nm_query_count_threads_st):
  env.AppendUnique (CPPFLAGS = [ '-DHAVE_QUERY_COUNT_THREADS_ST' ])
else:
  print "notmuch_query_*_count__st status versions are not available, some error checking is not possible - and tests will fail. consider upgrading notmuch to a version equal or later than 0.21."

# external libraries
env.ParseConfig ('pkg-config --libs --cflags glibmm-2.4')
env.ParseConfig ('pkg-config --libs --cflags gtkmm-3.0')
env.ParseConfig ('pkg-config --libs --cflags gmime-2.6')
env.ParseConfig ('pkg-config --libs --cflags webkitgtk-3.0')
env.ParseConfig ('pkg-config --libs --cflags libsass')

if not conf.CheckLib ('boost_filesystem', language = 'c++'):
  print "boost_filesystem does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_system', language = 'c++'):
  print "boost_system does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_program_options', language = 'c++'):
  print "boost_program_options does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_log_setup', language = 'c++'):
  print "boost_log_setup does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_log', language = 'c++'):
  print "boost_log does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_thread', language = 'c++'):
  print "boost_thread does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_date_time', language = 'c++'):
  print "boost_date_time does not seem to be installed."
  Exit (1)

libs   = ['notmuch',
          'boost_filesystem',
          'boost_system',
          'boost_program_options',
          'boost_log_setup',
          'boost_log',
          'boost_thread',
          'boost_date_time',
          'stdc++']

env.AppendUnique (LIBS = libs)
env.AppendUnique (CPPFLAGS = ['-Wall', '-std=c++11', '-pthread', '-DBOOST_LOG_DYN_LINK'] )

if debug:
  env.AppendUnique (CPPFLAGS = ['-g', '-Wextra', '-DDEBUG'])

if profile:
  print ("profiling enabled.")
  env.AppendUnique (CPPFLAGS = ['-pg'])
  env.AppendUnique (LINKFLAGS = ['-pg'])

env = conf.Finish ()

## write config file
print ("writing src/build_config.hh..")
vfd = open ('src/build_config.hh', 'w')

vfd.write ("""// this is an automatically generated file. it will be overwritten by\n"""
           """// scons. do not check this file into git.\n""")

vfd.write ("# pragma once\n")
vfd.write ("# define GIT_DESC \"%s\"\n" % GIT_DESC)
vfd.write ("# define PREFIX \"%s\"\n" % prefix)

vfd.write ("\n")
vfd.close ()

env.Append (CPPPATH = 'src')
source = (
          Glob('src/*.cc', strings = True) +
          Glob('src/modes/*.cc', strings = True) +
          Glob('src/modes/thread_index/*.cc', strings = True) +
          Glob('src/modes/thread_view/*.cc', strings = True) +
          Glob('src/modes/editor/*.cc', strings = True) +
          Glob('src/actions/*.cc', strings = True) +
          Glob('src/utils/*.cc', strings = True)
          )

if not disable_plugins:
  source += Glob('src/plugin/*.cc', strings = True)

source.remove ('src/main.cc')

cenv = env.Clone ()
cenv['CPPFLAGS'].remove ('-std=c++11')

csource = (Glob('src/utils/gmime/*.c', strings = True))

source_objs =   [env.Object (s) for s in source]
source_objs +=  [cenv.Object (s) for s in csource]

## GIR for libpeas plugins
def add_gobject_introspection(env, gi_name, version,
                              sources, includepaths, program,
                              pkgs, includes):

  # borrowed from:
  # http://fossies.org/linux/privat/mypaint-1.2.0.tar.gz/mypaint-1.2.0/brushlib/SConscript?m=t
  pkgs = ' '.join('--pkg=%s' % dep for dep in pkgs)
  program = program[0] # there should be only one Node in the list

  # Strip the library path to get the library name
  prgname = os.path.basename (program.get_path ())
  prgname = os.path.splitext (prgname)[0]

  includeflags = ' '.join(['-I%s' % s for s in includepaths])
  gi_includes = ' '.join(['--include=%s' % s for s in includes])

  scanner_cmd = """LD_LIBRARY_PATH=./ g-ir-scanner -o $TARGET --warn-all \
      --namespace=%(gi_name)s  --include=GObject-2.0 --include=GMime-2.6 --nsversion=%(version)s \
      %(pkgs)s %(includeflags)s  \
      --program=./%(prgname)s $SOURCES""" % locals()

  gir_file = env.Command("%s-%s.gir" % (gi_name, version), sources, scanner_cmd)
  env.Depends(gir_file, program)
  typelib_file = env.Command("%s-%s.typelib" % (gi_name, version), gir_file,
                         "g-ir-compiler -o $TARGET $SOURCE")

  return (gir_file, typelib_file)


if not disable_plugins:
  girsource = (Glob ('src/plugin/*.c', strings = True))
  girm      = cenv.Program (source = girsource, target = 'girmain')

  source_objs +=  [cenv.Object (s) for s in girsource if s != 'src/plugin/gir_main.c']

  ## generate GIR and typelib
  gir, typelib = add_gobject_introspection (env, "Astroid", "0.1",
     girsource + Glob('src/plugin/*.h', strings = True), ['src/'], girm, ['gobject-introspection-1.0', 'gmime-2.6'], [])



Export ('source')
Export ('source_objs')
Export ('debug')

astroid = env.Program (source = ['src/main.cc', source_objs], target = 'astroid')
build = env.Alias ('build', 'astroid')

if disable_libsass:
  env.Depends ('astroid', css)

if not disable_plugins:
  env.Depends ('astroid', girm)
  env.Depends ('astroid', gir)
  env.Depends ('astroid', typelib)

Export ('env')
Export ('astroid')

## tests
# http://drowcode.blogspot.no/2008/12/few-days-ago-i-decided-i-wanted-to-use.html
testEnv = env.Clone()
testEnv.Append (CPPPATH = '../src')
testEnv.Tool ('unittest',
              toolpath=['test/bt/'],
              UTEST_MAIN_SRC=File('test/bt/boostautotestmain.cc'),
              LIBS=['boost_unit_test_framework']
)

Export ('testEnv')
# grab stuff from sub-directories.
env.SConscript(dirs = ['test'])

## Install target
idir_prefix     = prefix
idir_bin        = os.path.join (prefix, 'bin')
idir_shr        = os.path.join (prefix, 'share/astroid')
idir_ui         = os.path.join (idir_shr, 'ui')
idir_app        = os.path.join (prefix, 'share/applications')

inst_bin = env.Install (idir_bin, astroid)
inst_shr = env.Install (idir_ui,  Glob ('ui/*.glade') +
                                  Glob ('ui/*.png') +
                                  Glob ('ui/*.html'))

if not disable_plugins:
  inst_shr += env.Install (os.path.join (prefix, 'share/gir-1.0'), gir)
  inst_bin += env.Install (os.path.join (prefix, 'lib/girepository-1.0'), typelib)

if disable_libsass:
  inst_shr += env.Install (idir_ui, Glob ('ui/*.css'))
else:
  inst_shr += env.Install (idir_ui, Glob ('ui/*.scss'))

inst_app = env.Install (idir_app, 'ui/astroid.desktop')

env.Alias ('install', inst_bin)
env.Alias ('install', inst_shr)
env.Alias ('install', inst_app)

env.Depends (inst_bin, astroid)
env.Depends (inst_shr, astroid)
env.Depends (inst_app, astroid)
Ignore ('.', inst_bin)
Ignore ('.', inst_shr)
Ignore ('.', inst_app)

## Default target
Default (astroid)

