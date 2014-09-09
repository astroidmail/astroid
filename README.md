# astroid mua

  A graphical threads-with-tags mail user agent based on [sup] and [notmuch].
  Written in C++ using GTK+, [WebKit] and [gmime].

## design and user interface goals
* fully operatable by keyboard only - but accept mouse clicks
* lightweight.
* base interface on sup, but allow buffers to be dragged out
  or separated as windows so that multiple views/buffers can be
  seen at one time.
* allow several simultaneous windows.
* display html mail and some attachments inline.
* built-in crypto (gpg,..) support.
* render markdown (and friends) as well as syntax highlighting.
* editors: embed vim or emacs (possibly ship an simple editor)
* Support: Platforms supported by notmuch and other libraries, specifically:
  Linux, *BSD, Mac, Windows..

### considerations
  * Never use deprecated libraries - use as few libraries as possible.
  * Never rewrite something if it exists an active library for it.
  * All database operations / mail handling should be done by notmuch or other
    tools.

## licence

  GNU [GPL] v3 or later:

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

A fairly recent version of [gtk+] and [glib] with their
[C++ bindings](http://www.gtkmm.org/en/) are also required, along with
[boost], [gmime] and a compiler that supports [C++11]. Of course, the
[notmuch] libraries are also required.

## compiling

` $ scons `

## installing

Not yet set up, run from source root directory.

## configuration

astroid uses the `$XDG_CONFIG_HOME/astroid` directory (or `$HOME/.config/astroid` if it is not set) for storing its configuration file. When you first run astroid it will set up the default configuration file there. This is a JSON file created by [boost::property_tree]. Options not set here will be set to their default values as specified in `src/config.cc`.

By default astroid looks in `$HOME/.mail` for the notmuch database, but you can change this in the configuration file. You can also set up default queries and accounts for sending mail there.

## running

` $ ./astroid `

## patches and bugs

Report on the [github page](https://github.com/gauteh/astroid) or to the mailinglist whenever it is has been set up..

## acknowledgements

  The main inspiration for astroid is the [sup] mail user agent. [sup]
  provided inspiration for [notmuch] which is an mail indexer. astroid is
  using [notmuch] as a backend.

  Some parts of the user interface and layout have been copied from or
  been inspired by the [Geary] mail client.

[sup]: http://supmua.org
[alot]: https://github.com/pazz/alot
[notmuch]: http://notmuchmail.org/
[Geary]: http://www.yorba.org/projects/geary/
[gmime]: http://spruce.sourceforge.net/gmime/
[webkit]: http://webkitgtk.org/
[GPL]: https://www.gnu.org/copyleft/gpl.html
[scons]: http://www.scons.org/
[git]: http://git-scm.com/
[C++11]: http://en.wikipedia.org/wiki/C%2B%2B11
[boost]: http://www.boost.org/
[gtk+]: http://www.gtk.org/
[glib]: https://developer.gnome.org/glib/
[boost::property_tree]: http://www.boost.org/doc/libs/1_56_0/doc/html/property_tree.html

