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

// Tool to upload an exe/dll and its associated symbols to an HTTP server.
// The PDB file is located automatically, using the path embedded in the
// executable.  The upload is sent as a multipart/form-data POST request,
// with the following parameters:
//  code_file: the basename of the module, e.g. "app.exe"
//  debug_file: the basename of the debugging file, e.g. "app.pdb"
//  debug_identifier: the debug file's identifier, usually consisting of
//                    the guid and age embedded in the pdb, e.g.
//                    "11111111BBBB3333DDDD555555555555F"
//  version: the file version of the module, e.g. "1.2.3.4"
//  os: the operating system that the module was built for, always
//      "windows" in this implementation.
//  cpu: the CPU that the module was built for, typically "x86".
//  symbol_file: the contents of the breakpad-format symbol file

#include <Windows.h>
#include <DbgHelp.h>
#include <WinInet.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "common/windows/string_utils-inl.h"

#include "common/windows/http_upload.h"
#include "common/windows/pdb_source_line_writer.h"

using std::string;
using std::wstring;
using std::vector;
using std::map;
using google_breakpad::HTTPUpload;
using google_breakpad::PDBModuleInfo;
using google_breakpad::PDBSourceLineWriter;
using google_breakpad::WindowsStringUtils;

// Extracts the file version information for the given filename,
// as a string, for example, "1.2.3.4".  Returns true on success.
static bool GetFileVersionString(const wchar_t *filename, wstring *version) {
  DWORD handle;
  DWORD version_size = GetFileVersionInfoSize(filename, &handle);
  if (version_size < sizeof(VS_FIXEDFILEINFO)) {
    return false;
  }

  vector<char> version_info(version_size);
  if (!GetFileVersionInfo(filename, handle, version_size, &version_info[0])) {
    return false;
  }

  void *file_info_buffer = NULL;
  unsigned int file_info_length;
  if (!VerQueryValue(&version_info[0], L"\\",
                     &file_info_buffer, &file_info_length)) {
    return false;
  }

  // The maximum value of each version component is 65535 (0xffff),
  // so the max length is 24, including the terminating null.
  wchar_t ver_string[24];
  VS_FIXEDFILEINFO *file_info =
    reinterpret_cast<VS_FIXEDFILEINFO*>(file_info_buffer);
  swprintf(ver_string, sizeof(ver_string) / sizeof(ver_string[0]),
           L"%d.%d.%d.%d",
           file_info->dwFileVersionMS >> 16,
           file_info->dwFileVersionMS & 0xffff,
           file_info->dwFileVersionLS >> 16,
           file_info->dwFileVersionLS & 0xffff);

  // remove when VC++7.1 is no longer supported
  ver_string[sizeof(ver_string) / sizeof(ver_string[0]) - 1] = L'\0';

  *version = ver_string;
  return true;
}

// Creates a new temporary file and writes the symbol data from the given
// exe/dll file to it.  Returns the path to the temp file in temp_file_path
// and information about the pdb in pdb_info.
static bool DumpSymbolsToTempFile(const wchar_t *file,
                                  wstring *temp_file_path,
                                  PDBModuleInfo *pdb_info) {
  google_breakpad::PDBSourceLineWriter writer;
  // Use EXE_FILE to get information out of the exe/dll in addition to the
  // pdb.  The name and version number of the exe/dll are of value, and
  // there's no way to locate an exe/dll given a pdb.
  if (!writer.Open(file, PDBSourceLineWriter::EXE_FILE)) {
    return false;
  }

  wchar_t temp_path[_MAX_PATH];
  if (GetTempPath(_MAX_PATH, temp_path) == 0) {
    return false;
  }

  wchar_t temp_filename[_MAX_PATH];
  if (GetTempFileName(temp_path, L"sym", 0, temp_filename) == 0) {
    return false;
  }

  FILE *temp_file = NULL;
#if _MSC_VER >= 1400  // MSVC 2005/8
  if (_wfopen_s(&temp_file, temp_filename, L"w") != 0)
#else  // _MSC_VER >= 1400
  // _wfopen_s was introduced in MSVC8.  Use _wfopen for earlier environments.
  // Don't use it with MSVC8 and later, because it's deprecated.
  if (!(temp_file = _wfopen(temp_filename, L"w")))
#endif  // _MSC_VER >= 1400
  {
    return false;
  }

  bool success = writer.WriteMap(temp_file);
  fclose(temp_file);
  if (!success) {
    _wunlink(temp_filename);
    return false;
  }

  *temp_file_path = temp_filename;

  return writer.GetModuleInfo(pdb_info);
}

__declspec(noreturn) void printUsageAndExit() {
  wprintf(L"Usage: symupload [--timeout NN] <file.exe|file.dll> "
      L"<symbol upload URL> [...<symbol upload URLs>]\n\n");
  wprintf(L"Timeout is in milliseconds, or can be 0 to be unlimited\n\n");
  wprintf(L"Example:\n\n\tsymupload.exe --timeout 0 chrome.dll "
      L"http://no.free.symbol.server.for.you\n");
  exit(0);
}
int wmain(int argc, wchar_t *argv[]) {
  const wchar_t *module;
  int timeout = -1;
  int currentarg = 1;
  if (argc > 2) {
    if (!wcscmp(L"--timeout", argv[1])) {
      timeout = _wtoi(argv[2]);
      currentarg = 3;
    }
  } else {
    printUsageAndExit();
  }

  if (argc >= currentarg + 2)
    module = argv[currentarg++];
  else
    printUsageAndExit();

  wstring symbol_file;
  PDBModuleInfo pdb_info;
  if (!DumpSymbolsToTempFile(module, &symbol_file, &pdb_info)) {
    fwprintf(stderr, L"Could not get symbol data from %s\n", module);
    return 1;
  }

  wstring code_file = WindowsStringUtils::GetBaseName(wstring(module));

  map<wstring, wstring> parameters;
  parameters[L"code_file"] = code_file;
  parameters[L"debug_file"] = pdb_info.debug_file;
  parameters[L"debug_identifier"] = pdb_info.debug_identifier;
  parameters[L"os"] = L"windows";  // This version of symupload is Windows-only
  parameters[L"cpu"] = pdb_info.cpu;

  // Don't make a missing version a hard error.  Issue a warning, and let the
  // server decide whether to reject files without versions.
  wstring file_version;
  if (GetFileVersionString(module, &file_version)) {
    parameters[L"version"] = file_version;
  } else {
    fwprintf(stderr, L"Warning: Could not get file version for %s\n", module);
  }

  bool success = true;

  while (currentarg < argc) {
    if (!HTTPUpload::SendRequest(argv[currentarg], parameters,
                                 symbol_file, L"symbol_file",
                                 timeout == -1 ? NULL : &timeout,
                                 NULL, NULL)) {
      success = false;
      fwprintf(stderr, L"Symbol file upload to %s failed\n", argv[currentarg]);
    }
    currentarg++;
  }

  _wunlink(symbol_file.c_str());

  if (success) {
    wprintf(L"Uploaded symbols for windows-%s/%s/%s (%s %s)\n",
            pdb_info.cpu.c_str(), pdb_info.debug_file.c_str(),
            pdb_info.debug_identifier.c_str(), code_file.c_str(),
            file_version.c_str());
  }

  return success ? 0 : 1;
}
