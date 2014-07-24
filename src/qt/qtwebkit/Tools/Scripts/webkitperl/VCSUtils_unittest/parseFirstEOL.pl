#!/usr/bin/perl -w
#
# Copyright (C) 2011 Research In Motion Limited. All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Unit tests of VCSUtils::parseFirstEOL().

use strict;
use warnings;

use Test::Simple tests => 7;
use VCSUtils;

my $title;

# New test
$title = "parseFirstEOL: Empty string.";
ok(!defined(firstEOLInString("")), $title);

# New test
$title = "parseFirstEOL: Line without a line ending character";
ok(!defined(firstEOLInString("This line doesn't have a line ending character.")), $title);

# New test
$title = "parseFirstEOL: Line with Windows line ending.";
ok(firstEOLInString("This line ends with a Windows line ending.\r\n") eq "\r\n", $title);

# New test
$title = "parseFirstEOL: Line with Unix line ending.";
ok(firstEOLInString("This line ends with a Unix line ending.\n") eq "\n", $title);

# New test
$title = "parseFirstEOL: Line with Mac line ending.";
ok(firstEOLInString("This line ends with a Mac line ending.\r") eq "\r", $title);

# New test
$title = "parseFirstEOL: Line with Mac line ending followed by line without a line ending.";
ok(firstEOLInString("This line ends with a Mac line ending.\rThis line doesn't have a line ending.") eq "\r", $title);

# New test
$title = "parseFirstEOL: Line with a mix of line endings.";
ok(firstEOLInString("This line contains a mix of line endings.\r\n\r\n\r\r\n\n\n\n") eq "\r\n", $title);

sub firstEOLInString
{
    my ($string) = @_;
    my $fileHandle;
    open($fileHandle, "<", \$string);
    return parseFirstEOL($fileHandle);
}
