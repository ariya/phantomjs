// Copyright (c) 2007, Google Inc.
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

// ms_symbol_server_converter.cc: Obtain symbol files from a Microsoft
// symbol server, and convert them to Breakpad's dumped format.
//
// See ms_symbol_server_converter.h for documentation.
//
// Author: Mark Mentovai

#include <Windows.h>
#include <DbgHelp.h>

#include <cassert>
#include <cstdio>

#include "tools/windows/converter/ms_symbol_server_converter.h"
#include "common/windows/pdb_source_line_writer.h"
#include "common/windows/string_utils-inl.h"

// SYMOPT_NO_PROMPTS is not defined in earlier platform SDKs.  Define it
// in that case, in the event that this code is used with a newer version
// of DbgHelp at runtime that recognizes the option.  The presence of this
// bit in the symbol options should not harm earlier versions of DbgHelp.
#ifndef SYMOPT_NO_PROMPTS
#define SYMOPT_NO_PROMPTS 0x00080000
#endif  // SYMOPT_NO_PROMPTS

namespace google_breakpad {

// Use sscanf_s if it is available, to quench the warning about scanf being
// deprecated.  Use scanf where sscanf_is not available.  Note that the
// parameters passed to sscanf and sscanf_s are only compatible as long as
// fields of type c, C, s, S, and [ are not used.
#if _MSC_VER >= 1400  // MSVC 2005/8
#define SSCANF sscanf_s
#else  // _MSC_VER >= 1400
#define SSCANF sscanf
#endif  // _MSC_VER >= 1400

bool GUIDOrSignatureIdentifier::InitializeFromString(
    const string &identifier) {
  type_ = TYPE_NONE;

  size_t length = identifier.length();

  if (length > 32 && length <= 40) {
    // GUID
    if (SSCANF(identifier.c_str(),
               "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
               &guid_.Data1, &guid_.Data2, &guid_.Data3,
               &guid_.Data4[0], &guid_.Data4[1],
               &guid_.Data4[2], &guid_.Data4[3],
               &guid_.Data4[4], &guid_.Data4[5],
               &guid_.Data4[6], &guid_.Data4[7],
               &age_) != 12) {
      return false;
    }

    type_ = TYPE_GUID;
  } else if (length > 8 && length <= 15) {
    // Signature
    if (SSCANF(identifier.c_str(), "%08X%x", &signature_, &age_) != 2) {
      return false;
    }

    type_ = TYPE_SIGNATURE;
  } else {
    return false;
  }

  return true;
}

#undef SSCANF

MSSymbolServerConverter::MSSymbolServerConverter(
    const string &local_cache, const vector<string> &symbol_servers)
    : symbol_path_(),
      fail_dns_(false),
      fail_timeout_(false),
      fail_not_found_(false) {
  // Setting local_cache can be done without verifying that it exists because
  // SymSrv will create it if it is missing - any creation failures will occur
  // at that time, so there's nothing to check here, making it safe to
  // assign this in the constructor.

  assert(symbol_servers.size() > 0);

#if !defined(NDEBUG)
  // These are characters that are interpreted as having special meanings in
  // symbol_path_.
  const char kInvalidCharacters[] = "*;";
  assert(local_cache.find_first_of(kInvalidCharacters) == string::npos);
#endif  // !defined(NDEBUG)

  for (vector<string>::const_iterator symbol_server = symbol_servers.begin();
       symbol_server != symbol_servers.end();
       ++symbol_server) {
    // The symbol path format is explained by
    // http://msdn.microsoft.com/library/en-us/debug/base/using_symsrv.asp .
    // "srv*" is the same as "symsrv*symsrv.dll*", which means that
    // symsrv.dll is to be responsible for locating symbols.  symsrv.dll
    // interprets the rest of the string as a series of symbol stores separated
    // by '*'.  "srv*local_cache*symbol_server" means to check local_cache
    // first for the symbol file, and if it is not found there, to check
    // symbol_server.  Symbol files found on the symbol server will be placed
    // in the local cache, decompressed.
    //
    // Multiple specifications in this format may be presented, separated by
    // semicolons.

    assert((*symbol_server).find_first_of(kInvalidCharacters) == string::npos);
    symbol_path_ += "srv*" + local_cache + "*" + *symbol_server + ";";
  }

  // Strip the trailing semicolon.
  symbol_path_.erase(symbol_path_.length() - 1);
}

// A stack-based class that manages SymInitialize and SymCleanup calls.
class AutoSymSrv {
 public:
  AutoSymSrv() : initialized_(false) {}

  ~AutoSymSrv() {
    if (!Cleanup()) {
      // Print the error message here, because destructors have no return
      // value.
      fprintf(stderr, "~AutoSymSrv: SymCleanup: error %d\n", GetLastError());
    }
  }

  bool Initialize(HANDLE process, char *path, bool invade_process) {
    process_ = process;
    initialized_ = SymInitialize(process, path, invade_process) == TRUE;
    return initialized_;
  }

  bool Cleanup() {
    if (initialized_) {
      if (SymCleanup(process_)) {
        initialized_ = false;
        return true;
      }
      return false;
    }

    return true;
  }

 private:
  HANDLE process_;
  bool initialized_;
};

// A stack-based class that "owns" a pathname and deletes it when destroyed,
// unless told not to by having its Release() method called.  Early deletions
// are supported by calling Delete().
class AutoDeleter {
 public:
  explicit AutoDeleter(const string &path) : path_(path) {}

  ~AutoDeleter() {
    int error;
    if ((error = Delete()) != 0) {
      // Print the error message here, because destructors have no return
      // value.
      fprintf(stderr, "~AutoDeleter: Delete: error %d for %s\n",
              error, path_.c_str());
    }
  }

  int Delete() {
    if (path_.empty())
      return 0;

    int error = remove(path_.c_str());
    Release();
    return error;
  }

  void Release() {
    path_.clear();
  }

 private:
  string path_;
};

MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocateFile(const string &debug_or_code_file,
                                    const string &debug_or_code_id,
                                    const string &version,
                                    string *file_name) {
  assert(file_name);
  file_name->clear();

  GUIDOrSignatureIdentifier identifier;
  if (!identifier.InitializeFromString(debug_or_code_id)) {
    fprintf(stderr,
            "LocateFile: Unparseable identifier for %s %s %s\n",
            debug_or_code_file.c_str(),
            debug_or_code_id.c_str(),
            version.c_str());
    return LOCATE_FAILURE;
  }

  HANDLE process = GetCurrentProcess();  // CloseHandle is not needed.
  AutoSymSrv symsrv;
  if (!symsrv.Initialize(process,
                         const_cast<char *>(symbol_path_.c_str()),
                         false)) {
    fprintf(stderr, "LocateFile: SymInitialize: error %d for %s %s %s\n",
            GetLastError(),
            debug_or_code_file.c_str(),
            debug_or_code_id.c_str(),
            version.c_str());
    return LOCATE_FAILURE;
  }

  if (!SymRegisterCallback64(process, SymCallback,
                             reinterpret_cast<ULONG64>(this))) {
    fprintf(stderr,
            "LocateFile: SymRegisterCallback64: error %d for %s %s %s\n",
            GetLastError(),
            debug_or_code_file.c_str(),
            debug_or_code_id.c_str(),
            version.c_str());
    return LOCATE_FAILURE;
  }

  // SYMOPT_DEBUG arranges for SymCallback to be called with additional
  // debugging information.  This is used to determine the nature of failures.
  DWORD options = SymGetOptions() | SYMOPT_DEBUG | SYMOPT_NO_PROMPTS |
                  SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_SECURE;
  SymSetOptions(options);

  // SymCallback will set these as needed inisde the SymFindFileInPath call.
  fail_dns_ = false;
  fail_timeout_ = false;
  fail_not_found_ = false;

  // Do the lookup.
  char path[MAX_PATH];
  if (!SymFindFileInPath(
          process, NULL,
          const_cast<char *>(debug_or_code_file.c_str()),
          const_cast<void *>(identifier.guid_or_signature_pointer()),
          identifier.age(), 0,
          identifier.type() == GUIDOrSignatureIdentifier::TYPE_GUID ?
              SSRVOPT_GUIDPTR : SSRVOPT_DWORDPTR,
          path, SymFindFileInPathCallback, this)) {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND) {
      // This can be returned for a number of reasons.  Use the crumbs
      // collected by SymCallback to determine which one is relevant.

      // These errors are possibly transient.
      if (fail_dns_ || fail_timeout_) {
        return LOCATE_RETRY;
      }

      // This is an authoritiative file-not-found message.
      if (fail_not_found_) {
        fprintf(stderr,
                "LocateFile: SymFindFileInPath: LOCATE_NOT_FOUND error "
                "for %s %s %s\n",
                debug_or_code_file.c_str(),
                debug_or_code_id.c_str(),
                version.c_str());
        return LOCATE_NOT_FOUND;
      }

      // If the error is FILE_NOT_FOUND but none of the known error
      // conditions are matched, fall through to LOCATE_FAILURE.
    }

    fprintf(stderr,
            "LocateFile: SymFindFileInPath: error %d for %s %s %s\n",
            error,
            debug_or_code_file.c_str(),
            debug_or_code_id.c_str(),
            version.c_str());
    return LOCATE_FAILURE;
  }

  // Making sure path is null-terminated.
  path[MAX_PATH - 1] = '\0';

  // The AutoDeleter ensures that the file is only kept when returning
  // LOCATE_SUCCESS.
  AutoDeleter deleter(path);

  // Do the cleanup here even though it will happen when symsrv goes out of
  // scope, to allow it to influence the return value.
  if (!symsrv.Cleanup()) {
    fprintf(stderr, "LocateFile: SymCleanup: error %d for %s %s %s\n",
            GetLastError(),
            debug_or_code_file.c_str(),
            debug_or_code_id.c_str(),
            version.c_str());
    return LOCATE_FAILURE;
  }

  deleter.Release();

  printf("Downloaded: %s\n", path);
  *file_name = path;
  return LOCATE_SUCCESS;
}


MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocatePEFile(const MissingSymbolInfo &missing,
                                      string *pe_file) {
  return LocateFile(missing.code_file, missing.code_identifier,
                    missing.version, pe_file);
}

MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocateSymbolFile(const MissingSymbolInfo &missing,
                                          string *symbol_file) {
  return LocateFile(missing.debug_file, missing.debug_identifier,
                    missing.version, symbol_file);
}


// static
BOOL CALLBACK MSSymbolServerConverter::SymCallback(HANDLE process,
                                                   ULONG action,
                                                   ULONG64 data,
                                                   ULONG64 context) {
  MSSymbolServerConverter *self =
      reinterpret_cast<MSSymbolServerConverter *>(context);

  switch (action) {
    case CBA_EVENT: {
      IMAGEHLP_CBA_EVENT *cba_event =
          reinterpret_cast<IMAGEHLP_CBA_EVENT *>(data);

      // Put the string into a string object to be able to use string::find
      // for substring matching.  This is important because the not-found
      // message does not use the entire string but is appended to the URL
      // that SymSrv attempted to retrieve.
      string desc(cba_event->desc);

      // desc_action maps strings (in desc) to boolean pointers that are to
      // be set to true if the string matches.
      struct desc_action {
        const char *desc;  // The substring to match.
        bool *action;      // On match, this pointer will be set to true.
      };

      static const desc_action desc_actions[] = {
        // When a DNS error occurs, it could be indiciative of network
        // problems.
        { "SYMSRV:  The server name or address could not be resolved\n",
          &self->fail_dns_ },

        // This message is produced if no connection is opened.
        { "SYMSRV:  A connection with the server could not be established\n",
          &self->fail_timeout_ },

        // This message is produced if a connection is established but the
        // server fails to respond to the HTTP request.
        { "SYMSRV:  The operation timed out\n",
          &self->fail_timeout_ },

        // This message is produced when the requested file is not found,
        // even if one or more of the above messages are also produced.
        // It's trapped to distinguish between not-found and unknown-failure
        // conditions.  Note that this message will not be produced if a
        // connection is established and the server begins to respond to the
        // HTTP request but does not finish transmitting the file.
        { " not found\n",
          &self->fail_not_found_ }
      };

      for (int desc_action_index = 0;
           desc_action_index < sizeof(desc_actions) / sizeof(desc_action);
           ++desc_action_index) {
        if (desc.find(desc_actions[desc_action_index].desc) != string::npos) {
          *(desc_actions[desc_action_index].action) = true;
          break;
        }
      }

      break;
    }
  }

  // This function is a mere fly on the wall.  Treat everything as unhandled.
  return FALSE;
}

// static
BOOL CALLBACK MSSymbolServerConverter::SymFindFileInPathCallback(
    const char *filename, void *context) {
  // FALSE ends the search, indicating that the located symbol file is
  // satisfactory.
  return FALSE;
}

MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocateAndConvertSymbolFile(
    const MissingSymbolInfo &missing,
    bool keep_symbol_file,
    bool keep_pe_file,
    string *converted_symbol_file,
    string *symbol_file,
    string *out_pe_file) {
  assert(converted_symbol_file);
  converted_symbol_file->clear();
  if (symbol_file) {
    symbol_file->clear();
  }

  string pdb_file;
  LocateResult result = LocateSymbolFile(missing, &pdb_file);
  if (result != LOCATE_SUCCESS) {
    return result;
  }

  if (symbol_file && keep_symbol_file) {
    *symbol_file = pdb_file;
  }

  // The conversion of a symbol file for a Windows 64-bit module requires
  // loading of the executable file.  If there is no executable file, convert
  // using only the PDB file.  Without an executable file, the conversion will
  // fail for 64-bit modules but it should succeed for 32-bit modules.
  string pe_file;
  result = LocatePEFile(missing, &pe_file);
  if (result != LOCATE_SUCCESS) {
    fprintf(stderr, "WARNING: Could not download: %s\n", pe_file.c_str());
  }

  if (out_pe_file && keep_pe_file) {
    *out_pe_file = pe_file;
  }

  // Conversion may fail because the file is corrupt.  If a broken file is
  // kept in the local cache, LocateSymbolFile will not hit the network again
  // to attempt to locate it.  To guard against problems like this, the
  // symbol file in the local cache will be removed if conversion fails.
  AutoDeleter pdb_deleter(pdb_file);
  AutoDeleter pe_deleter(pe_file);

  // Be sure that it's a .pdb file, since we'll be replacing .pdb with .sym
  // for the converted file's name.
  string pdb_extension = pdb_file.substr(pdb_file.length() - 4);
  // strcasecmp is called _stricmp here.
  if (_stricmp(pdb_extension.c_str(), ".pdb") != 0) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "no .pdb extension for %s %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file.c_str());
    return LOCATE_FAILURE;
  }

  PDBSourceLineWriter writer;
  wstring pe_file_w;
  if (!WindowsStringUtils::safe_mbstowcs(pe_file, &pe_file_w)) {
    fprintf(stderr,
            "LocateAndConvertSymbolFile: "
                "WindowsStringUtils::safe_mbstowcs failed for %s\n",
            pe_file.c_str());
    return LOCATE_FAILURE;
  }
  wstring pdb_file_w;
  if (!WindowsStringUtils::safe_mbstowcs(pdb_file, &pdb_file_w)) {
    fprintf(stderr,
            "LocateAndConvertSymbolFile: "
                "WindowsStringUtils::safe_mbstowcs failed for %s\n",
            pdb_file_w.c_str());
    return LOCATE_FAILURE;
  }
  if (!writer.Open(pdb_file_w, PDBSourceLineWriter::PDB_FILE)) {
    fprintf(stderr,
            "ERROR: PDBSourceLineWriter::Open failed for %s %s %s %ws\n",
            missing.debug_file.c_str(), missing.debug_identifier.c_str(),
            missing.version.c_str(), pdb_file_w.c_str());
    return LOCATE_FAILURE;
  }
  if (!writer.SetCodeFile(pe_file_w)) {
    fprintf(stderr,
            "ERROR: PDBSourceLineWriter::SetCodeFile failed for %s %s %s %ws\n",
            missing.debug_file.c_str(), missing.debug_identifier.c_str(),
            missing.version.c_str(), pe_file_w.c_str());
    return LOCATE_FAILURE;
  }

  *converted_symbol_file = pdb_file.substr(0, pdb_file.length() - 4) + ".sym";

  FILE *converted_output = NULL;
#if _MSC_VER >= 1400  // MSVC 2005/8
  errno_t err;
  if ((err = fopen_s(&converted_output, converted_symbol_file->c_str(), "w"))
      != 0) {
#else  // _MSC_VER >= 1400
  // fopen_s and errno_t were introduced in MSVC8.  Use fopen for earlier
  // environments.  Don't use fopen with MSVC8 and later, because it's
  // deprecated.  fopen does not provide reliable error codes, so just use
  // -1 in the event of a failure.
  int err;
  if (!(converted_output = fopen(converted_symbol_file->c_str(), "w"))) {
    err = -1;
#endif  // _MSC_VER >= 1400
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "fopen_s: error %d for %s %s %s %s\n",
            err,
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            converted_symbol_file->c_str());
    return LOCATE_FAILURE;
  }

  AutoDeleter sym_deleter(*converted_symbol_file);

  bool success = writer.WriteMap(converted_output);
  fclose(converted_output);

  if (!success) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "PDBSourceLineWriter::WriteMap failed for %s %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file.c_str());
    return LOCATE_FAILURE;
  }

  if (keep_symbol_file) {
    pdb_deleter.Release();
  }

  if (keep_pe_file) {
    pe_deleter.Release();
  }

  sym_deleter.Release();

  return LOCATE_SUCCESS;
}

}  // namespace google_breakpad
