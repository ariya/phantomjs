#!/usr/bin/perl -w

use strict;
use Getopt::Long;
use File::Basename;
use File::Path;

our $inputDirectory;
our $outputDirectory;
our $outputScriptName;
our $outputStylesheetName;
our $derivedSourcesDirectory;
our $htmlDirectory;
our $htmlFile;

GetOptions('output-dir=s' => \$outputDirectory,
           'output-script-name=s' => \$outputScriptName,
           'output-style-name=s' => \$outputStylesheetName,
           'derived-sources-dir=s' => \$derivedSourcesDirectory,
           'input-dir=s' => \$inputDirectory,
           'input-html-dir=s' => \$htmlDirectory,
           'input-html=s' => \$htmlFile);

unless (defined $htmlFile and defined $derivedSourcesDirectory and defined $outputDirectory and defined $outputScriptName and defined $outputStylesheetName) {
    print "Usage: $0 --input-html <path> --derived-sources-dir <path> --output-dir <path> --output-script-name <name> --output-style-name <name>\n";
    exit;
}

$htmlDirectory = dirname($htmlFile) unless $htmlDirectory;

our $htmlContents;

{
    local $/;
    open HTML, $htmlFile or die;
    $htmlContents = <HTML>;
    close HTML;
}

$htmlContents =~ m/<head>(.*)<\/head>/si;
our $headContents = $1;

mkpath $outputDirectory;

sub concatinateFiles($$$)
{
    my $filename = shift;
    my $tagExpression = shift;
    my $concatinatedTag = shift;
    my $fileCount = 0;

    open OUT, ">", "$outputDirectory/$filename" or die "Can't open $outputDirectory/$filename: $!";

    while ($headContents =~ m/$tagExpression/gi) {
        local $/;
        open IN, "$htmlDirectory/$1" or open IN, "$derivedSourcesDirectory/$1" or die "Can't open $htmlDirectory/$1: $!";
        print OUT "\n" if $fileCount++;
        print OUT "/* $1 */\n\n";
        print OUT <IN>;
        close IN;
    }

    close OUT;

    # Don't use \s so we can control the newlines we consume.
    my $replacementExpression = "([\t ]*)" . $tagExpression . "[\t ]*\n+";

    # Replace the first occurance with a token so we can inject the concatinated tag in the same place
    # as the first file that got consolidated. This makes sure we preserve some order if there are other
    # items in the head that we didn't consolidate.
    $headContents =~ s/$replacementExpression/$1%CONCATINATED%\n/i;
    $headContents =~ s/$replacementExpression//gi;
    $headContents =~ s/%CONCATINATED%/$concatinatedTag/;
}

my $inputDirectoryPattern = "(?!External\/)[^\"]*";
$inputDirectoryPattern = $inputDirectory . "\/[^\"]*" if $inputDirectory;

concatinateFiles($outputStylesheetName, "<link rel=\"stylesheet\" href=\"($inputDirectoryPattern)\">", "<link rel=\"stylesheet\" href=\"$outputStylesheetName\">");
concatinateFiles($outputScriptName, "<script src=\"($inputDirectoryPattern)\"><\/script>", "<script src=\"$outputScriptName\"></script>");

$htmlContents =~ s/<head>.*<\/head>/<head>$headContents<\/head>/si;

open HTML, ">", "$outputDirectory/" . basename($htmlFile) or die "Can't open $outputDirectory/" . basename($htmlFile) . ": $!";
print HTML $htmlContents;
close HTML;
