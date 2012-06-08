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

// minidump_upload.cc: Upload a minidump to a HTTP server.
// The upload is sent as a multipart/form-data POST request with
// the following parameters:
//  prod: the product name
//  ver: the product version
//  symbol_file: the breakpad format symbol file

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "common/linux/http_upload.h"

using google_breakpad::HTTPUpload;

struct Options {
  std::string minidumpPath;
  std::string uploadURLStr;
  std::string product;
  std::string version;
  std::string proxy;
  std::string proxy_user_pwd;
  bool success;
};

//=============================================================================
static void Start(Options *options) {
  std::map<std::string, std::string> parameters;
  // Add parameters
  parameters["prod"] = options->product;
  parameters["ver"] = options->version;

  // Send it
  std::string response, error;
  bool success = HTTPUpload::SendRequest(options->uploadURLStr,
                                         parameters,
                                         options->minidumpPath,
                                         "upload_file_minidump",
                                         options->proxy,
                                         options->proxy_user_pwd,
                                         "",
                                         &response,
                                         NULL,
                                         &error);

  if (success) {
    printf("Successfully sent the minidump file.\n");
  } else {
    printf("Failed to send minidump: %s\n", error.c_str());
    printf("Response:\n");
    printf("%s\n", response.c_str());
  }
  options->success = success;
}

//=============================================================================
static void
Usage(int argc, const char *argv[]) {
  fprintf(stderr, "Submit minidump information.\n");
  fprintf(stderr, "Usage: %s [options...] -p <product> -v <version> <minidump> "
          "<upload-URL>\n", argv[0]);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "<minidump> should be a minidump.\n");
  fprintf(stderr, "<upload-URL> is the destination for the upload\n");

  fprintf(stderr, "-p:\t <product> Product name\n");
  fprintf(stderr, "-v:\t <version> Product version\n");
  fprintf(stderr, "-x:\t <host[:port]> Use HTTP proxy on given port\n");
  fprintf(stderr, "-u:\t <user[:password]> Set proxy user and password\n");
  fprintf(stderr, "-h:\t Usage\n");
  fprintf(stderr, "-?:\t Usage\n");
}

//=============================================================================
static void
SetupOptions(int argc, const char *argv[], Options *options) {
  extern int optind;
  char ch;

  while ((ch = getopt(argc, (char * const *)argv, "p:u:v:x:h?")) != -1) {
    switch (ch) {
      case 'p':
        options->product = optarg;
        break;
      case 'u':
        options->proxy_user_pwd = optarg;
        break;
      case 'v':
        options->version = optarg;
        break;
      case 'x':
        options->proxy = optarg;
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

  options->minidumpPath = argv[optind];
  options->uploadURLStr = argv[optind + 1];
}

//=============================================================================
int main (int argc, const char * argv[]) {
  Options options;
  SetupOptions(argc, argv, &options);
  Start(&options);
  return options.success ? 0 : 1;
}
