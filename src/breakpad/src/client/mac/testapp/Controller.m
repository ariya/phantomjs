// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Breakpad/Breakpad.h>

#import "Controller.h"
#import "TestClass.h"
#import "GTMDefines.h"
#include <unistd.h>
#include <mach/mach.h>

@implementation Controller

- (void)causeCrash {
  float *aPtr = nil;
  NSLog(@"Crash!");
  NSLog(@"Bad programmer: %f", *aPtr);
}

- (void)generateReportWithoutCrash:(id)sender {
  BreakpadGenerateAndSendReport(breakpad_);
}

- (IBAction)showForkTestWindow:(id) sender {
  [forkTestOptions_ setIsVisible:YES];
}

- (IBAction)forkTestOptions:(id)sender {
  NSInteger tag = [[sender selectedCell] tag];
  NSLog(@"sender tag: %d", tag);
  if (tag <= 2) {
    bpForkOption = tag;
  }

  if (tag == 3) {
    useVFork = NO;
  }

  if (tag == 4) {
    useVFork = YES;
  }

  if (tag >= 5 && tag <= 7) {
    progCrashPoint = tag;
  }

}

- (IBAction)forkTestGo:(id)sender {

  NSString *resourcePath = [[NSBundle bundleForClass:
                                        [self class]] resourcePath];
  NSString *execProgname = nil;
  if (progCrashPoint == DURINGLAUNCH) {
    execProgname = [resourcePath stringByAppendingString:@"/crashduringload"];
  } else if (progCrashPoint == AFTERLAUNCH) {
    execProgname = [resourcePath stringByAppendingString:@"/crashInMain"];
  }

  const char *progName = NULL;
  if (progCrashPoint != BETWEENFORKEXEC) {
    progName = [execProgname UTF8String];
  }

  int pid;

  if (bpForkOption == UNINSTALL) {
    BreakpadRelease(breakpad_);
  }

  if (useVFork) {
    pid = vfork();
  } else {
    pid = fork();
  }

  if (pid == 0) {
    sleep(3);
    NSLog(@"Child continuing");
    FILE *fd = fopen("/tmp/childlog.txt","wt");
    kern_return_t kr;
    if (bpForkOption == RESETEXCEPTIONPORT) {
      kr = task_set_exception_ports(mach_task_self(),
                               EXC_MASK_BAD_ACCESS | EXC_MASK_BAD_INSTRUCTION |
                               EXC_MASK_ARITHMETIC | EXC_MASK_BREAKPOINT,
                               MACH_PORT_NULL,
                               EXCEPTION_DEFAULT,
                               THREAD_STATE_NONE);
      fprintf(fd,"task_set_exception_ports returned %d\n", kr);
    }

    if (progCrashPoint == BETWEENFORKEXEC) {
      fprintf(fd,"crashing post-fork\n");
      int *a = NULL;
      printf("%d\n",*a++);
    }

    fprintf(fd,"about to call exec with %s\n", progName);
    fclose(fd);
    int i = execl(progName, progName, NULL);
    fprintf(fd, "exec returned! %d\n", i);
    fclose(fd);
  }
}

- (IBAction)crash:(id)sender {
  NSInteger tag = [sender tag];

  if (tag == 1) {
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    [self performSelector:@selector(causeCrash) withObject:nil afterDelay:10.0];
    [sender setState:NSOnState];
    return;
  }

  if (tag == 2 && breakpad_) {
    BreakpadRelease(breakpad_);
    breakpad_ = NULL;
    return;
  }

  [self causeCrash];
}

- (void)anotherThread {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  TestClass *tc = [[TestClass alloc] init];

  [tc wait];

  [pool release];
}

- (void)awakeFromNib {
  NSBundle *bundle = [NSBundle mainBundle];
  NSDictionary *info = [bundle infoDictionary];


  breakpad_ = BreakpadCreate(info);

  // Do some unit tests with keys
  // first a series of bogus values
  BreakpadSetKeyValue(breakpad_, nil, @"bad2");
  BreakpadSetKeyValue(nil, @"bad3", @"bad3");

  // Now some good ones
  BreakpadSetKeyValue(breakpad_,@"key1", @"value1");
  BreakpadSetKeyValue(breakpad_,@"key2", @"value2");
  BreakpadSetKeyValue(breakpad_,@"key3", @"value3");

  // Look for a bogus one that we didn't try to set
  NSString *test = BreakpadKeyValue(breakpad_, @"bad4");
  if (test) {
    NSLog(@"Bad BreakpadKeyValue (bad4)");
  }

  // Look for a bogus one we did try to set
  test = BreakpadKeyValue(breakpad_, @"bad1");
  if (test) {
    NSLog(@"Bad BreakpadKeyValue (bad1)");
  }

  // Test some bad args for BreakpadKeyValue
  test = BreakpadKeyValue(nil, @"bad5");
  if (test) {
    NSLog(@"Bad BreakpadKeyValue (bad5)");
  }

  test = BreakpadKeyValue(breakpad_, nil);
  if (test) {
    NSLog(@"Bad BreakpadKeyValue (nil)");
  }

  // Find some we did set
  test = BreakpadKeyValue(breakpad_, @"key1");
  if (![test isEqualToString:@"value1"]) {
    NSLog(@"Can't find BreakpadKeyValue (key1)");
  }
  test = BreakpadKeyValue(breakpad_, @"key2");
  if (![test isEqualToString:@"value2"]) {
    NSLog(@"Can't find BreakpadKeyValue (key2)");
  }
  test = BreakpadKeyValue(breakpad_, @"key3");
  if (![test isEqualToString:@"value3"]) {
    NSLog(@"Can't find BreakpadKeyValue (key3)");
  }

  // Bad args for BreakpadRemoveKeyValue
  BreakpadRemoveKeyValue(nil, @"bad6");
  BreakpadRemoveKeyValue(breakpad_, nil);

  // Remove one that is valid
  BreakpadRemoveKeyValue(breakpad_, @"key3");

  // Try and find it
  test = BreakpadKeyValue(breakpad_, @"key3");
  if (test) {
    NSLog(@"Shouldn't find BreakpadKeyValue (key3)");
  }

  // Try and remove it again
  BreakpadRemoveKeyValue(breakpad_, @"key3");

  // Try removal by setting to nil
  BreakpadSetKeyValue(breakpad_,@"key2", nil);
  // Try and find it
  test = BreakpadKeyValue(breakpad_, @"key2");
  if (test) {
    NSLog(@"Shouldn't find BreakpadKeyValue (key2)");
  }

  BreakpadAddUploadParameter(breakpad_,
                             @"MeaningOfLife",
                             @"42");
  [NSThread detachNewThreadSelector:@selector(anotherThread)
                           toTarget:self withObject:nil];

  NSUserDefaults *args = [NSUserDefaults standardUserDefaults];

  // If the user specified autocrash on the command line, toggle
  // Breakpad to not confirm and crash immediately.  This is for
  // automated testing.
  if ([args boolForKey:@"autocrash"]) {
    BreakpadSetKeyValue(breakpad_,
                        @BREAKPAD_SKIP_CONFIRM,
                        @"YES");
    [self causeCrash];
  }

  progCrashPoint = DURINGLAUNCH;
  [window_ center];
  [window_ makeKeyAndOrderFront:self];
}

@end
