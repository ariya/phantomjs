// Copyright (c) 2011, Google Inc.
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
// This component uses the HTTPMultipartUpload of the breakpad project to send
// the minidump and associated data to the crash reporting servers.
// It will perform throttling based on the parameters passed to it and will
// prompt the user to send the minidump.

#include <Foundation/Foundation.h>

#import "common/mac/GTMDefines.h"

#define kClientIdPreferenceKey @"clientid"

extern NSString *const kGoogleServerType;
extern NSString *const kSocorroServerType;
extern NSString *const kDefaultServerType;

@interface Uploader : NSObject {
 @private
  NSMutableDictionary *parameters_;        // Key value pairs of data (STRONG)
  NSData *minidumpContents_;               // The data in the minidump (STRONG)
  NSData *logFileData_;                    // An NSdata for the tar,
                                           // bz2'd log file.
  NSMutableDictionary *serverDictionary_;  // The dictionary mapping a
                                           // server type name to a
                                           // dictionary of server
                                           // parameter names.
  NSMutableDictionary *socorroDictionary_; // The dictionary for
                                           // Socorro.
  NSMutableDictionary *googleDictionary_;  // The dictionary for
                                           // Google.
  NSMutableDictionary *extraServerVars_;   // A dictionary containing
                                           // extra key/value pairs
                                           // that are uploaded to the
                                           // crash server with the
                                           // minidump.
}

- (id)initWithConfigFile:(const char *)configFile;

- (id)initWithConfig:(NSDictionary *)config;

- (NSMutableDictionary *)parameters;

- (void)report;

// Upload the given data to the crash server.
- (void)uploadData:(NSData *)data name:(NSString *)name;

// This method adds a key/value pair to the dictionary that
// will be uploaded to the crash server.
- (void)addServerParameter:(id)value forKey:(NSString *)key;

@end
