import os, sys
import SCons
from subprocess import *

def getGitDesc():
  return Popen('git describe --abbrev=8 --tags --always', stdout=PIPE, shell=True).stdout.read ().strip ()

env = Environment ()

AddOption ("--release", action="store", dest="release", default="git", help="Make a release (default: git describe output)")
AddOption ("--enable-debug", action="store_true", dest="debug", default=None, help="Enable the -g flag for debugging (default: true when release is git)")
AddOption ("--prefix", action="store", dest="prefix", default=None, help="Directory to install astroid under")

prefix = GetOption ("prefix")
default_prefix = (prefix == None)
prefix = prefix if prefix else '/usr'

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

print "debug flag enabled: " + str(debug)

if 'clean_test' in COMMAND_LINE_TARGETS:
  print "cleaning out tests.."
  for fn in os.listdir('./test/'):
    if '.passed' in fn:
      print "delting: " + fn
      os.remove (os.path.join ('./test', fn))

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

# http://www.scons.org/doc/1.2.0/HTML/scons-user/x4076.html
def check_notmuch_get_revision (ctx):
  ctx.Message ("Checking for C function notmuch_database_get_revision()..")
  result = ctx.TryCompile (nm_db_get_revision_test_src, '.cpp')
  ctx.Result (result)
  return result

conf = Configure(env, custom_tests = { 'CheckPKGConfig' : CheckPKGConfig,
                                       'CheckPKG' : CheckPKG,
                                       'CheckNotmuchGetRev' : check_notmuch_get_revision})


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

if not conf.CheckLibWithHeader ('notmuch', 'notmuch.h', 'c'):
  print "notmuch does not seem to be installed."
  Exit (1)

if conf.CheckNotmuchGetRev ():
  have_get_rev = True
  env.AppendUnique (CPPFLAGS = [ '-DHAVE_NOTMUCH_GET_REV' ])
else:
  have_get_rev = False
  print "notmuch_database_get_revision() not available. notmuch with lastmod capabilities will result in smoother polls."

# external libraries
env.ParseConfig ('pkg-config --libs --cflags glibmm-2.4')
env.ParseConfig ('pkg-config --libs --cflags gtkmm-3.0')
env.ParseConfig ('pkg-config --libs --cflags gmime-2.6')
env.ParseConfig ('pkg-config --libs --cflags webkitgtk-3.0')

if not conf.CheckLib ('boost_filesystem', language = 'c++'):
  print "boost_filesystem does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_system', language = 'c++'):
  print "boost_system does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('boost_program_options', language = 'c++'):
  print "boost_program_options does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('ssl', language = 'c++'):
  print "OpenSSL does not seem to be installed."
  Exit (1)

if not conf.CheckLib ('crypto', language = 'c++'):
  print "OpenSSL (libcrypto) does not seem to be installed."
  Exit (1)

libs   = ['notmuch',
          'boost_filesystem',
          'boost_system',
          'boost_program_options',
          'ssl',
          'crypto',
          'stdc++']

env.AppendUnique (LIBS = libs)
env.AppendUnique (CPPFLAGS = ['-Wall', '-std=c++11', '-pthread'] )

if debug:
  env.AppendUnique (CPPFLAGS = ['-g', '-Wextra'])

## write config file
print ("writing src/build_config.hh..")
vfd = open ('src/build_config.hh', 'w')

vfd.write ("""// this is an automatically generated file. it will be overwritten by\n"""
           """// scons. do not check this file into git.\n""")

vfd.write ("# pragma once\n")
vfd.write ("# define GIT_DESC \"%s\"\n" % GIT_DESC)

# write a prefix only if it is specified or
# we are in release.
if (in_release or not default_prefix):
  vfd.write ("# define PREFIX \"%s\"\n" % prefix)
else:
  vfd.write ("// # define PREFIX\n")

vfd.write ("\n")
vfd.close ()

env.Append (CPPPATH = 'src')
source = (

          Glob('src/*.cc', strings = True) +
          Glob('src/modes/*.cc', strings = True) +
          Glob('src/modes/thread_index/*.cc', strings = True) +
          Glob('src/actions/*.cc', strings = True) +
          Glob('src/utils/*.cc', strings = True)

          )

source.remove ('src/main.cc')
source_objs = [env.Object (s) for s in source]

Export ('source')
Export ('source_objs')
Export ('debug')

env = conf.Finish ()

astroid = env.Program (source = ['src/main.cc', source_objs], target = 'astroid')
build = env.Alias ('build', 'astroid')

Export ('env')

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

inst_bin = env.Install (idir_bin, astroid)
inst_shr = env.Install (idir_ui,  Glob ('ui/*.glade') +
                                  Glob ('ui/*.css') +
                                  Glob ('ui/*.html'))
env.Alias ('install', inst_bin)
env.Alias ('install', inst_shr)
env.Depends (inst_bin, astroid)
env.Depends (inst_shr, astroid)
Ignore ('.', inst_bin)
Ignore ('.', inst_shr)

## Default target
Default (astroid)

