# encoding: utf-8
#
# a mode showing an index of threads in a query (e.g. inbox)

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

