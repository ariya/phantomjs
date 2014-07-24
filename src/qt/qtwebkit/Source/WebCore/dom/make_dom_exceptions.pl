#!/usr/bin/perl -w

# Copyright (C) 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
# Copyright (C) 2009, Julien Chaffraix <jchaffraix@webkit.org>
# Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
# Copyright (C) 2011 Ericsson AB. All rights reserved.
# Copyright (C) 2011 Google, Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use strict;

use InFilesCompiler;

my %defaultParameters = (
    'namespace' => 0
);

sub defaultItemFactory
{
    return (
        'interfaceName' => 0,
        'conditional' => 0
    );
}

my $InCompiler = InFilesCompiler->new(\%defaultParameters, \&defaultItemFactory);

my $outputDir = $InCompiler->initializeFromCommandLine();
$InCompiler->compile(\&generateCode);

sub generateCode()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    generateHeader($parsedParametersRef, $parsedItemsRef);
    generateImplementation($parsedParametersRef, $parsedItemsRef);
    $InCompiler->generateInterfacesHeader();
    $InCompiler->generateHeadersHeader()
}

sub generateHeader()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    my $F;
    my %parsedItems = %{ $parsedItemsRef };

    my $outputFile = "$outputDir/ExceptionCodeDescription.h";

    open F, ">$outputFile" or die "Failed to open file: $!";

    print F $InCompiler->license();

    print F "#ifndef ExceptionCodeDescription_h\n";
    print F "#define ExceptionCodeDescription_h\n";
    print F "\n";
    print F "namespace WebCore {\n";
    print F "\n";
    print F "typedef int ExceptionCode;\n";
    print F "\n";
    print F "enum ExceptionType {\n";

    for my $exceptionType (sort keys %parsedItems) {
        my $conditional = $parsedItems{$exceptionType}{"conditional"};

        print F "#if ENABLE($conditional)\n" if $conditional;
        print F "    ${exceptionType}Type,\n";
        print F "#endif\n" if $conditional;
    }

    print F "};\n";
    print F "\n";
    print F "struct ExceptionCodeDescription {\n";
    print F "    explicit ExceptionCodeDescription(ExceptionCode);\n";
    print F "\n";
    print F "    // |typeName| has spaces and is suitable for use in exception\n";
    print F "    // description strings; maximum length is 10 characters.\n";
    print F "    const char* typeName; \n";
    print F "\n";
    print F "    // |name| is the exception name, also intended for use in exception\n";
    print F "    // description strings; 0 if name not known; maximum length is 27\n";
    print F "    // characters.\n";
    print F "    const char* name; \n";
    print F "\n";
    print F "    // |description| is the exception description, intended for use in\n";
    print F "    // exception strings. It is a more readable explanation of error.\n";
    print F "    const char* description;\n";
    print F "\n";
    print F "    // |code| is the numeric value of the exception within a particular type.\n";
    print F "    int code; \n";
    print F "\n";
    print F "    ExceptionType type;\n";
    print F "};\n";
    print F "\n";
    print F "} // namespace WebCore\n";
    print F "\n";
    print F "#endif // ExceptionCodeDescription_h\n";

    close F;
}

sub generateImplementation()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    my $F;
    my %parsedItems = %{ $parsedItemsRef };

    my $outputFile = "$outputDir/ExceptionCodeDescription.cpp";

    open F, ">$outputFile" or die "Failed to open file: $!";

    print F $InCompiler->license();

    print F "#include \"config.h\"\n";
    print F "#include \"ExceptionCodeDescription.h\"\n";
    print F "\n";
    print F "#include \"ExceptionCode.h\"\n";

    for my $exceptionType (sort keys %parsedItems) {
        my $conditional = $parsedItems{$exceptionType}{"conditional"};

        print F "#if ENABLE($conditional)\n" if $conditional;
        print F "#include \"$exceptionType.h\"\n";
        print F "#endif\n" if $conditional;
    }

    print F "#if ENABLE(INDEXED_DATABASE)\n";
    print F "#include \"IDBDatabaseException.h\"\n";
    print F "#endif\n";

    print F "\n";
    print F "namespace WebCore {\n";
    print F "\n";
    print F "ExceptionCodeDescription::ExceptionCodeDescription(ExceptionCode ec)\n";
    print F "{\n";
    print F "    ASSERT(ec);\n";

    for my $exceptionType (sort keys %parsedItems) {
        # DOMCoreException needs to be last because it's a catch-all.
        next if $exceptionType eq "DOMCoreException";

        my $conditional = $parsedItems{$exceptionType}{"conditional"};

        print F "#if ENABLE($conditional)\n" if $conditional;
        print F "    if (${exceptionType}::initializeDescription(ec, this))\n";
        print F "        return;\n";
        print F "#endif\n" if $conditional;
    }

    # FIXME: This special case for IDB is undesirable. It is the first usage
    # of "new style" DOMExceptions where there is no IDL type, but there are
    # API-specific exception names and/or messages. Consider refactoring back
    # into the code generator when a common pattern emerges.
    print F "#if ENABLE(INDEXED_DATABASE)\n";
    print F "    if (IDBDatabaseException::initializeDescription(ec, this))\n";
    print F "        return;\n";
    print F "#endif\n";

    print F "    if (DOMCoreException::initializeDescription(ec, this))\n";
    print F "        return;\n";
    print F "    ASSERT_NOT_REACHED();\n";
    print F "}\n";
    print F "\n";
    print F "} // namespace WebCore\n";

    close F;
}
