#!/usr/bin/ruby
# lasthit, part of iExploder
# 
# Shows statistics about recent agents that have tested with iExploder. 
# It takes all or part of an apache logfile via stdin, and outputs a list
# of all the agents who tested within that section, what their last test
# was, and how many tests they have done.

# The usefulness is finding out where a browser crashed.


hostHash = Hash.new

if (ARGV[0])
	file = File.open(ARGV[0])
else
	file = $stdin
end
 
file.readlines.each { |line|
	if (line =~ /^(.*?) .*iexploder.*?test=(\d+).* HTTP.* \"(.*?)\"$/)
		host = $1
		testnum = $2
		agent = $3
		if (! hostHash[host])
			hostHash[host] = Hash.new
		end
		if (! hostHash[host][agent])
			hostHash[host][agent] = Hash.new
			hostHash[host][agent]['total'] = 0
		end

		hostHash[host][agent]['last'] = testnum
		if line =~ /subtest=(\d+)/
			hostHash[host][agent]['subtest'] = $1
		else
			hostHash[host][agent]['subtest'] = ''
		end
		hostHash[host][agent]['total'] = hostHash[host][agent]['total'] + 1
	end
}

printf("%14.14s | %8.8s | %3.3s | %8.8s | %s\n", 
	 "IP",    "Test", "SubTest", "Total", "Agent")
puts "---------------------------------------------------------------------------"
hostHash.each_key { |host|

	hostHash[host].each_key { |agent|
		printf("%14.14s | %8.8s | %3.3s | %8.8s | %s\n", 
			host, hostHash[host][agent]['last'],  hostHash[host][agent]['subtest'], hostHash[host][agent]['total'], agent);
	}
}

