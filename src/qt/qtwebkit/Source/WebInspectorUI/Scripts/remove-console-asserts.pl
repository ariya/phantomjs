#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use File::Copy qw/move/;
use File::Temp qw/tempfile/;

our $inputScriptFilename;
our $outputScriptFilename;

GetOptions('input-script=s' => \$inputScriptFilename,
           'output-script=s' => \$outputScriptFilename);

unless (defined $inputScriptFilename and defined $outputScriptFilename) {
    print "Usage: $0 --input-script <path> --output-script <path>\n";
    exit;
}

open IN, $inputScriptFilename or die;
our ($out, $tempFilename) = tempfile(UNLINK => 0) or die;

while (<IN>) {
    s/\s*console\.assert\(.*\);\s*//g;
    print $out $_;
    print "WARNING: Multi-line console.assert on line $.: $_" if $_ =~ /\s*console\.assert\(/;
}

close $out;
close IN;

move $tempFilename, $outputScriptFilename or die "$!";
