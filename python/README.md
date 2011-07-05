PyPhantomJS is a headless WebKit with JavaScript API, based on the [PhantomJS](http://www.phantomjs.org/) project.

It has **fast** and **native** support for DOM handling, CSS selector, JSON, Canvas, SVG, and of course JavaScript.

PyPhantomJS scripts can be written in JavaScript or [CoffeeScript](http://jashkenas.github.com/coffee-script/).

See the [quick start guide](http://code.google.com/p/phantomjs/wiki/QuickStart) and more [advanced examples](http://code.google.com/p/phantomjs/wiki/ServiceIntegration) which show various PhantomJS scripts, covering:

* running regression tests from command line
* getting driving direction
* showing weather forecast conditions
* finding pizza in New York
* looking up approximate location based on IP address
* pulling the list of seasonal food
* producing PDF version of a Wikipedia article
* rasterizing SVG to image

PyPhantomJS is written in PyQt4 and Python. It runs on Linux, Windows, and Mac OS X.  
Refer to the INSTALL file or Wiki links<sup>1</sup> for more information.

Do not forget to consult the concise API Reference<sup>2</sup>

If you want to contribute, please read the Contribution Guides<sup>3</sup>

You can find a list of downloadable plugins [here](http://dev.umaclan.com/projects/pyphantomjs/wiki/Plugins).

If you would like to know how to make plugins, check out this [article](http://dev.umaclan.com/projects/pyphantomjs/wiki/Writing_plugins).

1: http://code.google.com/p/phantomjs/w/list  
1: http://dev.umaclan.com/projects/pyphantomjs/wiki  
2: http://dev.umaclan.com/projects/pyphantomjs/wiki/Api_reference  
2: http://code.google.com/p/phantomjs/wiki/Interface  
3: http://code.google.com/p/phantomjs/wiki/ContributionGuide  
3: http://dev.umaclan.com/projects/pyphantomjs/wiki/Giving_back

LICENSING
------------------
Copyright (C) 2011 James Roe <<roejames12@hotmail.com>>  
Copyright (C) 2011 PyPhantomJS authors (see AUTHORS file)

PyPhantomJS is licensed with the GNU GPL v3.  
See the included file LICENSE for the licensing terms.

ADDITIONAL INFO
-----------------------------
This program is a port of PhantomJS to Python/PyQt4 (thus, PyPhantomJS).

  1. We try to make PyPhantomJS to be as compatible with PhantomJS as possible,
     this means having the exact same features as they do. It happens to be fully
     compatible, with the exception of some very small things.
  2. PyPhantomJS however, has a few differences as well:
       * Some of the features work a little differently than PhantomJS.
         In most of these cases, the features were only changed to improve their
         reliability and performance, so they work more often/better, and break less.
       * There may also be additional features present that aren't in PhantomJS.

PhantomJS was written by Ariya Hidayat, and I'd like to give him a BIG thanks
for all his work on PhantomJS! :)
