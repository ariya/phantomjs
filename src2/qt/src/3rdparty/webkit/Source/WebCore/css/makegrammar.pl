#! /usr/bin/perl
#
#   This file is part of the WebKit project
#
#   Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
use warnings;

my $grammar = $ARGV[0];
my $fileBase = $ARGV[1];

system("bison -d -p cssyy " . $grammar . " -o " . $fileBase . ".tab.c");

open HEADER, ">" . $fileBase . ".h" or die;
print HEADER << "EOF";
#ifndef CSSGRAMMAR_H
#define CSSGRAMMAR_H
EOF

open HPP, "<" . $fileBase . ".tab.h" or die;
while (<HPP>) {
    print HEADER;
}
close HPP;

print HEADER "#endif\n";

close HEADER;

unlink($fileBase . ".tab.h");

open CPP, ">" . $fileBase . ".cpp" or die;
open GENSRC, "<" . $fileBase . ".tab.c" or die;
while (<GENSRC>) {
    print CPP;
}
close GENSRC;
close CPP;

unlink($fileBase . ".tab.c");
