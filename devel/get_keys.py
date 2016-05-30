#! /usr/bin/env python
#
# get keybindings from calls to register_key
#
# Author: Gaute Hope <eg@gaute.vetsj.com> / 2016-02-07
# Author: M. Dietrich <mdt@pyneo.org> / 2016-02-24
#
from __future__ import with_statement, print_function
from re import compile as re_compile
import os
import sys

def stripper(s):
	# bad hack to strip cc string " and brackets for arrays
	if s is None:
		return ''
	if s[0] == '"' and s[-1] == '"':
		return s[1:-1].strip()
	if s[0] == '{' and s[-1] == '}':
		return s[1:-1].strip()
	return s

def out_binding(multi, key, function, documentation, ):
	if 'UnboundKey' in key:
		multi = ', no defaults.'
	else:
		key  = stripper(key)
		if multi:
			multi = ', default: ' + key + ', ' + stripper(multi)
		else:
			multi = ', default: ' + key
	print('#', stripper(function) + '=' + key, '#', stripper(documentation) + multi)

def main(*options):
	# pattern to look for:
	pattern = re_compile("((->|\.)register_key *\([^;]*)")
	# quite mode?
	quiet = True
	for option in options:
		if '-d' == option:
			quiet = False
	# detect src root
	src_root = os.path.join(os.path.dirname(__file__), '../src')
	if not quiet:
		sys.stderr.write("source root: %s\n" % src_root)
	# walk through
	for dname, sdirs, flist in os.walk(src_root):
		for fname in flist:
			_, fext = os.path.splitext(fname)
			# consider cc source
			if fext == '.cc':
				fname = os.path.join(dname, fname)
				if not quiet:
					sys.stderr.write('checking: %s\n' % fname)
				# read whole file
				with open(fname, 'r') as fd:
					txt = fd.read()
				# unify
				txt = txt.replace('\n', ' ')
				txt = txt.replace('\t', ' ')
				# seach pattern
				for group in pattern.findall(txt):
					# only the pattern, not the inbetween
					group, _ = group
					# split args
					args = group.split(', ')
					# strip all whitespaces
					args = list([n.strip() for n in args])
					# cut away function call in the beginning
					args[0] = args[0].split('(',1)[1]
					# detect args in first arg
					if '(' in args[0]:
						while not ')' in args[0]:
							args[0] += ', ' + args.pop(1)
					# detect if mode 1 or 2 of method is used
					if args[1][0] == '"':
						# clear thing, 3-part 1:1 mapping
						out_binding(None, *args[:3])
					elif args[1][0] == '{':
						# multi mapping
						multi = args.pop(1)
						while not '}' in multi:
							multi += ', ' + args.pop(1)
						out_binding(multi, *args[:3])
					else:
						# undetected second param
						print('# not detected', args)

if __name__ == '__main__':
	main(*sys.argv[1:])
