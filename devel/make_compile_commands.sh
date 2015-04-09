#! /usr/bin/bash
#
# this uses bear to make compile_commands.json for for SCons
#
# https://github.com/gauteh/astroid/issues/14
#
# run from root source directory.

scons -c
bear scons --verbose

