#!/usr/bin/ruby

require 'fileutils'

def usage
  puts "usage: #{File.basename $0} <destination-to-update>"
  puts
  puts "<destination-to-update> values:"
  puts
  puts "  Tools         - Copy the UserInterface files to the Tools directory"
  puts "  UserInterface - Copy the Tools files to the UserInterface directory"
  exit 1
end

if ARGV.size != 1
  usage
end

destination = ARGV[0]
if destination != "Tools" && destination != "UserInterface"
  usage
end

# Copy the formatter and CodeMirror files from UserInterface to Tools.
USER_INTERFACE_TO_TOOLS_MAP = {
  "UserInterface/CodeMirrorFormatters.js"    => "Tools/PrettyPrinting/CodeMirrorFormatters.js",
  "UserInterface/Formatter.js"               => "Tools/PrettyPrinting/Formatter.js",
  "UserInterface/FormatterContentBuilder.js" => "Tools/PrettyPrinting/FormatterContentBuilder.js",

  "UserInterface/External/CodeMirror/codemirror.css" => "Tools/PrettyPrinting/codemirror.css",
  "UserInterface/External/CodeMirror/codemirror.js"  => "Tools/PrettyPrinting/codemirror.js",
  "UserInterface/External/CodeMirror/javascript.js"  => "Tools/PrettyPrinting/javascript.js",
  "UserInterface/External/CodeMirror/css.js"         => "Tools/PrettyPrinting/css.js",
}

# Copy only the formatter files from Tools to UserInterface.
TOOLS_TO_USER_INTERFACE_MAP = {
  "Tools/PrettyPrinting/CodeMirrorFormatters.js"    => "UserInterface/CodeMirrorFormatters.js",
  "Tools/PrettyPrinting/Formatter.js"               => "UserInterface/Formatter.js",
  "Tools/PrettyPrinting/FormatterContentBuilder.js" => "UserInterface/FormatterContentBuilder.js"
}

web_inspector_path = File.expand_path File.join(File.dirname(__FILE__), "..")
map = destination == "Tools" ? USER_INTERFACE_TO_TOOLS_MAP : TOOLS_TO_USER_INTERFACE_MAP

all_success = true

map.each do |from, to|
  from_path = File.join web_inspector_path, from
  to_path = File.join web_inspector_path, to
  begin
    puts "Copying #{from} to #{to}..."
    FileUtils.cp from_path, to_path
  rescue Exception => e
    puts "WARNING: #{e}"
    all_success = false
  end
end

exit all_success ? 0 : 1
