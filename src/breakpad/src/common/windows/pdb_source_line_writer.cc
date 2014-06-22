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

#include "common/windows/pdb_source_line_writer.h"

#include <windows.h>
#include <winnt.h>
#include <atlbase.h>
#include <dia2.h>
#include <ImageHlp.h>
#include <stdio.h>

#include <limits>
#include <set>

#include "common/windows/dia_util.h"
#include "common/windows/guid_string.h"
#include "common/windows/string_utils-inl.h"

// This constant may be missing from DbgHelp.h.  See the documentation for
// IDiaSymbol::get_undecoratedNameEx.
#ifndef UNDNAME_NO_ECSU
#define UNDNAME_NO_ECSU 0x8000  // Suppresses enum/class/struct/union.
#endif  // UNDNAME_NO_ECSU

/*
 * Not defined in WinNT.h for some reason. Definitions taken from:
 * http://uninformed.org/index.cgi?v=4&a=1&p=13
 *
 */
typedef unsigned char UBYTE;

#if !defined(_WIN64)
#define UNW_FLAG_EHANDLER  0x01
#define UNW_FLAG_UHANDLER  0x02
#define UNW_FLAG_CHAININFO 0x04
#endif

union UnwindCode {
  struct {
    UBYTE offset_in_prolog;
    UBYTE unwind_operation_code : 4;
    UBYTE operation_info        : 4;
  };
  USHORT frame_offset;
};

enum UnwindOperationCodes {
  UWOP_PUSH_NONVOL = 0, /* info == register number */
  UWOP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
  UWOP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
  UWOP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
  UWOP_SAVE_NONVOL,     /* info == register number, offset in next slot */
  UWOP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
  // XXX: these are missing from MSDN!
  // See: http://www.osronline.com/ddkx/kmarch/64bitamd_4rs7.htm
  UWOP_SAVE_XMM,
  UWOP_SAVE_XMM_FAR,
  UWOP_SAVE_XMM128,     /* info == XMM reg number, offset in next slot */
  UWOP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
  UWOP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
};

// See: http://msdn.microsoft.com/en-us/library/ddssxxy8.aspx
// Note: some fields removed as we don't use them.
struct UnwindInfo {
  UBYTE version       : 3;
  UBYTE flags         : 5;
  UBYTE size_of_prolog;
  UBYTE count_of_codes;
  UBYTE frame_register : 4;
  UBYTE frame_offset   : 4;
  UnwindCode unwind_code[1];
};

namespace google_breakpad {

namespace {

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

}  // namespace

PDBSourceLineWriter::PDBSourceLineWriter() : output_(NULL) {
}

PDBSourceLineWriter::~PDBSourceLineWriter() {
}

bool PDBSourceLineWriter::SetCodeFile(const wstring &exe_file) {
  if (code_file_.empty()) {
    code_file_ = exe_file;
    return true;
  }
  // Setting a different code file path is an error.  It is success only if the
  // file paths are the same.
  return exe_file == code_file_;
}

bool PDBSourceLineWriter::Open(const wstring &file, FileFormat format) {
  Close();
  code_file_.clear();

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
    // vc100 uses B86AE24D-BF2F-4AC9-B5A2-34B14E4CE11D.
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
          fprintf(stderr, "loadDataForPdb and loadDataFromExe failed for %ws\n",
                  file.c_str());
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

    AddressRangeVector ranges;
    MapAddressRange(image_map_, AddressRange(rva, length), &ranges);
    for (size_t i = 0; i < ranges.size(); ++i) {
      fprintf(output_, "%x %x %d %d\n", ranges[i].rva, ranges[i].length,
              line_num, source_id);
    }
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

  AddressRangeVector ranges;
  MapAddressRange(image_map_, AddressRange(rva, static_cast<DWORD>(length)),
                  &ranges);
  for (size_t i = 0; i < ranges.size(); ++i) {
    fprintf(output_, "FUNC %x %x %x %ws\n",
            ranges[i].rva, ranges[i].length, stack_param_size, name);
  }

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
  ULONG count = 0;
  DWORD rva = 0;
  CComPtr<IDiaSymbol> global;
  HRESULT hr;

  if (FAILED(session_->get_globalScope(&global))) {
    fprintf(stderr, "get_globalScope failed\n");
    return false;
  }

  CComPtr<IDiaEnumSymbols> symbols = NULL;

  // Find all function symbols first.
  std::set<DWORD> rvas;
  hr = global->findChildren(SymTagFunction, NULL, nsNone, &symbols);

  if (SUCCEEDED(hr)) {
    CComPtr<IDiaSymbol> symbol = NULL;

    while (SUCCEEDED(symbols->Next(1, &symbol, &count)) && count == 1) {
      if (SUCCEEDED(symbol->get_relativeVirtualAddress(&rva))) {
        // To maintain existing behavior of one symbol per address, place the
        // rva onto a set here to uniquify them.
        rvas.insert(rva);
      } else {
        fprintf(stderr, "get_relativeVirtualAddress failed on the symbol\n");
        return false;
      }

      symbol.Release();
    }

    symbols.Release();
  }

  // Find all public symbols.  Store public symbols that are not also private
  // symbols for later.
  std::set<DWORD> public_only_rvas;
  hr = global->findChildren(SymTagPublicSymbol, NULL, nsNone, &symbols);

  if (SUCCEEDED(hr)) {
    CComPtr<IDiaSymbol> symbol = NULL;

    while (SUCCEEDED(symbols->Next(1, &symbol, &count)) && count == 1) {
      if (SUCCEEDED(symbol->get_relativeVirtualAddress(&rva))) {
        if (rvas.count(rva) == 0) {
          rvas.insert(rva); // Keep symbols in rva order.
          public_only_rvas.insert(rva);
        }
      } else {
        fprintf(stderr, "get_relativeVirtualAddress failed on the symbol\n");
        return false;
      }

      symbol.Release();
    }

    symbols.Release();
  }

  std::set<DWORD>::iterator it;

  // For each rva, dump the first symbol DIA knows about at the address.
  for (it = rvas.begin(); it != rvas.end(); ++it) {
    CComPtr<IDiaSymbol> symbol = NULL;
    // If the symbol is not in the public list, look for SymTagFunction. This is
    // a workaround to a bug where DIA will hang if searching for a private
    // symbol at an address where only a public symbol exists.
    // See http://connect.microsoft.com/VisualStudio/feedback/details/722366
    if (public_only_rvas.count(*it) == 0) {
      if (SUCCEEDED(session_->findSymbolByRVA(*it, SymTagFunction, &symbol))) {
        // Sometimes findSymbolByRVA returns S_OK, but NULL.
        if (symbol) {
          if (!PrintFunction(symbol, symbol))
            return false;
          symbol.Release();
        }
      } else {
        fprintf(stderr, "findSymbolByRVA SymTagFunction failed\n");
        return false;
      }
    } else if (SUCCEEDED(session_->findSymbolByRVA(*it,
                                                   SymTagPublicSymbol,
                                                   &symbol))) {
      // Sometimes findSymbolByRVA returns S_OK, but NULL.
      if (symbol) {
        if (!PrintCodePublicSymbol(symbol))
          return false;
        symbol.Release();
      }
    } else {
      fprintf(stderr, "findSymbolByRVA SymTagPublicSymbol failed\n");
      return false;
    }
  }

  // When building with PGO, the compiler can split functions into
  // "hot" and "cold" blocks, and move the "cold" blocks out to separate
  // pages, so the function can be noncontiguous. To find these blocks,
  // we have to iterate over all the compilands, and then find blocks
  // that are children of them. We can then find the lexical parents
  // of those blocks and print out an extra FUNC line for blocks
  // that are not contained in their parent functions.
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

  global.Release();
  return true;
}

bool PDBSourceLineWriter::PrintFrameDataUsingPDB() {
  // It would be nice if it were possible to output frame data alongside the
  // associated function, as is done with line numbers, but the DIA API
  // doesn't make it possible to get the frame data in that way.

  CComPtr<IDiaEnumFrameData> frame_data_enum;
  if (!FindTable(session_, &frame_data_enum))
    return false;

  DWORD last_type = std::numeric_limits<DWORD>::max();
  DWORD last_rva = std::numeric_limits<DWORD>::max();
  DWORD last_code_size = 0;
  DWORD last_prolog_size = std::numeric_limits<DWORD>::max();

  CComPtr<IDiaFrameData> frame_data;
  ULONG count = 0;
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
      // The prolog and the code portions of the frame have to be treated
      // independently as they may have independently changed in size, or may
      // even have been split.
      // NOTE: If epilog size is ever non-zero, we have to do something
      //     similar with it.

      // Figure out where the prolog bytes have landed.
      AddressRangeVector prolog_ranges;
      if (prolog_size > 0) {
        MapAddressRange(image_map_, AddressRange(rva, prolog_size),
                        &prolog_ranges);
      }

      // And figure out where the code bytes have landed.
      AddressRangeVector code_ranges;
      MapAddressRange(image_map_,
                      AddressRange(rva + prolog_size,
                                   code_size - prolog_size),
                      &code_ranges);

      struct FrameInfo {
        DWORD rva;
        DWORD code_size;
        DWORD prolog_size;
      };
      std::vector<FrameInfo> frame_infos;

      // Special case: The prolog and the code bytes remain contiguous. This is
      // only done for compactness of the symbol file, and we could actually
      // be outputting independent frame info for the prolog and code portions.
      if (prolog_ranges.size() == 1 && code_ranges.size() == 1 &&
          prolog_ranges[0].end() == code_ranges[0].rva) {
        FrameInfo fi = { prolog_ranges[0].rva,
                         prolog_ranges[0].length + code_ranges[0].length,
                         prolog_ranges[0].length };
        frame_infos.push_back(fi);
      } else {
        // Otherwise we output the prolog and code frame info independently.
        for (size_t i = 0; i < prolog_ranges.size(); ++i) {
          FrameInfo fi = { prolog_ranges[i].rva,
                           prolog_ranges[i].length,
                           prolog_ranges[i].length };
          frame_infos.push_back(fi);
        }
        for (size_t i = 0; i < code_ranges.size(); ++i) {
          FrameInfo fi = { code_ranges[i].rva, code_ranges[i].length, 0 };
          frame_infos.push_back(fi);
        }
      }

      for (size_t i = 0; i < frame_infos.size(); ++i) {
        const FrameInfo& fi(frame_infos[i]);
        fprintf(output_, "STACK WIN %x %x %x %x %x %x %x %x %x %d ",
                type, fi.rva, fi.code_size, fi.prolog_size,
                0 /* epilog_size */, parameter_size, saved_register_size,
                local_size, max_stack_size, program_string_result == S_OK);
        if (program_string_result == S_OK) {
          fprintf(output_, "%ws\n", program_string);
        } else {
          fprintf(output_, "%d\n", allocates_base_pointer);
        }
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

bool PDBSourceLineWriter::PrintFrameDataUsingEXE() {
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
    fprintf(stderr, "Failed to load %s\n", code_file.c_str());
    return false;
  }
  PIMAGE_OPTIONAL_HEADER64 optional_header =
    &(reinterpret_cast<PIMAGE_NT_HEADERS64>(img->FileHeader))->OptionalHeader;
  if (optional_header->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    fprintf(stderr, "Not a PE32+ image\n");
    return false;
  }

  // Read Exception Directory
  DWORD exception_rva = optional_header->
    DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
  DWORD exception_size = optional_header->
    DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
  PIMAGE_RUNTIME_FUNCTION_ENTRY funcs =
    static_cast<PIMAGE_RUNTIME_FUNCTION_ENTRY>(
        ImageRvaToVa(img->FileHeader,
                     img->MappedAddress,
                     exception_rva,
                     &img->LastRvaSection));
  for (DWORD i = 0; i < exception_size / sizeof(*funcs); i++) {
    DWORD unwind_rva = funcs[i].UnwindInfoAddress;
    // handle chaining
    while (unwind_rva & 0x1) {
      unwind_rva ^= 0x1;
      PIMAGE_RUNTIME_FUNCTION_ENTRY chained_func =
        static_cast<PIMAGE_RUNTIME_FUNCTION_ENTRY>(
            ImageRvaToVa(img->FileHeader,
                         img->MappedAddress,
                         unwind_rva,
                         &img->LastRvaSection));
      unwind_rva = chained_func->UnwindInfoAddress;
    }

    UnwindInfo *unwind_info = static_cast<UnwindInfo *>(
        ImageRvaToVa(img->FileHeader,
                     img->MappedAddress,
                     unwind_rva,
                     &img->LastRvaSection));

    DWORD stack_size = 8;  // minimal stack size is 8 for RIP
    DWORD rip_offset = 8;
    do {
      for (UBYTE c = 0; c < unwind_info->count_of_codes; c++) {
        UnwindCode *unwind_code = &unwind_info->unwind_code[c];
        switch (unwind_code->unwind_operation_code) {
          case UWOP_PUSH_NONVOL: {
            stack_size += 8;
            break;
          }
          case UWOP_ALLOC_LARGE: {
            if (unwind_code->operation_info == 0) {
              c++;
              if (c < unwind_info->count_of_codes)
                stack_size += (unwind_code + 1)->frame_offset * 8;
            } else {
              c += 2;
              if (c < unwind_info->count_of_codes)
                stack_size += (unwind_code + 1)->frame_offset |
                              ((unwind_code + 2)->frame_offset << 16);
            }
            break;
          }
          case UWOP_ALLOC_SMALL: {
            stack_size += unwind_code->operation_info * 8 + 8;
            break;
          }
          case UWOP_SET_FPREG:
          case UWOP_SAVE_XMM:
          case UWOP_SAVE_XMM_FAR:
            break;
          case UWOP_SAVE_NONVOL:
          case UWOP_SAVE_XMM128: {
            c++;  // skip slot with offset
            break;
          }
          case UWOP_SAVE_NONVOL_FAR:
          case UWOP_SAVE_XMM128_FAR: {
            c += 2;  // skip 2 slots with offset
            break;
          }
          case UWOP_PUSH_MACHFRAME: {
            if (unwind_code->operation_info) {
              stack_size += 88;
            } else {
              stack_size += 80;
            }
            rip_offset += 80;
            break;
          }
        }
      }
      if (unwind_info->flags & UNW_FLAG_CHAININFO) {
        PIMAGE_RUNTIME_FUNCTION_ENTRY chained_func =
          reinterpret_cast<PIMAGE_RUNTIME_FUNCTION_ENTRY>(
              (unwind_info->unwind_code +
              ((unwind_info->count_of_codes + 1) & ~1)));

        unwind_info = static_cast<UnwindInfo *>(
            ImageRvaToVa(img->FileHeader,
                         img->MappedAddress,
                         chained_func->UnwindInfoAddress,
                         &img->LastRvaSection));
      } else {
        unwind_info = NULL;
      }
    } while (unwind_info);
    fprintf(output_, "STACK CFI INIT %x %x .cfa: $rsp .ra: .cfa %d - ^\n",
            funcs[i].BeginAddress,
            funcs[i].EndAddress - funcs[i].BeginAddress, rip_offset);
    fprintf(output_, "STACK CFI %x .cfa: $rsp %d +\n",
            funcs[i].BeginAddress, stack_size);
  }

  return true;
}

bool PDBSourceLineWriter::PrintFrameData() {
  PDBModuleInfo info;
  if (GetModuleInfo(&info) && info.cpu == L"x86_64") {
    return PrintFrameDataUsingEXE();
  } else {
    return PrintFrameDataUsingPDB();
  }
  return false;
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

  AddressRangeVector ranges;
  MapAddressRange(image_map_, AddressRange(rva, 1), &ranges);
  for (size_t i = 0; i < ranges.size(); ++i) {
    fprintf(output_, "PUBLIC %x %x %ws\n", ranges[i].rva,
            stack_param_size > 0 ? stack_param_size : 0, name);
  }
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

  // Load the OMAP information, and disable auto-translation of addresses in
  // preference of doing it ourselves.
  OmapData omap_data;
  if (!GetOmapDataAndDisableTranslation(session_, &omap_data))
    return false;
  BuildImageMap(omap_data, &image_map_);

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
  } else {
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
