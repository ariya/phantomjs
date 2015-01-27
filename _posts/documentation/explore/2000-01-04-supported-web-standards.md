---
layout: post
title: Supported Web Standards
categories: docs docs-explore
permalink: supported-web-standards.html
---

PhantomJS uses [QtWebKit](https://trac.webkit.org/wiki/QtWebKit). It supports many features which are part of [http://trac.webkit.org/wiki/QtWebKitFeatures22](http://trac.webkit.org/wiki/QtWebKitFeatures22).

## Unsupported Features

Support for **plugins** (such as Flash) was dropped a long time ago. The primary reasons:

* Pure headless (no X11) makes it impossible to have windowed plugin
* Issues and bugs are hard to debug, due to the proprietary nature of such plugins (binary blobs)

The following features, due to the nature of PhantomJS, are irrelevant:

**WebGL** would require an OpenGL-capable system. Since the goal of PhantomJS is to become 100% headless and self-contained, this is not acceptable. Using OpenGL emulation via Mesa can overcome the limitation, but then the performance would degrade.

**Video and Audio** would require shipping a variety of different codecs.

**CSS 3-D** needs a perspective-correct implementation of texture mapping. It can't be implemented without a penalty in performance.

Each of the above feature may be supported in the future if the technical challenges associated with the implementations are solved. Until then, do not rely on those features.

## Not Tested Features

XPath.

## Detecting Features

Using WebKit version and compare it against other WebKit-based browser is not encouraged. The reason is that every WebKit implementation may have varying support due to its architecture of having [interface/ abstraction layer](http://ariya.ofilabs.com/2011/06/your-webkit-port-is-special-just-like-every-other-port.html).

The best way to find out if a certain feature is supported or not is via feature detection, for example by using a library like [Modernizr](http://www.modernizr.com/docs/#s2). The included `examples/features.js` illustrates the use of Modernizr and dumps all the detected features, both supported and not supported.

Please note that although a certain feature might be supported, there is no real guarantee that it is 100% supported. You still have run your own extensive tests to make sure that the support level is up to what you need.
