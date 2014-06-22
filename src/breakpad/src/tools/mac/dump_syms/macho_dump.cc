// Copyright (c) 2010, Google Inc.
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

// Original author: Jim Blandy <jimb@mozilla.com> <jimb@red-bean.com>

// macho_dump.cc: Dump the contents of a Mach-O file. This is mostly
// a test program for the Mach_O::FatReader and Mach_O::Reader classes.

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <mach-o/arch.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "common/byte_cursor.h"
#include "common/mac/arch_utilities.h"
#include "common/mac/macho_reader.h"

using google_breakpad::ByteBuffer;
using std::ostringstream;
using std::string;
using std::vector;

namespace {
namespace mach_o = google_breakpad::mach_o;

string program_name;

int check_syscall(int result, const char *operation, const char *filename) {
  if (result < 0) {
    fprintf(stderr, "%s: %s '%s': %s\n",
            program_name.c_str(), operation,
            filename, strerror(errno));
    exit(1);
  }
  return result;
}

class DumpSection: public mach_o::Reader::SectionHandler {
 public:
  DumpSection() : index_(0) { }
  bool HandleSection(const mach_o::Section &section) {
    printf("        section %d '%s' in segment '%s'\n"
           "          address: 0x%llx\n"
           "          alignment: 1 << %d B\n"
           "          flags: %d\n"
           "          size: %ld\n",
           index_++, section.section_name.c_str(), section.segment_name.c_str(),
           section.address, section.align,
           mach_o::SectionFlags(section.flags),
           section.contents.Size());
    return true;
  }
         
 private:
  int index_;
};

class DumpCommand: public mach_o::Reader::LoadCommandHandler {
 public:
  DumpCommand(mach_o::Reader *reader) : reader_(reader), index_(0) { }
  bool UnknownCommand(mach_o::LoadCommandType type,
                      const ByteBuffer &contents) {
    printf("      load command %d: %d", index_++, type);
    return true;
  }
  bool SegmentCommand(const mach_o::Segment &segment) {
    printf("      load command %d: %s-bit segment '%s'\n"
           "        address: 0x%llx\n"
           "        memory size: 0x%llx\n"
           "        maximum protection: 0x%x\n"
           "        initial protection: 0x%x\n"
           "        flags: %d\n"
           "        section_list size: %ld B\n",
           index_++, (segment.bits_64 ? "64" : "32"), segment.name.c_str(),
           segment.vmaddr, segment.vmsize, segment.maxprot,
           segment.initprot, mach_o::SegmentFlags(segment.flags),
           segment.section_list.Size());
           
    DumpSection dump_section;
    return reader_->WalkSegmentSections(segment, &dump_section);
  }
 private:
  mach_o::Reader *reader_;
  int index_;
};

void DumpFile(const char *filename) {
  int fd = check_syscall(open(filename, O_RDONLY), "opening", filename);
  struct stat attributes;
  check_syscall(fstat(fd, &attributes),
                "getting file attributes for", filename);
  void *mapping = mmap(NULL, attributes.st_size, PROT_READ,
                       MAP_PRIVATE, fd, 0);
  close(fd);
  check_syscall(mapping == (void *)-1 ? -1 : 0,
                "mapping contents of", filename);

  mach_o::FatReader::Reporter fat_reporter(filename);
  mach_o::FatReader fat_reader(&fat_reporter);
  if (!fat_reader.Read(reinterpret_cast<uint8_t *>(mapping),
                       attributes.st_size)) {
    exit(1);
  }
  printf("filename: %s\n", filename);
  size_t object_files_size;
  const struct fat_arch *object_files 
    = fat_reader.object_files(&object_files_size);
  printf("  object file count: %ld\n", object_files_size);
  for (size_t i = 0; i < object_files_size; i++) {
    const struct fat_arch &file = object_files[i];
    const NXArchInfo *fat_arch_info =
        google_breakpad::BreakpadGetArchInfoFromCpuType(
            file.cputype, file.cpusubtype);
    printf("\n  object file %ld:\n"
           "    fat header:\n:"
           "      CPU type: %s (%s)\n"
           "      size: %d B\n"
           "      alignment: 1<<%d B\n",
           i, fat_arch_info->name, fat_arch_info->description,
           file.size, file.align);

    ostringstream name;
    name << filename;
    if (object_files_size > 1)
      name << ", object file #" << i;
    ByteBuffer file_contents(reinterpret_cast<uint8_t *>(mapping)
                             + file.offset, file.size);
    mach_o::Reader::Reporter reporter(name.str());
    mach_o::Reader reader(&reporter);
    if (!reader.Read(file_contents, file.cputype, file.cpusubtype)) {
      exit(1);
    }

    const NXArchInfo *macho_arch_info =
      NXGetArchInfoFromCpuType(reader.cpu_type(),
                               reader.cpu_subtype());
    printf("    Mach-O header:\n"
           "      word size: %s\n" 
           "      CPU type: %s (%s)\n"
           "      File type: %d\n"
           "      flags: %x\n",
           (reader.bits_64() ? "64 bits" : "32 bits"),
           macho_arch_info->name, macho_arch_info->description,
           reader.file_type(), reader.flags());

    DumpCommand dump_command(&reader);
    reader.WalkLoadCommands(&dump_command);
  }
  munmap(mapping, attributes.st_size);
}

}  // namespace

int main(int argc, char **argv) {
  program_name = basename(argv[0]);
  if (argc == 1) {
    fprintf(stderr, "Usage: %s FILE ...\n"
            "Dump the contents of the Mach-O or fat binary files "
            "'FILE ...'.\n", program_name.c_str());
  }
  for (int i = 1; i < argc; i++) {
    DumpFile(argv[i]);
  }
}
