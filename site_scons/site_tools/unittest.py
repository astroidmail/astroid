import os
import subprocess
import resource
from subprocess import Popen

def unitTestAction(target, source, env):
  '''
  Action for a 'UnitTest' builder object.
  Runs the supplied executable, reporting failure to scons via the test exit
  status.
  When the test succeeds, the file target.passed is created to indicate that
  the test was successful and doesn't need running again unless dependencies
  change.
  '''

  # merge in os environ to get utf-8 or users settings
  myenv = os.environ.copy ()
  for k,v in env['ENV'].items():
    myenv[k] = v

  # add notmuch path
  config = os.path.abspath(os.path.join (os.path.curdir, 'test/mail/test_config'))
  myenv['NOTMUCH_CONFIG'] = config

  # setup gpg
  setupGPG (myenv)

  # set a low number of maximum open files
  resource.setrlimit (resource.RLIMIT_NOFILE, (64, 64));

  app = str(source[0].abspath)
  process = subprocess.Popen (app, shell = True, env = myenv)
  process.wait ()
  ret = process.returncode
  if ret == 0:
    open(str(target[0]),'w').write("PASSED\n")
  else:
    return 1

def setupGPG (env):
  home = os.path.join (os.path.curdir, 'test/test_home')
  gpgh = os.path.abspath(os.path.join (home, 'gnupg'))
  env['GNUPGHOME'] = gpgh
  env['GPG_AGENT_INFO'] = ''

  if not os.path.exists (os.path.join (gpgh, '.ready')):
    print "test: setting up gpg environment in: " + gpgh
    if not os.path.exists (gpgh): os.mkdir (gpgh)

    os.chmod (gpgh, 0700)

    # make some keys
    Popen ("gpg --batch --gen-key ../../foo1.key", env = env, shell = True, cwd = gpgh).wait ()
    Popen ("gpg --batch --gen-key ../../foo2.key", env = env, shell = True, cwd = gpgh).wait ()

    # import
    Popen ("gpg --batch --always-trust --import one.pub", env = env, shell = True, cwd = gpgh).wait ()

    Popen ("gpg --batch --always-trust --import two.pub", env = env, shell = True, cwd = gpgh).wait ()

  open (os.path.join (gpgh, '.ready'), 'w').write ('ready')


def unitTestActionString(target, source, env):
  '''
  Return output string which will be seen when running unit tests.
  '''
  return 'Running tests in ' + str(source[0])

def addUnitTest(env, target=None, source=None, *args, **kwargs):
  '''
  Add a unit test
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
  if source is None:
    source = target
    target = None

  source = [source, env['UTEST_MAIN_SRC']]
  program = env.Program(target, source, *args, **kwargs)
  utest = env.UnitTest(program)
# add alias to run all unit tests.
  env.Alias('test', utest)
# make an alias to run the test in isolation from the rest of the tests.
  env.Alias(str(program[0]), utest)
  return utest

#-------------------------------------------------------------------------------
# Functions used to initialize the unit test tool.
def generate(env, UTEST_MAIN_SRC=[], LIBS=[]):
  env['BUILDERS']['UnitTest'] = env.Builder(
      action = env.Action(unitTestAction, unitTestActionString),
      suffix='.passed')
  env['UTEST_MAIN_SRC'] = UTEST_MAIN_SRC

  env.AppendUnique(LIBS=LIBS)

# The following is a bit of a nasty hack to add a wrapper function for the
# UnitTest builder, see http://www.scons.org/wiki/WrapperFunctions
from SCons.Script.SConscript import SConsEnvironment
SConsEnvironment.addUnitTest = addUnitTest

def exists(env):
  return 1
