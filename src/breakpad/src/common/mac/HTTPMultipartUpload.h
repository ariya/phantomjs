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

// HTTPMultipartUpload: A multipart/form-data HTTP uploader.
// Each parameter pair is sent as a boundary
// Each file is sent with a name field in addition to the filename and data
// The data will be sent synchronously.

#import <Foundation/Foundation.h>

@interface HTTPMultipartUpload : NSObject {
 @protected
  NSURL *url_;                  // The destination URL (STRONG)
  NSDictionary *parameters_;    // The key/value pairs for sending data (STRONG)
  NSMutableDictionary *files_;  // Dictionary of name/file-path (STRONG)
  NSString *boundary_;          // The boundary string (STRONG)
  NSHTTPURLResponse *response_; // The response from the send (STRONG)
}

- (id)initWithURL:(NSURL *)url;

- (NSURL *)URL;

- (void)setParameters:(NSDictionary *)parameters;
- (NSDictionary *)parameters;

- (void)addFileAtPath:(NSString *)path name:(NSString *)name;
- (void)addFileContents:(NSData *)data name:(NSString *)name;
- (NSDictionary *)files;

// Set the data and return the response
- (NSData *)send:(NSError **)error;
- (NSHTTPURLResponse *)response;

@end
