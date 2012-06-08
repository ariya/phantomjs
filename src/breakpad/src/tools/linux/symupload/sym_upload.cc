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

// symupload.cc: Upload a symbol file to a HTTP server.  The upload is sent as
// a multipart/form-data POST request with the following parameters:
//  code_file: the basename of the module, e.g. "app"
//  debug_file: the basename of the debugging file, e.g. "app"
//  debug_identifier: the debug file's identifier, usually consisting of
//                    the guid and age embedded in the pdb, e.g.
//                    "11111111BBBB3333DDDD555555555555F"
//  version: the file version of the module, e.g. "1.2.3.4"
//  os: the operating system that the module was built for
//  cpu: the CPU that the module was built for
//  symbol_file: the contents of the breakpad-format symbol file

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "common/linux/http_upload.h"

using google_breakpad::HTTPUpload;

typedef struct {
  std::string symbolsPath;
  std::string uploadURLStr;
  std::string proxy;
  std::string proxy_user_pwd;
  std::string version;
  bool success;
} Options;

static void TokenizeByChar(const std::string &source_string,
              int c, std::vector<std::string> *results) {
  assert(results);
  std::string::size_type cur_pos = 0, next_pos = 0;
  while ((next_pos = source_string.find(c, cur_pos)) != std::string::npos) {
    if (next_pos != cur_pos)
      results->push_back(source_string.substr(cur_pos, next_pos - cur_pos));
    cur_pos = next_pos + 1;
  }
  if (cur_pos < source_string.size() && next_pos != cur_pos)
    results->push_back(source_string.substr(cur_pos));
}

//=============================================================================
// Parse out the module line which have 5 parts.
// MODULE <os> <cpu> <uuid> <module-name>
static bool ModuleDataForSymbolFile(const std::string &file,
                                    std::vector<std::string> *module_parts) {
  assert(module_parts);
  const size_t kModulePartNumber = 5;
  FILE *fp = fopen(file.c_str(), "r");
  if (fp) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), fp)) {
      std::string line(buffer);
      std::string::size_type line_break_pos = line.find_first_of('\n');
      if (line_break_pos == std::string::npos) {
        assert(!"The file is invalid!");
        fclose(fp);
        return false;
      }
      line.resize(line_break_pos);
      const char kDelimiter = ' ';
      TokenizeByChar(line, kDelimiter, module_parts);
      if (module_parts->size() != kModulePartNumber)
        module_parts->clear();
    }
    fclose(fp);
  }

  return module_parts->size() == kModulePartNumber;
}

//=============================================================================
static std::string CompactIdentifier(const std::string &uuid) {
  std::vector<std::string> components;
  TokenizeByChar(uuid, '-', &components);
  std::string result;
  for (size_t i = 0; i < components.size(); ++i)
    result += components[i];
  return result;
}

//=============================================================================
static void Start(Options *options) {
  std::map<std::string, std::string> parameters;
  options->success = false;
  std::vector<std::string> module_parts;
  if (!ModuleDataForSymbolFile(options->symbolsPath, &module_parts)) {
    fprintf(stderr, "Failed to parse symbol file!\n");
    return;
  }

  std::string compacted_id = CompactIdentifier(module_parts[3]);

  // Add parameters
  if (!options->version.empty())
    parameters["version"] = options->version;

  // MODULE <os> <cpu> <uuid> <module-name>
  // 0      1    2     3      4
  parameters["os"] = module_parts[1];
  parameters["cpu"] = module_parts[2];
  parameters["debug_file"] = module_parts[4];
  parameters["code_file"] = module_parts[4];
  parameters["debug_identifier"] = compacted_id;
  std::string response, error;
  long response_code;
  bool success = HTTPUpload::SendRequest(options->uploadURLStr,
                                         parameters,
                                         options->symbolsPath,
                                         "symbol_file",
                                         options->proxy,
                                         options->proxy_user_pwd,
                                         "",
                                         &response,
                                         &response_code,
                                         &error);

  if (!success) {
    printf("Failed to send symbol file: %s\n", error.c_str());
    printf("Response:\n");
    printf("%s\n", response.c_str());
  } else if (response_code == 0) {
    printf("Failed to send symbol file: No response code\n");
  } else if (response_code != 200) {
    printf("Failed to send symbol file: Response code %ld\n", response_code);
    printf("Response:\n");
    printf("%s\n", response.c_str());
  } else {
    printf("Successfully sent the symbol file.\n");
  }
  options->success = success;
}

//=============================================================================
static void
Usage(int argc, const char *argv[]) {
  fprintf(stderr, "Submit symbol information.\n");
  fprintf(stderr, "Usage: %s [options...] <symbols> <upload-URL>\n", argv[0]);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "<symbols> should be created by using the dump_syms tool.\n");
  fprintf(stderr, "<upload-URL> is the destination for the upload\n");
  fprintf(stderr, "-v:\t Version information (e.g., 1.2.3.4)\n");
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

  while ((ch = getopt(argc, (char * const *)argv, "u:v:x:h?")) != -1) {
    switch (ch) {
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

  options->symbolsPath = argv[optind];
  options->uploadURLStr = argv[optind + 1];
}

//=============================================================================
int main (int argc, const char * argv[]) {
  Options options;
  SetupOptions(argc, argv, &options);
  Start(&options);
  return options.success ? 0 : 1;
}
