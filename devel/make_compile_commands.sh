#! /usr/bin/bash
#
# this uses bear to make compile_commands.json for for SCons
#
# https://github.com/astroidmail/astroid/issues/14
#
# run from root source directory.

export CXX=clang++
scons -c
bear scons --verbose

