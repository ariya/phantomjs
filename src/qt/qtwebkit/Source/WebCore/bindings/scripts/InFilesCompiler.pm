#!/usr/bin/perl -w

# Copyright (C) 2011 Adam Barth <abarth@webkit.org>
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
# THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
#

use strict;

use Config;
use Getopt::Long;
use File::Path;
use File::Spec;
use IO::File;
use InFilesParser;

require Config;

package InFilesCompiler;

my $inputFile = "";
my $outputDir = ".";
my $defaultItemFactory;

my %parsedItems;
my %parsedParameters;

sub itemHandler($$$)
{
    my ($itemName, $property, $value) = @_;

    $parsedItems{$itemName} = { &$defaultItemFactory($itemName) } if !defined($parsedItems{$itemName});

    return unless $property;

    die "Unknown property $property for $itemName\n" if !defined($parsedItems{$itemName}{$property});
    $parsedItems{$itemName}{$property} = $value;
}

sub parameterHandler($$)
{
    my ($parameter, $value) = @_;

    die "Unknown parameter $parameter\n" if !defined($parsedParameters{$parameter});
    $parsedParameters{$parameter} = $value;
}

sub new()
{
    my $object = shift;
    my $reference = { };

    my $defaultParametersRef = shift;
    %parsedParameters = %{ $defaultParametersRef };
    $defaultItemFactory = shift;

    %parsedItems = ();

    bless($reference, $object);
    return $reference;
}

sub initializeFromCommandLine()
{
    ::GetOptions(
        'input=s' => \$inputFile,
        'outputDir=s' => \$outputDir,
    );

    die "You must specify --input <file>" unless length($inputFile);

    ::mkpath($outputDir);

    # FIXME: Should we provide outputDir via an accessor?
    return $outputDir;
}

sub compile()
{
    my $object = shift;
    my $generateCode = shift;

    my $file = new IO::File;
    open($file, $inputFile) or die "Failed to open file: $!";

    my $InParser = InFilesParser->new();
    $InParser->parse($file, \&parameterHandler, \&itemHandler);

    close($file);
    die "Failed to read from file: $inputFile" if (keys %parsedItems == 0);

    &$generateCode(\%parsedParameters, \%parsedItems);
}

sub license()
{
    return "/*
 * THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT.
 *
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

";
}

sub interfaceForItem($)
{
    my $object = shift;
    my $itemName = shift;

    my $interfaceName = $parsedItems{$itemName}{"interfaceName"};
    $interfaceName = $itemName unless $interfaceName;

    return $interfaceName;
}

sub toMacroStyle($$)
{
    my $object = shift;
    my $camelCase = shift;

    return "EVENT" if $camelCase eq "Event";
    return "EVENT_TARGET" if $camelCase eq "EventTarget";
    return "EXCEPTION" if $camelCase eq "Exception";

    die "Ok, you got me. This script is really just a giant hack. (\$camelCase=${camelCase})";
}

sub preferredConditional()
{
    my $object = shift;
    my $conditional = shift;

    my @conditionals = split('\\|', $conditional);
    return $conditionals[0];
}

sub conditionalStringFromAttributeValue()
{
    my $object = shift;
    my $conditional = shift;
    
    return "ENABLE(" . join(') || ENABLE(', split('\\|', $conditional)) . ")";
}

sub generateInterfacesHeader()
{
    my $object = shift;

    my $F;
    my $namespace = $parsedParameters{"namespace"};
    my $outputFile = "$outputDir/${namespace}Interfaces.h";

    open F, ">$outputFile" or die "Failed to open file: $!";

    print F license();

    print F "#ifndef ${namespace}Interfaces_h\n";
    print F "#define ${namespace}Interfaces_h\n";
    print F "\n";

    my %unconditionalInterfaces = ();
    my %interfacesByConditional = ();

    for my $itemName (sort keys %parsedItems) {
        my $conditional = $parsedItems{$itemName}{"conditional"};
        my $interfaceName = $object->interfaceForItem($itemName);

        if ($conditional) {
            if (!defined($interfacesByConditional{$conditional})) {
                $interfacesByConditional{$conditional} = ();
            }
            $interfacesByConditional{$conditional}{$interfaceName} = 1;
        } else {
            $unconditionalInterfaces{$interfaceName} = 1
        }
    }

    my $macroStyledNamespace = $object->toMacroStyle($namespace);

    for my $conditional (sort keys %interfacesByConditional) {
        my $preferredConditional = $object->preferredConditional($conditional);
        print F "#if " . $object->conditionalStringFromAttributeValue($conditional) . "\n";
        print F "#define DOM_${macroStyledNamespace}_INTERFACES_FOR_EACH_$preferredConditional(macro) \\\n";

        for my $interface (sort keys %{ $interfacesByConditional{$conditional} }) {
            next if defined($unconditionalInterfaces{$interface});
            print F "    macro($interface) \\\n";
        }

        print F "// End of DOM_${macroStyledNamespace}_INTERFACES_FOR_EACH_$preferredConditional\n";
        print F "#else\n";
        print F "#define DOM_${macroStyledNamespace}_INTERFACES_FOR_EACH_$preferredConditional(macro)\n";
        print F "#endif\n";
        print F "\n";
    }

    print F "#define DOM_${macroStyledNamespace}_INTERFACES_FOR_EACH(macro) \\\n";
    print F "    \\\n";
    for my $interface (sort keys %unconditionalInterfaces) {
            print F "    macro($interface) \\\n";
    }
    print F "    \\\n";
    for my $conditional (sort keys %interfacesByConditional) {
        my $preferredConditional = $object->preferredConditional($conditional);
        print F "    DOM_${macroStyledNamespace}_INTERFACES_FOR_EACH_$preferredConditional(macro) \\\n";
    }

    print F "\n";
    print F "#endif // ${namespace}Interfaces_h\n";

    close F;
}

sub generateHeadersHeader()
{
    my $object = shift;

    my $F;
    my $namespace = $parsedParameters{"namespace"};
    my $outputFile = "$outputDir/${namespace}Headers.h";

    open F, ">$outputFile" or die "Failed to open file: $!";

    print F license();

    print F "#ifndef ${namespace}Headers_h\n";
    print F "#define ${namespace}Headers_h\n";
    print F "\n";

    my %includedInterfaces = ();

    for my $itemName (sort keys %parsedItems) {
        my $conditional = $parsedItems{$itemName}{"conditional"};
        my $interfaceName = $object->interfaceForItem($itemName);

        next if defined($includedInterfaces{$interfaceName});
        $includedInterfaces{$interfaceName} = 1;

        print F "#if " . $object->conditionalStringFromAttributeValue($conditional) . "\n" if $conditional;
        print F "#include \"$interfaceName.h\"\n";
        print F "#include \"JS$interfaceName.h\"\n";
        print F "#endif\n" if $conditional;
    }

    print F "\n";
    print F "#endif // ${namespace}Headers_h\n";

    close F;
}

1;
