#!/usr/bin/ruby
# iExploder browser Harness  (test a single web browser)
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
#
#----------------------------------------------------------------------------
# PLEASE NOTE:
#
# You must disable automatic session restoring for this to be useful.
#
# chrome --incognito
# opera --nosession -newprivatetab
# firefox -private
require 'cgi'
require 'open-uri'
require 'optparse'
require './iexploder.rb'
require './scanner.rb'

MAC_CRASH_PATH = "#{ENV['HOME']}/Library/Logs/CrashReporter"
TESTCASE_URL = "http://127.0.0.1:3100/iexploder.cgi"

class BrowserHarness
  def initialize(port, config_path, log_dir, test_dir, watchdog_timer, scan_timer)
    @app_base_url = "http://127.0.0.1:#{port}/"
    @app_url = "#{@app_base_url}iexploder.cgi"
    @port = port
    @log_dir = log_dir
    @server_log_path = "#{log_dir}/iexploder_webserver-#{port}.log"
    @client_log_path = "#{log_dir}/iexploder_harness-#{port}.log"
    @test_dir = test_dir
    @watchdog_timer = watchdog_timer
    @scan_timer = scan_timer
    @config_path = config_path

    @ie = IExploder.new(@config_path)
    @ie.cgi_url = @app_url

    @browser_id = nil
    @browser_name = nil
    msg("Client log: #{@client_log_path}")
    msg("Server log: #{@server_log_path}")
    @server_pid = launch_server()
  end

  def msg(text)
    now = Time.now()
    msg = ">>> #{@browser_name}:#{@port} | #{now}: #{text}"
    puts msg
    STDOUT.flush

    f = File.open(@client_log_path, 'a')
    f.puts msg
    f.close
  end

  def launch_server()
    args = ['./webserver.rb', "-p#{@port}", "-c#{@config_path}", "-l#{@server_log_path}"]
    pids = fork { exec(*args) }
    msg("Server args: #{args.inspect}")
    msg("Server pid: #{pids.inspect}")
    return pids
  end

  def launch_browser(args, url)
    if ! File.exist?(args[0])
      msg("First argument does not appear to be an executable file: #{args[0]}")
      kill_server()
      exit
    end

    browser = File.basename(args[0])
    @browser_name = File.basename(browser)
    if browser =~ /\.app$/
      pids = launch_mac_browser(args, url)
    else
      pids = launch_posix_browser(args, url)
    end
    sleep(@scan_timer * 3)
    if ! File.size?(@server_log_path)
      puts "#{@server_log_path} was never written to. Unable to launch browser?"
      kill_server()
      exit
    end
    return pids
  end

  def launch_posix_browser(args, url)
    browser = File.basename(args[0])
    msg("Killing browser processes: #{browser}")
    system("pkill #{browser} && pkill -9 #{browser}")
    args = args + [url]
    msg("Launching browser: #{args.inspect}")
    browser_pid = fork {
      exec(*args)
    }
    return [browser_pid]
  end

  def find_pids(text)
    # Only tested on Mac OS X.
    pids = []
    `ps -x`.each do |proc_line|
      if proc_line =~ /^ *(\d+).*#{text}/
        pid = $1.to_i
        # Do not include yourself.
        if pid != Process.pid
          pids << $1.to_i
        end
      end
    end
    return pids
  end

  def launch_mac_browser(args, url)
    # This is dedicated to Safari.
    if args.length > 1
      msg(".app type launches do not support arguments, ignoring #{args[1..99].inspect}")
    end
    browser = args[0]
    pids = find_pids(browser)
    if pids
      kill_pids(find_pids(browser))
      sleep(2)
    end
    command = "open -a \"#{browser}\" \"#{url}\""
    msg(".app open command: #{command}")
    system(command)
    return find_pids(browser)
  end

  def kill_pids(pids)
    pids.each do |pid|
      msg("Killing #{pid}")
      begin
        Process.kill("INT", pid)
        sleep(0.5)
        Process.kill("KILL", pid)
      rescue
        sleep(0.1)
      end
    end
  end

  def encode_browser()
    return @browser_id.gsub(' ', '_').gsub(';', '').gsub('/', '-').gsub(/[\(\):\!\@\#\$\%\^\&\*\+=\{\}\[\]\'\"\<\>\?\|\\]/, '').gsub(/_$/, '').gsub(/^_/, '')
  end

  def kill_server()
    kill_pids([@server_pid])
  end

  def parse_test_url(value)
    current_vars = nil
    test_num = nil
    subtest_data = nil
    lookup_values = false
    if value =~ /iexploder.cgi(.*)/
      current_vars = $1
      if current_vars =~ /[&\?]t=(\d+)/
        test_num = $1
      end
      if current_vars =~ /[&\?]s=([\d_,]+)/
        subtest_data = $1
      end
      if current_vars =~ /[&\?]l=(\w+)/
        lookup_value = $1
      end
    else
      msg("Unable to parse url in #{value}")
      return [nil, nil, nil, nil]
    end
    return [current_vars, test_num, subtest_data, lookup_value]
  end

  def check_log_status()
    timestamp, uri, user_agent = open("#{@app_base_url}last_page.cgi").read().chomp.split(' ')
    age = (Time.now() - timestamp.to_i).to_i
    if not @browser_id
      @browser_id = CGI.unescape(user_agent)
      msg("My browser is #{@browser_id}")
    end


    return [age, uri]
  end

  def save_testcase(url, case_type=nil)
    msg("Saving testcase: #{url}")
    vars, test_num, subtest_data, lookup_value = parse_test_url(url)
    if not case_type
      case_type = 'testcase'
    end

    testcase_name = ([case_type, encode_browser(), 'TEST', test_num, subtest_data].join('-')).gsub(/-$/, '') + ".html"
    testcase_path = "#{@test_dir}/#{testcase_name}"
    data = open(url).read()
    # Slow down our redirection time, and replace our testcase urls.
    data.gsub!(/0;URL=\/iexploder.*?\"/, "1;URL=#{testcase_name}\"")
    data.gsub!(/window\.location=\"\/iexploder.*?\"/, "window\.location=\"#{testcase_name}\"")

    # I wish I did not have to do this, but the reality is that I can't imitate header fuzzing
    # without a webservice in the backend. Change all URL's to use a well known localhost
    # port.
    data.gsub!(/\/iexploder.cgi/, TESTCASE_URL)

    f = File.open(testcase_path, 'w')
    f.write(data)
    f.close
    msg("Wrote testcase #{testcase_path}")
    return testcase_path
  end

  def calculate_next_url(test_num, subtest_data)
    @ie.test_num = test_num.to_i
    @ie.subtest_data = subtest_data
    if subtest_data and subtest_data.length > 0
      (width, offsets) = @ie.parseSubTestData(subtest_data)
      # We increment within combo_creator
      (width, offsets, lines) = combine_combo_creator(@ie.config['html_tags_per_page'], width, offsets)
      return @ie.generateTestUrl(@ie.nextTestNum(), width, offsets)
    else
      return @ie.generateTestUrl(@ie.nextTestNum())
    end
  end

  def find_crash_logs(max_age)
    crashed_files = []
    check_files = Dir.glob("*core*")
    if File.exists?(MAC_CRASH_PATH)
      check_files = check_files + Dir.glob("#{MAC_CRASH_PATH}/*.*")
    end
    check_files.each do |file|
      mtime = File.stat(file).mtime
      age = (Time.now() - mtime).to_i
      if age < max_age
        msg("#{file} is only #{age}s old: #{mtime}")
        crashed_files << file
      end
    end
    return crashed_files
  end

  def test_browser(args, test_num, random_mode=false)
    # NOTE: random_mode is not yet supported.

    browser_pids = []
    subtest_data = nil
    @ie.test_num = test_num
    @ie.random_mode = random_mode
    next_url = @ie.generateTestUrl(test_num)

    while next_url
      msg("Starting at: #{next_url}")
      if browser_pids
        kill_pids(browser_pids)
      end
      browser_pids = launch_browser(args, next_url)
      test_is_running = true
      crash_files = []

      while test_is_running
        sleep(@scan_timer)
        begin
          age, request_uri = check_log_status()
        rescue
          msg("Failed to get status. webserver likely crashed.")
          kill_pids([@server_pid])
          @server_pid = launch_server()
          next_url = @ie.generateTestUrl(test_num)
          test_is_running = false
          next
        end
        vars, test_num, subtest_data, lookup_value = parse_test_url(request_uri)
        if lookup_value == 'survived_redirect'
          msg("We survived #{vars}. Bummer, could not repeat crash. Moving on.")
          test_is_running = false
          next_url = calculate_next_url(test_num, subtest_data)
          next
        elsif age > @watchdog_timer
          msg("Stuck at #{vars}, waited for #{@watchdog_timer}s. Killing browser.")
          kill_pids(browser_pids)
          current_url = "#{@app_url}#{vars}"
#          save_testcase(current_url, 'possible')
          crash_files = find_crash_logs(@watchdog_timer + (@scan_timer * 2))
          if crash_files.length > 0
            msg("Found recent crash logs: #{crash_files.inspect} - last page: #{current_url}")
          end

          if vars =~ /THE_END/
            msg("We hung at the end. Saving a testcase just in case.")
            save_testcase(current_url)
            next_url = calculate_next_url(test_num, nil)
            test_is_running = false
            next
          end

          # This is for subtesting
          if subtest_data
            if lookup_value
              msg("Confirmed crashing/hanging page at #{current_url} - saving testcase.")
              save_testcase(current_url)
              next_url = calculate_next_url(test_num, nil)
              test_is_running = false
              next
            else
              msg("Stopped at #{current_url}. Attempting to reproduce simplified crash/hang condition.")
              browser_pids = launch_browser(args, "#{current_url}&l=test_redirect")
            end
          # Normal testing goes here
          else
            if lookup_value
              msg("Reproducible crash/hang at #{current_url}, generating smaller test case.")
              url = current_url.gsub(/&l=(\w+)/, '')
              browser_pids = launch_browser(args, "#{url}&s=0")
            else
              msg("Stopped at #{current_url}. Attempting to reproduce crash/hang condition.")
              browser_pids = launch_browser(args, "#{current_url}&l=test_redirect")
            end
          end
        elsif age > @scan_timer
          msg("Waiting for #{vars} to finish loading... (#{age}s of #{@watchdog_timer}s)")
        end
      end
    end
  end
end

if $0 == __FILE__
  options = {
    :port => rand(16000).to_i + 16000,
    :test_dir => File.dirname($0) + '/../output',
    :log_dir => File.dirname($0) + '/../output',
    :test_num => nil,
    :watchdog_timer => 60,
    :scan_timer => 5,
    :config_path => 'config.yaml',
    :random_mode => false
  }

  optparse = OptionParser.new do |opts|
    opts.banner = "Usage: browser_harness.rb [options] -- <browser path> <browser options>"
    opts.on( '-t', '--test NUM', 'Test to start at' ) { |test_num| options[:test_num] = test_num.to_i }
    opts.on( '-p', '--port NUM', 'Listen on TCP port NUM (random)' ) { |port| options[:port] = port.to_i }
    opts.on( '-c', '--config PATH', 'Use PATH for configuration file' ) { |path| options[:config_path] = path }
    opts.on( '-d', '--testdir PATH', 'Use PATH to save testcases (/tmp)' ) { |path| options[:test_dir] = path }
    opts.on( '-l', '--logdir PATH', 'Use PATH to save logs (/tmp)' ) { |path|  options[:log_dir] = path }
    opts.on( '-w', '--watchdog NUM', 'How many seconds to wait for pages to load (45s)' ) { |sec|  options[:watchdog_timer] = sec.to_i }
    opts.on( '-r', '--random', 'Generate test numbers pseudo-randomly' ) { options[:random_mode] = true }
    opts.on( '-s', '--scan NUM', 'How often to check for new log data (5s)' ) { |sec|  options[:scan_timer] = sec.to_i }
    opts.on( '-h', '--help', 'Display this screen' ) { puts opts; exit }
  end
  optparse.parse!

  if options[:port] == 0
    puts "Unable to parse port option. Try adding -- as an argument before you specify your browser location."
    exit
  end

  if ARGV.length < 1
    puts "No browser specified. Perhaps you need some --help?"
    exit
  end
  puts "options: #{options.inspect}"
  puts "browser: #{ARGV.inspect}"

  harness = BrowserHarness.new(
    options[:port],
    options[:config_path],
    options[:log_dir],
    options[:test_dir],
    options[:watchdog_timer],
    options[:scan_timer]
  )

  harness.test_browser(ARGV, options[:test_num], options[:random_mode])
end
