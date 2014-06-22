Welcome to iExploder. a highly inefficient, but fairly effective web
browser tester. The code still has a lot of work to be done, but it's
definitely usable. Here are some notable features:

* Tests all HTML tags and CSS attributes, as parsed from various
  open-source browsers.
* HTTP Header testing
* Basic Media format fuzzing (jpeg, png, snd, wav, etc.)
* Numeric, and String overflow and formatting tests
* Sequential and Randomized Test Case Generation
* Test Case Lookups
* Subtest generation
* Test harness mode that controls your browser process and testcase
  generation.

Requirements:
-------------
Make sure you have Ruby installed (comes with Mac OS X, most Linux
distributions). See http://www.ruby-lang.org/ if you do not.


Harness mode (Mac OS X, Linux, other UNIX based operating systems)
-------------------------------------------------------------------
In this mode, iExploder controls the stopping and starting of your
web browser, reproducing crashes, and writing test cases. This is
the recommended mode of operation for most cases.


Usage: browser_harness.rb [options] -- <browser path> <browser options>
    -t, --test NUM                   Test to start at
    -p, --port NUM                   Listen on TCP port NUM (random)
    -c, --config PATH                Use PATH for configuration file
    -d, --testdir PATH               Use PATH to save testcases (/tmp)
    -l, --logdir PATH                Use PATH to save logs (/tmp)
    -w, --watchdog NUM               How many seconds to wait for pages to load (45s)
    -r, --random                     Generate test numbers pseudo-randomly
    -s, --scan NUM                   How often to check for new log data (5s)
    -h, --help                       Display this screen

Here is an example use with Chrome starting at test number 1000:

% ./browser_harness.rb -t 1000 -- /usr/local/chrome-linux/chrome --incognito

For proper use, the harness mode must *ALWAYS* be used with the browser
configured to not restore sessions after a restart. Here are some example
command lines to use:

  chrome --incognito
  opera --nosession -newprivatetab
  firefox -private

On Mac OS X you may call the binary directly, or use the .app directory. The latter
is required for Safari.app, but does not allow arguments to be passed. Here is an
example:

% ./browser_harness.rb /Applications/Safari.app

By default, all testcases and logs will be written to ../output



Viewing testcases:
------------------
Many test-cases make use of references to external objects (ogg, jpg, etc.)
where we are fuzzing the HTTP header data. When the browser harness saves a
testcase in HTML form, it rewrites all references to these external objects
to refer to http://127.0.0.1:3100/

To properly view these saved .html testcases, please run the built-in
webserver in the background. 

% ruby webserver.rb


Standalone Webserver mode:
--------------------------
If you do not already have a webserver setup, you can use the server
built into iexploder. Simply go into the src/ directory and type:

% ruby webserver.rb

A webserver will then start on port 3100 with the iexploder form. You can
also pass a -p or --port option to select a different location:

% ruby webserver.rb -p 2001

All requests will be logged to the path specified in 'access_log_path'
parameter in config.yaml.



Third-party webserver mode:
---------------------------
Copy the contents of the src/ folder to any directory served
by your webserver. Make sure that directory can execute CGI scripts.
Performance is likely to be very slow unless you use something that
keeps the interpreter alive like mod_ruby.


FAQ:
----
1) Are the tests always the same?

  The test cases should always be the same on a single installation, but not
necessarily on different installations of iExploder. Random generator seeds
may differ between operating systems and platforms. If you alter config.yaml,
it is likely to change the test cases as well.


2) I found a crash - how do I stop testing for it?

See the 'exclude' section of config.yaml. It allows you to blacklist certain
tag combinations that are known to result in a crash condition.


3) How do I look up the last successful test for a client?

Look at your access log. There is a handy tool to parse access logs and show
the most recent test for each host and user-agent combo. Try:

tools/lasthit.rb /path/to/access_log


4) How do subtests work?

Subtests are how iexploder attempts to isolate the crashing line of code for
a particular HTML document. It's a multi-pass algorithm, which delivers
nasty tags to your browser in the following order:

* 1 combination, single line
* 2 combinations, 3 lines per combination
* 3 combinations, 5 lines per combination
* 4 combinations...
* 5 combinations...
* Your original document (in case we haven't crashed by now)


5) How come I can't seem to repeat the crash?

  Many browser crashes are race conditions that are not easy to repeat. Some
crashes only happen when going from test 4 -> test 5 -> test 6. If you can't
repeat the crash through subtests or a lookup of the failing test, try going
back a few tests.

That said, some crashes are due to race conditions that are very difficult
to replicate.


6) Why did you write this?

  I wanted to make sure that FireFox had as many bugs fixed in it as possible
before the 1.0 release. After 1.0 came out, I kept improving it.


7) Why does Internet Explorer run the tests so slowly?

  <META> refresh tags are very fragile in Internet Explorer, and can be easily
be rendered useless by other tags on the page. If this happens, a javascript
refresh will execute after a 1 second delay.


8) How do I change the number of tags iExploder tests per page?

See config.yaml.


9) What other performance enhancements can I make?

* Use Private Browsing or Incognito mode in your browser
* Before using iExploder, clear your browser history
* Minimize your browser while iExploder is running
* If you are using browser_harness, try adjusting the -w and -s options.


