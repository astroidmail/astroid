#! /usr/bin/env python
#
# get keybindings from calls to register_key
#
# Author: Gaute Hope <eg@gaute.vetsj.com> / 2016-02-07
#

import os, sys

quiet = False
if '-q' in sys.argv:
  quiet = True

src_root = os.path.join (os.path.dirname(__file__), '../src')
if not quiet: sys.stderr.write ("source root: %s\n" % src_root)

def check_register_key (fname):
  global pos
  header_printed = False
  with open (fname, 'r') as fd:
    txt = fd.read ()

    pos = 0
    search_len = 1024

    def find_next ():
      global pos
      a = txt.find ('.register_key', pos)
      if a == -1:
        a = txt.find ('->register_key', pos)
        pos = a + len('->register_key')
      else:
        pos = a + len('.register_key')

      return a

    for match in iter (find_next, -1):
      test_str = txt[pos:pos+search_len]

      end = test_str.find ('(Key', 2)
      if end == -1:
        end = test_str.find ('bind')

      if end == -1:
        print ("could not find end!")
        print (test_str)
        sys.exit (1)

      test_str = test_str[:end]

      ## spec
      name = ''
      desc = ''
      main_key = ''
      alternates = ''

      ## check if UnboundKey
      if 'UnboundKey' in test_str:
        main_key = 'UnboundKey'
        k = 'UnboundKey (), "'
        test_str = test_str[test_str.find (k) + len(k):]

        name = test_str[:test_str.find ("\"")]

        test_str = test_str[len(name)+1:]
        df = test_str.find ("\"")
        desc = test_str[df +1:test_str.find ("\"", df +1)]

      else:
        # print (test_str)
        test_str = test_str.strip ()
        main_key = test_str[1:test_str.find (",")]
        test_str = test_str[len(main_key) +2:]
        main_key = main_key.replace ("\"", "")

        alt = test_str[:test_str.find (',')]
        if '{' in alt:
          # print (alt)
          alt = test_str[:test_str.find ('}') +1]
          test_str = test_str[len(alt) + 1:]
          alternates = alt.strip ()
          # alternates = alternates.replace ("\n", " ")
          alternates = " ".join ([a.strip () for a in alternates.split ("\n")])

          name = test_str[:test_str.find (",")]
          test_str = test_str[len(name) + 1:]
          name = name.strip ().replace ("\"", "")

        else:
          test_str = test_str[len(alt) + 1:]
          name = alt.strip ().replace ("\"", "")

        desc = test_str[:test_str.find (",")].strip ().replace ("\"", "")

      if not header_printed:
        header_printed = True
        print ("## keys from: %s" % fname)

      if alternates:
        print (name + "=" + main_key + "\t # " + desc + ", alternates " + alternates)
      else:
        print (name + "=" + main_key + "\t # " + desc)

    if header_printed:
      print ("")

for dname, sdirs, flist in os.walk (src_root):
  for fname in flist:
    _, fext = os.path.splitext (fname)
    if fext == '.cc':
      fname = os.path.join (dname, fname)
      if not quiet: sys.stderr.write ('checking: %s\n' % fname)
      check_register_key (fname)

