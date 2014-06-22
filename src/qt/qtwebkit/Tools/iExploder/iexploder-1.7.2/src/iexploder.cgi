#!/usr/bin/ruby
# iExploder - Generates bad HTML files to perform QA for web browsers.
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

require 'cgi';
require 'iexploder';

$CONFIG_PATH = 'config.yaml'

ie = IExploder.new($CONFIG_PATH)
cgi = CGI.new("html4");
ie.cgi_url=ENV['SCRIPT_NAME'] || '?'
ie.browser=ENV['HTTP_USER_AGENT'] || 'unknown'
ie.test_num = cgi.params['t'][0].to_i
ie.subtest_data = cgi.params['s'][0] || nil
ie.random_mode = cgi.params['r'][0]
ie.lookup_mode = cgi.params['l'][0]
ie.stop_num = cgi.params['x'][0] || nil
ie.setRandomSeed()

mime_type = cgi.params['m'][0] || nil
if mime_type:
  header_options = ie.buildHeaders(mime_type)
  # The CGI library wants the Content-Type header to be named 'type'. It
  # will post two Content-Type headers otherwise.
  header_options['type'] = header_options['Content-Type'].dup
  header_options.delete('Content-Type')
  cgi.out(header_options) do
    ie.buildMediaFile(mime_type)
  end
else
  cgi.out('type' => 'text/html') do
    ie.buildPage()
  end
end
