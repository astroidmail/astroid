#! /bin/bash

# poll script for astroid

offlineimap || exit $?

notmuch new || exit $?

