// Copyright (c) 2011 Google Inc.
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

// Original author: Ted Mielczarek <ted.mielczarek@gmail.com>

// synth_elf_unittest.cc:
// Unittests for google_breakpad::synth_elf::ELF

#include <elf.h>

#include "breakpad_googletest_includes.h"
#include "common/linux/synth_elf.h"

using google_breakpad::synth_elf::ELF;
using google_breakpad::synth_elf::StringTable;
using google_breakpad::synth_elf::SymbolTable;
using google_breakpad::test_assembler::Endianness;
using google_breakpad::test_assembler::kBigEndian;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Label;
using std::string;
using ::testing::Test;

class StringTableTest : public Test {
public:
  StringTableTest() : table(kLittleEndian) {}

  StringTable table;
};

TEST_F(StringTableTest, Empty) {
  EXPECT_EQ(1, table.Size());
  string contents;
  ASSERT_TRUE(table.GetContents(&contents));
  const char* kExpectedContents = "\0";
  EXPECT_EQ(0, memcmp(kExpectedContents,
                      contents.c_str(),
                      contents.size()));
  ASSERT_TRUE(table.empty_string.IsKnownConstant());
  EXPECT_EQ(0, table.empty_string.Value());
}

TEST_F(StringTableTest, Basic) {
  const string s1("table fills with strings");
  const string s2("offsets preserved as labels");
  const string s3("verified with tests");
  const char* kExpectedContents = 
    "\0table fills with strings\0"
    "offsets preserved as labels\0"
    "verified with tests\0";
  Label l1(table.Add(s1));
  Label l2(table.Add(s2));
  Label l3(table.Add(s3));
  string contents;
  ASSERT_TRUE(table.GetContents(&contents));
  EXPECT_EQ(0, memcmp(kExpectedContents,
                      contents.c_str(),
                      contents.size()));
  // empty_string is at zero, other strings start at 1.
  ASSERT_TRUE(l1.IsKnownConstant());
  EXPECT_EQ(1, l1.Value());
  // Each string has an extra byte for a trailing null.
  EXPECT_EQ(1 + s1.length() + 1, l2.Value());
  EXPECT_EQ(1 + s1.length() + 1 + s2.length() + 1, l3.Value());
}

TEST_F(StringTableTest, Duplicates) {
  const string s1("string 1");
  const string s2("string 2");
  const string s3("");
  const char* kExpectedContents = "\0string 1\0string 2\0";
  Label l1(table.Add(s1));
  Label l2(table.Add(s2));
  // Adding strings twice should return the same Label.
  Label l3(table.Add(s3));
  Label l4(table.Add(s2));
  string contents;
  ASSERT_TRUE(table.GetContents(&contents));
  EXPECT_EQ(0, memcmp(kExpectedContents,
                      contents.c_str(),
                      contents.size()));
  EXPECT_EQ(0, table.empty_string.Value());
  EXPECT_EQ(table.empty_string.Value(), l3.Value());
  EXPECT_EQ(l2.Value(), l4.Value());
}

class SymbolTableTest : public Test {};

TEST_F(SymbolTableTest, Simple32) {
  StringTable table(kLittleEndian);
  SymbolTable syms(kLittleEndian, 4, table);

  const string kFuncName1 = "superfunc";
  const uint32_t kFuncAddr1 = 0x10001000;
  const uint32_t kFuncSize1 = 0x10;
  const string kFuncName2 = "awesomefunc";
  const uint32_t kFuncAddr2 = 0x20002000;
  const uint32_t kFuncSize2 = 0x2f;
  const string kFuncName3 = "megafunc";
  const uint32_t kFuncAddr3 = 0x30003000;
  const uint32_t kFuncSize3 = 0x3c;

  syms.AddSymbol(kFuncName1, kFuncAddr1, kFuncSize1,
                 ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
                 SHN_UNDEF + 1);
  syms.AddSymbol(kFuncName2, kFuncAddr2, kFuncSize2,
                 ELF32_ST_INFO(STB_LOCAL, STT_FUNC),
                 SHN_UNDEF + 2);
  syms.AddSymbol(kFuncName3, kFuncAddr3, kFuncSize3,
                 ELF32_ST_INFO(STB_LOCAL, STT_FUNC),
                 SHN_UNDEF + 3);

  const char kExpectedStringTable[] = "\0superfunc\0awesomefunc\0megafunc";
  const size_t kExpectedStringTableSize = sizeof(kExpectedStringTable);
  EXPECT_EQ(kExpectedStringTableSize, table.Size());
  string table_contents;
  table.GetContents(&table_contents);
  EXPECT_EQ(0, memcmp(kExpectedStringTable,
                      table_contents.c_str(),
                      table_contents.size()));

  const uint8_t kExpectedSymbolContents[] = {
    // Symbol 1
    0x01, 0x00, 0x00, 0x00, // name
    0x00, 0x10, 0x00, 0x10, // value
    0x10, 0x00, 0x00, 0x00, // size
    ELF32_ST_INFO(STB_GLOBAL, STT_FUNC), // info
    0x00, // other
    0x01, 0x00, // shndx
    // Symbol 2
    0x0B, 0x00, 0x00, 0x00, // name
    0x00, 0x20, 0x00, 0x20, // value
    0x2f, 0x00, 0x00, 0x00, // size
    ELF32_ST_INFO(STB_LOCAL, STT_FUNC), // info
    0x00, // other
    0x02, 0x00, // shndx
    // Symbol 3
    0x17, 0x00, 0x00, 0x00, // name
    0x00, 0x30, 0x00, 0x30, // value
    0x3c, 0x00, 0x00, 0x00, // size
    ELF32_ST_INFO(STB_LOCAL, STT_FUNC), // info
    0x00, // other
    0x03, 0x00, // shndx
  };
  const size_t kExpectedSymbolSize = sizeof(kExpectedSymbolContents);
  EXPECT_EQ(kExpectedSymbolSize, syms.Size());

  string symbol_contents;
  syms.GetContents(&symbol_contents);
  EXPECT_EQ(0, memcmp(kExpectedSymbolContents,
                      symbol_contents.c_str(),
                      symbol_contents.size()));
}

class BasicElf : public Test {};

// Doesn't seem worthwhile writing the tests to be endian-independent
// when they're unlikely to ever be run on big-endian systems.
#if defined(__i386__) || defined(__x86_64__)

TEST_F(BasicElf, EmptyLE32) {
  const size_t kStringTableSize = sizeof("\0.shstrtab");
  const size_t kStringTableAlign = 4 - kStringTableSize % 4;
  const size_t kExpectedSize = sizeof(Elf32_Ehdr) +
    // Two sections, SHT_NULL + the section header string table.
    2 * sizeof(Elf32_Shdr) +
    kStringTableSize + kStringTableAlign;

  ELF elf(EM_386, ELFCLASS32, kLittleEndian);
  elf.Finish();
  EXPECT_EQ(kExpectedSize, elf.Size());

  string contents;
  ASSERT_TRUE(elf.GetContents(&contents));
  ASSERT_EQ(kExpectedSize, contents.size());
  const Elf32_Ehdr* header =
    reinterpret_cast<const Elf32_Ehdr*>(contents.data());
  const uint8_t kIdent[] = {
    ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
    ELFCLASS32, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV,
    0, 0, 0, 0, 0, 0, 0, 0
  };
  EXPECT_EQ(0, memcmp(kIdent, header->e_ident, sizeof(kIdent)));
  EXPECT_EQ(ET_EXEC, header->e_type);
  EXPECT_EQ(EM_386, header->e_machine);
  EXPECT_EQ(EV_CURRENT, header->e_version);
  EXPECT_EQ(0, header->e_entry);
  EXPECT_EQ(0, header->e_phoff);
  EXPECT_EQ(sizeof(Elf32_Ehdr) + kStringTableSize + kStringTableAlign,
            header->e_shoff);
  EXPECT_EQ(0, header->e_flags);
  EXPECT_EQ(sizeof(Elf32_Ehdr), header->e_ehsize);
  EXPECT_EQ(sizeof(Elf32_Phdr), header->e_phentsize);
  EXPECT_EQ(0, header->e_phnum);
  EXPECT_EQ(sizeof(Elf32_Shdr), header->e_shentsize);
  EXPECT_EQ(2, header->e_shnum);
  EXPECT_EQ(1, header->e_shstrndx);
}

TEST_F(BasicElf, EmptyLE64) {
  const size_t kStringTableSize = sizeof("\0.shstrtab");
  const size_t kStringTableAlign = 4 - kStringTableSize % 4;
  const size_t kExpectedSize = sizeof(Elf64_Ehdr) +
    // Two sections, SHT_NULL + the section header string table.
    2 * sizeof(Elf64_Shdr) +
    kStringTableSize + kStringTableAlign;

  ELF elf(EM_X86_64, ELFCLASS64, kLittleEndian);
  elf.Finish();
  EXPECT_EQ(kExpectedSize, elf.Size());

  string contents;
  ASSERT_TRUE(elf.GetContents(&contents));
  ASSERT_EQ(kExpectedSize, contents.size());
  const Elf64_Ehdr* header =
    reinterpret_cast<const Elf64_Ehdr*>(contents.data());
  const uint8_t kIdent[] = {
    ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
    ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV,
    0, 0, 0, 0, 0, 0, 0, 0
  };
  EXPECT_EQ(0, memcmp(kIdent, header->e_ident, sizeof(kIdent)));
  EXPECT_EQ(ET_EXEC, header->e_type);
  EXPECT_EQ(EM_X86_64, header->e_machine);
  EXPECT_EQ(EV_CURRENT, header->e_version);
  EXPECT_EQ(0, header->e_entry);
  EXPECT_EQ(0, header->e_phoff);
  EXPECT_EQ(sizeof(Elf64_Ehdr) + kStringTableSize + kStringTableAlign,
            header->e_shoff);
  EXPECT_EQ(0, header->e_flags);
  EXPECT_EQ(sizeof(Elf64_Ehdr), header->e_ehsize);
  EXPECT_EQ(sizeof(Elf64_Phdr), header->e_phentsize);
  EXPECT_EQ(0, header->e_phnum);
  EXPECT_EQ(sizeof(Elf64_Shdr), header->e_shentsize);
  EXPECT_EQ(2, header->e_shnum);
  EXPECT_EQ(1, header->e_shstrndx);
}

#endif  // defined(__i386__) || defined(__x86_64__)
