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
# Gives you information about the most recent crash for each application
# that has crashed within the last 2 days

$LogDir=ENV['HOME'] + '/Library/Logs/CrashReporter'
$Days=1
$StackCount=5

files=`find #$LogDir -mtime -#$Days -type f | grep -v synergy`
files.each { |filename|
    filename.chop!
    record = 0
    date=''
    stackTrace = []

    File.open(filename).readlines.each { |line|
        #puts line

        if line =~ /^Date.*(200.*)/
            date = $1
        end

        if line =~ /^Thread \d+ Crashed/
            record = 1
            # reset the stack trace
            stackTrace = []
        end

        if record
            stackTrace << line
            record = record + 1

            # stop recording after $StackCount lines
            if record > ($StackCount + 2)
                record = nil
            end
        end
    }

    puts File.basename(filename) + " - " + date
    puts "==================================================="
    stackTrace.each { |line|
        puts line
    }
    puts ""
}


