# astroid mua

  A graphical threads-with-tags mail user agent based on [sup] and [notmuch].
  Written in C++ using [GTK+], [WebKit] and [gmime].

  <a href="http://gaute.vetsj.com/pages/astroid.html">
    <img src="https://raw.githubusercontent.com/gauteh/astroid/master/doc/astroid-full-window.png">
  </a>

## design and user interface goals
* (keyboard)    fully operatable by keyboard only - but accept mouse clicks
*               lightweight.
* (partly done) base interface on sup, but allow buffers to be dragged out
                or separated as windows so that multiple views/buffers can be
                seen at one time.
* (done)        allow several simultaneous windows.
* (done)        display html mail and some attachments inline.
* (done)        render math using MathJax
* (partly done) syntax highlighting between triple-backtick tags (markdown style)
* (not done)    built-in crypto (gpg,..) support.
* (only vim)    editors: embed vim or emacs (possibly ship a simple editor)
* (linux)       Support: Platforms supported by notmuch and other libraries, specifically:
                         Linux, *BSD, Mac, Windows..

### considerations
  * Never use deprecated libraries - use as few libraries as possible.
  * Never rewrite something if it exists an active library for it.
  * All database operations / mail handling should be done by notmuch or other
    tools.

## acquiring astroid

get astroid from:

` $ git clone https://github.com/gauteh/astroid.git `

[Instructions](#installation-and-usage) on how to [build](#compiling), [configure](#configuration) and [run astroid](#running-and-usage) can be found in this README. Once you get astroid running, press '?' to get a list of key bindings.

  [Distribution specific instructions](https://github.com/gauteh/astroid/wiki) can be found in the wiki.

## licence and attribution

  GNU [GPL] v3 or later:

    Copyright (c) 2014  Gaute Hope <eg@gaute.vetsj.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

## installation and usage

astroid uses [scons] for building, also you might need [git] for the build
process to work properly. Both should be available in most distributions.

A fairly recent version of [GTK+] and [glib] with their
[C++ bindings](http://www.gtkmm.org/en/) are also required, along with
[boost], [gmime] and a compiler that supports [C++11]. Of course, the
[notmuch] libraries are also required.

### compiling

` $ scons `

to run the tests do:

` $ scons test `

### installing

Configure with a prefix and install:
```
$ scons --prefix=/usr build
$ scons --prefix=/usr install
```

this will install the `astroid` binary into `/usr/bin/` and data files into `/usr/share/astroid/`.

### configuration

astroid uses the `$XDG_CONFIG_HOME/astroid` directory (or `$HOME/.config/astroid` if it is not set) for storing its configuration file. When you first run astroid it will set up the default configuration file there. This is a JSON file created by [boost::property_tree]. Options not set here will be set to their default values as specified in [`src/config.cc`](https://github.com/gauteh/astroid/blob/master/src/config.cc#L78).

By default astroid looks in `$HOME/.mail` for the notmuch database, but you can change this in the configuration file. You can also set up default queries and accounts for sending e-mail there.

you can run:

` $ astroid --new-config `

to create a new configuration file in the default location, you can also specify a location of the new config file with the `-c` argument.

### running and usage

` $ ./astroid `

press `?` to get a list of available key bindings in the current mode, navigate up and down using `j` and `k`. The list is updated depending on the mode you are in.

## patches, help, comments and bugs

Report on the [github page](https://github.com/gauteh/astroid) or to the mailinglist at: [astroidmail@googlegroups.com](https://groups.google.com/forum/#!forum/astroidmail), subscribe [online](https://groups.google.com/forum/#!forum/astroidmail) or by sending an email to:
[astroidmail+subscribe@googlegroups.com](mailto:astroidmail+subscribe@googlegroups.com).

Contributions to Astroid in the form of patches, documentation and testing are
very welcome. Information on how to
[contribute](https://github.com/gauteh/astroid/wiki/Contributing) to astroid
can be found in the wiki.

Also check out #astroid or #notmuch on irc.freenode.net.

## acknowledgements

  The main inspiration for astroid is the [sup] mail user agent. [sup]
  provided inspiration for [notmuch] which is a mail indexer. astroid is
  using [notmuch] as a backend.

  Some parts of the user interface and layout have been copied from or has been
  inspired by the [Geary] mail client. One of the other notmuch email clients,
  [ner], has also used for inspiration and code.

[sup]: http://supmua.org
[notmuch]: http://notmuchmail.org/
[Geary]: http://www.yorba.org/projects/geary/
[gmime]: http://spruce.sourceforge.net/gmime/
[webkit]: http://webkitgtk.org/
[GPL]: https://www.gnu.org/copyleft/gpl.html
[scons]: http://www.scons.org/
[git]: http://git-scm.com/
[C++11]: http://en.wikipedia.org/wiki/C%2B%2B11
[boost]: http://www.boost.org/
[GTK+]: http://www.gtk.org/
[glib]: https://developer.gnome.org/glib/
[boost::property_tree]: http://www.boost.org/doc/libs/1_56_0/doc/html/property_tree.html
[ner]: http://the-ner.org/

