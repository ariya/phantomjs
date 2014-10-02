# [PhantomJS](http://phantomjs.org) - Scriptable Headless WebKit

PhantomJS ([www.phantomjs.org](http://phantomjs.org)) is a headless WebKit scriptable with JavaScript. It is used by hundreds of [developers](http://phantomjs.org/buzz.html) and dozens of [organizations](http://phantomjs.org/users.html) for web-related development workflow.

The latest [stable release](http://phantomjs.org/release-1.9.html) is version 1.9 (codenamed <a href="http://phantomjs.org/release-names.html">"Sakura"</a>). Follow the official Twitter stream [@PhantomJS](http://twitter.com/PhantomJS) to get the frequent development updates.

The next major version, PhantomJS 2, is a significant upgrade. It is still in [heavy development](https://github.com/ariya/phantomjs/wiki/PhantomJS-2). There is **no timeline** for the release yet, please monitor the [mailing-list](https://groups.google.com/forum/#!forum/phantomjs) for the progress.

**Note**: Please **do not** create a GitHub pull request **without** reading the [Contribution Guide](https://github.com/ariya/phantomjs/blob/master/CONTRIBUTING.md) first. Failure to do so may result in the rejection of the pull request.

## Use Cases

- **Headless web testing**. Lightning-fast testing without the browser is now possible! Various [test frameworks](http://phantomjs.org/headless-testing.html) such as Jasmine, Capybara, QUnit, Mocha, WebDriver, YUI Test, BusterJS, FuncUnit, Robot Framework, and many others are supported.
- **Page automation**. [Access and manipulate](http://phantomjs.org/page-automation.html) web pages with the standard DOM API, or with usual libraries like jQuery.
- **Screen capture**. Programmatically [capture web contents](http://phantomjs.org/screen-capture.html), including CSS, SVG and Canvas. Build server-side web graphics apps, from a screenshot service to a vector chart rasterizer.
- **Network monitoring**. Automate performance analysis, track [page loading](http://phantomjs.org/network-monitoring.html) and export as standard HAR format.

## Features

- **Multiplatform**, available on major operating systems: Windows, Mac OS X, Linux, and other Unices.
- **Fast and native implementation** of web standards: DOM, CSS, JavaScript, Canvas, and SVG. No emulation!
- **Pure headless (no X11) on Linux**, ideal for continuous integration systems. Also runs on Amazon EC2, Heroku, and Iron.io.
- **Easy to install**: [Download](http://phantomjs.org/download.html), unpack, and start having fun in just 5 minutes.

## Ecosystem

PhantomJS needs not be used only as a stand-alone tool. Check also some excellent related projects:

- [CasperJS](http://casperjs.org) enables easy navigation scripting and common high-level testing.
- [Poltergeist](https://github.com/jonleighton/poltergeist) allows running Capybara tests headlessly.
- [Guard::Jasmine](https://github.com/netzpirat/guard-jasmine) automatically tests Jasmine specs on Rails when files are modified.
- [GhostDriver](http://github.com/detro/ghostdriver/) complements Selenium tests with a PhantomJS WebDriver implementation.
- [PhantomRobot](https://github.com/datakurre/phantomrobot) runs Robot Framework acceptance tests in the background via PhantomJS.
- [Mocha-PhantomJS](https://github.com/metaskills/mocha-phantomjs) run Mocha tests using PhantomJS.

and many others [related projects](http://phantomjs.org/related-projects.html).

## Questions?

- Explore the complete [documentation](http://phantomjs.org/documentation/).
- Read tons of [user articles](http://phantomjs.org/buzz.html) on using PhantomJS.
- Join the [mailing-list](http://groups.google.com/group/phantomjs) and discuss with other PhantomJS fans.

PhantomJS is free software/open source, and is distributed under the [BSD license](http://opensource.org/licenses/BSD-3-Clause). It contains third-party code, see the included `third-party.txt` file for the license information on third-party code.

PhantomJS is created and maintained by [Ariya Hidayat](http://ariya.ofilabs.com/about) (Twitter: [@ariyahidayat](http://twitter.com/ariyahidayat)), with the help of [many contributors](https://github.com/ariya/phantomjs/contributors).

