# encoding: utf-8
#
# keeps track of main window and others

module Gulp
  class MainWindow < Gtk::Window
    def initialize
      super

      info "mainwindow started.."
      set_title "Gulp"

      set_default_size 800, 600

      #@notebook = Notebook.new

      #@inbox_mode = ThreadIndex.new "Inbox", { :label => :inbox }
      #@log_mode   = Log.new

      #Redwood::Logger.add_sink @log_mode

      #@default_modes = [@inbox_mode,
                        #@log_mode ]

      #@default_modes.each do |m|
        #@notebook.append_page m, m.tab_widget
      #end

      #add @notebook

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

    def quit
      #Gulp::Logger.remove_sink @log_mode

      Gtk.main_quit
    end

  end
end
