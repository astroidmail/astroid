#!/usr/bin/env bash
#
# poll.sh - An example poll script for astroid.
#
# Intended to be run by astroid. See the following link for more information:
# https://github.com/astroidmail/astroid/wiki/Polling
#

# Exit as soon as one of the commands fail.
set -e


# Fetch new mail.
offlineimap

# Import new mail into the notmuch database.
notmuch new


# Here follow some examples of processing the new mail. The respective section
# of code needs to be uncommented to be enabled. See also notmuch-hooks(5) for
# an alternative place to put these commands.


# Automatic tagging of new mail. See notmuch-tag(1) for details.

#notmuch tag --batch <<EOF
#
# # Tag urgent mail
# +urgent subject:URGENT
#
# # Tag all mail from GitHub as "work"
# +work from:github
#
#EOF


# Notify user of new mail. See the following link for more information:
# https://github.com/astroidmail/astroid/wiki/Desktop-notification

#notifymuch

