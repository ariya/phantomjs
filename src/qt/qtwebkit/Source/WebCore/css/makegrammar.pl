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

use File::Basename;
use File::Spec;
use Getopt::Long;

my $outputDir = ".";
my $extraDefines = "";
my $symbolsPrefix = "";
my $preprocessor = "";
my $preprocessOnly = 0;
my $bison = "bison";

GetOptions(
    'outputDir=s' => \$outputDir,
    'extraDefines=s' => \$extraDefines,
    'bison=s' => \$bison,
    'preprocessor=s' => \$preprocessor,
    'preprocessOnly' => \$preprocessOnly,
    'symbolsPrefix=s' => \$symbolsPrefix
);

my $grammarFilePath = $ARGV[0];
my $grammarIncludesFilePath = @ARGV > 0 ? $ARGV[1] : "";

if (!length($symbolsPrefix) && !$preprocessOnly) {
    die "Need a symbols prefix to give to bison (e.g. cssyy, xpathyy)";
}

my ($filename, $basePath, $suffix) = fileparse($grammarFilePath, (".y", ".y.in"));

if ($suffix eq ".y.in") {
    my $grammarFileOutPath = File::Spec->join($outputDir, "$filename.y");
    if (!$grammarIncludesFilePath) {
        $grammarIncludesFilePath = "${basePath}${filename}.y.includes";
    }

    open GRAMMAR, ">$grammarFileOutPath" or die;
    open INCLUDES, "<$grammarIncludesFilePath" or die;

    require preprocessor;

    while (<INCLUDES>) {
        print GRAMMAR;
    }
    print GRAMMAR join("", applyPreprocessor($grammarFilePath, $extraDefines, $preprocessor));
    close GRAMMAR;

    $grammarFilePath = $grammarFileOutPath;

    exit if $preprocessOnly;
}

my $fileBase = File::Spec->join($outputDir, $filename);
system("$bison -d -p $symbolsPrefix $grammarFilePath -o $fileBase.cpp");

open HEADER, ">$fileBase.h" or die;
print HEADER << "EOF";
#ifndef CSSGRAMMAR_H
#define CSSGRAMMAR_H
EOF

open HPP, "<$fileBase.cpp.h" or open HPP, "<$fileBase.hpp" or die;
while (<HPP>) {
    print HEADER;
}
close HPP;

print HEADER "#endif\n";
close HEADER;

unlink("$fileBase.cpp.h");
unlink("$fileBase.hpp");

