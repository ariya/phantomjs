---
layout: post
title: Crash Reporting
categories: docs docs-contribute
permalink: crash-reporting.html
---

This page contains information about what to do if PhantomJS crashes.

You will have seen a message like this:

> PhantomJS has crashed. Please file a bug report at https://github.com/ariya/phantomjs/issues/new
> and attach the crash dump file: /tmp/598e080c-58dd-e183-12fb7ba8-29a93fff.dmp"

## If you're using an official binary ([downloaded](https://bitbucket.org/ariya/phantomjs/downloads) or installed via Homebrew)

You can post the crash dump to an issue, but it will help us solve your problem more quickly if you convert the crash dump to a stack trace. Here's how.

### Linux

1. Obtain the appropriate symbol files from the [downloads page](https://bitbucket.org/ariya/phantomjs/downloads). Make sure to get the correct tarball which matches the binary you are running.

2. Extract the tarball and cd into the directory. (`tar -xjvf phantomjs-[version]-symbols.tar.bz2`)

3. Run:

    `./minidump_stackwalk /tmp/598e080c-58dd-e183-12fb7ba8-29a93fff.dmp . 2>/dev/null`

   Obviously, substitute the path to your own crash dump.

   If there is no output at all, remove the error output redirect and look for permission issues.

4. You should see a trace containing lines like:

```
11 libQtWebKit.so.4!WebCore::FrameLoader::load [FrameLoader.cpp : 1460 + 0x3c]
    rbx = 0x00007f423800b640 r12 = 0x00007fff928334a0
    r13 = 0x00007fff92833880 r14 = 0x0000000000000000
    r15 = 0x0000000000000000 rip = 0x00007f42444e9261
    rsp = 0x00007fff92832d60 rbp = 0x00007fff92832e70
    Found by: call frame info
```
If your trace just contains addresses, e.g.:

```
 0  libQtWebKit.so.4.9.2 + 0x13f0b49
    rbx = 0x00007fff9e3dbb40   r12 = 0x00007fff9e3dbb40
    r13 = 0x000000000041bcd0   r14 = 0x0000000000000000
    r15 = 0x0000000000000000   rip = 0x00007fee477dfb49
    rsp = 0x00007fff9e3dba90   rbp = 0x0000000002397890
    Found by: given as instruction pointer in context
```

Then there is no point posting it - it is not useful for debugging in this form. (But this shouldn't happen if you are using an official binary with the correct symbol files.)

### OS X

At the moment the process for obtaining OS X stack traces is not ideal. It would be great if someone could work on this.

In the mean time, the following steps can be used to obtain a somewhat useful trace. These steps must be completed on a Linux OS -- it's not currently possible to analyse an OS X crash dump on OS X. This is because we don't have a way of building the minidump_stackwalk program on OS X yet. You can perform the steps in a linux virtual machine, or any linux environment that you can put your .dmp file on.

**You only need to do steps 1 - 6 the first time you get a stack trace. After that, you can skip to step 7**

1. Obtain the macosx symbol files from the [downloads page](https://bitbucket.org/ariya/phantomjs/downloads). Also download the equivalent linux symbol files, as you will need to copy the minidump_stackwalk program from here.

2. Extract both tarballs and cd into the macosx symbols directory.

3. Copy the `minidump_stackwalk` binary from the linux symbols directory to the mac osx symbols directory.

4. Run:

   `./minidump_stackwalk /tmp/598e080c-58dd-e183-12fb7ba8-29a93fff.dmp . 2>&1 | egrep "No symbol file at .*phantomjs"`

   Obviously, substitute the path to your own crash dump.

5. You'll see something like:

   ```
   2012-08-04 14:18:40: simple_symbol_supplier.cc:192: INFO: No symbol file at ./phantomjs/D41D8CD98F00B204E9800998ECF8427E0/phantomjs.sym
   2012-08-04 14:18:40: simple_symbol_supplier.cc:192: INFO: No symbol file at ./phantomjs/D41D8CD98F00B204E9800998ECF8427E0/phantomjs.sym
   2012-08-04 14:18:40: simple_symbol_supplier.cc:192: INFO: No symbol file at ./phantomjs/D41D8CD98F00B204E9800998ECF8427E0/phantomjs.sym
   2012-08-04 14:18:40: simple_symbol_supplier.cc:192: INFO: No symbol file at ./phantomjs/D41D8CD98F00B204E9800998ECF8427E0/phantomjs.sym
   2012-08-04 14:18:40: simple_symbol_supplier.cc:192: INFO: No symbol file at ./phantomjs/D41D8CD98F00B204E9800998ECF8427E0/phantomjs.sym
   ```

   Copy the hash value (D41D8CD98F00B204E9800998ECF8427E0)

6. Run:

   cp -r phantomjs/\`ls phantomjs\` phantomjs/D41D8CD98F00B204E9800998ECF8427E0

   (Substitute in your own hash value if it is different.)

7. **You only need to perform the above steps the first time.** To get the stack trace run:

   `./minidump_stackwalk /tmp/598e080c-58dd-e183-12fb7ba8-29a93fff.dmp . 2>/dev/null`

   Obviously, substitute the path to your own crash dump.

8. You should see a trace containing lines like:

   ```
   0  phantomjs!__ZN7WebCoreL15requiresLineBoxERKNS_14InlineIteratorERKNS_8LineInfoE + 0x35
      eip = 0x009fd2f5   esp = 0xbfffc820   ebp = 0xbfffc848   ebx = 0xbfffcdcc
      esi = 0x0bd8e9b8   edi = 0xbfffcfe4   eax = 0x00000000   ecx = 0xbfffcdcc
      edx = 0x0bd8e9b8   efl = 0x00210246
      Found by: given as instruction pointer in context
   ```

   These show the [mangled names](https://en.wikipedia.org/wiki/Name_mangling) of the stack frames.

9. It is suggested you write the output to a file and attach it to your ticket.

   `./minidump_stackwalk /tmp/598e080c-58dd-e183-12fb7ba8-29a93fff.dmp . 2>/dev/null > dump.txt`

## If you're not using an official binary

If you did not use an official binary, the crash dump is useless. So please don't post it on an issue if you compiled from source. Do one of the following instead.

### Obtaining a stack trace from gdb

If you're building with `./build.sh`, and have an easily reproducible crash, this might be easiest.

1. Build with debugging symbols enabled:

   `CFLAGS=-g CXXFLAGS=-g ./build.sh --qt-config '-webkit-debug' --qmake-args "QMAKE_CFLAGS=-g QMAKE_CXXFLAGS=-g"`

2. Start gdb with phantomjs:

   `gdb bin/phantomjs`

3. This will give you the "gdb console". If you normally run "bin/phantomjs my_script.js", then in the gdb console you should substitute "bin/phantomjs" for "run":

   `run my_script.js`

4. This will run the program until it crashes. When this happens, you can type "bt" and gdb will print out a backtrace. Copy this into a bug report.

### Creating your own symbol files

If you have a crash that only happens occasionally and is hard to reproduce, it might be better to create your own symbol files. Then you can follow the instructions under "If you're using an official binary", above.

1. Use the `deploy/build-and-package.sh` script to build PhantomJS.

2. There will be two archives created - one containing PhantomJS and one containing symbol files.

3. Simply use the PhantomJS binary until you get a crash dump, and then use the instructions above with your own symbol files to get a stack trace.
