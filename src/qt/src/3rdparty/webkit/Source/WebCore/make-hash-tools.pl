#! /usr/bin/perl
#
#   This file is part of the WebKit project
#
#   Copyright (C) 2010 Andras Becsi (abecsi@inf.u-szeged.hu), University of Szeged
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public License
#   along with this library; see the file COPYING.LIB.  If not, write to
#   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.

use strict;
use File::Basename;

my $outdir = $ARGV[0];
shift;
my $option = basename($ARGV[0],".gperf");


if ($option eq "DocTypeStrings") {

    my $docTypeStringsGenerated    = "$outdir/DocTypeStrings.cpp";
    my $docTypeStringsGperf        = $ARGV[0];
    shift;

    system("gperf --key-positions=\"*\" -s 2 $docTypeStringsGperf > $docTypeStringsGenerated") == 0 || die "calling gperf failed: $?";

} elsif ($option eq "ColorData") {

    my $colorDataGenerated         = "$outdir/ColorData.cpp";
    my $colorDataGperf             = $ARGV[0];
    shift;

    system("gperf --key-positions=\"*\" -D -s 2 $colorDataGperf > $colorDataGenerated") == 0 || die "calling gperf failed: $?";

} else {
    die "Unknown option.";
}
