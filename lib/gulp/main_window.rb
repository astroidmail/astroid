# encoding: utf-8
#
# keeps track of main window and others

require 'gulp/modes/log'
require 'gulp/modes/help'
require 'gulp/modes/thread_index'

module Gulp
  class MainWindow < Gtk::Window
    @modes = [] # modes currently showing in the mainwindow notebook

    attr_reader :modes

    def initialize
      super

      info "mainwindow started.."
      set_title "Gulp"

      @modes = []

      set_default_size 800, 600

      @notebook = Notebook.new
      add @notebook

      signal_connect "destroy" do
        quit
      end

      signal_connect "key-press-event" do |widget, event|
        k = Gdk::Keyval.to_name(event.keyval)
        debug "main_window: got: #{k}"
        handled = false
        case k
          when 'q'
            quit
            handled = true
        end

        handled
      end

      show_all
    end

    def add_mode m
      @modes.push m
      @notebook.append_page m, m.tab_widget

      show_all
    end

    def del_mode m
      @modes.delete m
    end

    def quit
      #Gulp::Logger.remove_sink @log_mode

      Gtk.main_quit
    end

    # main notebook
    class Notebook < Gtk::Notebook
      def initialize
        super

      end

    end

  end
end
