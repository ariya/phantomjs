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

#include <sys/stat.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/pathname_stripper.h"

#include "on_demand_symbol_supplier.h"
#include "dump_syms.h"

using std::map;
using std::string;

using google_breakpad::OnDemandSymbolSupplier;
using google_breakpad::PathnameStripper;
using google_breakpad::SymbolSupplier;
using google_breakpad::SystemInfo;

OnDemandSymbolSupplier::OnDemandSymbolSupplier(const string &search_dir,
                                               const string &symbol_search_dir)
  : search_dir_(search_dir) {
  NSFileManager *mgr = [NSFileManager defaultManager];
  size_t length = symbol_search_dir.length();
  if (length) {
    // Load all sym files in symbol_search_dir into our module_file_map
    // A symbol file always starts with a line like this:
    // MODULE mac x86 BBF0A8F9BEADDD2048E6464001CA193F0 GoogleDesktopDaemon
    // or
    // MODULE mac ppc BBF0A8F9BEADDD2048E6464001CA193F0 GoogleDesktopDaemon
    const char *symbolSearchStr = symbol_search_dir.c_str();
    NSString *symbolSearchPath = 
      [mgr stringWithFileSystemRepresentation:symbolSearchStr 
                                       length:strlen(symbolSearchStr)];
    NSDirectoryEnumerator *dirEnum = [mgr enumeratorAtPath:symbolSearchPath];
    NSString *fileName;
    NSCharacterSet *hexSet = 
      [NSCharacterSet characterSetWithCharactersInString:@"0123456789ABCDEF"];
    NSCharacterSet *newlineSet = 
      [NSCharacterSet characterSetWithCharactersInString:@"\r\n"];
    while ((fileName = [dirEnum nextObject])) {
      // Check to see what type of file we have
      NSDictionary *attrib = [dirEnum fileAttributes];
      NSString *fileType = [attrib objectForKey:NSFileType];
      if ([fileType isEqualToString:NSFileTypeDirectory]) {
        // Skip subdirectories
        [dirEnum skipDescendents];
      } else {
        NSString *filePath = [symbolSearchPath stringByAppendingPathComponent:fileName];
        NSString *dataStr = [[[NSString alloc] initWithContentsOfFile:filePath] autorelease];
        if (dataStr) {
          // Check file to see if it is of appropriate type, and grab module
          // name.
          NSScanner *scanner = [NSScanner scannerWithString:dataStr];
          BOOL goodScan = [scanner scanString:@"MODULE mac " intoString:nil];
          if (goodScan) {
            goodScan = ([scanner scanString:@"x86 " intoString:nil] ||
                        [scanner scanString:@"x86_64 " intoString:nil] ||
                        [scanner scanString:@"ppc " intoString:nil]);
            if (goodScan) {
              NSString *moduleID;
              goodScan = [scanner scanCharactersFromSet:hexSet 
                                             intoString:&moduleID];
              if (goodScan) {
                // Module IDs are always 33 chars long
                goodScan = [moduleID length] == 33;
                if (goodScan) {
                  NSString *moduleName;
                  goodScan = [scanner scanUpToCharactersFromSet:newlineSet 
                                                     intoString:&moduleName];
                  if (goodScan) {
                    goodScan = [moduleName length] > 0;
                    if (goodScan) {
                      const char *moduleNameStr = [moduleName UTF8String];
                      const char *filePathStr = [filePath fileSystemRepresentation];
                      // Map our file
                      module_file_map_[moduleNameStr] = filePathStr;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

SymbolSupplier::SymbolResult
OnDemandSymbolSupplier::GetSymbolFile(const CodeModule *module,
                                      const SystemInfo *system_info,
                                      string *symbol_file) {
  string path(GetModuleSymbolFile(module));

  if (path.empty()) {
    if (!GenerateSymbolFile(module, system_info))
      return NOT_FOUND;

    path = GetModuleSymbolFile(module);
  }

  if (path.empty())
    return NOT_FOUND;

  *symbol_file = path;
  return FOUND;
}

SymbolSupplier::SymbolResult
OnDemandSymbolSupplier::GetSymbolFile(const CodeModule *module,
                                      const SystemInfo *system_info,
                                      string *symbol_file,
                                      string *symbol_data) {
  SymbolSupplier::SymbolResult s = GetSymbolFile(module,
                                                 system_info,
                                                 symbol_file);


  if (s == FOUND) {
    std::ifstream in(symbol_file->c_str());
    getline(in, *symbol_data, std::string::traits_type::to_char_type(
                std::string::traits_type::eof()));
    in.close();
  }

  return s;
}

SymbolSupplier::SymbolResult
OnDemandSymbolSupplier::GetCStringSymbolData(const CodeModule *module,
                                             const SystemInfo *system_info,
                                             string *symbol_file,
                                             char **symbol_data) {
  std::string symbol_data_string;
  SymbolSupplier::SymbolResult result = GetSymbolFile(module,
                                                      system_info,
                                                      symbol_file,
                                                      &symbol_data_string);
  if (result == FOUND) {
    *symbol_data = new char[symbol_data_string.size() + 1];
    if (*symbol_data == NULL) {
      // Should return INTERRUPT on memory allocation failure.
      return INTERRUPT;
    }
    strcpy(*symbol_data, symbol_data_string.c_str());
    memory_buffers_.insert(make_pair(module->code_file(), *symbol_data));
  }
  return result;
}

void OnDemandSymbolSupplier::FreeSymbolData(const CodeModule *module) {
  map<string, char *>::iterator it = memory_buffers_.find(module->code_file());
  if (it != memory_buffers_.end()) {
    delete [] it->second;
    memory_buffers_.erase(it);
  }
}

string OnDemandSymbolSupplier::GetLocalModulePath(const CodeModule *module) {
  NSFileManager *mgr = [NSFileManager defaultManager];
  const char *moduleStr = module->code_file().c_str();
  NSString *modulePath =
    [mgr stringWithFileSystemRepresentation:moduleStr length:strlen(moduleStr)];
  const char *searchStr = search_dir_.c_str();
  NSString *searchDir =
    [mgr stringWithFileSystemRepresentation:searchStr length:strlen(searchStr)];

  if ([mgr fileExistsAtPath:modulePath])
    return module->code_file();

  // If the module is not found, try to start appending the components to the
  // search string and stop if a file (not dir) is found or all components
  // have been appended
  NSArray *pathComponents = [modulePath componentsSeparatedByString:@"/"];
  size_t count = [pathComponents count];
  NSMutableString *path = [NSMutableString string];

  for (size_t i = 0; i < count; ++i) {
    [path setString:searchDir];

    for (size_t j = 0; j < i + 1; ++j) {
      size_t idx = count - 1 - i + j;
      [path appendFormat:@"/%@", [pathComponents objectAtIndex:idx]];
    }

    BOOL isDir;
    if ([mgr fileExistsAtPath:path isDirectory:&isDir] && (!isDir)) {
      return [path fileSystemRepresentation];
    }
  }

  return "";
}

string OnDemandSymbolSupplier::GetModulePath(const CodeModule *module) {
  return module->code_file();
}

string OnDemandSymbolSupplier::GetNameForModule(const CodeModule *module) {
  return PathnameStripper::File(module->code_file());
}

string OnDemandSymbolSupplier::GetModuleSymbolFile(const CodeModule *module) {
  string name(GetNameForModule(module));
  map<string, string>::iterator result = module_file_map_.find(name);

  return (result == module_file_map_.end()) ? "" : (*result).second;
}

static float GetFileModificationTime(const char *path) {
  float result = 0;
  struct stat file_stat;
  if (stat(path, &file_stat) == 0)
    result = (float)file_stat.st_mtimespec.tv_sec +
      (float)file_stat.st_mtimespec.tv_nsec / 1.0e9f;

  return result;
}

bool OnDemandSymbolSupplier::GenerateSymbolFile(const CodeModule *module,
                                                const SystemInfo *system_info) {
  bool result = true;
  string name = GetNameForModule(module);
  string module_path = GetLocalModulePath(module);
  NSString *symbol_path = [NSString stringWithFormat:@"/tmp/%s.%s.sym",
    name.c_str(), system_info->cpu.c_str()];

  if (module_path.empty())
    return false;

  // Check if there's already a symbol file cached.  Ensure that the file is
  // newer than the module.  Otherwise, generate a new one.
  BOOL generate_file = YES;
  if ([[NSFileManager defaultManager] fileExistsAtPath:symbol_path]) {
    // Check if the module file is newer than the saved symbols
    float cache_time =
      GetFileModificationTime([symbol_path fileSystemRepresentation]);
    float module_time =
      GetFileModificationTime(module_path.c_str());

    if (cache_time > module_time)
      generate_file = NO;
  }

  if (generate_file) {
    NSString *module_str = [[NSFileManager defaultManager]
      stringWithFileSystemRepresentation:module_path.c_str()
                                  length:module_path.length()];
    DumpSymbols dump;
    if (dump.Read(module_str)) {
      // What Breakpad calls "x86" should be given to the system as "i386".
      std::string architecture;
      if (system_info->cpu.compare("x86") == 0) {
        architecture = "i386";
      } else {
        architecture = system_info->cpu;
      }

      if (dump.SetArchitecture(architecture)) {
        std::fstream file([symbol_path fileSystemRepresentation],
                          std::ios_base::out | std::ios_base::trunc);
        dump.WriteSymbolFile(file, true);
      } else {
        printf("Architecture %s not available for %s\n",
               system_info->cpu.c_str(), name.c_str());
        result = false;
      }
    } else {
      printf("Unable to open %s\n", [module_str UTF8String]);
      result = false;
    }
  }

  // Add the mapping
  if (result)
    module_file_map_[name] = [symbol_path fileSystemRepresentation];

  return result;
}
