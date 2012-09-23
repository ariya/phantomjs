# [PhantomJS](http://phantomjs.org) - Scriptable Headless WebKit

PhantomJS ([www.phantomjs.org](http://phantomjs.org)) is a headless WebKit scriptable with JavaScript or CoffeeScript. It is used by hundreds of [developers](https://github.com/ariya/phantomjs/wiki/Buzz) and dozens of [organizations](https://github.com/ariya/phantomjs/wiki/Users) for web-related development workflow.

The latest [stable release](http://phantomjs.org/release-1.7.html) is version 1.7 (codenamed <a href="http://phantomjs.org/release-names.html">"Blazing Star"</a>). Follow the official Twitter stream [@PhantomJS](http://twitter.com/PhantomJS) to get the frequent development updates.

**Note**: Please **do not** create a GitHub pull request **without** reading the [Contribution Guide](https://github.com/ariya/phantomjs/blob/master/CONTRIBUTING.md) first. Failure to do so may result in the rejection of the pull request.

## Use Cases

- **Headless web testing**. Lightning-fast testing without the browser is now possible! Various [test frameworks](http://code.google.com/p/phantomjs/wiki/TestFrameworkIntegration) such as Jasmine, Capybara, QUnit, Mocha, WebDriver, YUI Test, BusterJS, FuncUnit, Robot Framework, and many others are supported.

- **Site scraping**. [Access and manipulate](http://code.google.com/p/phantomjs/wiki/QuickStart#DOM_Manipulation) webpages with the standard DOM API, or with usual libraries like jQuery.

- **Page rendering**. [Capture](http://code.google.com/p/phantomjs/wiki/QuickStart#Rendering) the full contents, even with SVG and Canvas, to an image. Build server-side web graphics apps, from a screenshot service to a vector chart rasterizer.

- **Network monitoring**. [Monitor](http://code.google.com/p/phantomjs/wiki/QuickStart#Network_traffic) network activity, track resource loading, perform load-balancing tests, verify contents optimization, and many others.

## Features

- **Multiplatform**, available on major operating systems: Windows, Mac OS X, Linux, other Unices.
- **Fast and native implementation** of web standards: DOM, CSS, JavaScript, Canvas, SVG. No emulation!
- **Pure headless (no X11) on Linux**, ideal for continuous integration systems. Also runs on Amazon EC2, Heroku, Iron.io.
- **Easy to install**: [Download](http://phantomjs.org/download.html), unpack, and start having fun in just 5 minutes.

## Ecosystem

PhantomJS needs not be used only as a stand-alone tool. Check also some excellent related projects:

- [CasperJS](http://casperjs.org) enables easy navigation scripting and common high-level testing.
- [Poltergeist](https://github.com/jonleighton/poltergeist) allows running Capybara tests headlessly.
- [Guard::Jasmine](https://github.com/netzpirat/guard-jasmine) automatically tests Jasmine specs on Rails when files are modified.
- [GhostDriver](http://github.com/detro/ghostdriver/) complements Selenium tests with a PhantomJS WebDriver implementation.
- [PhantomRobot](https://github.com/datakurre/phantomrobot) runs Robot Framework acceptance tests in the background via PhantomJS.

and many others [related projects](https://github.com/ariya/phantomjs/wiki/Related-Projects).

## Questions?

- Explore the complete [documentation](http://code.google.com/p/phantomjs/wiki/PhantomJS?tm=6).
- Read tons of [user articles](https://github.com/ariya/phantomjs/wiki/Buzz) on using PhantomJS.
- Join the [mailing-list](http://groups.google.com/group/phantomjs) and discuss with other PhantomJS fans.

PhantomJS is free software/open source, and is distributed under the [BSD license](http://opensource.org/licenses/BSD-3-Clause). It contains third-party code, see the included `third-party.txt` file for the license information on third-party code.

PhantomJS is created and maintained by [Ariya Hidayat](http://ariya.ofilabs.com/about) (Twitter: [@ariyahidayat](http://twitter.com/ariyahidayat)), with the help of [many contributors](https://github.com/ariya/phantomjs/contributors).

