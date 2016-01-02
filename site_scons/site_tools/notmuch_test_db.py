import SCons.Builder
import os
import shutil
from subprocess import Popen

def nmAction(target, source, env):
  '''
  set up notmuch test db in target directory
  '''

  config = os.path.abspath(os.path.join (os.path.curdir, 'test/mail/test_config'))

  env['ENV']['NOTMUCH_CONFIG'] = config

  # run notmuch
  myenv = os.environ.copy()
  myenv['NOTMUCH_CONFIG'] = config

  # remove old db
  print "Remove test/mail/.notmuch.."
  shutil.rmtree ('test/mail/test_mail/.notmuch', ignore_errors = True)

  t = open ('test/mail/test_config.template', 'r')
  o = open ('test/mail/test_config', 'w')

  for l in t.readlines ():
    if l == 'path=\n':
      o.write ("path=" + os.path.abspath (os.path.join (os.path.curdir, 'test/mail/test_mail')) + "\n")
    else:
      o.write (l)

  t.close ()
  o.flush ()
  o.close ()

  p = Popen ("notmuch new", env = myenv, shell = True)
  p.wait ()

  open(str(target[0]),'w').write("SETUP\n")


def nmActionString(target, source, env):
  '''
  Return output string which will be seen when setting up test db
  '''
  return 'Setting up test database in ' + str(source[0])

def generate (env):
  env['BUILDERS']['NotmuchTestDb'] = env.Builder(
      action = env.Action(nmAction, nmActionString),
      suffix='.setup')


class NotmuchNotFound (SCons.Warnings.Warning):
  pass

def _detect (env):
  """ Try to detect notmuch """
  # from http://www.scons.org/wiki/ToolsForFools
  try:
    return env['notmuch']
  except KeyError:
    pass

  nm = env.WhereIs('notmuch')
  if nm:
    return nm

  raise SCons.Errors.StopError(
      NotmuchNotFound,
      "Could not find notmuch binary")
  return None

def exists (env):
  return _detect (env)

