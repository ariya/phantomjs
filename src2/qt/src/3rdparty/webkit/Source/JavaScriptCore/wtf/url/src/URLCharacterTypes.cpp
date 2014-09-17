// Copyright 2010, Google Inc. All rights reserved.
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

#include "config.h"
#include "URLCharacterTypes.h"

namespace WTF {

const unsigned char URLCharacterTypes::characterTypeTable[0x100] = {
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0x00 - 0x0f
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0x10 - 0x1f
    InvalidCharacter, // 0x20  ' ' (escape spaces in queries)
    QueryCharacter | UserInfoCharacter, // 0x21  !
    InvalidCharacter, // 0x22  "
    InvalidCharacter, // 0x23  #  (invalid in query since it marks the ref)
    QueryCharacter | UserInfoCharacter, // 0x24  $
    QueryCharacter | UserInfoCharacter, // 0x25  %
    QueryCharacter | UserInfoCharacter, // 0x26  &
    QueryCharacter | UserInfoCharacter, // 0x27  '
    QueryCharacter | UserInfoCharacter, // 0x28  (
    QueryCharacter | UserInfoCharacter, // 0x29  )
    QueryCharacter | UserInfoCharacter, // 0x2a  *
    QueryCharacter | UserInfoCharacter, // 0x2b  +
    QueryCharacter | UserInfoCharacter, // 0x2c  ,
    QueryCharacter | UserInfoCharacter, // 0x2d  -
    QueryCharacter | UserInfoCharacter | IPv4Character, // 0x2e  .
    QueryCharacter, // 0x2f  /
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x30  0
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x31  1
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x32  2
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x33  3
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x34  4
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x35  5
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x36  6
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter | OctalCharacter, // 0x37  7
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter, // 0x38  8
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter | DecimalCharacter, // 0x39  9
    QueryCharacter, // 0x3a  :
    QueryCharacter, // 0x3b  ;
    InvalidCharacter, // 0x3c  <
    QueryCharacter, // 0x3d  =
    InvalidCharacter, // 0x3e  >
    QueryCharacter, // 0x3f  ?
    QueryCharacter, // 0x40  @
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x41  A
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x42  B
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x43  C
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x44  D
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x45  E
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x46  F
    QueryCharacter | UserInfoCharacter, // 0x47  G
    QueryCharacter | UserInfoCharacter, // 0x48  H
    QueryCharacter | UserInfoCharacter, // 0x49  I
    QueryCharacter | UserInfoCharacter, // 0x4a  J
    QueryCharacter | UserInfoCharacter, // 0x4b  K
    QueryCharacter | UserInfoCharacter, // 0x4c  L
    QueryCharacter | UserInfoCharacter, // 0x4d  M
    QueryCharacter | UserInfoCharacter, // 0x4e  N
    QueryCharacter | UserInfoCharacter, // 0x4f  O
    QueryCharacter | UserInfoCharacter, // 0x50  P
    QueryCharacter | UserInfoCharacter, // 0x51  Q
    QueryCharacter | UserInfoCharacter, // 0x52  R
    QueryCharacter | UserInfoCharacter, // 0x53  S
    QueryCharacter | UserInfoCharacter, // 0x54  T
    QueryCharacter | UserInfoCharacter, // 0x55  U
    QueryCharacter | UserInfoCharacter, // 0x56  V
    QueryCharacter | UserInfoCharacter, // 0x57  W
    QueryCharacter | UserInfoCharacter | IPv4Character, // 0x58  X
    QueryCharacter | UserInfoCharacter, // 0x59  Y
    QueryCharacter | UserInfoCharacter, // 0x5a  Z
    QueryCharacter, // 0x5b  [
    QueryCharacter, // 0x5c  '\'
    QueryCharacter, // 0x5d  ]
    QueryCharacter, // 0x5e  ^
    QueryCharacter | UserInfoCharacter, // 0x5f  _
    QueryCharacter, // 0x60  `
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x61  a
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x62  b
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x63  c
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x64  d
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x65  e
    QueryCharacter | UserInfoCharacter | IPv4Character | HexCharacter, // 0x66  f
    QueryCharacter | UserInfoCharacter, // 0x67  g
    QueryCharacter | UserInfoCharacter, // 0x68  h
    QueryCharacter | UserInfoCharacter, // 0x69  i
    QueryCharacter | UserInfoCharacter, // 0x6a  j
    QueryCharacter | UserInfoCharacter, // 0x6b  k
    QueryCharacter | UserInfoCharacter, // 0x6c  l
    QueryCharacter | UserInfoCharacter, // 0x6d  m
    QueryCharacter | UserInfoCharacter, // 0x6e  n
    QueryCharacter | UserInfoCharacter, // 0x6f  o
    QueryCharacter | UserInfoCharacter, // 0x70  p
    QueryCharacter | UserInfoCharacter, // 0x71  q
    QueryCharacter | UserInfoCharacter, // 0x72  r
    QueryCharacter | UserInfoCharacter, // 0x73  s
    QueryCharacter | UserInfoCharacter, // 0x74  t
    QueryCharacter | UserInfoCharacter, // 0x75  u
    QueryCharacter | UserInfoCharacter, // 0x76  v
    QueryCharacter | UserInfoCharacter, // 0x77  w
    QueryCharacter | UserInfoCharacter | IPv4Character, // 0x78  x
    QueryCharacter | UserInfoCharacter, // 0x79  y
    QueryCharacter | UserInfoCharacter, // 0x7a  z
    QueryCharacter, // 0x7b  {
    QueryCharacter, // 0x7c  |
    QueryCharacter, // 0x7d  }
    QueryCharacter | UserInfoCharacter, // 0x7e  ~
    InvalidCharacter, // 0x7f
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0x80 - 0x8f
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0x90 - 0x9f
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xa0 - 0xaf
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xb0 - 0xbf
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xc0 - 0xcf
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xd0 - 0xdf
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xe0 - 0xef
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter,
    InvalidCharacter, InvalidCharacter, InvalidCharacter, InvalidCharacter, // 0xf0 - 0xff
};

}
