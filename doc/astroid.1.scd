astroid(1)

# NAME

astroid - graphical threads-with-tags style, lightweight and fast, e-mail
client for Notmuch

# SYNOPSIS

*astroid* [options]

# DESCRIPTION

Astroid is a lightweight and fast *Mail User Agent* that provides a graphical
interface to searching, displaying and composing email, organized in threads
and tags. It uses the Notmuch backend for blazingly fast searches through
tons of e-mail.

# OPTIONS

*-c, --config* <config>
	Override the default configuration path. Please refer to the *CONFIGURATION*
	section for details.

*--disable-log*
	Disable logging.

*-h, --help*
	Show the help message and quit.

*--log-level <level>*
	Override the configured log level. Level options are: trace, debug (default),
	info, warn, error and fatal.

*--log-stdout*
	Output logs to the standard output, regardless of any other logging
	configuration.

*-m, --mailto* <dest>
	Open the composition window, sending to the <dest> url or address.

*-n, --new-config*
	Generate a new configuration file and quit.

*--no-auto-poll*
	Disable automatic polling.

*--refresh* <revision>
	Update the user view of a running astroid instance with any changes detected
	in the mail directory since <revision>. You can obtain the current revision
	with *notmuch count --lastmod | cut -f3* (*0* will refresh all
	thread-indexes). --{start,stop}-polling can be used as an *alternative*, but
	not with --refresh.

*--start-polling*
	Make a running astroid instance watch for changes in the mail directory and
	display a polling spinner. One must call --stop-polling at the end of the
	external polling (e.g. by offlineimap) *even if it fails*. --refresh can be
	used as an *alternative*, but not with --{start,stop}-polling.

*--stop-polling*
	Stop the polling initiated by --start-polling and update the user view with
	any changes detected since --start-polling was executed.

*-t, --test-config*
	Use test configuration file. Only makes sense from the source root.

*--disable-plugins*
	Disable all the plugins.

# CONFIGURATION

The configuration file location defaults to $XDG\_CONFIG\_HOME/astroid/config.
The preferred way to configure a new astroid setup is to use --new-config to
generate an initial configuration file, then edit it to match the user's needs.
You will find all the details on Astroid's online documentation:
https://github.com/astroidmail/astroid/wiki. If $NOTMUCH\_CONFIG is set it
takes precedence over the configured value for the notmuch database in the
configuration file.

You can customized the thread view by putting your own HTML and SCSS files in
$XDG\_CONFIG\_HOME/astroid/ui/, as explained on
https://github.com/astroidmail/astroid/wiki/Customizing-the-user-interface.

If $XDG\_CONFIG\_HOME is not set, it defaults to ~/.config.

# NOTE

Astroid is a *Mail User Agent*: all Astroid provides is a graphical interface
to your email (read, view, write, modify, organise). Thus, Astroid enables you
to launch actions that rely on the performance of other programs to actually
fetch, sync, index, search and send your email. It depends on Notmuch for
indexing and searching, but others actions can be handled many different
programs:

- Fetching and syncing your mail between your local computer and email service
provider can be handled by any program supporting the Maildir format:
	* *OfflineIMAP*
	* *mbsync*
	* *Gmailleer*
	* ...

- Sending mail through you email provider can be handler by any sendmail
compatible program:
	* *msmtp*
	* ...

- Editing can be handled by almost any editor:
	* *vim*
	* *emacs*
	* *gedit*
	* ...

While all these programs (or equivalent programs) need to be setup; once
they're configured, you shouldn't need to think about them beyond using
astroid. The configuration options may be more or less tedious and complicated,
depending on what email service provider you use.

# AUTHORS

Maintained by Gaute Hope <eg@gaute.vetsj.com>, who is assisted by other open
source contributors. For more information about astroid development, see
<https://github.com/astroidmail/astroid>.

# SEE ALSO

*notmuch*(1) *offlineimap*(1) *msmtp*(1)
