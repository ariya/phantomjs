iExploder 1.3.2
===============

Welcome to iExploder. a highly inefficient, but fairly effective web
browser tester. The code still has a lot of work to be done, but it's
definitely usable. Here are some notable features:

* Tests all common HTML and CSS tags and attributes, as parsed from 
the KHTML, WebKit and Mozilla source trees, as well as tags for 
Internet Explorer from MSDN. This also includes a few Javascript hooks.
* Numeric, and String overflow and formatting tests
* Sequential and Randomized Test Case Generation
* Test Case Lookups
* Subtest generation


Installation (Standalone)
-------------------------
Make sure you have Ruby installed (comes with Mac OS X, most Linux
distributions). See http://www.ruby-lang.org/ if you do not.

If you do not already have a webserver setup, you can use the server
built into iexploder. Simply go into the htdocs directory and type:

% ruby webserver.rb

A webserver will then start on port 2000 with the iexploder form. If
port 2000 is not preferable, you can pass it another port on the command
line:

% ruby webserver.rb 2001

Please note that lasthit.rb does not currently work with the logs output
from this method. I recommend using a seperate instance/port number
for each browser you test simultaneous using this method.


Installation (External Webserver)
---------------------------------
If you wish to use an external webserver (required for lasthit.rb use), 
you may do so. IExploder has been tested with apache.

Copy the contents of the htdocs/ folder to any directory served
by your webserver. Make sure that directory can execute CGI scripts. If
performance seems to be low, please try using mod_ruby.


FAQ:
----
1) Are the tests always the same? 

  The test cases should always be the same on a single installation, but not
necessarily on different installations of iExploder. Random generator seeds
may differ between operating systems and platforms. If you alter the tag and
property counts in config.rb, it will change the test cases as well.


2) How do I look up the last successful test for a client?

Use tools/lasthit.rb. When I get a crash, I usually do something like:

     % tail -15000 /var/log/apache2/access_log | ./lasthit.rb

Letting you know how many tests and what the last test id was for each
client tested. You can then try to repeat the test, or go through the
subtests to see if you can repeat the crash.


3) How do subtests work?

If you see a crash on a particular test, and would like to determine the exact
line that is crashing it,  you can use subtests. To do so, go back to the test
submission form, and start the test from the number that a crash was indicated
on. Instead of leaving the "subtest" field blank, set it to 1. This will rotate
through each subtest for a particular test.

Each subtest will rotate through a tag offset and a number of tags to 
garble, which should help you isolate the instance. The number of tags
used doubles each cycle. Here is an idea of how many subtests to expect
based on your $HTML_MAX_TAGS settings:

tags    subtests
----------------
32      138
48      236
64      332
96      558
128     782

Most of the time you will be able to replicate a crash within the first 
$HTML_MAX_TAGS subtests, but sometimes crashes are due to a combination
of corrupted tags.


4) How come I can't seem to repeat the crash?

  Many browser crashes are race conditions that are not easy to repeat. Some
crashes only happen when going from test 4 -> test 5 -> test 6. If you can't
repeat the crash through subtests or a lookup of the failing test, try going
back a few tests.

That said, some crashes are due to race conditions that are very difficult
to replicate.


5) Why did you write this?

  I wanted to make sure that FireFox had as many bugs fixed in it as possible
before the 1.0 release. After 1.0 came out, I kept improving it.


6) Why does Internet Explorer run the tests so slowly?

  <META> refresh tags are very fragile in Internet Explorer, and can be easily
be rendered useless by other tags on the page. If this happens, a javascript
refresh will execute after a 1 second delay. 



7) How do I change the number of tags iExploder tests per page?

See config.rb. I personally recommend 32-128 HTML tags per page. While this
seems to be a lot to go through when designing a test case, that's why the
subtest engine was made. Different web browsers will have different
performance characteristics when it comes to the number of tags per page. 

Here are the results with Firefox 2.0b1 (Bon Echo) and the iExploder
built-in webserver running tests 1-250.

tags  seconds  pages/second  tags/second
-----------------------------------------
32    60       4.0           131
48    85       2.9           141
64    95       2.6           168
96    120      2.1           200 *DEFAULT*
128   140      1.8           228
196   228      1.1           210
256   308      0.8           207

If you find pages/second to be more important than tags/second, I would
change $HTML_MAX_TAGS to 32. Do keep in mind that large tag counts mean
longer subtest generation periods. 


8) What other performance enhancements can I make?

* Before using iExploder, reset your browser history
* Minimize your browser while iExploder is running
* If using Apache, make use of mod_ruby
