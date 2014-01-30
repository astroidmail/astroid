= cup =

A graphical mail user agent based on sup and notmuch. Written in ruby
utilizing the Mail library, GTK+ and clutter.

== user interface goals ==
* operated by keyboard only - but accept mouse clicks
* base interface on sup, but allow buffers to be dragged out
  or separated as windows so that multiple views/buffers can be
  seen at one time.
* don't lock index: allow several simultaneous instances.
* display html mail and some attachments inline - steal from
  thunderbird or something.

== design goals ==
* Always use the latest Ruby
* Never use deprecated libraries - use as few libraries as possible
* Never rewrite something there exists an active library for
* All database operations / mail handling should be done by notmuch
* If features are missing in notmuch; create a temporary standalone
  program that can easily later be removed.
  specifically thinking about: maildir <-> tag sync.
* Support: Platforms supported by notmuch and other libraries, specifically:
  Linux, *BSD, Mac, Windows, Android..

