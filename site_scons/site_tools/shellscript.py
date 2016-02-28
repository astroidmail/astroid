import SCons.Builder
import os
import shutil
from subprocess import Popen

def shAction(target, source, env):
  '''
  run shell script
  '''
  env['ENV']['NOTMUCH_CONFIG'] = 'mail/test_config'

  # run notmuch
  myenv = os.environ.copy()
  myenv['NOTMUCH_CONFIG'] = 'test/mail/test_config'

  process = Popen (str(source[0]), env = myenv, shell = True)

  process.wait ()
  ret = process.returncode
  if ret == 0:
    open(str(target[0]),'w').write("PASSED\n")
  else:
    return 1


def shActionString(target, source, env):
  '''
  Return output string which will be seen when setting up test db
  '''
  return 'Running: ' + str(source[0])

def generate (env):
  env['BUILDERS']['ShellScript'] = env.Builder(
      action = env.Action(shAction, shActionString),
      suffix='.passed')

def addSh (env, target=None, *args, **kwargs):
  '''
  Add a shell script test
  Parameters:
         target - If the target parameter is present, it is the name of the test
                         executable
         source - list of source files to create the test executable.
         any additional parameters are passed along directly to env.Program().
  Returns:
         The scons node for the unit test.
  Any additional files listed in the env['UTEST_MAIN_SRC'] build variable are
  also included in the source list.
  All tests added with addUnitTest can be run with the test alias:
         "scons test"
  Any test can be run in isolation from other tests, using the name of the
  test executable provided in the target parameter:
         "scons target"
  '''

  utest = env.ShellScript (target)

# add alias to run all unit tests.
  env.Alias('test', utest)
# make an alias to run the test in isolation from the rest of the tests.
  env.Alias(str(target), utest)

  return utest

from SCons.Script.SConscript import SConsEnvironment
SConsEnvironment.addSh = addSh

def exists (env):
  return true

