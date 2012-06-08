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

#include <atlbase.h>
#include <dia2.h>
#include <ImageHlp.h>
#include <stdio.h>

#include "common/windows/string_utils-inl.h"

#include "common/windows/pdb_source_line_writer.h"
#include "common/windows/guid_string.h"

// This constant may be missing from DbgHelp.h.  See the documentation for
// IDiaSymbol::get_undecoratedNameEx.
#ifndef UNDNAME_NO_ECSU
#define UNDNAME_NO_ECSU 0x8000  // Suppresses enum/class/struct/union.
#endif  // UNDNAME_NO_ECSU

namespace google_breakpad {

using std::vector;

// A helper class to scope a PLOADED_IMAGE.
class AutoImage {
 public:
  explicit AutoImage(PLOADED_IMAGE img) : img_(img) {}
  ~AutoImage() {
    if (img_)
      ImageUnload(img_);
  }

  operator PLOADED_IMAGE() { return img_; }
  PLOADED_IMAGE operator->() { return img_; }

 private:
  PLOADED_IMAGE img_;
};

PDBSourceLineWriter::PDBSourceLineWriter() : output_(NULL) {
}

PDBSourceLineWriter::~PDBSourceLineWriter() {
}

bool PDBSourceLineWriter::Open(const wstring &file, FileFormat format) {
  Close();

  if (FAILED(CoInitialize(NULL))) {
    fprintf(stderr, "CoInitialize failed\n");
    return false;
  }

  CComPtr<IDiaDataSource> data_source;
  if (FAILED(data_source.CoCreateInstance(CLSID_DiaSource))) {
    const int kGuidSize = 64;
    wchar_t classid[kGuidSize] = {0};
    StringFromGUID2(CLSID_DiaSource, classid, kGuidSize);
    // vc80 uses bce36434-2c24-499e-bf49-8bd99b0eeb68.
    // vc90 uses 4C41678E-887B-4365-A09E-925D28DB33C2.
    fprintf(stderr, "CoCreateInstance CLSID_DiaSource %S failed "
            "(msdia*.dll unregistered?)\n", classid);
    return false;
  }

  switch (format) {
    case PDB_FILE:
      if (FAILED(data_source->loadDataFromPdb(file.c_str()))) {
        fprintf(stderr, "loadDataFromPdb failed for %ws\n", file.c_str());
        return false;
      }
      break;
    case EXE_FILE:
      if (FAILED(data_source->loadDataForExe(file.c_str(), NULL, NULL))) {
        fprintf(stderr, "loadDataForExe failed for %ws\n", file.c_str());
        return false;
      }
      code_file_ = file;
      break;
    case ANY_FILE:
      if (FAILED(data_source->loadDataFromPdb(file.c_str()))) {
        if (FAILED(data_source->loadDataForExe(file.c_str(), NULL, NULL))) {
          fprintf(stderr, "loadDataForPdb and loadDataFromExe failed for %ws\n", file.c_str());
          return false;
        }
	code_file_ = file;
      }
      break;
    default:
      fprintf(stderr, "Unknown file format\n");
      return false;
  }

  if (FAILED(data_source->openSession(&session_))) {
    fprintf(stderr, "openSession failed\n");
  }

  return true;
}

bool PDBSourceLineWriter::PrintLines(IDiaEnumLineNumbers *lines) {
  // The line number format is:
  // <rva> <line number> <source file id>
  CComPtr<IDiaLineNumber> line;
  ULONG count;

  while (SUCCEEDED(lines->Next(1, &line, &count)) && count == 1) {
    DWORD rva;
    if (FAILED(line->get_relativeVirtualAddress(&rva))) {
      fprintf(stderr, "failed to get line rva\n");
      return false;
    }

    DWORD length;
    if (FAILED(line->get_length(&length))) {
      fprintf(stderr, "failed to get line code length\n");
      return false;
    }

    DWORD dia_source_id;
    if (FAILED(line->get_sourceFileId(&dia_source_id))) {
      fprintf(stderr, "failed to get line source file id\n");
      return false;
    }
    // duplicate file names are coalesced to share one ID
    DWORD source_id = GetRealFileID(dia_source_id);

    DWORD line_num;
    if (FAILED(line->get_lineNumber(&line_num))) {
      fprintf(stderr, "failed to get line number\n");
      return false;
    }

    fprintf(output_, "%x %x %d %d\n", rva, length, line_num, source_id);
    line.Release();
  }
  return true;
}

bool PDBSourceLineWriter::PrintFunction(IDiaSymbol *function,
                                        IDiaSymbol *block) {
  // The function format is:
  // FUNC <address> <length> <param_stack_size> <function>
  DWORD rva;
  if (FAILED(block->get_relativeVirtualAddress(&rva))) {
    fprintf(stderr, "couldn't get rva\n");
    return false;
  }

  ULONGLONG length;
  if (FAILED(block->get_length(&length))) {
    fprintf(stderr, "failed to get function length\n");
    return false;
  }

  if (length == 0) {
    // Silently ignore zero-length functions, which can infrequently pop up.
    return true;
  }

  CComBSTR name;
  int stack_param_size;
  if (!GetSymbolFunctionName(function, &name, &stack_param_size)) {
    return false;
  }

  // If the decorated name didn't give the parameter size, try to
  // calculate it.
  if (stack_param_size < 0) {
    stack_param_size = GetFunctionStackParamSize(function);
  }

  fprintf(output_, "FUNC %x %" WIN_STRING_FORMAT_LL "x %x %ws\n",
          rva, length, stack_param_size, name);

  CComPtr<IDiaEnumLineNumbers> lines;
  if (FAILED(session_->findLinesByRVA(rva, DWORD(length), &lines))) {
    return false;
  }

  if (!PrintLines(lines)) {
    return false;
  }
  return true;
}

bool PDBSourceLineWriter::PrintSourceFiles() {
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComPtr<IDiaEnumSymbols> compilands;
  if (FAILED(global->findChildren(SymTagCompiland, NULL,
                                  nsNone, &compilands))) {
    fprintf(stderr, "findChildren failed\n");
    return false;
  }

  CComPtr<IDiaSymbol> compiland;
  ULONG count;
  while (SUCCEEDED(compilands->Next(1, &compiland, &count)) && count == 1) {
    CComPtr<IDiaEnumSourceFiles> source_files;
    if (FAILED(session_->findFile(compiland, NULL, nsNone, &source_files))) {
      return false;
    }
    CComPtr<IDiaSourceFile> file;
    while (SUCCEEDED(source_files->Next(1, &file, &count)) && count == 1) {
      DWORD file_id;
      if (FAILED(file->get_uniqueId(&file_id))) {
        return false;
      }

      CComBSTR file_name;
      if (FAILED(file->get_fileName(&file_name))) {
        return false;
      }

      wstring file_name_string(file_name);
      if (!FileIDIsCached(file_name_string)) {
        // this is a new file name, cache it and output a FILE line.
        CacheFileID(file_name_string, file_id);
        fwprintf(output_, L"FILE %d %s\n", file_id, file_name);
      } else {
        // this file name has already been seen, just save this
        // ID for later lookup.
        StoreDuplicateFileID(file_name_string, file_id);
      }
      file.Release();
    }
    compiland.Release();
  }
  return true;
}

bool PDBSourceLineWriter::PrintFunctions() {
  CComPtr<IDiaEnumSymbolsByAddr> symbols;
  if (FAILED(session_->getSymbolsByAddr(&symbols))) {
    fprintf(stderr, "failed to get symbol enumerator\n");
    return false;
  }

  CComPtr<IDiaSymbol> symbol;
  if (FAILED(symbols->symbolByAddr(1, 0, &symbol))) {
    fprintf(stderr, "failed to enumerate symbols\n");
    return false;
  }

  DWORD rva_last = 0;
  if (FAILED(symbol->get_relativeVirtualAddress(&rva_last))) {
    fprintf(stderr, "failed to get symbol rva\n");
    return false;
  }

  ULONG count;
  do {
    DWORD tag;
    if (FAILED(symbol->get_symTag(&tag))) {
      fprintf(stderr, "failed to get symbol tag\n");
      return false;
    }

    // For a given function, DIA seems to give either a symbol with
    // SymTagFunction or SymTagPublicSymbol, but not both.  This means
    // that PDBSourceLineWriter will output either a FUNC or PUBLIC line,
    // but not both.
    if (tag == SymTagFunction) {
      if (!PrintFunction(symbol, symbol)) {
        return false;
      }
    } else if (tag == SymTagPublicSymbol) {
      if (!PrintCodePublicSymbol(symbol)) {
        return false;
      }
    }
    symbol.Release();
  } while (SUCCEEDED(symbols->Next(1, &symbol, &count)) && count == 1);

  // When building with PGO, the compiler can split functions into
  // "hot" and "cold" blocks, and move the "cold" blocks out to separate
  // pages, so the function can be noncontiguous. To find these blocks,
  // we have to iterate over all the compilands, and then find blocks
  // that are children of them. We can then find the lexical parents
  // of those blocks and print out an extra FUNC line for blocks
  // that are not contained in their parent functions.
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComPtr<IDiaEnumSymbols> compilands;
  if (FAILED(global->findChildren(SymTagCompiland, NULL,
                                  nsNone, &compilands))) {
    fprintf(stderr, "findChildren failed on the global\n");
    return false;
  }

  CComPtr<IDiaSymbol> compiland;
  while (SUCCEEDED(compilands->Next(1, &compiland, &count)) && count == 1) {
    CComPtr<IDiaEnumSymbols> blocks;
    if (FAILED(compiland->findChildren(SymTagBlock, NULL,
                                       nsNone, &blocks))) {
      fprintf(stderr, "findChildren failed on a compiland\n");
      return false;
    }

    CComPtr<IDiaSymbol> block;
    while (SUCCEEDED(blocks->Next(1, &block, &count)) && count == 1) {
      // find this block's lexical parent function
      CComPtr<IDiaSymbol> parent;
      DWORD tag;
      if (SUCCEEDED(block->get_lexicalParent(&parent)) &&
          SUCCEEDED(parent->get_symTag(&tag)) &&
          tag == SymTagFunction) {
        // now get the block's offset and the function's offset and size,
        // and determine if the block is outside of the function
        DWORD func_rva, block_rva;
        ULONGLONG func_length;
        if (SUCCEEDED(block->get_relativeVirtualAddress(&block_rva)) &&
            SUCCEEDED(parent->get_relativeVirtualAddress(&func_rva)) &&
            SUCCEEDED(parent->get_length(&func_length))) {
          if (block_rva < func_rva || block_rva > (func_rva + func_length)) {
            if (!PrintFunction(parent, block)) {
              return false;
            }
          }
        }
      }
      parent.Release();
      block.Release();
    }
    blocks.Release();
    compiland.Release();
  }

  return true;
}

bool PDBSourceLineWriter::PrintFrameData() {
  // It would be nice if it were possible to output frame data alongside the
  // associated function, as is done with line numbers, but the DIA API
  // doesn't make it possible to get the frame data in that way.

  CComPtr<IDiaEnumTables> tables;
  if (FAILED(session_->getEnumTables(&tables)))
    return false;

  // Pick up the first table that supports IDiaEnumFrameData.
  CComPtr<IDiaEnumFrameData> frame_data_enum;
  CComPtr<IDiaTable> table;
  ULONG count;
  while (!frame_data_enum &&
         SUCCEEDED(tables->Next(1, &table, &count)) &&
         count == 1) {
    table->QueryInterface(_uuidof(IDiaEnumFrameData),
                          reinterpret_cast<void**>(&frame_data_enum));
    table.Release();
  }
  if (!frame_data_enum)
    return false;

  DWORD last_type = -1;
  DWORD last_rva = -1;
  DWORD last_code_size = 0;
  DWORD last_prolog_size = -1;

  CComPtr<IDiaFrameData> frame_data;
  while (SUCCEEDED(frame_data_enum->Next(1, &frame_data, &count)) &&
         count == 1) {
    DWORD type;
    if (FAILED(frame_data->get_type(&type)))
      return false;

    DWORD rva;
    if (FAILED(frame_data->get_relativeVirtualAddress(&rva)))
      return false;

    DWORD code_size;
    if (FAILED(frame_data->get_lengthBlock(&code_size)))
      return false;

    DWORD prolog_size;
    if (FAILED(frame_data->get_lengthProlog(&prolog_size)))
      return false;

    // epliog_size is always 0.
    DWORD epilog_size = 0;

    // parameter_size is the size of parameters passed on the stack.  If any
    // parameters are not passed on the stack (such as in registers), their
    // sizes will not be included in parameter_size.
    DWORD parameter_size;
    if (FAILED(frame_data->get_lengthParams(&parameter_size)))
      return false;

    DWORD saved_register_size;
    if (FAILED(frame_data->get_lengthSavedRegisters(&saved_register_size)))
      return false;

    DWORD local_size;
    if (FAILED(frame_data->get_lengthLocals(&local_size)))
      return false;

    // get_maxStack can return S_FALSE, just use 0 in that case.
    DWORD max_stack_size = 0;
    if (FAILED(frame_data->get_maxStack(&max_stack_size)))
      return false;

    // get_programString can return S_FALSE, indicating that there is no
    // program string.  In that case, check whether %ebp is used.
    HRESULT program_string_result;
    CComBSTR program_string;
    if (FAILED(program_string_result = frame_data->get_program(
        &program_string))) {
      return false;
    }

    // get_allocatesBasePointer can return S_FALSE, treat that as though
    // %ebp is not used.
    BOOL allocates_base_pointer = FALSE;
    if (program_string_result != S_OK) {
      if (FAILED(frame_data->get_allocatesBasePointer(
          &allocates_base_pointer))) {
        return false;
      }
    }

    // Only print out a line if type, rva, code_size, or prolog_size have
    // changed from the last line.  It is surprisingly common (especially in
    // system library PDBs) for DIA to return a series of identical
    // IDiaFrameData objects.  For kernel32.pdb from Windows XP SP2 on x86,
    // this check reduces the size of the dumped symbol file by a third.
    if (type != last_type || rva != last_rva || code_size != last_code_size ||
        prolog_size != last_prolog_size) {
      fprintf(output_, "STACK WIN %x %x %x %x %x %x %x %x %x %d ",
              type, rva, code_size, prolog_size, epilog_size,
              parameter_size, saved_register_size, local_size, max_stack_size,
              program_string_result == S_OK);
      if (program_string_result == S_OK) {
        fprintf(output_, "%ws\n", program_string);
      } else {
        fprintf(output_, "%d\n", allocates_base_pointer);
      }

      last_type = type;
      last_rva = rva;
      last_code_size = code_size;
      last_prolog_size = prolog_size;
    }

    frame_data.Release();
  }

  return true;
}

bool PDBSourceLineWriter::PrintCodePublicSymbol(IDiaSymbol *symbol) {
  BOOL is_code;
  if (FAILED(symbol->get_code(&is_code))) {
    return false;
  }
  if (!is_code) {
    return true;
  }

  DWORD rva;
  if (FAILED(symbol->get_relativeVirtualAddress(&rva))) {
    return false;
  }

  CComBSTR name;
  int stack_param_size;
  if (!GetSymbolFunctionName(symbol, &name, &stack_param_size)) {
    return false;
  }

  fprintf(output_, "PUBLIC %x %x %ws\n", rva,
          stack_param_size > 0 ? stack_param_size : 0, name);
  return true;
}

bool PDBSourceLineWriter::PrintPDBInfo() {
  PDBModuleInfo info;
  if (!GetModuleInfo(&info)) {
    return false;
  }

  // Hard-code "windows" for the OS because that's the only thing that makes
  // sense for PDB files.  (This might not be strictly correct for Windows CE
  // support, but we don't care about that at the moment.)
  fprintf(output_, "MODULE windows %ws %ws %ws\n",
          info.cpu.c_str(), info.debug_identifier.c_str(),
          info.debug_file.c_str());

  return true;
}

bool PDBSourceLineWriter::PrintPEInfo() {
  PEModuleInfo info;
  if (!GetPEInfo(&info)) {
    return false;
  }

  fprintf(output_, "INFO CODE_ID %ws %ws\n",
	  info.code_identifier.c_str(),
	  info.code_file.c_str());
  return true;
}

// wcstol_positive_strict is sort of like wcstol, but much stricter.  string
// should be a buffer pointing to a null-terminated string containing only
// decimal digits.  If the entire string can be converted to an integer
// without overflowing, and there are no non-digit characters before the
// result is set to the value and this function returns true.  Otherwise,
// this function returns false.  This is an alternative to the strtol, atoi,
// and scanf families, which are not as strict about input and in some cases
// don't provide a good way for the caller to determine if a conversion was
// successful.
static bool wcstol_positive_strict(wchar_t *string, int *result) {
  int value = 0;
  for (wchar_t *c = string; *c != '\0'; ++c) {
    int last_value = value;
    value *= 10;
    // Detect overflow.
    if (value / 10 != last_value || value < 0) {
      return false;
    }
    if (*c < '0' || *c > '9') {
      return false;
    }
    unsigned int c_value = *c - '0';
    last_value = value;
    value += c_value;
    // Detect overflow.
    if (value < last_value) {
      return false;
    }
    // Forbid leading zeroes unless the string is just "0".
    if (value == 0 && *(c+1) != '\0') {
      return false;
    }
  }
  *result = value;
  return true;
}

bool PDBSourceLineWriter::FindPEFile() {
  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComBSTR symbols_file;
  if (SUCCEEDED(global->get_symbolsFileName(&symbols_file))) {
    wstring file(symbols_file);
    
    // Look for an EXE or DLL file.
    const wchar_t *extensions[] = { L"exe", L"dll" };
    for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
      size_t dot_pos = file.find_last_of(L".");
      if (dot_pos != wstring::npos) {
	file.replace(dot_pos + 1, wstring::npos, extensions[i]);
	// Check if this file exists.
	if (GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES) {
	  code_file_ = file;
	  return true;
	}
      }
    }
  }

  return false;
}

// static
bool PDBSourceLineWriter::GetSymbolFunctionName(IDiaSymbol *function,
                                                BSTR *name,
                                                int *stack_param_size) {
  *stack_param_size = -1;
  const DWORD undecorate_options = UNDNAME_NO_MS_KEYWORDS |
                                   UNDNAME_NO_FUNCTION_RETURNS |
                                   UNDNAME_NO_ALLOCATION_MODEL |
                                   UNDNAME_NO_ALLOCATION_LANGUAGE |
                                   UNDNAME_NO_THISTYPE |
                                   UNDNAME_NO_ACCESS_SPECIFIERS |
                                   UNDNAME_NO_THROW_SIGNATURES |
                                   UNDNAME_NO_MEMBER_TYPE |
                                   UNDNAME_NO_RETURN_UDT_MODEL |
                                   UNDNAME_NO_ECSU;

  // Use get_undecoratedNameEx to get readable C++ names with arguments.
  if (function->get_undecoratedNameEx(undecorate_options, name) != S_OK) {
    if (function->get_name(name) != S_OK) {
      fprintf(stderr, "failed to get function name\n");
      return false;
    }
    // If a name comes from get_name because no undecorated form existed,
    // it's already formatted properly to be used as output.  Don't do any
    // additional processing.
    //
    // MSVC7's DIA seems to not undecorate names in as many cases as MSVC8's.
    // This will result in calling get_name for some C++ symbols, so
    // all of the parameter and return type information may not be included in
    // the name string.
  } else {
    // C++ uses a bogus "void" argument for functions and methods that don't
    // take any parameters.  Take it out of the undecorated name because it's
    // ugly and unnecessary.
    const wchar_t *replace_string = L"(void)";
    const size_t replace_length = wcslen(replace_string);
    const wchar_t *replacement_string = L"()";
    size_t length = wcslen(*name);
    if (length >= replace_length) {
      wchar_t *name_end = *name + length - replace_length;
      if (wcscmp(name_end, replace_string) == 0) {
        WindowsStringUtils::safe_wcscpy(name_end, replace_length,
                                        replacement_string);
        length = wcslen(*name);
      }
    }

    // Undecorate names used for stdcall and fastcall.  These names prefix
    // the identifier with '_' (stdcall) or '@' (fastcall) and suffix it
    // with '@' followed by the number of bytes of parameters, in decimal.
    // If such a name is found, take note of the size and undecorate it.
    // Only do this for names that aren't C++, which is determined based on
    // whether the undecorated name contains any ':' or '(' characters.
    if (!wcschr(*name, ':') && !wcschr(*name, '(') &&
        (*name[0] == '_' || *name[0] == '@')) {
      wchar_t *last_at = wcsrchr(*name + 1, '@');
      if (last_at && wcstol_positive_strict(last_at + 1, stack_param_size)) {
        // If this function adheres to the fastcall convention, it accepts up
        // to the first 8 bytes of parameters in registers (%ecx and %edx).
        // We're only interested in the stack space used for parameters, so
        // so subtract 8 and don't let the size go below 0.
        if (*name[0] == '@') {
          if (*stack_param_size > 8) {
            *stack_param_size -= 8;
          } else {
            *stack_param_size = 0;
          }
        }

        // Undecorate the name by moving it one character to the left in its
        // buffer, and terminating it where the last '@' had been.
        WindowsStringUtils::safe_wcsncpy(*name, length,
                                         *name + 1, last_at - *name - 1);
     } else if (*name[0] == '_') {
        // This symbol's name is encoded according to the cdecl rules.  The
        // name doesn't end in a '@' character followed by a decimal positive
        // integer, so it's not a stdcall name.  Strip off the leading
        // underscore.
        WindowsStringUtils::safe_wcsncpy(*name, length, *name + 1, length);
      }
    }
  }

  return true;
}

// static
int PDBSourceLineWriter::GetFunctionStackParamSize(IDiaSymbol *function) {
  // This implementation is highly x86-specific.

  // Gather the symbols corresponding to data.
  CComPtr<IDiaEnumSymbols> data_children;
  if (FAILED(function->findChildren(SymTagData, NULL, nsNone,
                                    &data_children))) {
    return 0;
  }

  // lowest_base is the lowest %ebp-relative byte offset used for a parameter.
  // highest_end is one greater than the highest offset (i.e. base + length).
  // Stack parameters are assumed to be contiguous, because in reality, they
  // are.
  int lowest_base = INT_MAX;
  int highest_end = INT_MIN;

  CComPtr<IDiaSymbol> child;
  DWORD count;
  while (SUCCEEDED(data_children->Next(1, &child, &count)) && count == 1) {
    // If any operation fails at this point, just proceed to the next child.
    // Use the next_child label instead of continue because child needs to
    // be released before it's reused.  Declare constructable/destructable
    // types early to avoid gotos that cross initializations.
    CComPtr<IDiaSymbol> child_type;

    // DataIsObjectPtr is only used for |this|.  Because |this| can be passed
    // as a stack parameter, look for it in addition to traditional
    // parameters.
    DWORD child_kind;
    if (FAILED(child->get_dataKind(&child_kind)) ||
        (child_kind != DataIsParam && child_kind != DataIsObjectPtr)) {
      goto next_child;
    }

    // Only concentrate on register-relative parameters.  Parameters may also
    // be enregistered (passed directly in a register), but those don't
    // consume any stack space, so they're not of interest.
    DWORD child_location_type;
    if (FAILED(child->get_locationType(&child_location_type)) ||
        child_location_type != LocIsRegRel) {
      goto next_child;
    }

    // Of register-relative parameters, the only ones that make any sense are
    // %ebp- or %esp-relative.  Note that MSVC's debugging information always
    // gives parameters as %ebp-relative even when a function doesn't use a
    // traditional frame pointer and stack parameters are accessed relative to
    // %esp, so just look for %ebp-relative parameters.  If you wanted to
    // access parameters, you'd probably want to treat these %ebp-relative
    // offsets as if they were relative to %esp before a function's prolog
    // executed.
    DWORD child_register;
    if (FAILED(child->get_registerId(&child_register)) ||
        child_register != CV_REG_EBP) {
      goto next_child;
    }

    LONG child_register_offset;
    if (FAILED(child->get_offset(&child_register_offset))) {
      goto next_child;
    }

    // IDiaSymbol::get_type can succeed but still pass back a NULL value.
    if (FAILED(child->get_type(&child_type)) || !child_type) {
      goto next_child;
    }

    ULONGLONG child_length;
    if (FAILED(child_type->get_length(&child_length))) {
      goto next_child;
    }

    int child_end = child_register_offset + static_cast<ULONG>(child_length);
    if (child_register_offset < lowest_base) {
      lowest_base = child_register_offset;
    }
    if (child_end > highest_end) {
      highest_end = child_end;
    }

next_child:
    child.Release();
  }

  int param_size = 0;
  // Make sure lowest_base isn't less than 4, because [%esp+4] is the lowest
  // possible address to find a stack parameter before executing a function's
  // prolog (see above).  Some optimizations cause parameter offsets to be
  // lower than 4, but we're not concerned with those because we're only
  // looking for parameters contained in addresses higher than where the
  // return address is stored.
  if (lowest_base < 4) {
    lowest_base = 4;
  }
  if (highest_end > lowest_base) {
    // All stack parameters are pushed as at least 4-byte quantities.  If the
    // last type was narrower than 4 bytes, promote it.  This assumes that all
    // parameters' offsets are 4-byte-aligned, which is always the case.  Only
    // worry about the last type, because we're not summing the type sizes,
    // just looking at the lowest and highest offsets.
    int remainder = highest_end % 4;
    if (remainder) {
      highest_end += 4 - remainder;
    }

    param_size = highest_end - lowest_base;
  }

  return param_size;
}

bool PDBSourceLineWriter::WriteMap(FILE *map_file) {
  output_ = map_file;

  bool ret = PrintPDBInfo();
  // This is not a critical piece of the symbol file.
  PrintPEInfo();
  ret = ret &&
    PrintSourceFiles() && 
    PrintFunctions() &&
    PrintFrameData();

  output_ = NULL;
  return ret;
}

void PDBSourceLineWriter::Close() {
  session_.Release();
}

bool PDBSourceLineWriter::GetModuleInfo(PDBModuleInfo *info) {
  if (!info) {
    return false;
  }

  info->debug_file.clear();
  info->debug_identifier.clear();
  info->cpu.clear();

  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global))) {
    return false;
  }

  DWORD machine_type;
  // get_machineType can return S_FALSE.
  if (global->get_machineType(&machine_type) == S_OK) {
    // The documentation claims that get_machineType returns a value from
    // the CV_CPU_TYPE_e enumeration, but that's not the case.
    // Instead, it returns one of the IMAGE_FILE_MACHINE values as
    // defined here:
    // http://msdn.microsoft.com/en-us/library/ms680313%28VS.85%29.aspx
    switch (machine_type) {
      case IMAGE_FILE_MACHINE_I386:
        info->cpu = L"x86";
        break;
      case IMAGE_FILE_MACHINE_AMD64:
        info->cpu = L"x86_64";
        break;
      default:
        info->cpu = L"unknown";
        break;
    }
  } else {
    // Unexpected, but handle gracefully.
    info->cpu = L"unknown";
  }

  // DWORD* and int* are not compatible.  This is clean and avoids a cast.
  DWORD age;
  if (FAILED(global->get_age(&age))) {
    return false;
  }

  bool uses_guid;
  if (!UsesGUID(&uses_guid)) {
    return false;
  }

  if (uses_guid) {
    GUID guid;
    if (FAILED(global->get_guid(&guid))) {
      return false;
    }

    // Use the same format that the MS symbol server uses in filesystem
    // hierarchies.
    wchar_t age_string[9];
    swprintf(age_string, sizeof(age_string) / sizeof(age_string[0]),
             L"%x", age);

    // remove when VC++7.1 is no longer supported
    age_string[sizeof(age_string) / sizeof(age_string[0]) - 1] = L'\0';

    info->debug_identifier = GUIDString::GUIDToSymbolServerWString(&guid);
    info->debug_identifier.append(age_string);
  } else {
    DWORD signature;
    if (FAILED(global->get_signature(&signature))) {
      return false;
    }

    // Use the same format that the MS symbol server uses in filesystem
    // hierarchies.
    wchar_t identifier_string[17];
    swprintf(identifier_string,
             sizeof(identifier_string) / sizeof(identifier_string[0]),
             L"%08X%x", signature, age);

    // remove when VC++7.1 is no longer supported
    identifier_string[sizeof(identifier_string) /
                      sizeof(identifier_string[0]) - 1] = L'\0';

    info->debug_identifier = identifier_string;
  }

  CComBSTR debug_file_string;
  if (FAILED(global->get_symbolsFileName(&debug_file_string))) {
    return false;
  }
  info->debug_file =
      WindowsStringUtils::GetBaseName(wstring(debug_file_string));

  return true;
}

bool PDBSourceLineWriter::GetPEInfo(PEModuleInfo *info) {
  if (!info) {
    return false;
  }

  if (code_file_.empty() && !FindPEFile()) {
    fprintf(stderr, "Couldn't locate EXE or DLL file.\n");
    return false;
  }

  // Convert wchar to native charset because ImageLoad only takes
  // a PSTR as input.
  string code_file;
  if (!WindowsStringUtils::safe_wcstombs(code_file_, &code_file)) {
    return false;
  }

  AutoImage img(ImageLoad((PSTR)code_file.c_str(), NULL));
  if (!img) {
    fprintf(stderr, "Failed to open PE file: %s\n", code_file.c_str());
    return false;
  }

  info->code_file = WindowsStringUtils::GetBaseName(code_file_);

  // The date and time that the file was created by the linker.
  DWORD TimeDateStamp = img->FileHeader->FileHeader.TimeDateStamp;
  // The size of the file in bytes, including all headers.
  DWORD SizeOfImage = 0;
  PIMAGE_OPTIONAL_HEADER64 opt =
    &((PIMAGE_NT_HEADERS64)img->FileHeader)->OptionalHeader;
  if (opt->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    // 64-bit PE file.
    SizeOfImage = opt->SizeOfImage;
  }
  else {
    // 32-bit PE file.
    SizeOfImage = img->FileHeader->OptionalHeader.SizeOfImage;
  }
  wchar_t code_identifier[32];
  swprintf(code_identifier,
	   sizeof(code_identifier) / sizeof(code_identifier[0]),
	   L"%08X%X", TimeDateStamp, SizeOfImage);
  info->code_identifier = code_identifier;

  return true;
}

bool PDBSourceLineWriter::UsesGUID(bool *uses_guid) {
  if (!uses_guid)
    return false;

  CComPtr<IDiaSymbol> global;
  if (FAILED(session_->get_globalScope(&global)))
    return false;

  GUID guid;
  if (FAILED(global->get_guid(&guid)))
    return false;

  DWORD signature;
  if (FAILED(global->get_signature(&signature)))
    return false;

  // There are two possibilities for guid: either it's a real 128-bit GUID
  // as identified in a code module by a new-style CodeView record, or it's
  // a 32-bit signature (timestamp) as identified by an old-style record.
  // See MDCVInfoPDB70 and MDCVInfoPDB20 in minidump_format.h.
  //
  // Because DIA doesn't provide a way to directly determine whether a module
  // uses a GUID or a 32-bit signature, this code checks whether the first 32
  // bits of guid are the same as the signature, and if the rest of guid is
  // zero.  If so, then with a pretty high degree of certainty, there's an
  // old-style CodeView record in use.  This method will only falsely find an
  // an old-style CodeView record if a real 128-bit GUID has its first 32
  // bits set the same as the module's signature (timestamp) and the rest of
  // the GUID is set to 0.  This is highly unlikely.

  GUID signature_guid = {signature};  // 0-initializes other members
  *uses_guid = !IsEqualGUID(guid, signature_guid);
  return true;
}

}  // namespace google_breakpad
