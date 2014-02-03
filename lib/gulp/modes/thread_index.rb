# encoding: utf-8
#
# a mode showing an index of threads in a query (e.g. inbox)

require 'gulp/modes/thread_index_list_store'
require 'gulp/modes/thread_index_list_view'

module Gulp
  class ThreadIndex < Gtk::Box

    attr_accessor :tab_widget

    def initialize query
      super :vertical

      # set up query
      @query = Db.db.query(query)

      @tab_widget = Gtk::Label.new
      self.name = "index: #{@query.to_s} (#{@query.count_messages})"

      debug "#{to_s}: estimated #{@query.count_messages} messages in current query."

      @list_store = ThreadIndexListStore.new

      @query.search_threads.each do |t|
        i = @list_store.append
        i[0] = t.to_s
        i[1] = t
        #@list_store.set_value i, 0, "#{m.to_s}"
      end

      @list_view = ThreadIndexListView.new @list_store
      pack_start @list_view

    end


    def name= n
      @name = n
      @tab_widget.set_text n
    end

    def name
      @name
    end

    def to_s
      "ThreadIndex: #{name}"
    end

  end
end

