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
        'conditional' => 0,
        'runtimeConditional' => 0
    );
}

my $InCompiler = InFilesCompiler->new(\%defaultParameters, \&defaultItemFactory);

my $outputDir = $InCompiler->initializeFromCommandLine();
$InCompiler->compile(\&generateCode);

sub generateCode()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    generateImplementation($parsedParametersRef, $parsedItemsRef);
    $InCompiler->generateInterfacesHeader();
    $InCompiler->generateHeadersHeader()
}

sub generateImplementation()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    my $F;
    my %parsedEvents = %{ $parsedItemsRef };
    my %parsedParameters = %{ $parsedParametersRef };

    my $namespace = $parsedParameters{"namespace"};

    # Currently, only Events have factory files.
    return if $namespace ne "Event";

    my $outputFile = "$outputDir/${namespace}Factory.cpp";

    open F, ">$outputFile" or die "Failed to open file: $!";

    print F $InCompiler->license();

    print F "#include \"config.h\"\n";
    print F "#include \"${namespace}Factory.h\"\n";
    print F "\n";
    print F "#include \"${namespace}Headers.h\"\n";
    print F "\n";
    print F "namespace WebCore {\n";
    print F "\n";
    print F "PassRefPtr<$namespace> ${namespace}Factory::create(const String& type)\n";
    print F "{\n";

    for my $eventName (sort keys %parsedEvents) {
        my $conditional = $parsedEvents{$eventName}{"conditional"};
        my $runtimeConditional = $parsedEvents{$eventName}{"runtimeConditional"};
        my $interfaceName = $InCompiler->interfaceForItem($eventName);

        print F "#if ENABLE($conditional)\n" if $conditional;
        # FIXEME JSC should support RuntimeEnabledFeatures
        print F "    if (type == \"$eventName\")\n";
        print F "        return ${interfaceName}::create();\n";
        print F "#endif\n" if $conditional;
    }

    print F "    return 0;\n";
    print F "}\n";
    print F "\n";
    print F "} // namespace WebCore\n";

    close F;
}
