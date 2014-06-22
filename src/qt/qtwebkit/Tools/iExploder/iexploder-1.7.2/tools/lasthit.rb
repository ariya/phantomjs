#!/usr/bin/ruby
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
#
#
# lasthit, part of iExploder
# 
# Shows statistics about recent agents that have tested with iExploder. 
# It takes all or part of an apache logfile via stdin, and outputs a list
# of all the agents who tested within that section, what their last test
# was, and how many tests they have done.

# The usefulness is finding out where a browser crashed.

require 'cgi'

hostHash = Hash.new

if (ARGV[0])
	file = File.open(ARGV[0])
else
  puts "No filename specified, waiting for data via stdin..."
	file = $stdin
end

last_index = nil
file.readlines.each_with_index { |line, index|
  # filter out mime hits as they produce a lot of odd user agents
  next if line =~ /&m=/
	if (line =~ /([\w\.]+) - - .*iexploder.cgi\?(.*?)&b=([\w\%-\.+]+)/)
		host = $1
		test_url = $2
		agent = $3
		if (! hostHash[host])
			hostHash[host] = Hash.new
		end
		if (! hostHash[host][agent])
			hostHash[host][agent] = Hash.new
			hostHash[host][agent]['total'] = 0
		end
		hostHash[host][agent]['last'] = test_url
		hostHash[host][agent]['total'] = hostHash[host][agent]['total'] + 1
		hostHash[host][agent]['last_line'] = index
	end
  last_index = index
}

printf("%-14.14s | %-25.25s | %6.6s | %7.7s | %s\n",
	 "Host", "Test URL", "Total", "LineAgo", "Agent")
puts "-" * 78
hostHash.each_key { |host|
	hostHash[host].each_key { |agent|
    next if agent.length < 8
    display_agent = CGI::unescape(agent).sub('U; ', '')
		printf("%-14.14s | %-25.25s | %6.6s | %7.7s | %s\n",
			host, hostHash[host][agent]['last'],
      hostHash[host][agent]['total'],
      hostHash[host][agent]['last_line'] - last_index,
      display_agent);
	}
}

