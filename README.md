<img src="https://github.com/astroidmail/astroid/raw/master/ui/icons/horizontal_color.png" width="400px" alt="astroid logo" />

> _Astroid_ is a lightweight and fast **Mail User Agent** that provides a graphical interface to searching, displaying and composing email, organized in threads and tags. _Astroid_ uses the [notmuch](http://notmuchmail.org/) backend for blazingly fast searches through tons of email. _Astroid_ searches, displays and composes emails - and rely on other programs for fetching, syncing and sending email. Check out [Astroid in your general mail setup](https://github.com/astroidmail/astroid/wiki/Astroid-in-your-general-mail-setup) for a suggested complete e-mail solution.

The [tour of how to install, configure and use astroid](https://github.com/astroidmail/astroid/wiki) provides detailed information on setup and usage, while brief instructions are provided [below](#acquiring-astroid).

## Main features
* lightweight and fast!
* fully operable by [keyboard](https://github.com/astroidmail/astroid/wiki/Customizing-key-bindings).
* graphical interface. buffers can be placed in separate windows.
* display html mail and common attachments inline.
* [themable and configurable](https://github.com/astroidmail/astroid/wiki/Customizing-the-user-interface).
* built-in [crypto (PGP/MIME) support](https://github.com/astroidmail/astroid/wiki/Signing%2C-Encrypting-and-Decrypting).
* editors: [embedded or external vim or emacs](https://github.com/astroidmail/astroid/wiki/Customizing-editor) (or your favourite editor).
* [syntax highlighting](https://github.com/astroidmail/astroid-syntax-highlight) of patches and code segments.
* [python and lua plugins](https://github.com/astroidmail/astroid/wiki/Plugins).
* and much more...

  <a href="https://raw.githubusercontent.com/astroidmail/astroid/master/doc/full-demo-embedded.png">
    <img alt="Astroid (with embedded editor)" src="https://raw.githubusercontent.com/astroidmail/astroid/master/doc/full-demo-embedded.png">
  </a>

## Acquiring astroid

Get astroid through git by:

```sh
$ git clone https://github.com/astroidmail/astroid.git
```

## Installation and Usage

### Building

```sh
$ cd astroid
$ cmake -H. -Bbuild -GNinja # to use the ninja backend
$ cmake --build build
```

Run `cmake -DOPTION=VALUE ..` from `build/` to set any build options (list with `cmake -L`). Subsequent builds can be done by running `ninja` (or `make` if you are using that) from the build directory.

And to run the tests do:

```sh
$ cd build
$ ctest
```

### Installing

Configure with a prefix and install:
```sh
$ cmake -H. -Bbuild -GNinja -DCMAKE_INSTALL_PREFIX=/usr/local
$ cmake --build build --target install
```

This will install the `astroid` binary into `/usr/local/bin/`, and data files into `/usr/local/share/astroid/`. Refer to the [installing section](https://github.com/astroidmail/astroid/wiki/Compiling-and-Installing) in the wiki for more information.

### Configuration

The configuration of `astroid` is kept in the directory `$XDG_CONFIG_HOME/astroid` (normally: `~/.config/astroid/`). Refer to the [configuration section](https://github.com/astroidmail/astroid/wiki/Astroid-setup) in the wiki for how to configure astroid. You can use `astroid --new-config` to create a configuration file filled with the default values. If no file exists, the [default values](https://github.com/astroidmail/astroid/blob/master/src/config.cc#L116) are used.

### Execution and Usage

```sh
$ ./build/astroid # to run from source repository
```

Press `?` to get a list of available key bindings in the current mode, navigate up and down using `j` and `k`. Refer to the [usage section](https://github.com/astroidmail/astroid/wiki#usage) in the wiki for more information on usage and customization.

## Patches, Help, Comments and Bugs

Report on the [github page](https://github.com/astroidmail/astroid) or to the mailinglist at: [astroidmail@googlegroups.com](https://groups.google.com/forum/#!forum/astroidmail), subscribe [online](https://groups.google.com/forum/#!forum/astroidmail) or by sending an email to:
[astroidmail+subscribe@googlegroups.com](mailto:astroidmail+subscribe@googlegroups.com).

Contributions to Astroid in the form of patches, documentation and testing are
very welcome. Information on how to
[contribute](https://github.com/astroidmail/astroid/wiki/Contributing) to astroid
can be found in the wiki.

You can usually find us at <a href="irc://irc.freenode.net/#astroid">#astroid</a> ([web](https://webchat.freenode.net/?channels=#astroid)) or <a href="irc://irc.freenode.net/#notmuch">#notmuch</a> ([web](https://webchat.freenode.net/?channels=#notmuch)) at irc.freenode.net.

This project adheres to [Contributor Covenant Code of Conduct v1.4](http://contributor-covenant.org/version/1/4/).

## Acknowledgements

  The main inspiration for astroid is the [sup] mail user agent. [sup]
  provided inspiration for [notmuch] which is a mail indexer. astroid is
  using [notmuch] as a backend.

  Some parts of the user interface and layout has been
  inspired by the [Geary] mail client. Also, some inspiration and code stems from ner (another notmuch email client).

## License

See [LICENSE.md](./LICENSE.md) for licensing information.

[sup]: http://sup-heliotrope.github.io
[notmuch]: http://notmuchmail.org/
[Geary]: http://www.yorba.org/projects/geary/
[gmime]: http://spruce.sourceforge.net/gmime/
[webkit]: http://webkitgtk.org/
[GPL]: https://www.gnu.org/copyleft/gpl.html
[git]: http://git-scm.com/
[C++11]: http://en.wikipedia.org/wiki/C%2B%2B11
[boost]: http://www.boost.org/
[GTK+]: http://www.gtk.org/
[glib]: https://developer.gnome.org/glib/
[boost::property_tree]: http://www.boost.org/doc/libs/1_56_0/doc/html/property_tree.html

