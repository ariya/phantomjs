// Copyright (c) 2009, Google Inc.
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
//
// BreakpadFramework_Test.mm
// Test case file for Breakpad.h/mm.
//

#import "GTMSenTestCase.h"
#import "Breakpad.h"

#include <mach/mach.h>

@interface BreakpadFramework_Test : GTMTestCase {
 @private
  int last_exception_code_;
  int last_exception_type_;
  mach_port_t last_exception_thread_;
  // We're not using Obj-C BOOL because we need to interop with
  // Breakpad's callback.
  bool shouldHandleException_;
}

// This method is used by a callback used by test cases to determine
// whether to return true or false to Breakpad when handling an
// exception.
- (bool)shouldHandleException;
// This method returns a minimal dictionary that has what
// Breakpad needs to initialize.
- (NSMutableDictionary *)breakpadInitializationDictionary;
// This method is used by the exception handling callback
// to communicate to test cases the properites of the last
// exception.
- (void)setLastExceptionType:(int)type andCode:(int)code
                   andThread:(mach_port_t)thread;
@end

// Callback for Breakpad exceptions
bool myBreakpadCallback(int exception_type,
                        int exception_code,
                        mach_port_t crashing_thread,
                        void *context);

bool myBreakpadCallback(int exception_type,
                        int exception_code,
                        mach_port_t crashing_thread,
                        void *context) {
  BreakpadFramework_Test *testCaseClass =
    (BreakpadFramework_Test *)context;
  [testCaseClass setLastExceptionType:exception_type
                              andCode:exception_code
                            andThread:crashing_thread];
  bool shouldHandleException =
    [testCaseClass shouldHandleException];
  NSLog(@"Callback returning %d", shouldHandleException);
  return shouldHandleException;
}
const int kNoLastExceptionCode = -1;
const int kNoLastExceptionType = -1;
const mach_port_t kNoLastExceptionThread = MACH_PORT_NULL;

@implementation BreakpadFramework_Test
- (void) initializeExceptionStateVariables {
  last_exception_code_ = kNoLastExceptionCode;
  last_exception_type_ = kNoLastExceptionType;
  last_exception_thread_ = kNoLastExceptionThread;
}

- (NSMutableDictionary *)breakpadInitializationDictionary {
  NSMutableDictionary *breakpadParams =
    [NSMutableDictionary dictionaryWithCapacity:3];

  [breakpadParams setObject:@"UnitTests" forKey:@BREAKPAD_PRODUCT];
  [breakpadParams setObject:@"1.0" forKey:@BREAKPAD_VERSION];
  [breakpadParams setObject:@"http://staging" forKey:@BREAKPAD_URL];
  return breakpadParams;
}

- (bool)shouldHandleException {
  return shouldHandleException_;
}

- (void)setLastExceptionType:(int)type 
		     andCode:(int)code
                   andThread:(mach_port_t)thread {
  last_exception_type_ = type;
  last_exception_code_ = code;
  last_exception_thread_ = thread;
}

// Test that the parameters mark required actually enable Breakpad to
// be initialized.
- (void)testBreakpadInstantiationWithRequiredParameters {
  BreakpadRef b = BreakpadCreate([self breakpadInitializationDictionary]);
  STAssertNotNULL(b, @"BreakpadCreate failed with required parameters");
  BreakpadRelease(b);
}

// Test that Breakpad fails to initialize cleanly when required
// parameters are not present.
- (void)testBreakpadInstantiationWithoutRequiredParameters {
  NSMutableDictionary *breakpadDictionary =
    [self breakpadInitializationDictionary];

  // Skip setting version, so that BreakpadCreate fails.
  [breakpadDictionary removeObjectForKey:@BREAKPAD_VERSION];
  BreakpadRef b = BreakpadCreate(breakpadDictionary);
  STAssertNULL(b, @"BreakpadCreate did not fail when missing a required"
               " parameter!");

  breakpadDictionary = [self breakpadInitializationDictionary];
  // Now test with no product
  [breakpadDictionary removeObjectForKey:@BREAKPAD_PRODUCT];
  b = BreakpadCreate(breakpadDictionary);
  STAssertNULL(b, @"BreakpadCreate did not fail when missing a required"
               " parameter!");

  breakpadDictionary = [self breakpadInitializationDictionary];
  // Now test with no URL
  [breakpadDictionary removeObjectForKey:@BREAKPAD_URL];
  b = BreakpadCreate(breakpadDictionary);
  STAssertNULL(b, @"BreakpadCreate did not fail when missing a required"
               " parameter!");
  BreakpadRelease(b);
}

// Test to ensure that when we call BreakpadAddUploadParameter,
// it's added to the dictionary correctly(this test depends on
// some internal details of Breakpad, namely, the special prefix
// that it uses to figure out which key/value pairs to upload).
- (void)testAddingBreakpadServerVariable {
  NSMutableDictionary *breakpadDictionary =
    [self breakpadInitializationDictionary];

  BreakpadRef b = BreakpadCreate(breakpadDictionary);
  STAssertNotNULL(b, @"BreakpadCreate failed with valid dictionary!");

  BreakpadAddUploadParameter(b,
                             @"key",
                             @"value");

  // Test that it did not add the key/value directly, e.g. without
  // prepending the key with the prefix.
  STAssertNil(BreakpadKeyValue(b, @"key"),
              @"AddUploadParameter added key directly to dictionary"
              " instead of prepending it!");

  NSString *prependedKeyname =
    [@BREAKPAD_SERVER_PARAMETER_PREFIX stringByAppendingString:@"key"];
    
  STAssertEqualStrings(BreakpadKeyValue(b, prependedKeyname),
                       @"value",
                       @"Calling BreakpadAddUploadParameter did not prepend "
                       "key name");
  BreakpadRelease(b);
}

// Test that when we do on-demand minidump generation,
// the exception code/type/thread are set properly.
- (void)testFilterCallbackReturnsFalse {
  NSMutableDictionary *breakpadDictionary =
    [self breakpadInitializationDictionary];

  BreakpadRef b = BreakpadCreate(breakpadDictionary);
  STAssertNotNULL(b, @"BreakpadCreate failed with valid dictionary!");
  BreakpadSetFilterCallback(b, &myBreakpadCallback, self);

  // This causes the callback to return false, meaning
  // Breakpad won't take the exception
  shouldHandleException_ = false;

  [self initializeExceptionStateVariables];
  STAssertEquals(last_exception_type_, kNoLastExceptionType,
                 @"Last exception type not initialized correctly.");
  STAssertEquals(last_exception_code_, kNoLastExceptionCode,
                 @"Last exception code not initialized correctly.");
  STAssertEquals(last_exception_thread_, kNoLastExceptionThread,
                 @"Last exception thread is not initialized correctly.");

  // Cause Breakpad's exception handler to be invoked.
  BreakpadGenerateAndSendReport(b);

  STAssertEquals(last_exception_type_, 0,
                 @"Last exception type is not 0 for on demand");
  STAssertEquals(last_exception_code_, 0,
                 @"Last exception code is not 0 for on demand");
  STAssertEquals(last_exception_thread_, mach_thread_self(),
                 @"Last exception thread is not mach_thread_self() "
                 "for on demand");
}

@end
