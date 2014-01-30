# encoding: utf-8
#
# takes care of the interface to notmuch

require 'notmuch'

module Gulp
  class Db
    include Singleton

    @db = nil

    attr_accessor :db

    def initialize
      debug "setting up notmuch db.."

      begin
        @db = Notmuch::Database.new File.join(ENV['HOME'], ".mail")
      rescue Notmuch::FileError => e
        # try to create
        error "no database could be found. please set notmuch up in your root mail folder: ~/.mail"
        die
      end

      debug "notmuch db open, version: #{@db.version}"
      if @db.needs_upgrade?
        info "notmuch db needs upgrade.."
        @db.upgrade! do |progress|
          progress = progress * 100
          debug "upgrading: #{progress}%.."
        end
      end

      info "there's an estimated #{total_count} messages in the database"
    end

    def total_count
      @db.query('').count_messages
    end

    def close
      info "closing notmuch db.."
      @db.close if @db
    end

  end
end
