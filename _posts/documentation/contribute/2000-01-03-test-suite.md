---
layout: post
title: Test Suite
categories: docs docs-contribute
permalink: test-suite.html
---

## Unit Tests

Run the test driver as follows:

```bash
cd test/
../bin/phantomjs run-tests.js
```

As of now, the tests are still very basic. The coverage of the tests will be expanded with time.

## Manual Tests

Note 1: tests marked with + (plus) requires an Internet connection.

### A. Basic sanity checks

Approximate test duration: 2 minutes.

**1. Hello, world**

Command to run: `phantomjs examples/hello.js`

Expected output:

```bash
Hello, world!
```

**2. Version number**

Command to run: `phantomjs examples/version.js`

Expected output:

```bash
using PhantomJS version 1.8.0
```

**3. Script arguments**

Command to run: `phantomjs examples/arguments.js foo bar baz`

Expected output:

```bash
0: foo
1: bar
2: baz
```

**4. Fibonacci series**

Command to run: `phantomjs examples/fibo.js`

Expected output:

```bash
1
1
2
3
5
8
13
21
34
```

**5. Timed countdown**

Command to run: `phantomjs examples/countdown.js`

Expected output (each number shows up 1 second after the previous):

```bash
10
9
8
7
6
5
4
3
2
1
```

### B. Rendering checks
Approximate test duration: 2 minutes.

**1. Color wheel using canvas**

Command to run: `phantomjs examples/colorwheel.js`

Expected output is a file colorwheel.png, it is a PNG image, 400x400 pixel that looks like:

![colorwheel](https://lh3.googleusercontent.com/_Oijhf1ZPv-4/TVzeP4NPMDI/AAAAAAAAB10/FhFzcvQLXLw/s800/colorwheel.png)

**2. Snapshot of Google News Mobile (+)**

Command to run: `phantomjs examples/technews.js`

Expected output is a file `technews.png`, it is PNG image, 320px wide and can be up to 3500px long.
