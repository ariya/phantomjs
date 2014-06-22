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

// symupload.m: Upload a symbol file to a HTTP server.  The upload is sent as
// a multipart/form-data POST request with the following parameters:
//  code_file: the basename of the module, e.g. "app"
//  debug_file: the basename of the debugging file, e.g. "app"
//  debug_identifier: the debug file's identifier, usually consisting of
//                    the guid and age embedded in the pdb, e.g.
//                    "11111111BBBB3333DDDD555555555555F"
//  os: the operating system that the module was built for
//  cpu: the CPU that the module was built for (x86 or ppc)
//  symbol_file: the contents of the breakpad-format symbol file

#include <unistd.h>

#include <Foundation/Foundation.h>
#include "HTTPMultipartUpload.h"

typedef struct {
  NSString *symbolsPath;
  NSString *uploadURLStr;
  BOOL success;
} Options;

//=============================================================================
static NSArray *ModuleDataForSymbolFile(NSString *file) {
  NSFileHandle *fh = [NSFileHandle fileHandleForReadingAtPath:file];
  NSData *data = [fh readDataOfLength:1024];
  NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  NSScanner *scanner = [NSScanner scannerWithString:str];
  NSString *line;
  NSMutableArray *parts = nil;
  const int MODULE_ID_INDEX = 3;
  
  if ([scanner scanUpToString:@"\n" intoString:&line]) {
    parts = [[NSMutableArray alloc] init];
    NSScanner *moduleInfoScanner = [NSScanner scannerWithString:line];
    NSString *moduleInfo;
    // Get everything BEFORE the module name.  None of these properties
    // can have spaces.
    for (int i = 0; i <= MODULE_ID_INDEX; i++) {
      [moduleInfoScanner scanUpToString:@" " intoString:&moduleInfo];
      [parts addObject:moduleInfo];
    }

    // Now get the module name. This can have a space so we scan to
    // the end of the line.
    [moduleInfoScanner scanUpToString:@"\n" intoString:&moduleInfo];
    [parts addObject:moduleInfo];
  }

  [str release];

  return parts;
}

//=============================================================================
static NSString *CompactIdentifier(NSString *uuid) {
  NSMutableString *str = [NSMutableString stringWithString:uuid];
  [str replaceOccurrencesOfString:@"-" withString:@"" options:0
                            range:NSMakeRange(0, [str length])];
  
  return str;
}

//=============================================================================
static void Start(Options *options) {
  NSURL *url = [NSURL URLWithString:options->uploadURLStr];
  HTTPMultipartUpload *ul = [[HTTPMultipartUpload alloc] initWithURL:url];
  NSMutableDictionary *parameters = [NSMutableDictionary dictionary];
  NSArray *moduleParts = ModuleDataForSymbolFile(options->symbolsPath);
  NSMutableString *compactedID =
    [NSMutableString stringWithString:[moduleParts objectAtIndex:3]];
  [compactedID replaceOccurrencesOfString:@"-" withString:@"" options:0
                                    range:NSMakeRange(0, [compactedID length])];

  // Add parameters
  [parameters setObject:compactedID forKey:@"debug_identifier"];

  // MODULE <os> <cpu> <uuid> <module-name>
  // 0      1    2     3      4
  [parameters setObject:[moduleParts objectAtIndex:1] forKey:@"os"];
  [parameters setObject:[moduleParts objectAtIndex:2] forKey:@"cpu"];
  [parameters setObject:[moduleParts objectAtIndex:4] forKey:@"debug_file"];
  [parameters setObject:[moduleParts objectAtIndex:4] forKey:@"code_file"];
  [ul setParameters:parameters];
  
  NSArray *keys = [parameters allKeys];
  int count = [keys count];
  for (int i = 0; i < count; ++i) {
    NSString *key = [keys objectAtIndex:i];
    NSString *value = [parameters objectForKey:key];
    fprintf(stdout, "'%s' = '%s'\n", [key UTF8String], 
            [value UTF8String]);
  }

  // Add file
  [ul addFileAtPath:options->symbolsPath name:@"symbol_file"];

  // Send it
  NSError *error = nil;
  NSData *data = [ul send:&error];
  NSString *result = [[NSString alloc] initWithData:data
                                           encoding:NSUTF8StringEncoding];
  int status = [[ul response] statusCode];

  fprintf(stdout, "Send: %s\n", error ? [[error description] UTF8String] :
          "No Error");
  fprintf(stdout, "Response: %d\n", status);
  fprintf(stdout, "Result: %lu bytes\n%s\n",
          (unsigned long)[data length], [result UTF8String]);

  [result release];
  [ul release];
  options->success = !error && status==200;
}

//=============================================================================
static void
Usage(int argc, const char *argv[]) {
  fprintf(stderr, "Submit symbol information.\n");
  fprintf(stderr, "Usage: %s <symbols> <upload-URL>\n", argv[0]);
  fprintf(stderr, "<symbols> should be created by using the dump_syms tool.\n");
  fprintf(stderr, "<upload-URL> is the destination for the upload\n");
  fprintf(stderr, "\t-h: Usage\n");
  fprintf(stderr, "\t-?: Usage\n");
}

//=============================================================================
static void
SetupOptions(int argc, const char *argv[], Options *options) {
  extern int optind;
  char ch;

  while ((ch = getopt(argc, (char * const *)argv, "h?")) != -1) {
    switch (ch) {
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

  options->symbolsPath = [NSString stringWithUTF8String:argv[optind]];
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
