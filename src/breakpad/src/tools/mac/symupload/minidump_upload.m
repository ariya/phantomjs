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

// minidump_upload.m: Upload a minidump to a HTTP server.  The upload is sent as
// a multipart/form-data POST request with the following parameters:
//  prod: the product name
//  ver: the product version
//  symbol_file: the breakpad format symbol file

#import <unistd.h>

#import <Foundation/Foundation.h>

#import "common/mac/HTTPMultipartUpload.h"

typedef struct {
  NSString *minidumpPath;
  NSString *uploadURLStr;
  NSString *product;
  NSString *version;
  BOOL success;
} Options;

//=============================================================================
static void Start(Options *options) {
  NSURL *url = [NSURL URLWithString:options->uploadURLStr];
  HTTPMultipartUpload *ul = [[HTTPMultipartUpload alloc] initWithURL:url];
  NSMutableDictionary *parameters = [NSMutableDictionary dictionary];

  // Add parameters
  [parameters setObject:options->product forKey:@"prod"];
  [parameters setObject:options->version forKey:@"ver"];
  [ul setParameters:parameters];

  // Add file
  [ul addFileAtPath:options->minidumpPath name:@"upload_file_minidump"];

  // Send it
  NSError *error = nil;
  NSData *data = [ul send:&error];
  NSString *result = [[NSString alloc] initWithData:data
                                           encoding:NSUTF8StringEncoding];

  NSLog(@"Send: %@", error ? [error description] : @"No Error");
  NSLog(@"Response: %d", [[ul response] statusCode]);
  NSLog(@"Result: %d bytes\n%@", [data length], result);

  [result release];
  [ul release];
  options->success = !error;
}

//=============================================================================
static void
Usage(int argc, const char *argv[]) {
  fprintf(stderr, "Submit minidump information.\n");
  fprintf(stderr, "Usage: %s -p <product> -v <version> <minidump> "
          "<upload-URL>\n", argv[0]);
  fprintf(stderr, "<minidump> should be a minidump.\n");
  fprintf(stderr, "<upload-URL> is the destination for the upload\n");

  fprintf(stderr, "\t-h: Usage\n");
  fprintf(stderr, "\t-?: Usage\n");
}

//=============================================================================
static void
SetupOptions(int argc, const char *argv[], Options *options) {
  extern int optind;
  char ch;

  while ((ch = getopt(argc, (char * const *)argv, "p:v:h?")) != -1) {
    switch (ch) {
      case 'p':
        options->product = [NSString stringWithUTF8String:optarg];
        break;
      case 'v':
        options->version = [NSString stringWithUTF8String:optarg];
        break;

      default:
        Usage(argc, argv);
        exit(0);
        break;
    }
  }

  if ((argc - optind) != 2) {
    fprintf(stderr, "%s: Missing symbols file and/or upload-URL\n", argv[0]);
    Usage(argc, argv);
    exit(1);
  }

  options->minidumpPath = [NSString stringWithUTF8String:argv[optind]];
  options->uploadURLStr = [NSString stringWithUTF8String:argv[optind + 1]];
}

//=============================================================================
int main (int argc, const char * argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  Options options;

  bzero(&options, sizeof(Options));
  SetupOptions(argc, argv, &options);
  Start(&options);

  [pool release];
  return options.success ? 0 : 1;
}
