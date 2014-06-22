#!/usr/bin/ruby
# iExploder - Generates bad HTML files to perform QA for web browsers.
# Developed for the Mozilla Foundation.
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

require 'webrick'
require 'iexploder';
require 'config';

include WEBrick
### THE INTERACTION ##################################
$ie_preload = IExploder.new($HTML_MAX_TAGS, $HTML_MAX_ATTRS, $CSS_MAX_PROPS)
$ie_preload.readTagFiles()
$ie_preload.url='/iexploder.cgi'

if ARGV[0]
	port = ARGV[0].to_i
else
	port = 2000
end

puts "* iExploder #{$VERSION} will be available at http://localhost:#{port}"
puts "* Max Tags: #$HTML_MAX_TAGS Max Attrs: #$HTML_MAX_ATTRS Max Props: #$CSS_MAX_PROPS"
puts

s = HTTPServer.new( :Port => port )
class IEServlet < HTTPServlet::AbstractServlet
    def do_GET(req, res)
        ie = $ie_preload.dup
        ie.test_num = req.query['test'].to_i
        ie.subtest_num = req.query['subtest'].to_i || 0
        ie.random_mode = req.query['random']
        ie.lookup_mode = req.query['lookup']
        ie.stop_num = req.query['stop'].to_i
        ie.setRandomSeed
        
        res['Content-Type'] = 'text/html'  
        res.body = ie.buildPage()
    end
end

class IEForm < HTTPServlet::AbstractServlet
    def do_GET(req, res)   
        res['Content-Type'] = 'text/html'  
        res.body = File.open("index.html").readlines.join("\n")
    end
end



s.mount("/iexploder.cgi", IEServlet)
s.mount("/", IEForm)
trap("INT") { s.shutdown }

s.start
