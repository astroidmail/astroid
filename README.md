<img src="https://github.com/astroidmail/astroid/raw/master/ui/icons/horizontal_color.png" width="400px" alt="astroid logo" />

> Astroid is a lightweight and fast **Mail User Agent** that provides a graphical interface to searching, display and composing email, organized in thread and tags. Astroid uses the [notmuch](http://notmuchmail.org/) backend for blazingly fast searches through tons of email. Astroid searches, displays and composes emails - and rely on other programs for fetching, syncing and sending email. Check out [Astroid in your general mail setup](https://github.com/astroidmail/astroid/wiki/Astroid-in-your-general-mail-setup) for a suggested complete  mail solution.

Check out the [tour of how to install, configure and use astroid](https://github.com/astroidmail/astroid/wiki). Brief instructions are provided [below](#acquiring-astroid).

## main features and goals include:
* lightweight and fast!
* fully operatable by [keyboard](https://github.com/astroidmail/astroid/wiki/Customizing-key-bindings).
* graphical interface, inspired by sup. but allow buffers to be separated and placed in several windows.
* display html mail and some attachments inline.
* syntax highlighting of code (markdown style) and patches.
* [themable and configurable](https://github.com/astroidmail/astroid/wiki/Customizing-the-user-interface).
* built-in [crypto (PGP/MIME) support](https://github.com/astroidmail/astroid/wiki/Signing%2C-Encrypting-and-Decrypting).
* editors: [embedded or external vim or emacs](https://github.com/astroidmail/astroid/wiki/Customizing-editor) (or your favourite editor, embedded: must support XEmbed).
* [python and lua plugins](https://github.com/astroidmail/astroid/wiki/Plugins).
* and much more...

  <a href="https://raw.githubusercontent.com/astroidmail/astroid/master/doc/full-demo-external.png">
    <img alt="Astroid (with external editor)" src="https://raw.githubusercontent.com/astroidmail/astroid/master/doc/full-demo-external.png">
  </a>

## acquiring astroid

get astroid from:

```sh
$ git clone https://github.com/astroidmail/astroid.git
```

## installation and usage

### compiling

```sh
$ scons -j 8    # compile up to 8 targets at the same time
```

to run the tests do:

```sh
$ scons test
```

### installing

Configure with a prefix and install:
```sh
$ scons -j 8 --prefix=/usr build
$ scons --prefix=/usr install
```

this will install the `astroid` binary into `/usr/bin/` and data files into `/usr/share/astroid/`. refer to the [installing section](https://github.com/astroidmail/astroid/wiki/Compiling-and-Installing) in the wiki for more information.

### configuration

running astroid will make a new configuration file in `$XDG_CONFIG_HOME/astroid` (normally: `~/.config/astroid/`. refer to the [configuration section](https://github.com/astroidmail/astroid/wiki/Astroid-setup) in the wiki for more information.

### running and usage

```sh
$ ./astroid
```

press `?` to get a list of available key bindings in the current mode, navigate up and down using `j` and `k`. refer to the [usage section](https://github.com/astroidmail/astroid/wiki#usage) in the wiki for more information on usage and customization.

## patches, help, comments and bugs

Report on the [github page](https://github.com/astroidmail/astroid) or to the mailinglist at: [astroidmail@googlegroups.com](https://groups.google.com/forum/#!forum/astroidmail), subscribe [online](https://groups.google.com/forum/#!forum/astroidmail) or by sending an email to:
[astroidmail+subscribe@googlegroups.com](mailto:astroidmail+subscribe@googlegroups.com).

Contributions to Astroid in the form of patches, documentation and testing are
very welcome. Information on how to
[contribute](https://github.com/astroidmail/astroid/wiki/Contributing) to astroid
can be found in the wiki.

You can usually find us at <a href="irc://irc.freenode.net/#astroid">#astroid</a> ([web](https://webchat.freenode.net/?channels=#astroid)) or <a href="irc://irc.freenode.net/#notmuch">#notmuch</a> ([web](https://webchat.freenode.net/?channels=#notmuch)) at irc.freenode.net.

This project adheres to [Contributor Covenant Code of Conduct v1.4](http://contributor-covenant.org/version/1/4/).

## acknowledgements

  The main inspiration for astroid is the [sup] mail user agent. [sup]
  provided inspiration for [notmuch] which is a mail indexer. astroid is
  using [notmuch] as a backend.

  Some parts of the user interface and layout have been copied from or has been
  inspired by the [Geary] mail client. Also, some inspiration and code stems from
  ner, another notmuch email client.

## licensing

See [LICENSE.md](./LICENSE.md) for licensing information.

[sup]: http://sup-heliotrope.github.io
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

