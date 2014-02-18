# astroid mua

  A graphical threads-with-tags mail user agent based on [sup] and [notmuch].
  Written in C++ using GTK+ and WebKit.

## user interface goals
* operated by keyboard only - but accept mouse clicks
* very very lightweight - but scratch the itch!
* base interface on sup, but allow buffers to be dragged out
  or separated as windows so that multiple views/buffers can be
  seen at one time.
* don't lock index: allow several simultaneous instances.
* display html mail and some attachments inline - steal from
  thunderbird or geary or something.

## design goals
* Never use deprecated libraries - use as few libraries as possible
* Never rewrite something there exists an active library for
* All database operations / mail handling should be done by notmuch
* If features are missing in notmuch; create a temporary standalone
  program that can easily later be removed.
  specifically thinking about: maildir <-> tag sync.
* Support: Platforms supported by notmuch and other libraries, specifically:
  Linux, *BSD, Mac, Windows..

## acknowledgements

  The main inspiration for astroid is the [sup] mail user agent. [sup] provided
  inspiration for [notmuch] which clearly separates the mail index from the
  user interface. astroid is a therefore a graphical front-end for [notmuch].
  Additional inspiration for the user interface has been gathered from the
  [Geary] mail suite.

## licence

MIT

[sup]: http://supmua.org
[notmuch]: http://notmuchmail.org/
[Geary]: http://www.yorba.org/projects/geary/

