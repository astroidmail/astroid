# encoding: utf-8
#
# keeps track of main window and others

require 'gulp/util'

require 'gulp/main_window'

module Gulp
  class Main
    include Singleton

    def initialize

    end

    def Go
      @main_windows = []

      # start Gtk / Gdk loops
      debug "setting up gulp main.."

      ## set up threads and gui class
      debug "setting up Gdk threads and ui.."
      Gdk::Threads.init
      Gtk.init

      # open one @main_window by default
      @main_windows.push MainWindow.new

      ## start main loop
      Gtk.main

      ## fix up threads
      Gdk::Threads.leave
      debug "leaving Gdk threads."

    end


  end
end
