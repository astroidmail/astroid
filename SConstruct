import os, sys
from subprocess import *

def getGitDesc():
  return Popen('git describe --abbrev=8 --tags --always', stdout=PIPE, shell=True).stdout.read ().strip ()


GIT_DESC = getGitDesc ()
print "building " + getGitDesc () + ".."
env = Environment ()

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

# external libraries
env.ParseConfig ('pkg-config --libs --cflags glibmm-2.4')
env.ParseConfig ('pkg-config --libs --cflags gtkmm-3.0')
env.ParseConfig ('pkg-config --libs --cflags gmime-2.6')
env.ParseConfig ('pkg-config --libs --cflags webkitgtk-3.0')

libs   = ['notmuch',
          'boost_system',
          'boost_filesystem',
          'boost_program_options',]

env.Append (LIBS = libs)
env.Append (CPPFLAGS = ['-g', '-std=c++11', '-pthread'] )

# write version file
print ("writing src/version.hh..")
vfd = open ('src/version.hh', 'w')
vfd.write ("# pragma once\n")
vfd.write ("# define GIT_DESC \"%s\"\n" % GIT_DESC)
vfd.close ()

env.Append (CPPPATH = 'src')
source = Glob('src/*.cc') + Glob('src/modes/*.cc') + Glob('src/actions/*.cc') + Glob('src/utils/*.cc')

conf = Configure(env)
if not conf.CheckFunc ('gtk_socket_focus_forward'):
  print "gtk_socket_focus_forward is missing, please see: https://bugzilla.gnome.org/show_bug.cgi?id=729248"
  exit (1)

env = conf.Finish ()

env.Program (source = source, target = 'astroid')

