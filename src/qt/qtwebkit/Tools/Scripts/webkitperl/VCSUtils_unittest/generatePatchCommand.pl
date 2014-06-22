#!/usr/bin/perl
#
# Copyright (C) 2009, 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Unit tests of VCSUtils::generatePatchCommand().

use Test::Simple tests => 10;
use VCSUtils;

# New test
$title = "generatePatchCommand: Undefined optional arguments.";

my $argsHashRef;
my ($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0", $title);
ok($isForcing == 0, $title);

# New test
$title = "generatePatchCommand: Undefined options.";

my $options;
$argsHashRef = {options => $options};
($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0", $title);
ok($isForcing == 0, $title);

# New test
$title = "generatePatchCommand: --force and no \"ensure force\".";

$argsHashRef = {options => ["--force"]};
($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0 --force", $title);
ok($isForcing == 1, $title);

# New test
$title = "generatePatchCommand: no --force and \"ensure force\".";

$argsHashRef = {ensureForce => 1};
($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0 --force", $title);
ok($isForcing == 1, $title);

# New test
$title = "generatePatchCommand: \"should reverse\".";

$argsHashRef = {shouldReverse => 1};
($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0 --reverse", $title);

# New test
$title = "generatePatchCommand: --fuzz=3, --force.";

$argsHashRef = {options => ["--fuzz=3", "--force"]};
($patchCommand, $isForcing) = VCSUtils::generatePatchCommand($argsHashRef);

ok($patchCommand eq "patch -p0 --force --fuzz=3", $title);
