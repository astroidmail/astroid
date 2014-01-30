# encoding: utf-8


module Gulp
  BASE_DIR   = ENV["GULP_BASE"] || File.join(ENV["HOME"], ".gulp")

  ## record exceptions thrown in threads nicely
  @exceptions = []
  @exception_mutex = Mutex.new

  attr_reader :exceptions
  def record_exception e, name
    @exception_mutex.synchronize do
      @exceptions ||= []
      @exceptions << [e, name]
    end
  end

  def reporting_thread name
    if $opts[:no_threads]
      yield
    else
      ::Thread.new do
        begin
          yield
        rescue Exception => e
          record_exception e, name
        end
      end
    end
  end

  module_function :reporting_thread, :record_exception, :exceptions

  def start

  end

  def finish

  end

  module_function :start, :finish
end

require 'gulp/util'
require 'gulp/logger/singleton'

info "oy gulp!"

$encoding = 'UTF-8' # there is only one - we don't support anything else
info "using encoding: #{$encoding} - there is no choice."

