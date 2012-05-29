#!/usr/bin/perl -w

# Copyright (C) 2008 Apple Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Script to combine multiple JavaScript files into one file, based on
# the script tags in the head of an input HTML file.

use strict;
use Getopt::Long;
use File::Basename;
use File::Path;

my $generatedScriptsDirectory;
my $outputDirectory;
my $scriptName;
my $htmlFile;

GetOptions('output-dir=s' => \$outputDirectory,
           'output-script-name=s' => \$scriptName,
           'generated-scripts-dir=s' => \$generatedScriptsDirectory,
           'input-html=s' => \$htmlFile);

unless (defined $htmlFile and defined $scriptName and defined $outputDirectory) {
    print "Usage: $0 --input-html <path> --output-dir path --output-script-name <name>\n";
    exit;
}

my $htmlDirectory = dirname($htmlFile);
my $htmlContents;

{
    local $/;
    open HTML, $htmlFile or die;
    $htmlContents = <HTML>;
    close HTML;
}

$htmlContents =~ m/<head>(.*)<\/head>/si;
my $headContents = $1;

mkpath $outputDirectory;
open SCRIPT_OUT, ">", "$outputDirectory/$scriptName" or die "Can't open $outputDirectory/$scriptName: $!";

while ($headContents =~ m/<script.*src="([^"]*)"[^>]*>/gi) {
    local $/;
    open SCRIPT_IN, "$generatedScriptsDirectory/$1" or open SCRIPT_IN, "$htmlDirectory/$1" or die "Can't open $htmlDirectory/$1: $!";
    print SCRIPT_OUT "/* $1 */\n\n";
    print SCRIPT_OUT <SCRIPT_IN>;
    close SCRIPT_IN;
}

close SCRIPT_OUT;

$headContents =~ s/<script.*src="[^"]*"[^>]*><\/script>\s*//gi;
$headContents .= "<script type=\"text/javascript\" src=\"$scriptName\"></script>\n";
$htmlContents =~ s/<head>.*<\/head>/<head>$headContents<\/head>/si;

open HTML, ">", "$outputDirectory/" . basename($htmlFile) or die "Can't open $outputDirectory/" . basename($htmlFile) . ": $!";
print HTML $htmlContents;
close HTML;
