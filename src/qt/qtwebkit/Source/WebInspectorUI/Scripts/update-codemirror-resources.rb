#!/usr/bin/ruby

require 'fileutils'

if ARGV.size != 1
  puts "usage: #{File.basename $0} <codemirror-repo-path>"
  exit 1
end

def verify_code_mirror_repository_path(path)
  if !File.directory? path
    puts "ERROR: Provided CodeMirror path is not a directory."
    exit 1
  end

  Dir.chdir(path) do
    results = `git config --list | grep 'marijnh/CodeMirror\.git'`
    if $?.exitstatus != 0 || results.split("\n").empty?
      puts "ERROR: Provided CodeMirror path does not appear to be a CodeMirror checkout."
      exit 1
    end
  end
end

code_mirror_repository_path = File.expand_path ARGV[0]
verify_code_mirror_repository_path code_mirror_repository_path

web_inspector_user_interface_path = File.expand_path File.join(File.dirname(__FILE__), "../UserInterface")
web_inspector_code_mirror_resources_path = File.join web_inspector_user_interface_path, "/External/CodeMirror"

CODE_MIRROR_FILES_TO_COPY = %w(
  addon/comment/comment.js
  addon/display/placeholder.js
  addon/edit/closebrackets.js
  addon/edit/matchbrackets.js
  addon/mode/overlay.js
  addon/runmode/runmode.js
  addon/search/searchcursor.js
  lib/codemirror.css
  lib/codemirror.js
  mode/clojure/clojure.js
  mode/coffeescript/coffeescript.js
  mode/css/css.js
  mode/htmlmixed/htmlmixed.js
  mode/javascript/javascript.js
  mode/less/less.js
  mode/livescript/livescript.js
  mode/sass/sass.js
  mode/sql/sql.js
  mode/xml/xml.js
)

all_success = true

CODE_MIRROR_FILES_TO_COPY.each do |subpath|
  from_path = File.join code_mirror_repository_path, subpath
  to_path = File.join web_inspector_code_mirror_resources_path, File.basename(subpath)
  begin
    puts "Copying #{File.basename(subpath)}..."
    FileUtils.cp from_path, to_path
  rescue Exception => e
    puts "WARNING: #{e}"
    all_success = false
  end
end

exit all_success ? 0 : 1
