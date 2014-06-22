#!/usr/bin/ruby
# showtest.rb - simple CLI interface to grab a testcase
#####################
#
# Copyright (c) 2006 Thomas Stromberg <thomas%stromberg.org>
# 
# This software is provided 'as-is', without any express or implied warranty.
# In no event will the authors be held liable for any damages arising from the
# use of this software.
# 
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software in a
# product, an acknowledgment in the product documentation would be appreciated
# but is not required.
# 
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
# 
# 3. This notice may not be removed or altered from any source distribution.

Dir.chdir('../htdocs')
require 'iexploder';
require 'config';

### THE INTERACTION ##################################
ie = IExploder.new($HTML_MAX_TAGS, $HTML_MAX_ATTRS, $CSS_MAX_PROPS)
ie.readTagFiles()

if ! ARGV[0]
  puts "syntax: showtest.rb [test#] [subtest#]"
  exit
end

ie.test_num = ARGV[0].to_i
ie.subtest_num = ARGV[1].to_i || 0
ie.lookup_mode = 1
ie.setRandomSeed

puts ie.buildPage()
