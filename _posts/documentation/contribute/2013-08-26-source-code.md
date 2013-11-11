---
layout: post
title: Source Code
categories: docs docs-contribute
permalink: source-code.html
---

While the main repository is hosted on GitHub, the source code can be obtained from other repositories. If GitHub is down or slow, check those other Git remotes. Non-GitHub online viewers also support side-by-side commit view.

### GitHub

Checkout: git clone git://github.com/ariya/phantomjs.git

Source tree: https://github.com/ariya/phantomjs

Commit logs: https://github.com/ariya/phantomjs/commits/master

### Google Code

Checkout: git clone https://code.google.com/p/phantomjs/

Source tree: http://code.google.com/p/phantomjs/source/browse/

Commit logs: http://code.google.com/p/phantomjs/source/list

### Gitorious

Checkout: git clone git://gitorious.org/ofi-labs/phantomjs.git

Source tree: http://gitorious.org/ofi-labs/phantomjs/trees/master

Commit logs: http://gitorious.org/ofi-labs/phantomjs/commits/master

### BitBucket

Checkout: git clone https://bitbucket.org/ariya/phantomjs.git

Source tree: http://bitbucket.org/ariya/phantomjs/src

Commit logs: http://bitbucket.org/ariya/phantomjs/changesets

### Building from source

After getting the source code in one of the ways above, the build can be done with the ./build.sh script. This asks for confirmation, then does a parallel make with as many threads as you have CPU cores. It is recommended to use as many as you can.

The default build is optimised with minimal debug info. If you want to debug the binary, you can build a debug binary using the following command (at least it worked for me)

```bash
CONFIG=debug ./build.sh --qt-config "-debug -webkit-debug"
```

The resulting binary is likely to be very large and use a lot of memory to link. I recommend a minimum of 4Gb of physical memory on a x86_64 system.
