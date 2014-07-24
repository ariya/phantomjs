#!/usr/bin/env ruby

# Copyright (C) 2012 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# Testing Tools/Scripts/check-for-macros-in-external-headers
$test_directory = File.dirname(__FILE__)
$tool = File.expand_path(File.join($test_directory, '..', '..', 'check-for-inappropriate-macros-in-external-headers'))
puts "Testing: Tools/Scripts/check-for-inappropriate-macros-in-external-headers"

$was_failure = false

def sanitized_output(output)
  lines = output.split("\n").map { |line| line.sub(/\'(.*)?\/(.*)?\.framework/, "'--stripped--/\\2.framework") }
  lines.join("\n") + (lines.empty? ? "" : "\n")
end

def run_test(config)
  ENV['TARGET_BUILD_DIR'] = File.join($test_directory, 'resources')
  ENV['PROJECT_NAME'] = config[:framework]
  ENV['SHALLOW_BUNDLE'] = config[:shallow] ? 'YES' : 'NO'
  output = sanitized_output %x{ #{$tool} #{config[:paths].join(' ')} 2>&1 }

  if config[:expectedToPass] != ($?.exitstatus == 0)
    pass = false
  else
    expected_output = File.read File.join($test_directory, config[:expectedOutput])
    pass = output == expected_output
  end

  puts "#{pass ? "PASS" : "FAIL"} - #{config[:name]}"
  $was_failure = true if !pass
end

[
  {
    :name => 'test_good_fake_data',
    :framework => 'Fake',
    :shallow => true,
    :paths => ['Headers/Pass.h'],
    :expectedToPass => true,
    :expectedOutput => 'pass-expected.txt'
  },
  {
    :name => 'test_bad_fake_data',
    :framework => 'Fake',
    :shallow => true,
    :paths => ['Headers/Fail.h'],
    :expectedToPass => false,
    :expectedOutput => 'fake-data-failing-expected.txt'
  }
].each { |x| run_test(x) }

exit 1 if $was_failure
