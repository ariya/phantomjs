// Copyright (c) 2010 Google Inc.
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

// macho_reader_unittest.cc: Unit tests for google_breakpad::Mach_O::FatReader
// and google_breakpad::Mach_O::Reader.

#include <map>
#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/mac/macho_reader.h"
#include "common/test_assembler.h"

namespace mach_o = google_breakpad::mach_o;
namespace test_assembler = google_breakpad::test_assembler;

using mach_o::FatReader;
using mach_o::FileFlags;
using mach_o::FileType;
using mach_o::LoadCommandType;
using mach_o::Reader;
using mach_o::Section;
using mach_o::SectionMap;
using mach_o::Segment;
using test_assembler::Endianness;
using test_assembler::Label;
using test_assembler::kBigEndian;
using test_assembler::kLittleEndian;
using test_assembler::kUnsetEndian;
using google_breakpad::ByteBuffer;
using std::map;
using std::string;
using std::vector;
using testing::AllOf;
using testing::DoAll;
using testing::Field;
using testing::InSequence;
using testing::Matcher;
using testing::Return;
using testing::SaveArg;
using testing::Test;
using testing::_;


// Mock classes for the reader's various reporters and handlers.

class MockFatReaderReporter: public FatReader::Reporter {
 public:
  MockFatReaderReporter(const string &filename)
      : FatReader::Reporter(filename) { }
  MOCK_METHOD0(BadHeader, void());
  MOCK_METHOD0(MisplacedObjectFile, void());
  MOCK_METHOD0(TooShort, void());
};

class MockReaderReporter: public Reader::Reporter {
 public:
  MockReaderReporter(const string &filename) : Reader::Reporter(filename) { }
  MOCK_METHOD0(BadHeader, void());
  MOCK_METHOD4(CPUTypeMismatch, void(cpu_type_t cpu_type,
                                     cpu_subtype_t cpu_subtype,
                                     cpu_type_t expected_cpu_type,
                                     cpu_subtype_t expected_cpu_subtype));
  MOCK_METHOD0(HeaderTruncated, void());
  MOCK_METHOD0(LoadCommandRegionTruncated, void());
  MOCK_METHOD3(LoadCommandsOverrun, void(size_t claimed, size_t i,
                                         LoadCommandType type));
  MOCK_METHOD2(LoadCommandTooShort, void(size_t i, LoadCommandType type));
  MOCK_METHOD1(SectionsMissing, void(const string &name));
  MOCK_METHOD1(MisplacedSegmentData, void(const string &name));
  MOCK_METHOD2(MisplacedSectionData, void(const string &section,
                                          const string &segment));
  MOCK_METHOD0(MisplacedSymbolTable, void());
  MOCK_METHOD1(UnsupportedCPUType, void(cpu_type_t cpu_type));
};

class MockLoadCommandHandler: public Reader::LoadCommandHandler {
 public:
  MOCK_METHOD2(UnknownCommand, bool(LoadCommandType, const ByteBuffer &));
  MOCK_METHOD1(SegmentCommand, bool(const Segment &));
  MOCK_METHOD2(SymtabCommand,  bool(const ByteBuffer &, const ByteBuffer &));
};

class MockSectionHandler: public Reader::SectionHandler {
 public:
  MOCK_METHOD1(HandleSection, bool(const Section &section));
};


// Tests for mach_o::FatReader.

// Since the effect of these functions is to write to stderr, the
// results of these tests must be inspected by hand.
TEST(FatReaderReporter, BadHeader) {
  FatReader::Reporter reporter("filename");
  reporter.BadHeader();
}

TEST(FatReaderReporter, MisplacedObjectFile) {
  FatReader::Reporter reporter("filename");
  reporter.MisplacedObjectFile();
}

TEST(FatReaderReporter, TooShort) {
  FatReader::Reporter reporter("filename");
  reporter.TooShort();
}

TEST(MachOReaderReporter, BadHeader) {
  Reader::Reporter reporter("filename");
  reporter.BadHeader();
}

TEST(MachOReaderReporter, CPUTypeMismatch) {
  Reader::Reporter reporter("filename");
  reporter.CPUTypeMismatch(CPU_TYPE_I386, CPU_SUBTYPE_X86_ALL,
                           CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL);
}

TEST(MachOReaderReporter, HeaderTruncated) {
  Reader::Reporter reporter("filename");
  reporter.HeaderTruncated();
}

TEST(MachOReaderReporter, LoadCommandRegionTruncated) {
  Reader::Reporter reporter("filename");
  reporter.LoadCommandRegionTruncated();
}

TEST(MachOReaderReporter, LoadCommandsOverrun) {
  Reader::Reporter reporter("filename");
  reporter.LoadCommandsOverrun(10, 9, LC_DYSYMTAB);
  reporter.LoadCommandsOverrun(10, 9, 0);
}

TEST(MachOReaderReporter, LoadCommandTooShort) {
  Reader::Reporter reporter("filename");
  reporter.LoadCommandTooShort(11, LC_SYMTAB);
}

TEST(MachOReaderReporter, SectionsMissing) {
  Reader::Reporter reporter("filename");
  reporter.SectionsMissing("segment name");
}

TEST(MachOReaderReporter, MisplacedSegmentData) {
  Reader::Reporter reporter("filename");
  reporter.MisplacedSegmentData("segment name");
}

TEST(MachOReaderReporter, MisplacedSectionData) {
  Reader::Reporter reporter("filename");
  reporter.MisplacedSectionData("section name", "segment name");
}

TEST(MachOReaderReporter, MisplacedSymbolTable) {
  Reader::Reporter reporter("filename");
  reporter.MisplacedSymbolTable();
}

TEST(MachOReaderReporter, UnsupportedCPUType) {
  Reader::Reporter reporter("filename");
  reporter.UnsupportedCPUType(CPU_TYPE_HPPA);
}

struct FatReaderFixture {
  FatReaderFixture()
      : fat(kBigEndian),
        reporter("reporter filename"),
        reader(&reporter), object_files(), object_files_size() { 
    EXPECT_CALL(reporter, BadHeader()).Times(0);
    EXPECT_CALL(reporter, TooShort()).Times(0);

    // here, start, and Mark are file offsets in 'fat'.
    fat.start() = 0;
  }
  // Append a 'fat_arch' entry to 'fat', with the given field values.
  void AppendFatArch(cpu_type_t type, cpu_subtype_t subtype,
                     Label offset, Label size, uint32_t align) {
    fat
        .B32(type)
        .B32(subtype)
        .B32(offset)
        .B32(size)
        .B32(align);
  }
  // Append |n| dummy 'fat_arch' entries to 'fat'. The cpu type and
  // subtype have unrealistic values.
  void AppendDummyArchEntries(int n) {
    for (int i = 0; i < n; i++)
      AppendFatArch(0xb68ad617, 0x715a0840, 0, 0, 1);
  }
  void ReadFat(bool expect_parse_success = true) {
    ASSERT_TRUE(fat.GetContents(&contents));
    fat_bytes = reinterpret_cast<const uint8_t *>(contents.data());
    if (expect_parse_success) {
      EXPECT_TRUE(reader.Read(fat_bytes, contents.size()));
      object_files = reader.object_files(&object_files_size);
    }
    else
      EXPECT_FALSE(reader.Read(fat_bytes, contents.size()));
  }
  test_assembler::Section fat;
  MockFatReaderReporter reporter;
  FatReader reader;
  string contents;
  const uint8_t *fat_bytes;
  const struct fat_arch *object_files;
  size_t object_files_size;
};

class FatReaderTest: public FatReaderFixture, public Test { };

TEST_F(FatReaderTest, BadMagic) {
  EXPECT_CALL(reporter, BadHeader()).Times(1);
  fat
      .B32(0xcafed00d)           // magic number (incorrect)
      .B32(10);                  // number of architectures
  AppendDummyArchEntries(10);
  ReadFat(false);
}

TEST_F(FatReaderTest, HeaderTooShort) {
  EXPECT_CALL(reporter, TooShort()).Times(1);
  fat
      .B32(0xcafebabe);             // magic number
  ReadFat(false);
}

TEST_F(FatReaderTest, ObjectListTooShort) {
  EXPECT_CALL(reporter, TooShort()).Times(1);
  fat
      .B32(0xcafebabe)              // magic number
      .B32(10);                     // number of architectures
  AppendDummyArchEntries(9);        // nine dummy architecture entries...
  fat                               // and a tenth, missing a byte at the end
      .B32(0x3d46c8fc)              // cpu type
      .B32(0x8a7bfb01)              // cpu subtype
      .B32(0)                       // offset
      .B32(0)                       // size
      .Append(3, '*');              // one byte short of a four-byte alignment
  ReadFat(false);
}

TEST_F(FatReaderTest, DataTooShort) {
  EXPECT_CALL(reporter, MisplacedObjectFile()).Times(1);
  Label arch_data;
  fat
      .B32(0xcafebabe)              // magic number
      .B32(1);                      // number of architectures
  AppendFatArch(0xb4d4a366, 0x4ba4f525, arch_data, 40, 0);
  fat
      .Mark(&arch_data)             // file data begins here
      .Append(30, '*');             // only 30 bytes, not 40 as header claims
  ReadFat(false);
}

TEST_F(FatReaderTest, NoObjectFiles) {
  fat
      .B32(0xcafebabe)              // magic number
      .B32(0);                      // number of architectures
  ReadFat();
  EXPECT_EQ(0U, object_files_size);
}

TEST_F(FatReaderTest, OneObjectFile) {
  Label obj1_offset;
  fat
      .B32(0xcafebabe)              // magic number
      .B32(1);                      // number of architectures
  // First object file list entry
  AppendFatArch(0x5e3a6e91, 0x52ccd852, obj1_offset, 0x42, 0x355b15b2);
  // First object file data
  fat
      .Mark(&obj1_offset)           
      .Append(0x42, '*');           // dummy contents
  ReadFat();
  ASSERT_EQ(1U, object_files_size);
  EXPECT_EQ(0x5e3a6e91, object_files[0].cputype);
  EXPECT_EQ(0x52ccd852, object_files[0].cpusubtype);
  EXPECT_EQ(obj1_offset.Value(), object_files[0].offset);
  EXPECT_EQ(0x42U, object_files[0].size);
  EXPECT_EQ(0x355b15b2U, object_files[0].align);
}

TEST_F(FatReaderTest, ThreeObjectFiles) {
  Label obj1, obj2, obj3;
  fat
      .B32(0xcafebabe)              // magic number
      .B32(3);                      // number of architectures
  // Three object file list entries.
  AppendFatArch(0x0cb92c30, 0x6a159a71, obj1, 0xfb4, 0x2615dbe8);
  AppendFatArch(0x0f3f1cbb, 0x6c55e90f, obj2, 0xc31, 0x83af6ffd);
  AppendFatArch(0x3717276d, 0x10ecdc84, obj3, 0x4b3, 0x035267d7);
  fat
      // First object file data
      .Mark(&obj1)           
      .Append(0xfb4, '*')           // dummy contents
      // Second object file data
      .Mark(&obj2)           
      .Append(0xc31, '%')           // dummy contents
      // Third object file data
      .Mark(&obj3)           
      .Append(0x4b3, '^');          // dummy contents
  
  ReadFat();

  ASSERT_EQ(3U, object_files_size);

  // First object file.
  EXPECT_EQ(0x0cb92c30, object_files[0].cputype);
  EXPECT_EQ(0x6a159a71, object_files[0].cpusubtype);
  EXPECT_EQ(obj1.Value(), object_files[0].offset);
  EXPECT_EQ(0xfb4U, object_files[0].size);
  EXPECT_EQ(0x2615dbe8U, object_files[0].align);

  // Second object file.
  EXPECT_EQ(0x0f3f1cbb, object_files[1].cputype);
  EXPECT_EQ(0x6c55e90f, object_files[1].cpusubtype);
  EXPECT_EQ(obj2.Value(), object_files[1].offset);
  EXPECT_EQ(0xc31U, object_files[1].size);
  EXPECT_EQ(0x83af6ffdU, object_files[1].align);

  // Third object file.
  EXPECT_EQ(0x3717276d, object_files[2].cputype);
  EXPECT_EQ(0x10ecdc84, object_files[2].cpusubtype);
  EXPECT_EQ(obj3.Value(), object_files[2].offset);
  EXPECT_EQ(0x4b3U, object_files[2].size);
  EXPECT_EQ(0x035267d7U, object_files[2].align);
}

TEST_F(FatReaderTest, BigEndianMachO32) {
  fat.set_endianness(kBigEndian);
  fat
      .D32(0xfeedface)                  // Mach-O file magic number
      .D32(0x1a9d0518)                  // cpu type
      .D32(0x1b779357)                  // cpu subtype
      .D32(0x009df67e)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // the load commands occupy no bytes
      .D32(0x21987a99);                 // flags

  ReadFat();

  // FatReader should treat a Mach-O file as if it were a fat binary file
  // containing one object file --- the whole thing.
  ASSERT_EQ(1U, object_files_size);
  EXPECT_EQ(0x1a9d0518, object_files[0].cputype);
  EXPECT_EQ(0x1b779357, object_files[0].cpusubtype);
  EXPECT_EQ(0U, object_files[0].offset);
  EXPECT_EQ(contents.size(), object_files[0].size);
}

TEST_F(FatReaderTest, BigEndianMachO64) {
  fat.set_endianness(kBigEndian);
  fat
      .D32(0xfeedfacf)                  // Mach-O 64-bit file magic number
      .D32(0x5aff8487)                  // cpu type
      .D32(0x4c6a57f7)                  // cpu subtype
      .D32(0x4392d2c8)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // the load commands occupy no bytes
      .D32(0x1b033eea);                 // flags

  ReadFat();

  // FatReader should treat a Mach-O file as if it were a fat binary file
  // containing one object file --- the whole thing.
  ASSERT_EQ(1U, object_files_size);
  EXPECT_EQ(0x5aff8487, object_files[0].cputype);
  EXPECT_EQ(0x4c6a57f7, object_files[0].cpusubtype);
  EXPECT_EQ(0U, object_files[0].offset);
  EXPECT_EQ(contents.size(), object_files[0].size);
}

TEST_F(FatReaderTest, LittleEndianMachO32) {
  fat.set_endianness(kLittleEndian);
  fat
      .D32(0xfeedface)                  // Mach-O file magic number
      .D32(0x1a9d0518)                  // cpu type
      .D32(0x1b779357)                  // cpu subtype
      .D32(0x009df67e)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // the load commands occupy no bytes
      .D32(0x21987a99);                 // flags

  ReadFat();

  // FatReader should treat a Mach-O file as if it were a fat binary file
  // containing one object file --- the whole thing.
  ASSERT_EQ(1U, object_files_size);
  EXPECT_EQ(0x1a9d0518, object_files[0].cputype);
  EXPECT_EQ(0x1b779357, object_files[0].cpusubtype);
  EXPECT_EQ(0U, object_files[0].offset);
  EXPECT_EQ(contents.size(), object_files[0].size);
}

TEST_F(FatReaderTest, LittleEndianMachO64) {
  fat.set_endianness(kLittleEndian);
  fat
      .D32(0xfeedfacf)                  // Mach-O 64-bit file magic number
      .D32(0x5aff8487)                  // cpu type
      .D32(0x4c6a57f7)                  // cpu subtype
      .D32(0x4392d2c8)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // the load commands occupy no bytes
      .D32(0x1b033eea);                 // flags

  ReadFat();

  // FatReader should treat a Mach-O file as if it were a fat binary file
  // containing one object file --- the whole thing.
  ASSERT_EQ(1U, object_files_size);
  EXPECT_EQ(0x5aff8487, object_files[0].cputype);
  EXPECT_EQ(0x4c6a57f7, object_files[0].cpusubtype);
  EXPECT_EQ(0U, object_files[0].offset);
  EXPECT_EQ(contents.size(), object_files[0].size);
}

TEST_F(FatReaderTest, IncompleteMach) {
  fat.set_endianness(kLittleEndian);
  fat
      .D32(0xfeedfacf)                  // Mach-O 64-bit file magic number
      .D32(0x5aff8487);                 // cpu type
      // Truncated!

  EXPECT_CALL(reporter, TooShort()).WillOnce(Return());

  ReadFat(false);
}


// General mach_o::Reader tests.

// Dynamically scoped configuration data.
class WithConfiguration {
 public:
  // Establish the given parameters as the default for SizedSections
  // created within the dynamic scope of this instance.
  WithConfiguration(Endianness endianness, size_t word_size)
      : endianness_(endianness), word_size_(word_size), saved_(current_) {
    current_ = this;
  }
  ~WithConfiguration() { current_ = saved_; }
  static Endianness endianness() { 
    assert(current_);
    return current_->endianness_;
  }
  static size_t word_size() { 
    assert(current_);
    return current_->word_size_;
  }

 private:
  // The innermost WithConfiguration in whose dynamic scope we are
  // currently executing.
  static WithConfiguration *current_;

  // The innermost WithConfiguration whose dynamic scope encloses this
  // WithConfiguration.
  Endianness endianness_;
  size_t word_size_;
  WithConfiguration *saved_;
};

WithConfiguration *WithConfiguration::current_ = NULL;

// A test_assembler::Section with a size that we can cite. The start(),
// Here() and Mark() member functions of a SizedSection always represent
// offsets within the overall file.
class SizedSection: public test_assembler::Section {
 public:
  // Construct a section of the given endianness and word size.
  explicit SizedSection(Endianness endianness, size_t word_size)
      : test_assembler::Section(endianness), word_size_(word_size) {
    assert(word_size_ == 32 || word_size_ == 64);
  }
  SizedSection()
      : test_assembler::Section(WithConfiguration::endianness()),
        word_size_(WithConfiguration::word_size()) {
    assert(word_size_ == 32 || word_size_ == 64);
  }

  // Access/set this section's word size.
  size_t word_size() const { return word_size_; }
  void set_word_size(size_t word_size) { 
    assert(word_size_ == 32 || word_size_ == 64);
    word_size_ = word_size;
  }

  // Return a label representing the size this section will have when it
  // is Placed in some containing section.
  Label final_size() const { return final_size_; }

  // Append SECTION to the end of this section, and call its Finish member.
  // Return a reference to this section.
  SizedSection &Place(SizedSection *section) {
    assert(section->endianness() == endianness());
    section->Finish();
    section->start() = Here();
    test_assembler::Section::Append(*section);
    return *this;
  }

 protected:
  // Mark this section's contents as complete. For plain SizedSections, we
  // set SECTION's start to its position in this section, and its final_size
  // label to its current size. Derived classes can extend this as needed
  // for their additional semantics.
  virtual void Finish() {
    final_size_ = Size();
  }

  // The word size for this data: either 32 or 64.
  size_t word_size_;

 private:
  // This section's final size, set when we are placed in some other
  // SizedSection.
  Label final_size_;
};

// A SizedSection that is loaded into memory at a particular address.
class LoadedSection: public SizedSection {
 public:
  explicit LoadedSection(Label address = Label()) : address_(address) { }

  // Return a label representing this section's address.
  Label address() const { return address_; }

  // Placing a loaded section within a loaded section sets the relationship
  // between their addresses.
  LoadedSection &Place(LoadedSection *section) {
    section->address() = address() + Size();
    SizedSection::Place(section);
    return *this;
  }

 protected:
  // The address at which this section's contents will be loaded.
  Label address_;
};
  
// A SizedSection representing a segment load command.
class SegmentLoadCommand: public SizedSection {
 public:
  SegmentLoadCommand() : section_count_(0) { }

  // Append a segment load command header with the given characteristics.
  // The load command will refer to CONTENTS, which must be Placed in the
  // file separately, at the desired position. Return a reference to this
  // section.
  SegmentLoadCommand &Header(const string &name, const LoadedSection &contents,
                             uint32_t maxprot, uint32_t initprot,
                             uint32_t flags) {
    assert(contents.word_size() == word_size());
    D32(word_size() == 32 ? LC_SEGMENT : LC_SEGMENT_64);
    D32(final_size());
    AppendCString(name, 16);
    Append(endianness(), word_size() / 8, contents.address());
    Append(endianness(), word_size() / 8, vmsize_);
    Append(endianness(), word_size() / 8, contents.start());
    Append(endianness(), word_size() / 8, contents.final_size());
    D32(maxprot);
    D32(initprot);
    D32(final_section_count_);
    D32(flags);

    content_final_size_ = contents.final_size();

    return *this;
  }

  // Return a label representing the size of this segment when loaded into
  // memory. If this label is still undefined by the time we place this
  // segment, it defaults to the final size of the segment's in-file
  // contents. Return a reference to this load command.
  Label &vmsize() { return vmsize_; }

  // Add a section entry with the given characteristics to this segment
  // load command. Return a reference to this. The section entry will refer
  // to CONTENTS, which must be Placed in the segment's contents
  // separately, at the desired position.
  SegmentLoadCommand &AppendSectionEntry(const string &section_name,
                                         const string &segment_name,
                                         uint32_t alignment, uint32_t flags,
                                         const LoadedSection &contents) {
    AppendCString(section_name, 16);
    AppendCString(segment_name, 16);
    Append(endianness(), word_size() / 8, contents.address());
    Append(endianness(), word_size() / 8, contents.final_size());
    D32(contents.start());
    D32(alignment);
    D32(0);                  // relocations start
    D32(0);                  // relocations size
    D32(flags);
    D32(0x93656b95);         // reserved1
    D32(0xc35a2473);         // reserved2
    if (word_size() == 64)
      D32(0x70284b95);       // reserved3

    section_count_++;

    return *this;
  }

 protected:
  void Finish() {
    final_section_count_ = section_count_;
    if (!vmsize_.IsKnownConstant())
      vmsize_ = content_final_size_;
    SizedSection::Finish();
  }

 private:
  // The number of sections that have been added to this segment so far.
  size_t section_count_;

  // A label representing the final number of sections this segment will hold.
  Label final_section_count_;

  // The size of the contents for this segment present in the file.
  Label content_final_size_;

  // A label representing the size of this segment when loaded; this can be
  // larger than the size of its file contents, the difference being
  // zero-filled. If not set explicitly by calling set_vmsize, this is set
  // equal to the size of the contents.
  Label vmsize_;
};

// A SizedSection holding a list of Mach-O load commands.
class LoadCommands: public SizedSection {
 public:
  LoadCommands() : command_count_(0) { }

  // Return a label representing the final load command count.
  Label final_command_count() const { return final_command_count_; }

  // Increment the command count; return a reference to this section.
  LoadCommands &CountCommand() {
    command_count_++;
    return *this;
  }

  // Place COMMAND, containing a load command, at the end of this section.
  // Return a reference to this section.
  LoadCommands &Place(SizedSection *section) {
    SizedSection::Place(section);
    CountCommand();
    return *this;
  }

 protected:
  // Mark this load command list as complete.
  void Finish() {
    SizedSection::Finish();
    final_command_count_ = command_count_;
  }

 private:
  // The number of load commands we have added to this file so far.
  size_t command_count_;

  // A label representing the final command count.
  Label final_command_count_;
};

// A SizedSection holding the contents of a Mach-O file. Within a
// MachOFile, the start, Here, and Mark members refer to file offsets.
class MachOFile: public SizedSection {
 public:
  MachOFile() { 
    start() = 0;
  }

  // Create a Mach-O file header using the given characteristics and load
  // command list. This Places COMMANDS immediately after the header.
  // Return a reference to this section.
  MachOFile &Header(LoadCommands *commands,
                    cpu_type_t cpu_type = CPU_TYPE_X86,
                    cpu_subtype_t cpu_subtype = CPU_SUBTYPE_I386_ALL,
                    FileType file_type = MH_EXECUTE,
                    uint32_t file_flags = (MH_TWOLEVEL |
                                           MH_DYLDLINK |
                                           MH_NOUNDEFS)) {
    D32(word_size() == 32 ? 0xfeedface : 0xfeedfacf);  // magic number
    D32(cpu_type);                              // cpu type
    D32(cpu_subtype);                           // cpu subtype
    D32(file_type);                             // file type
    D32(commands->final_command_count());       // number of load commands
    D32(commands->final_size());                // their size in bytes
    D32(file_flags);                            // flags
    if (word_size() == 64)
      D32(0x55638b90);                          // reserved
    Place(commands);
    return *this;
  }
};


struct ReaderFixture {
  ReaderFixture()
      : reporter("reporter filename"),
        reader(&reporter) { 
    EXPECT_CALL(reporter, BadHeader()).Times(0);
    EXPECT_CALL(reporter, CPUTypeMismatch(_, _, _, _)).Times(0);
    EXPECT_CALL(reporter, HeaderTruncated()).Times(0);
    EXPECT_CALL(reporter, LoadCommandRegionTruncated()).Times(0);
    EXPECT_CALL(reporter, LoadCommandsOverrun(_, _, _)).Times(0);
    EXPECT_CALL(reporter, LoadCommandTooShort(_, _)).Times(0);
    EXPECT_CALL(reporter, SectionsMissing(_)).Times(0);
    EXPECT_CALL(reporter, MisplacedSegmentData(_)).Times(0);
    EXPECT_CALL(reporter, MisplacedSectionData(_, _)).Times(0);
    EXPECT_CALL(reporter, MisplacedSymbolTable()).Times(0);
    EXPECT_CALL(reporter, UnsupportedCPUType(_)).Times(0);

    EXPECT_CALL(load_command_handler, UnknownCommand(_, _)).Times(0);
    EXPECT_CALL(load_command_handler, SegmentCommand(_)).Times(0);
  }

  void ReadFile(MachOFile *file,
                bool expect_parse_success,
                cpu_type_t expected_cpu_type,
                cpu_subtype_t expected_cpu_subtype) {
    ASSERT_TRUE(file->GetContents(&file_contents));
    file_bytes = reinterpret_cast<const uint8_t *>(file_contents.data());
    if (expect_parse_success) {
      EXPECT_TRUE(reader.Read(file_bytes,
                              file_contents.size(),
                              expected_cpu_type,
                              expected_cpu_subtype));
    } else {
      EXPECT_FALSE(reader.Read(file_bytes,
                               file_contents.size(),
                               expected_cpu_type,
                               expected_cpu_subtype));
    }
  }

  string file_contents;
  const uint8_t *file_bytes;
  MockReaderReporter reporter;
  Reader reader;
  MockLoadCommandHandler load_command_handler;
  MockSectionHandler section_handler;
};

class ReaderTest: public ReaderFixture, public Test { };

TEST_F(ReaderTest, BadMagic) {
  WithConfiguration config(kLittleEndian, 32);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0x67bdebe1)                  // Not a proper magic number.
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  EXPECT_CALL(reporter, BadHeader()).WillOnce(Return());
  ReadFile(&file, false, CPU_TYPE_ANY, kCPUSubType);
}

TEST_F(ReaderTest, MismatchedMagic) {
  WithConfiguration config(kLittleEndian, 32);
  const cpu_type_t kCPUType = CPU_TYPE_I386;
  const cpu_subtype_t kCPUSubType = CPU_SUBTYPE_X86_ALL;
  MachOFile file;
  file
      .D32(MH_CIGAM)                    // Right magic, but winds up wrong
                                        // due to bitswapping
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  EXPECT_CALL(reporter, BadHeader()).WillOnce(Return());
  ReadFile(&file, false, kCPUType, kCPUSubType);
}

TEST_F(ReaderTest, ShortMagic) {
  WithConfiguration config(kBigEndian, 32);
  MachOFile file;
  file
      .D16(0xfeed);                     // magic number
                                        // truncated!
  EXPECT_CALL(reporter, HeaderTruncated()).WillOnce(Return());
  ReadFile(&file, false, CPU_TYPE_ANY, 0);
}

TEST_F(ReaderTest, ShortHeader) {
  WithConfiguration config(kBigEndian, 32);
  const cpu_type_t kCPUType = CPU_TYPE_ANY;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedface)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0);                          // they occupy no bytes
  EXPECT_CALL(reporter, HeaderTruncated()).WillOnce(Return());
  ReadFile(&file, false, CPU_TYPE_ANY, kCPUSubType);
}

TEST_F(ReaderTest, MismatchedCPU) {
  WithConfiguration config(kBigEndian, 32);
  const cpu_type_t kCPUType = CPU_TYPE_I386;
  const cpu_subtype_t kCPUSubType = CPU_SUBTYPE_X86_ALL;
  MachOFile file;
  file
      .D32(MH_MAGIC)                    // Right magic for PPC (once bitswapped)
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  EXPECT_CALL(reporter,
              CPUTypeMismatch(CPU_TYPE_I386, CPU_SUBTYPE_X86_ALL,
                              CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL))
    .WillOnce(Return());
  ReadFile(&file, false, CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL);
}

TEST_F(ReaderTest, LittleEndian32Bit) {
  WithConfiguration config(kLittleEndian, 32);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedface)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
           ReadFile(&file, true, CPU_TYPE_ANY, kCPUSubType);
  EXPECT_FALSE(reader.bits_64());
  EXPECT_FALSE(reader.big_endian());
  EXPECT_EQ(kCPUType,               reader.cpu_type());
  EXPECT_EQ(kCPUSubType,            reader.cpu_subtype());
  EXPECT_EQ(FileType(0x149fc717),   reader.file_type());
  EXPECT_EQ(FileFlags(0x80e71d64),  reader.flags());
}

TEST_F(ReaderTest, LittleEndian64Bit) {
  WithConfiguration config(kLittleEndian, 64);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedfacf)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  ReadFile(&file, true, CPU_TYPE_ANY, kCPUSubType);
  EXPECT_TRUE(reader.bits_64());
  EXPECT_FALSE(reader.big_endian());
  EXPECT_EQ(kCPUType,               reader.cpu_type());
  EXPECT_EQ(kCPUSubType,            reader.cpu_subtype());
  EXPECT_EQ(FileType(0x149fc717),   reader.file_type());
  EXPECT_EQ(FileFlags(0x80e71d64),  reader.flags());
}

TEST_F(ReaderTest, BigEndian32Bit) {
  WithConfiguration config(kBigEndian, 32);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedface)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  ReadFile(&file, true, CPU_TYPE_ANY, kCPUSubType);
  EXPECT_FALSE(reader.bits_64());
  EXPECT_TRUE(reader.big_endian());
  EXPECT_EQ(kCPUType,               reader.cpu_type());
  EXPECT_EQ(kCPUSubType,            reader.cpu_subtype());
  EXPECT_EQ(FileType(0x149fc717),   reader.file_type());
  EXPECT_EQ(FileFlags(0x80e71d64),  reader.flags());
}

TEST_F(ReaderTest, BigEndian64Bit) {
  WithConfiguration config(kBigEndian, 64);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedfacf)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(0)                           // no load commands
      .D32(0)                           // they occupy no bytes
      .D32(0x80e71d64)                  // flags
      .D32(0);                          // reserved
  ReadFile(&file, true, CPU_TYPE_ANY, kCPUSubType);
  EXPECT_TRUE(reader.bits_64());
  EXPECT_TRUE(reader.big_endian());
  EXPECT_EQ(kCPUType,               reader.cpu_type());
  EXPECT_EQ(kCPUSubType,            reader.cpu_subtype());
  EXPECT_EQ(FileType(0x149fc717),   reader.file_type());
  EXPECT_EQ(FileFlags(0x80e71d64),  reader.flags());
}


// Load command tests.

class LoadCommand: public ReaderFixture, public Test { };

TEST_F(LoadCommand, RegionTruncated) {
  WithConfiguration config(kBigEndian, 64);
  const cpu_type_t kCPUType = 0x46b760df;
  const cpu_subtype_t kCPUSubType = 0x76a0e7f7;
  MachOFile file;
  file
      .D32(0xfeedfacf)                  // magic number
      .D32(kCPUType)                    // cpu type
      .D32(kCPUSubType)                 // cpu subtype
      .D32(0x149fc717)                  // file type
      .D32(1)                           // one load command
      .D32(40)                          // occupying 40 bytes
      .D32(0x80e71d64)                  // flags
      .D32(0)                           // reserved
      .Append(20, 0);                   // load command region, not as long as
                                        // Mach-O header promised

  EXPECT_CALL(reporter, LoadCommandRegionTruncated()).WillOnce(Return());

  ReadFile(&file, false, CPU_TYPE_ANY, kCPUSubType);
}

TEST_F(LoadCommand, None) {
  WithConfiguration config(kLittleEndian, 32);
  LoadCommands load_commands;
  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL);

  EXPECT_FALSE(reader.bits_64());
  EXPECT_FALSE(reader.big_endian());
  EXPECT_EQ(CPU_TYPE_X86,         reader.cpu_type());
  EXPECT_EQ(CPU_SUBTYPE_I386_ALL, reader.cpu_subtype());
  EXPECT_EQ(static_cast<uint32_t>(MH_EXECUTE), reader.file_type());
  EXPECT_EQ(FileFlags(MH_TWOLEVEL |
                      MH_DYLDLINK |
                      MH_NOUNDEFS),
            FileFlags(reader.flags()));
  
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, Unknown) {
  WithConfiguration config(kBigEndian, 32);
  LoadCommands load_commands;
  load_commands
      .CountCommand()
      .D32(0x33293d4a)                  // unknown load command
      .D32(40)                          // total size in bytes
      .Append(32, '*');                 // dummy data
  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_FALSE(reader.bits_64());
  EXPECT_TRUE(reader.big_endian());
  EXPECT_EQ(CPU_TYPE_X86,         reader.cpu_type());
  EXPECT_EQ(CPU_SUBTYPE_I386_ALL, reader.cpu_subtype());
  EXPECT_EQ(static_cast<uint32_t>(MH_EXECUTE), reader.file_type());
  EXPECT_EQ(FileFlags(MH_TWOLEVEL |
                      MH_DYLDLINK |
                      MH_NOUNDEFS),
            reader.flags());

  ByteBuffer expected;
  expected.start = file_bytes + load_commands.start().Value();
  expected.end = expected.start + load_commands.final_size().Value();
  EXPECT_CALL(load_command_handler, UnknownCommand(0x33293d4a,
                                                   expected))
      .WillOnce(Return(true));

  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, TypeIncomplete) {
  WithConfiguration config(kLittleEndian, 32);
  LoadCommands load_commands;
  load_commands
      .CountCommand()
      .Append(3, 0);                    // load command type, incomplete

  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, LoadCommandsOverrun(1, 0, 0))
      .WillOnce(Return());
  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, LengthIncomplete) {
  WithConfiguration config(kBigEndian, 64);
  LoadCommands load_commands;
  load_commands
      .CountCommand()
      .D32(LC_SEGMENT);                 // load command
                                                // no length
  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, LoadCommandsOverrun(1, 0, LC_SEGMENT))
      .WillOnce(Return());
  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, ContentIncomplete) {
  WithConfiguration config(kLittleEndian, 64);
  LoadCommands load_commands;
  load_commands
      .CountCommand()
      .D32(LC_SEGMENT)          // load command
      .D32(40)                          // total size in bytes
      .Append(28, '*');                 // not enough dummy data
  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, LoadCommandsOverrun(1, 0, LC_SEGMENT))
      .WillOnce(Return());
  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, SegmentBE32) {
  WithConfiguration config(kBigEndian, 32);
  LoadedSection segment;
  segment.address() = 0x1891139c;
  segment.Append(42, '*');              // segment contents
  SegmentLoadCommand segment_command;
  segment_command
      .Header("froon", segment, 0x94d6dd22, 0x8bdbc319, 0x990a16dd);
  segment_command.vmsize() = 0xcb76584fU;
  LoadCommands load_commands;
  load_commands.Place(&segment_command);
  MachOFile file;
  file
      .Header(&load_commands)
      .Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_CALL(load_command_handler, SegmentCommand(_))
    .WillOnce(DoAll(SaveArg<0>(&actual_segment),
                    Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(false,                        actual_segment.bits_64);
  EXPECT_EQ("froon",                      actual_segment.name);
  EXPECT_EQ(0x1891139cU,                  actual_segment.vmaddr);
  EXPECT_EQ(0xcb76584fU,                  actual_segment.vmsize);
  EXPECT_EQ(0x94d6dd22U,                  actual_segment.maxprot);
  EXPECT_EQ(0x8bdbc319U,                  actual_segment.initprot);
  EXPECT_EQ(0x990a16ddU,                  actual_segment.flags);
  EXPECT_EQ(0U,                           actual_segment.nsects);
  EXPECT_EQ(0U,                           actual_segment.section_list.Size());
  EXPECT_EQ(segment.final_size().Value(), actual_segment.contents.Size());
}

TEST_F(LoadCommand, SegmentLE32) {
  WithConfiguration config(kLittleEndian, 32);
  LoadedSection segment;
  segment.address() = 0x4b877866;
  segment.Append(42, '*');              // segment contents
  SegmentLoadCommand segment_command;
  segment_command
      .Header("sixteenprecisely", segment,
              0x350759ed, 0x6cf5a62e, 0x990a16dd);
  segment_command.vmsize() = 0xcb76584fU;
  LoadCommands load_commands;
  load_commands.Place(&segment_command);
  MachOFile file;
  file
      .Header(&load_commands)
      .Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_CALL(load_command_handler, SegmentCommand(_))
    .WillOnce(DoAll(SaveArg<0>(&actual_segment),
                    Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(false,                        actual_segment.bits_64);
  EXPECT_EQ("sixteenprecisely",           actual_segment.name);
  EXPECT_EQ(0x4b877866U,                  actual_segment.vmaddr);
  EXPECT_EQ(0xcb76584fU,                  actual_segment.vmsize);
  EXPECT_EQ(0x350759edU,                  actual_segment.maxprot);
  EXPECT_EQ(0x6cf5a62eU,                  actual_segment.initprot);
  EXPECT_EQ(0x990a16ddU,                  actual_segment.flags);
  EXPECT_EQ(0U,                           actual_segment.nsects);
  EXPECT_EQ(0U,                           actual_segment.section_list.Size());
  EXPECT_EQ(segment.final_size().Value(), actual_segment.contents.Size());
}

TEST_F(LoadCommand, SegmentBE64) {
  WithConfiguration config(kBigEndian, 64);
  LoadedSection segment;
  segment.address() = 0x79f484f77009e511ULL;
  segment.Append(42, '*');              // segment contents
  SegmentLoadCommand segment_command;
  segment_command
      .Header("froon", segment, 0x42b45da5, 0x8bdbc319, 0xb2335220);
  segment_command.vmsize() = 0x8d92397ce6248abaULL;
  LoadCommands load_commands;
  load_commands.Place(&segment_command);
  MachOFile file;
  file
      .Header(&load_commands)
      .Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_CALL(load_command_handler, SegmentCommand(_))
    .WillOnce(DoAll(SaveArg<0>(&actual_segment),
                    Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(true,                         actual_segment.bits_64);
  EXPECT_EQ("froon",                      actual_segment.name);
  EXPECT_EQ(0x79f484f77009e511ULL,        actual_segment.vmaddr);
  EXPECT_EQ(0x8d92397ce6248abaULL,        actual_segment.vmsize);
  EXPECT_EQ(0x42b45da5U,                  actual_segment.maxprot);
  EXPECT_EQ(0x8bdbc319U,                  actual_segment.initprot);
  EXPECT_EQ(0xb2335220U,                  actual_segment.flags);
  EXPECT_EQ(0U,                           actual_segment.nsects);
  EXPECT_EQ(0U,                           actual_segment.section_list.Size());
  EXPECT_EQ(segment.final_size().Value(), actual_segment.contents.Size());
}

TEST_F(LoadCommand, SegmentLE64) {
  WithConfiguration config(kLittleEndian, 64);
  LoadedSection segment;
  segment.address() = 0x50c0501dc5922d35ULL;
  segment.Append(42, '*');              // segment contents
  SegmentLoadCommand segment_command;
  segment_command
      .Header("sixteenprecisely", segment,
              0x917c339d, 0xdbc446fa, 0xb650b563);
  segment_command.vmsize() = 0x84ae73e7c75469bfULL;
  LoadCommands load_commands;
  load_commands.Place(&segment_command);
  MachOFile file;
  file
      .Header(&load_commands)
      .Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_CALL(load_command_handler, SegmentCommand(_))
    .WillOnce(DoAll(SaveArg<0>(&actual_segment),
                    Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(true,                         actual_segment.bits_64);
  EXPECT_EQ("sixteenprecisely",           actual_segment.name);
  EXPECT_EQ(0x50c0501dc5922d35ULL,        actual_segment.vmaddr);
  EXPECT_EQ(0x84ae73e7c75469bfULL,        actual_segment.vmsize);
  EXPECT_EQ(0x917c339dU,                  actual_segment.maxprot);
  EXPECT_EQ(0xdbc446faU,                  actual_segment.initprot);
  EXPECT_EQ(0xb650b563U,                  actual_segment.flags);
  EXPECT_EQ(0U,                           actual_segment.nsects);
  EXPECT_EQ(0U,                           actual_segment.section_list.Size());
  EXPECT_EQ(segment.final_size().Value(), actual_segment.contents.Size());
}

TEST_F(LoadCommand, SegmentCommandTruncated) {
  WithConfiguration config(kBigEndian, 32);
  LoadedSection segment_contents;
  segment_contents.Append(20, '*');     	// lah di dah
  SizedSection command;
  command
      .D32(LC_SEGMENT)          	// command type
      .D32(command.final_size())                // command size
      .AppendCString("too-short", 16)           // segment name
      .D32(0x9c759211)                          // vmaddr
      .D32(segment_contents.final_size())       // vmsize
      .D32(segment_contents.start())            // file offset
      .D32(segment_contents.final_size())       // file size
      .D32(0x56f28446)                          // max protection
      .D32(0xe7910dcb)                          // initial protection
      .D32(0)                                   // no sections
      .Append(3, 0);                            // flags (one byte short!)
  LoadCommands load_commands;
  load_commands.Place(&command);
  MachOFile file;
  file
      .Header(&load_commands)
      .Place(&segment_contents);
  
  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, LoadCommandTooShort(0, LC_SEGMENT))
      .WillOnce(Return());

  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, SegmentBadContentOffset) {
  WithConfiguration config(kLittleEndian, 32);
  // Instead of letting a Place call set the segment's file offset and size,
  // set them ourselves, to check that the parser catches invalid offsets
  // instead of handing us bogus pointers.
  LoadedSection segment;
  segment.address() = 0x4db5489c;
  segment.start() = 0x7e189e76;         // beyond end of file
  segment.final_size() = 0x98b9c3ab;
  SegmentLoadCommand segment_command;
  segment_command
      .Header("notmerelyfifteen", segment, 0xcbab25ee, 0x359a20db, 0x68a3933f);
  LoadCommands load_commands;
  load_commands.Place(&segment_command);
  MachOFile file;
  file.Header(&load_commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, MisplacedSegmentData("notmerelyfifteen"))
      .WillOnce(Return());

  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(LoadCommand, ThreeLoadCommands) {
  WithConfiguration config(kBigEndian, 32);
  LoadedSection seg1, seg2, seg3;
  SegmentLoadCommand cmd1, cmd2, cmd3;

  seg1.Append(128, '@');
  seg1.address() = 0xa7f61ef6;
  cmd1.Header("head", seg1, 0x88bf1cc7, 0x889a26a4, 0xe9b80d87);
  // Include some dummy data at the end of the load command. Since we
  // didn't claim to have any sections, the reader should ignore this. But
  // making sure the commands have different lengths ensures that we're
  // using the right command's length to advance the LoadCommandIterator.
  cmd1.Append(128, '!');

  seg2.Append(42, '*');
  seg2.address() = 0xc70fc909;
  cmd2.Header("thorax", seg2, 0xde7327f4, 0xfdaf771d, 0x65e74b30);
  // More dummy data at the end of the load command. 
  cmd2.Append(32, '^');

  seg3.Append(42, '%');
  seg3.address() = 0x46b3ab05;
  cmd3.Header("abdomen", seg3, 0x7098b70d, 0x8d8d7728, 0x5131419b);
  // More dummy data at the end of the load command. 
  cmd3.Append(64, '&');

  LoadCommands load_commands;
  load_commands.Place(&cmd1).Place(&cmd2).Place(&cmd3);

  MachOFile file;
  file.Header(&load_commands).Place(&seg1).Place(&seg2).Place(&seg3);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  {
    InSequence s;
    EXPECT_CALL(load_command_handler,
                SegmentCommand(Field(&Segment::name, "head")))
      .WillOnce(Return(true));
    EXPECT_CALL(load_command_handler,
                SegmentCommand(Field(&Segment::name, "thorax")))
      .WillOnce(Return(true));
    EXPECT_CALL(load_command_handler,
                SegmentCommand(Field(&Segment::name, "abdomen")))
      .WillOnce(Return(true));
  }

  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));
}

static inline Matcher<const Section &> MatchSection(
    Matcher<bool> bits_64,
    Matcher<const string &> section_name,
    Matcher<const string &> segment_name,
    Matcher<uint64_t> address,
    Matcher<uint32_t> alignment,
    Matcher<uint32_t> flags,
    Matcher<const ByteBuffer &> contents) {
  return AllOf(AllOf(Field(&Section::bits_64, bits_64),
                     Field(&Section::section_name, section_name),
                     Field(&Section::segment_name, segment_name),
                     Field(&Section::address, address)),
               AllOf(Field(&Section::align, alignment),
                     Field(&Section::flags, flags),
                     Field(&Section::contents, contents)));
}

static inline Matcher<const Section &> MatchSection(
    Matcher<bool> bits_64,
    Matcher<const string &> section_name,
    Matcher<const string &> segment_name,
    Matcher<uint64_t> address) {
  return AllOf(Field(&Section::bits_64, bits_64),
               Field(&Section::section_name, section_name),
               Field(&Section::segment_name, segment_name),
               Field(&Section::address, address));
}

TEST_F(LoadCommand, OneSegmentTwoSections) {
  WithConfiguration config(kBigEndian, 64);

  // Create some sections with some data.
  LoadedSection section1, section2;
  section1.Append("buddha's hand");
  section2.Append("kumquat");

  // Create a segment to hold them.
  LoadedSection segment;
  segment.address() = 0xe1d0eeec;
  segment.Place(&section2).Place(&section1);

  SegmentLoadCommand segment_command;
  segment_command
      .Header("head", segment, 0x92c9568c, 0xa89f2627, 0x4dc7a1e2)
      .AppendSectionEntry("mandarin", "kishu", 12, 0x8cd4604bU, section1)
      .AppendSectionEntry("bergamot", "cara cara", 12, 0x98746efaU, section2);

  LoadCommands commands;
  commands.Place(&segment_command);

  MachOFile file;
  file.Header(&commands).Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);
  
  Segment actual_segment;
  EXPECT_CALL(load_command_handler, SegmentCommand(_))
      .WillOnce(DoAll(SaveArg<0>(&actual_segment),
                      Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  {
    InSequence s;
    ByteBuffer contents1;
    contents1.start = file_bytes + section1.start().Value();
    contents1.end = contents1.start + section1.final_size().Value();
    EXPECT_EQ("buddha's hand",
              string(reinterpret_cast<const char *>(contents1.start),
                     contents1.Size()));
    EXPECT_CALL(section_handler,
                HandleSection(MatchSection(true, "mandarin", "kishu",
                                           section1.address().Value(), 12,
                                           0x8cd4604bU, contents1)))
      .WillOnce(Return(true));
    
    ByteBuffer contents2;
    contents2.start = file_bytes + section2.start().Value();
    contents2.end = contents2.start + section2.final_size().Value();
    EXPECT_EQ("kumquat",
              string(reinterpret_cast<const char *>(contents2.start),
                     contents2.Size()));
    EXPECT_CALL(section_handler,
                HandleSection(MatchSection(true, "bergamot", "cara cara",
                                           section2.address().Value(), 12,
                                           0x98746efaU, contents2)))
      .WillOnce(Return(true));
  }

  EXPECT_TRUE(reader.WalkSegmentSections(actual_segment, &section_handler));
}

TEST_F(LoadCommand, MisplacedSectionBefore) {
  WithConfiguration config(kLittleEndian, 64);

  // The segment.
  LoadedSection segment;
  segment.address() = 0x696d83cc;
  segment.Append(10, '0');

  // The contents of the following sections don't matter, because
  // we're not really going to Place them in segment; we're just going
  // to set all their labels by hand to get the (impossible)
  // configurations we want.

  // A section whose starting offset is before that of its section.
  LoadedSection before;
  before.Append(10, '1');
  before.start()   = segment.start() - 1;
  before.address() = segment.address() - 1;
  before.final_size() = before.Size();
  
  SegmentLoadCommand command;
  command
    .Header("segment", segment, 0x173baa29, 0x8407275d, 0xed8f7057)
    .AppendSectionEntry("before",     "segment", 0, 0x686c6921, before);

  LoadCommands commands;
  commands.Place(&command);

  MachOFile file;
  file.Header(&commands).Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_TRUE(reader.FindSegment("segment", &actual_segment));

  EXPECT_CALL(reporter, MisplacedSectionData("before", "segment"))
    .WillOnce(Return());
  EXPECT_FALSE(reader.WalkSegmentSections(actual_segment, &section_handler));
}

TEST_F(LoadCommand, MisplacedSectionAfter) {
  WithConfiguration config(kLittleEndian, 64);

  // The segment.
  LoadedSection segment;
  segment.address() = 0x696d83cc;
  segment.Append(10, '0');

  // The contents of the following sections don't matter, because
  // we're not really going to Place them in segment; we're just going
  // to set all their labels by hand to get the (impossible)
  // configurations we want.

  // A section whose starting offset is after the end of its section.
  LoadedSection after;
  after.Append(10, '2');
  after.start()    = segment.start() + 11;
  after.address()   = segment.address() + 11;
  after.final_size() = after.Size();

  SegmentLoadCommand command;
  command
    .Header("segment", segment, 0x173baa29, 0x8407275d, 0xed8f7057)
    .AppendSectionEntry("after",      "segment", 0, 0x2ee50124, after);

  LoadCommands commands;
  commands.Place(&command);

  MachOFile file;
  file.Header(&commands).Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_TRUE(reader.FindSegment("segment", &actual_segment));

  EXPECT_CALL(reporter, MisplacedSectionData("after", "segment"))
    .WillOnce(Return());
  EXPECT_FALSE(reader.WalkSegmentSections(actual_segment, &section_handler));
}

TEST_F(LoadCommand, MisplacedSectionTooBig) {
  WithConfiguration config(kLittleEndian, 64);

  // The segment.
  LoadedSection segment;
  segment.address() = 0x696d83cc;
  segment.Append(10, '0');

  // The contents of the following sections don't matter, because
  // we're not really going to Place them in segment; we're just going
  // to set all their labels by hand to get the (impossible)
  // configurations we want.

  // A section that extends beyond the end of its section.
  LoadedSection too_big;
  too_big.Append(10, '3');
  too_big.start()   = segment.start() + 1;
  too_big.address() = segment.address() + 1;
  too_big.final_size() = too_big.Size();

  SegmentLoadCommand command;
  command
    .Header("segment", segment, 0x173baa29, 0x8407275d, 0xed8f7057)
    .AppendSectionEntry("too big",    "segment", 0, 0x8b53ae5c, too_big);

  LoadCommands commands;
  commands.Place(&command);

  MachOFile file;
  file.Header(&commands).Place(&segment);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_TRUE(reader.FindSegment("segment", &actual_segment));

  EXPECT_CALL(reporter, MisplacedSectionData("too big", "segment"))
    .WillOnce(Return());
  EXPECT_FALSE(reader.WalkSegmentSections(actual_segment, &section_handler));
}


// The segments in a .dSYM bundle's Mach-O file have their file offset
// and size set to zero, but the sections don't.  The reader shouldn't
// report an error in this case.
TEST_F(LoadCommand, ZappedSegment) {
  WithConfiguration config(kBigEndian, 32);

  // The segment.
  LoadedSection segment;
  segment.address() = 0x696d83cc;
  segment.start() = 0;
  segment.final_size() = 0;

  // The section.
  LoadedSection section;
  section.address() = segment.address();
  section.start() = 0;
  section.final_size() = 1000;          // extends beyond its segment
  
  SegmentLoadCommand command;
  command
    .Header("zapped", segment, 0x0861a5cb, 0x68ccff67, 0x0b66255c)
    .AppendSectionEntry("twitching", "zapped", 0, 0x93b3bd42, section);

  LoadCommands commands;
  commands.Place(&command);

  MachOFile file;
  file.Header(&commands);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;
  EXPECT_TRUE(reader.FindSegment("zapped", &actual_segment));

  ByteBuffer zapped_extent(NULL, 0);
  EXPECT_CALL(section_handler,
              HandleSection(MatchSection(false, "twitching", "zapped",
                                         0x696d83cc, 0, 0x93b3bd42,
                                         zapped_extent)))
    .WillOnce(Return(true));

  EXPECT_TRUE(reader.WalkSegmentSections(actual_segment, &section_handler));
}

TEST_F(LoadCommand, MapSegmentSections) {
  WithConfiguration config(kLittleEndian, 32);

  // Create some sections with some data.
  LoadedSection section1, section2, section3, section4;
  section1.Append("buddha's hand");
  section2.start() = 0;                 // Section 2 is an S_ZEROFILL section.
  section2.final_size() = 0;
  section3.Append("shasta gold");
  section4.Append("satsuma");

  // Create two segments to hold them.
  LoadedSection segment1, segment2;
  segment1.address() = 0x13e6c8a9;
  segment1.Place(&section3).Place(&section1);
  segment2.set_word_size(64);
  segment2.address() = 0x04d462e2;
  segment2.Place(&section4);
  section2.address() = segment2.address() + segment2.Size();

  SegmentLoadCommand segment_command1, segment_command2;
  segment_command1
      .Header("head", segment1, 0x67d955a6, 0x7a61c13e, 0xe3e50c64)
      .AppendSectionEntry("mandarin", "head", 12, 0x5bb565d7, section1)
      .AppendSectionEntry("bergamot", "head", 12, 0x8620de73, section3);
  segment_command2.set_word_size(64);
  segment_command2
      .Header("thorax", segment2, 0x7aab2419, 0xe908007f, 0x17961d33)
      .AppendSectionEntry("sixteenprecisely", "thorax",
                          12, S_ZEROFILL, section2)
      .AppendSectionEntry("cara cara", "thorax", 12, 0xb6c5dd8a, section4);

  LoadCommands commands;
  commands.Place(&segment_command1).Place(&segment_command2);

  MachOFile file;
  file.Header(&commands).Place(&segment1).Place(&segment2);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment segment;
  SectionMap section_map;

  EXPECT_FALSE(reader.FindSegment("smoot", &segment));

  ASSERT_TRUE(reader.FindSegment("thorax", &segment));
  ASSERT_TRUE(reader.MapSegmentSections(segment, &section_map));

  EXPECT_FALSE(section_map.find("sixteenpreciselyandthensome")
               != section_map.end());
  EXPECT_FALSE(section_map.find("mandarin") != section_map.end());
  ASSERT_TRUE(section_map.find("cara cara") != section_map.end());
  EXPECT_THAT(section_map["cara cara"],
              MatchSection(true, "cara cara", "thorax", 0x04d462e2));
  ASSERT_TRUE(section_map.find("sixteenprecisely")
              != section_map.end());
  ByteBuffer sixteenprecisely_contents(NULL, 0);
  EXPECT_THAT(section_map["sixteenprecisely"],
              MatchSection(true, "sixteenprecisely", "thorax",
                           0x04d462e2 + 7, 12, S_ZEROFILL,
                           sixteenprecisely_contents));

  ASSERT_TRUE(reader.FindSegment("head", &segment));
  ASSERT_TRUE(reader.MapSegmentSections(segment, &section_map));

  ASSERT_TRUE(section_map.find("mandarin") != section_map.end());
  EXPECT_THAT(section_map["mandarin"],
              MatchSection(false, "mandarin", "head", 0x13e6c8a9 + 11));
  ASSERT_TRUE(section_map.find("bergamot") != section_map.end());
  EXPECT_THAT(section_map["bergamot"],
              MatchSection(false, "bergamot", "head", 0x13e6c8a9));
}

TEST_F(LoadCommand, FindSegment) {
  WithConfiguration config(kBigEndian, 32);

  LoadedSection segment1, segment2, segment3;
  segment1.address() = 0xb8ae5752;
  segment1.Append("Some contents!");
  segment2.address() = 0xd6b0ce83;
  segment2.Append("Different stuff.");
  segment3.address() = 0x7374fd2a;
  segment3.Append("Further materials.");

  SegmentLoadCommand cmd1, cmd2, cmd3;
  cmd1.Header("first",  segment1, 0xfadb6932, 0x175bf529, 0x0de790ad);
  cmd2.Header("second", segment2, 0xeef716e0, 0xe103a9d7, 0x7d38a8ef);
  cmd3.Header("third",  segment3, 0xe172b39e, 0x86012f07, 0x080ac94d);

  LoadCommands commands;
  commands.Place(&cmd1).Place(&cmd2).Place(&cmd3);

  MachOFile file;
  file.Header(&commands).Place(&segment1).Place(&segment2).Place(&segment3);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  Segment actual_segment;

  EXPECT_FALSE(reader.FindSegment("murphy", &actual_segment));

  EXPECT_TRUE(reader.FindSegment("second", &actual_segment));
  EXPECT_EQ(0xd6b0ce83, actual_segment.vmaddr);
}


// Symtab tests.

// A StringAssembler is a class for generating .stabstr sections to present
// as input to the STABS parser.
class StringAssembler: public SizedSection {
 public:
  // Add the string S to this StringAssembler, and return the string's
  // offset within this compilation unit's strings.
  size_t Add(const string &s) {
    size_t offset = Size();
    AppendCString(s);
    return offset;
  }
};

// A SymbolAssembler is a class for generating .stab sections to present as
// test input for the STABS parser.
class SymbolAssembler: public SizedSection {
 public:
  // Create a SymbolAssembler that uses StringAssembler for its strings.
  explicit SymbolAssembler(StringAssembler *string_assembler) 
      : string_assembler_(string_assembler),
        entry_count_(0) { }

  // Append a STAB entry to the end of this section with the given
  // characteristics. NAME is the offset of this entry's name string within
  // its compilation unit's portion of the .stabstr section; this can be a
  // value generated by a StringAssembler. Return a reference to this
  // SymbolAssembler.
  SymbolAssembler &Symbol(uint8_t type, uint8_t other, Label descriptor,
                          Label value, Label name) {
    D32(name);
    D8(type);
    D8(other);
    D16(descriptor);
    Append(endianness(), word_size_ / 8, value);
    entry_count_++;
    return *this;
  }

  // As above, but automatically add NAME to our StringAssembler.
  SymbolAssembler &Symbol(uint8_t type, uint8_t other, Label descriptor,
                       Label value, const string &name) {
    return Symbol(type, other, descriptor, value, string_assembler_->Add(name));
  }

 private:
  // The strings for our STABS entries.
  StringAssembler *string_assembler_;

  // The number of entries in this compilation unit so far.
  size_t entry_count_;
};

class Symtab: public ReaderFixture, public Test { };

TEST_F(Symtab, Symtab32) {
  WithConfiguration config(kLittleEndian, 32);

  StringAssembler strings;
  SymbolAssembler symbols(&strings);
  symbols
      .Symbol(0x52, 0x7c, 0x3470, 0x9bb02e7c, "hrududu")
      .Symbol(0x50, 0x90, 0x7520, 0x1122525d, "Frith");

  SizedSection symtab_command;
  symtab_command
      .D32(LC_SYMTAB)                    // command
      .D32(symtab_command.final_size())  // size
      .D32(symbols.start())              // file offset of symbols
      .D32(2)                            // symbol count
      .D32(strings.start())              // file offset of strings
      .D32(strings.final_size());        // strings size

  LoadCommands load_commands;
  load_commands.Place(&symtab_command);

  MachOFile file;
  file.Header(&load_commands).Place(&symbols).Place(&strings);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  ByteBuffer symbols_found, strings_found;
  EXPECT_CALL(load_command_handler, SymtabCommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&symbols_found),
                      SaveArg<1>(&strings_found),
                      Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(24U, symbols_found.Size());
  EXPECT_EQ(14U, strings_found.Size());
}

TEST_F(Symtab, Symtab64) {
  WithConfiguration config(kBigEndian, 64);

  StringAssembler strings;
  SymbolAssembler symbols(&strings);
  symbols
      .Symbol(0xa7, 0xaf, 0x03af, 0x42f3072c74335181ULL, "foo")
      .Symbol(0xb0, 0x9a, 0x2aa7, 0x2e2d349b3d5744a0ULL, "bar");

  SizedSection symtab_command;
  symtab_command
      .D32(LC_SYMTAB)                    // command
      .D32(symtab_command.final_size())  // size
      .D32(symbols.start())              // file offset of symbols
      .D32(2)                            // symbol count
      .D32(strings.start())              // file offset of strings
      .D32(strings.final_size());        // strings size

  LoadCommands load_commands;
  load_commands.Place(&symtab_command);

  MachOFile file;
  file.Header(&load_commands).Place(&symbols).Place(&strings);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  ByteBuffer symbols_found, strings_found;
  EXPECT_CALL(load_command_handler, SymtabCommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&symbols_found),
                      SaveArg<1>(&strings_found),
                      Return(true)));
  EXPECT_TRUE(reader.WalkLoadCommands(&load_command_handler));

  EXPECT_EQ(32U, symbols_found.Size());
  EXPECT_EQ(8U,  strings_found.Size());
}

TEST_F(Symtab, SymtabMisplacedSymbols) {
  WithConfiguration config(kBigEndian, 32);

  StringAssembler strings;
  SymbolAssembler symbols(&strings);
  symbols
      .Symbol(0xa7, 0xaf, 0x03af, 0x42f3072c74335181ULL, "foo")
      .Symbol(0xb0, 0x9a, 0x2aa7, 0x2e2d349b3d5744a0ULL, "bar");

  SizedSection symtab_command;
  symtab_command
      .D32(LC_SYMTAB)                    // command
      .D32(symtab_command.final_size())  // size
      .D32(symbols.start())              // file offset of symbols
      .D32(3)                            // symbol count (too many)
      .D32(strings.start())              // file offset of strings
      .D32(strings.final_size());        // strings size

  LoadCommands load_commands;
  load_commands.Place(&symtab_command);

  MachOFile file;
  // Put symbols at end, so the excessive length will be noticed.
  file.Header(&load_commands).Place(&strings).Place(&symbols);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, MisplacedSymbolTable()).Times(1);
  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

TEST_F(Symtab, SymtabMisplacedStrings) {
  WithConfiguration config(kLittleEndian, 32);

  StringAssembler strings;
  SymbolAssembler symbols(&strings);
  symbols
      .Symbol(0xa7, 0xaf, 0x03af, 0x42f3072c74335181ULL, "foo")
      .Symbol(0xb0, 0x9a, 0x2aa7, 0x2e2d349b3d5744a0ULL, "bar");

  SizedSection symtab_command;
  symtab_command
      .D32(LC_SYMTAB)                    // command
      .D32(symtab_command.final_size())  // size
      .D32(symbols.start())              // file offset of symbols
      .D32(2)                            // symbol count
      .D32(strings.start())              // file offset of strings
      .D32(strings.final_size() + 1);    // strings size (too long)

  LoadCommands load_commands;
  load_commands.Place(&symtab_command);

  MachOFile file;
  // Put strings at end, so the excessive length will be noticed.
  file.Header(&load_commands).Place(&symbols).Place(&strings);

  ReadFile(&file, true, CPU_TYPE_ANY, 0);

  EXPECT_CALL(reporter, MisplacedSymbolTable()).Times(1);
  EXPECT_FALSE(reader.WalkLoadCommands(&load_command_handler));
}

