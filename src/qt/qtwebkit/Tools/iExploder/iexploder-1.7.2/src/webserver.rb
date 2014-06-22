#!/usr/bin/ruby
# iExploder Web Server (using webrick)
#
# Copyright 2010 Thomas Stromberg - All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

require 'cgi'
require 'webrick'
require 'optparse'
require './iexploder.rb'

include WEBrick

$INSTANCE = nil
$last_page_requested = [Time.now().to_i, 0]

# Main CGI - Pass requests to iexploder
class IEServlet < HTTPServlet::AbstractServlet
  def do_GET(request, response)
    ie = $INSTANCE.dup
    ie.test_num = request.query['t'].to_i || 0
    ie.subtest_data = request.query['s'] || nil
    ie.random_mode = request.query['r']
    ie.lookup_mode = request.query['l']
    ie.claimed_browser = request.query['b'] || nil
    ie.stop_num = request.query['x'] || nil
    user_agent = request['User-agent'] || 'unknown'
    raw_user_agent = user_agent.dup
    
    # Shorten the user-agent displayed
    user_agent.gsub!('Mozilla/5.0', '')
    user_agent.gsub!('X11; ', '')
    user_agent.gsub!('Macintosh; ', '')
    user_agent.gsub!(' U;', '')
    user_agent.gsub!(/^ +/, '')
    user_agent.gsub!(' (KHTML, like Gecko)', '')
    if user_agent =~ /Chrome/
      user_agent.gsub!(/Safari\/[\d\.]+/, '')
    end
    ie.browser = user_agent
    ie.setRandomSeed()
    # If we are a dependency image, fiddle with the headers!
    mime_type = request.query['m']
    headers = []
    if mime_type
      for (key, value) in ie.buildHeaders(mime_type)
        headers << "#{key}[#{value.length}]"
        response[key] = value
      end
      response.body = ie.buildMediaFile(mime_type)
    else
      response['Content-Type'] = 'text/html'
      response.body = ie.buildPage()
    end

    details = "?t=#{ie.test_num}"
    if ie.subtest_data
      details << "&s=#{ie.subtest_data}"
    end
    if ie.random_mode
      details << "&r=1"
    end
    if ie.lookup_mode
      details << "&l=#{ie.lookup_mode}"
    end
    if mime_type
      details << "&m=#{mime_type}"
    else
      $last_page_requested = [Time.now().to_i, request.unparsed_uri, CGI.escape(user_agent)]
    end
    printf("%-45.45s %s\n", details, user_agent)
    if headers.length > 0
      printf("%-45.45s %s\n", "Headers for #{mime_type}:", headers.join(', '))
    end
  end
end


# Simple form
class IEForm < HTTPServlet::AbstractServlet
  def do_GET(request, response)
    response['Content-Type'] = 'text/html'
    response.body = File.read("index.html")
  end
end

class IELogo < HTTPServlet::AbstractServlet
  def do_GET(request, response)
    response['Content-Type'] = 'image/png'
    response.body = File.read("media/bug.png")
  end
end

class NoPage < HTTPServlet::AbstractServlet
  def do_GET(request, response)
    response.body = 'OHAI'
  end
end

class LastPage < HTTPServlet::AbstractServlet
  def do_GET(request, response)
    response.body = $last_page_requested.join(' ')
  end
end


def start_server(port, config_path, log_path)
  puts "* iExploder #{$VERSION} is loading (config=#{config_path}, port=#{port})"
  puts "=" * 80
  $INSTANCE = IExploder.new(config_path)
  warn_logger = Log.new($stderr, Log::WARN)
  config = YAML::load(File.open(config_path))
  if not log_path
    log_path = config['access_log_path']
  end
  puts "- Setting up logging to #{log_path}"
  access_log_stream = Log.new(log_path)
  access_log = [[ access_log_stream, AccessLog::COMMON_LOG_FORMAT ]]
  s = WEBrick::HTTPServer.new(:Port => port, :Logger => warn_logger, :AccessLog => access_log)
  s.mount("/", IEForm)
  s.mount("/favicon.ico", NoPage)
  s.mount("/media/bug.png", IELogo)
  s.mount("/iexploder.cgi", IEServlet)
  s.mount("/last_page.cgi", LastPage)
  ['INT', 'TERM'].each {|signal| trap(signal) { puts "SERVER SHUTDOWN: #{signal}"; s.shutdown }}
  puts "- iExploder is at http://127.0.0.1:#{port}"
  s.start
  puts ""
  puts "Goodbye! Have a fantastic day."
end



if $0 == __FILE__
  options = {
    :port => 3100,
    :config_path => 'config.yaml',
    :log_path => nil
  }

  optparse = OptionParser.new do|opts|
    opts.banner = "Usage: webserver.rb [options]"
    opts.on( '-p', '--port NUM', 'Listen on TCP port NUM' ) { |port| options[:port] = port }
    opts.on( '-c', '--config PATH', 'Use PATH for configuration file' ) { |path| options[:config_path] = path }
    opts.on( '-l', '--log PATH', 'Use PATH for log file' ) { |path|  options[:log_path] = path }
    opts.on( '-h', '--help', 'Display this screen' ) { puts opts; exit }
  end
  optparse.parse!
  start_server(options[:port], options[:config_path], options[:log_path])
end
