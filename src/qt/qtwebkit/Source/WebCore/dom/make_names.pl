#!/usr/bin/perl -w

# Copyright (C) 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
# Copyright (C) 2009, Julien Chaffraix <jchaffraix@webkit.org>
# Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
# Copyright (C) 2011 Ericsson AB. All rights reserved.
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

use StaticString;
use Config;
use Getopt::Long;
use File::Path;
use File::Spec;
use IO::File;
use InFilesParser;

sub readTags($$);
sub readAttrs($$);

my $printFactory = 0; 
my $printWrapperFactory = 0; 
my $fontNamesIn = "";
my $tagsFile = "";
my $attrsFile = "";
my $outputDir = ".";
my %parsedTags = ();
my %parsedAttrs = ();
my %enabledTags = ();
my %enabledAttrs = ();
my %allTags = ();
my %allAttrs = ();
my %allStrings = ();
my %parameters = ();
my $extraDefines = 0;
my $initDefaults = 1;
my %extensionAttrs = ();

require Config;

my $gccLocation = "";
if ($ENV{CC}) {
    $gccLocation = $ENV{CC};
} elsif (($Config::Config{'osname'}) =~ /solaris/i) {
    $gccLocation = "/usr/sfw/bin/gcc";
} else {
    $gccLocation = "/usr/bin/gcc";
}
my $preprocessor = $gccLocation . " -E -x c++";

GetOptions(
    'tags=s' => \$tagsFile, 
    'attrs=s' => \$attrsFile,
    'factory' => \$printFactory,
    'outputDir=s' => \$outputDir,
    'extraDefines=s' => \$extraDefines,
    'preprocessor=s' => \$preprocessor,
    'wrapperFactory' => \$printWrapperFactory,
    'fonts=s' => \$fontNamesIn
);

mkpath($outputDir);

if (length($fontNamesIn)) {
    my $names = new IO::File;
    my $familyNamesFileBase = "WebKitFontFamily";

    open($names, $fontNamesIn) or die "Failed to open file: $fontNamesIn";

    $initDefaults = 0;
    my $Parser = InFilesParser->new();
    my $dummy;
    $Parser->parse($names, \&parametersHandler, \&dummy);

    my $F;
    my $header = File::Spec->catfile($outputDir, "${familyNamesFileBase}Names.h");
    open F, ">$header" or die "Unable to open $header for writing.";

    printLicenseHeader($F);
    printHeaderHead($F, "CSS", $familyNamesFileBase, "#include <wtf/text/AtomicString.h>");

    printMacros($F, "extern const WTF::AtomicString", "", \%parameters);
    print F "#endif\n\n";

    printInit($F, 1);
    close F;

    my $source = File::Spec->catfile($outputDir, "${familyNamesFileBase}Names.cpp");
    open F, ">$source" or die "Unable to open $source for writing.";

    printLicenseHeader($F);
    printCppHead($F, "CSS", $familyNamesFileBase, "WTF");

    print F StaticString::GenerateStrings(\%parameters);

    while ( my ($name, $identifier) = each %parameters ) {
        print F "DEFINE_GLOBAL(AtomicString, $name)\n";
    }

    printInit($F, 0);

    print F "\n";
    print F StaticString::GenerateStringAsserts(\%parameters);

    while ( my ($name, $identifier) = each %parameters ) {
        print F "    new (NotNull, (void*)&$name) AtomicString(${name}Impl);\n";
    }

    print F "}\n}\n}\n";
    close F;
    exit 0;
}

die "You must specify at least one of --tags <file> or --attrs <file>" unless (length($tagsFile) || length($attrsFile));

if (length($tagsFile)) {
    %allTags = %{readTags($tagsFile, 0)};
    %enabledTags = %{readTags($tagsFile, 1)};
    namesToStrings(\%allTags, \%allStrings);
}

if (length($attrsFile)) {
    %allAttrs = %{readAttrs($attrsFile, 0)};
    %enabledAttrs = %{readAttrs($attrsFile, 1)};
    namesToStrings(\%allAttrs, \%allStrings);
}

die "You must specify a namespace (e.g. SVG) for <namespace>Names.h" unless $parameters{namespace};
die "You must specify a namespaceURI (e.g. http://www.w3.org/2000/svg)" unless $parameters{namespaceURI};

$parameters{namespacePrefix} = $parameters{namespace} unless $parameters{namespacePrefix};

my $namesBasePath = "$outputDir/$parameters{namespace}Names";
my $factoryBasePath = "$outputDir/$parameters{namespace}ElementFactory";
my $wrapperFactoryFileName = "$parameters{namespace}ElementWrapperFactory";

printNamesHeaderFile("$namesBasePath.h");
printNamesCppFile("$namesBasePath.cpp");

if ($printFactory) {
    printFactoryCppFile("$factoryBasePath.cpp");
    printFactoryHeaderFile("$factoryBasePath.h");
}

if ($printWrapperFactory) {
    printWrapperFactoryCppFile($outputDir, $wrapperFactoryFileName);
    printWrapperFactoryHeaderFile($outputDir, $wrapperFactoryFileName);
}

### Hash initialization

sub defaultTagPropertyHash
{
    return (
        'constructorNeedsCreatedByParser' => 0,
        'constructorNeedsFormElement' => 0,
        'noConstructor' => 0,
        'interfaceName' => defaultInterfaceName($_[0]),
        # By default, the JSInterfaceName is the same as the interfaceName.
        'JSInterfaceName' => defaultInterfaceName($_[0]),
        'mapToTagName' => '',
        'wrapperOnlyIfMediaIsAvailable' => 0,
        'conditional' => 0,
        'contextConditional' => 0,
        'runtimeConditional' => 0
    );
}

sub defaultParametersHash
{
    return (
        'namespace' => '',
        'namespacePrefix' => '',
        'namespaceURI' => '',
        'guardFactoryWith' => '',
        'tagsNullNamespace' => 0,
        'attrsNullNamespace' => 0,
        'fallbackInterfaceName' => ''
    );
}

sub defaultInterfaceName
{
    die "No namespace found" if !$parameters{namespace};
    return $parameters{namespace} . upperCaseName($_[0]) . "Element"
}

### Parsing handlers

sub valueForName
{
    my $name = shift;
    my $value = $extensionAttrs{$name};

    if (!$value) {
        $value = $name;
        $value =~ s/_/-/g;
    }

    return $value;
}

sub namesToStrings
{
    my $namesRef = shift;
    my $stringsRef = shift;

    my %names = %$namesRef;

    for my $name (keys %names) {
        $stringsRef->{$name} = valueForName($name);
    }
}

sub tagsHandler
{
    my ($tag, $property, $value) = @_;

    $tag =~ s/-/_/g;

    # Initialize default property values.
    $parsedTags{$tag} = { defaultTagPropertyHash($tag) } if !defined($parsedTags{$tag});

    if ($property) {
        die "Unknown property $property for tag $tag\n" if !defined($parsedTags{$tag}{$property});

        # The code relies on JSInterfaceName deriving from interfaceName to check for custom JSInterfaceName.
        # So override JSInterfaceName if it was not already set.
        $parsedTags{$tag}{JSInterfaceName} = $value if $property eq "interfaceName" && $parsedTags{$tag}{JSInterfaceName} eq $parsedTags{$tag}{interfaceName};

        $parsedTags{$tag}{$property} = $value;
    }
}

sub attrsHandler
{
    my ($attr, $property, $value) = @_;
    # Translate HTML5 extension attributes of the form 'x-webkit-feature' to 'webkitfeature'.
    # We don't just check for the 'x-' prefix because there are attributes such as x-height
    # which should follow the default path below.
    if ($attr =~ m/^x-webkit-(.*)/) {
        my $newAttr = "webkit$1";
        $extensionAttrs{$newAttr} = $attr;
        $attr = $newAttr;
    }
    $attr =~ s/-/_/g;

    # Initialize default properties' values.
    $parsedAttrs{$attr} = {} if !defined($parsedAttrs{$attr});

    if ($property) {
        die "Unknown property $property for attribute $attr\n" if !defined($parsedAttrs{$attr}{$property});
        $parsedAttrs{$attr}{$property} = $value;
    }
}

sub parametersHandler
{
    my ($parameter, $value) = @_;

    # Initialize default properties' values.
    %parameters = defaultParametersHash() if (!(keys %parameters) && $initDefaults);

    die "Unknown parameter $parameter for tags/attrs\n" if (!defined($parameters{$parameter}) && $initDefaults);
    $parameters{$parameter} = $value;
}

## Support routines

sub preprocessorCommand()
{
    return $preprocessor if $extraDefines eq 0;
    return $preprocessor . " -D" . join(" -D", split(" ", $extraDefines));
}

sub readNames($$$$)
{
    my ($namesFile, $hashToFillRef, $handler, $usePreprocessor) = @_;

    my $names = new IO::File;
    if ($usePreprocessor) {
        open($names, preprocessorCommand() . " " . $namesFile . "|") or die "Failed to open file: $namesFile";
    } else {
        open($names, $namesFile) or die "Failed to open file: $namesFile";
    }

    my $InParser = InFilesParser->new();
    $InParser->parse($names, \&parametersHandler, $handler);

    close($names);
    die "Failed to read names from file: $namesFile" if (keys %{$hashToFillRef} == 0);
    return $hashToFillRef;
}

sub readAttrs($$)
{
    my ($namesFile, $usePreprocessor) = @_;
    %parsedAttrs = ();
    return readNames($namesFile, \%parsedAttrs, \&attrsHandler, $usePreprocessor);
}

sub readTags($$)
{
    my ($namesFile, $usePreprocessor) = @_;
    %parsedTags = ();
    return readNames($namesFile, \%parsedTags, \&tagsHandler, $usePreprocessor);
}

sub printMacros
{
    my ($F, $macro, $suffix, $namesRef) = @_;
    my %names = %$namesRef;

    for my $name (sort keys %names) {
        print F "$macro $name","$suffix;\n";
    }
}

sub usesDefaultWrapper
{
    my $tagName = shift;
    return $tagName eq $parameters{namespace} . "Element";
}

# Build a direct mapping from the tags to the Element to create.
sub buildConstructorMap
{
    my %tagConstructorMap = ();
    for my $tagName (keys %enabledTags) {
        my $interfaceName = $enabledTags{$tagName}{interfaceName};

        if ($enabledTags{$tagName}{mapToTagName}) {
            die "Cannot handle multiple mapToTagName for $tagName\n" if $enabledTags{$enabledTags{$tagName}{mapToTagName}}{mapToTagName};
            $interfaceName = $enabledTags{ $enabledTags{$tagName}{mapToTagName} }{interfaceName};
        }

        # Chop the string to keep the interesting part.
        $interfaceName =~ s/$parameters{namespace}(.*)Element/$1/;
        $tagConstructorMap{$tagName} = lc($interfaceName);
    }

    return %tagConstructorMap;
}

# Helper method that print the constructor's signature avoiding
# unneeded arguments.
sub printConstructorSignature
{
    my ($F, $tagName, $constructorName, $constructorTagName) = @_;

    print F "static PassRefPtr<$parameters{namespace}Element> ${constructorName}Constructor(const QualifiedName& $constructorTagName, Document* document";
    if ($parameters{namespace} eq "HTML") {
        print F ", HTMLFormElement*";
        print F " formElement" if $enabledTags{$tagName}{constructorNeedsFormElement};
    }
    print F ", bool";
    print F " createdByParser" if $enabledTags{$tagName}{constructorNeedsCreatedByParser};
    print F ")\n{\n";
}

# Helper method to dump the constructor interior and call the 
# Element constructor with the right arguments.
# The variable names should be kept in sync with the previous method.
sub printConstructorInterior
{
    my ($F, $tagName, $interfaceName, $constructorTagName) = @_;

    # Handle media elements.
    if ($enabledTags{$tagName}{wrapperOnlyIfMediaIsAvailable}) {
        print F <<END
    Settings* settings = document->settings();
    if (!MediaPlayer::isAvailable() || (settings && !settings->mediaEnabled()))
        return 0;
    
END
;
    }

    my $contextConditional = $enabledTags{$tagName}{contextConditional};
    if ($contextConditional) {
        print F <<END
    if (!ContextFeatures::${contextConditional}Enabled(document))
        return 0;
END
;
    }

    my $runtimeConditional = $enabledTags{$tagName}{runtimeConditional};
    if ($runtimeConditional) {
        print F <<END
    if (!RuntimeEnabledFeatures::${runtimeConditional}Enabled())
        return 0;
END
;
    }

    # Call the constructor with the right parameters.
    print F "    return ${interfaceName}::create($constructorTagName, document";
    print F ", formElement" if $enabledTags{$tagName}{constructorNeedsFormElement};
    print F ", createdByParser" if $enabledTags{$tagName}{constructorNeedsCreatedByParser};
    print F ");\n}\n\n";
}

sub printConstructors
{
    my ($F, $tagConstructorMapRef) = @_;
    my %tagConstructorMap = %$tagConstructorMapRef;

    # This is to avoid generating the same constructor several times.
    my %uniqueTags = ();
    for my $tagName (sort keys %tagConstructorMap) {
        my $interfaceName = $enabledTags{$tagName}{interfaceName};

        # Ignore the mapped tag
        # FIXME: It could be moved inside this loop but was split for readibility.
        next if (defined($uniqueTags{$interfaceName}) || $enabledTags{$tagName}{mapToTagName});
        # Tags can have wrappers without constructors.
        # This is useful to make user-agent shadow elements internally testable
        # while keeping them from being avaialble in the HTML markup.
        next if $enabledTags{$tagName}{noConstructor};

        $uniqueTags{$interfaceName} = '1';

        my $conditional = $enabledTags{$tagName}{conditional};
        if ($conditional) {
            my $conditionalString = "ENABLE(" . join(") && ENABLE(", split(/&/, $conditional)) . ")";
            print F "#if ${conditionalString}\n\n";
        }

        printConstructorSignature($F, $tagName, $tagConstructorMap{$tagName}, "tagName");
        printConstructorInterior($F, $tagName, $interfaceName, "tagName");

        if ($conditional) {
            print F "#endif\n";
        }
    }

    # Mapped tag name uses a special wrapper to keep their prefix and namespaceURI while using the mapped localname.
    for my $tagName (sort keys %tagConstructorMap) {
        if ($enabledTags{$tagName}{mapToTagName}) {
            my $mappedName = $enabledTags{$tagName}{mapToTagName};
            printConstructorSignature($F, $mappedName, $mappedName . "To" . $tagName, "tagName");
            printConstructorInterior($F, $mappedName, $enabledTags{$mappedName}{interfaceName}, "QualifiedName(tagName.prefix(), ${mappedName}Tag.localName(), tagName.namespaceURI())");
        }
    }
}

sub printFunctionInits
{
    my ($F, $tagConstructorMap) = @_;
    my %tagConstructorMap = %$tagConstructorMap;

    for my $tagName (sort keys %tagConstructorMap) {
        next if $enabledTags{$tagName}{noConstructor};

        my $conditional = $enabledTags{$tagName}{conditional};
        if ($conditional) {
            my $conditionalString = "ENABLE(" . join(") && ENABLE(", split(/&/, $conditional)) . ")";
            print F "#if ${conditionalString}\n";
        }

        if ($enabledTags{$tagName}{mapToTagName}) {
            print F "    addTag(${tagName}Tag, $enabledTags{$tagName}{mapToTagName}To${tagName}Constructor);\n";
        } else {
            print F "    addTag(${tagName}Tag, $tagConstructorMap{$tagName}Constructor);\n";
        }

        if ($conditional) {
            print F "#endif\n\n";
        }
    }
}

sub svgCapitalizationHacks
{
    my $name = shift;

    $name = "FE" . ucfirst $1 if $name =~ /^fe(.+)$/;

    return $name;
}

sub upperCaseName
{
    my $name = shift;
    
    $name = svgCapitalizationHacks($name) if ($parameters{namespace} eq "SVG");

    while ($name =~ /^(.*?)_(.*)/) {
        $name = $1 . ucfirst $2;
    }
    
    return ucfirst $name;
}

sub printHeaderHead
{
    my ($F, $prefix, $nsName, $includes) = @_;

    print F "#ifndef ${prefix}_${nsName}Names_h\n";
    print F "#define ${prefix}_${nsName}Names_h\n\n";
    print F "$includes\n\n";

    print F "namespace WebCore {\n\n";
    print F "namespace ${nsName}Names {\n\n";

    print F "#ifndef ${prefix}_${nsName}NAMES_HIDE_GLOBALS\n";
}

sub printCppHead
{
    my ($F, $prefix, $nsName, $usedNamespace) = @_;

    print F "#include \"config.h\"\n\n";
    print F "#ifdef SKIP_STATIC_CONSTRUCTORS_ON_GCC\n";
    print F "#define ${prefix}_${nsName}NAMES_HIDE_GLOBALS 1\n";
    print F "#else\n";
    print F "#define QNAME_DEFAULT_CONSTRUCTOR 1\n";
    print F "#endif\n\n";

    print F "#include \"${nsName}Names.h\"\n\n";
    print F "#include <wtf/StaticConstructors.h>\n";

    print F "namespace WebCore {\n\n";
    print F "namespace ${nsName}Names {\n\n";
    print F "using namespace $usedNamespace;\n\n";
}

sub printInit
{
    my ($F, $isDefinition) = @_;

    if ($isDefinition) {
        print F "\nvoid init();\n\n";
        print F "} }\n\n";
        print F "#endif\n\n";
        return;
    }

print F "\nvoid init()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    // Use placement new to initialize the globals.

    AtomicString::init();
";
}

sub printLicenseHeader
{
    my $F = shift;
    print F "/*
 * THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT.
 *
 * This file was generated by the dom/make_names.pl script.
 *
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

sub printNamesHeaderFile
{
    my ($headerPath) = shift;
    my $F;
    open F, ">$headerPath";

    printLicenseHeader($F);
    printHeaderHead($F, "DOM", $parameters{namespace}, "#include \"QualifiedName.h\"");

    my $lowerNamespace = lc($parameters{namespacePrefix});
    print F "// Namespace\n";
    print F "extern const WTF::AtomicString ${lowerNamespace}NamespaceURI;\n\n";

    if (keys %allTags) {
        print F "// Tags\n";
        printMacros($F, "extern const WebCore::QualifiedName", "Tag", \%allTags);
    }

    if (keys %allAttrs) {
        print F "// Attributes\n";
        printMacros($F, "extern const WebCore::QualifiedName", "Attr", \%allAttrs);
    }
    print F "#endif\n\n";

    if (keys %allTags) {
        print F "const unsigned $parameters{namespace}TagsCount = ", scalar(keys %allTags), ";\n";
        print F "WebCore::QualifiedName** get$parameters{namespace}Tags();\n";
    }

    if (keys %allAttrs) {
        print F "const unsigned $parameters{namespace}AttrsCount = ", scalar(keys %allAttrs), ";\n";
        print F "WebCore::QualifiedName** get$parameters{namespace}Attrs();\n";
    }

    printInit($F, 1);
    close F;
}

sub printNamesCppFile
{
    my $cppPath = shift;
    my $F;
    open F, ">$cppPath";
    
    printLicenseHeader($F);
    printCppHead($F, "DOM", $parameters{namespace}, "WebCore");
    
    my $lowerNamespace = lc($parameters{namespacePrefix});

    print F "DEFINE_GLOBAL(AtomicString, ${lowerNamespace}NamespaceURI)\n\n";

    print F StaticString::GenerateStrings(\%allStrings);

    if (keys %allTags) {
        print F "// Tags\n";
        for my $name (sort keys %allTags) {
            print F "DEFINE_GLOBAL(QualifiedName, ", $name, "Tag)\n";
        }
        
        print F "\n\nWebCore::QualifiedName** get$parameters{namespace}Tags()\n";
        print F "{\n    static WebCore::QualifiedName* $parameters{namespace}Tags[] = {\n";
        for my $name (sort keys %allTags) {
            print F "        (WebCore::QualifiedName*)&${name}Tag,\n";
        }
        print F "    };\n";
        print F "    return $parameters{namespace}Tags;\n";
        print F "}\n";
    }

    if (keys %allAttrs) {
        print F "\n// Attributes\n";
        for my $name (sort keys %allAttrs) {
            print F "DEFINE_GLOBAL(QualifiedName, ", $name, "Attr)\n";
        }
        print F "\n\nWebCore::QualifiedName** get$parameters{namespace}Attrs()\n";
        print F "{\n    static WebCore::QualifiedName* $parameters{namespace}Attr[] = {\n";
        for my $name (sort keys %allAttrs) {
            print F "        (WebCore::QualifiedName*)&${name}Attr,\n";
        }
        print F "    };\n";
        print F "    return $parameters{namespace}Attr;\n";
        print F "}\n";
    }

    printInit($F, 0);

    print(F "    AtomicString ${lowerNamespace}NS(\"$parameters{namespaceURI}\", AtomicString::ConstructFromLiteral);\n\n");

    print(F "    // Namespace\n");
    print(F "    new (NotNull, (void*)&${lowerNamespace}NamespaceURI) AtomicString(${lowerNamespace}NS);\n");
    print(F "\n");
    print F StaticString::GenerateStringAsserts(\%allStrings);

    if (keys %allTags) {
        my $tagsNamespace = $parameters{tagsNullNamespace} ? "nullAtom" : "${lowerNamespace}NS";
        printDefinitions($F, \%allTags, "tags", $tagsNamespace);
    }
    if (keys %allAttrs) {
        my $attrsNamespace = $parameters{attrsNullNamespace} ? "nullAtom" : "${lowerNamespace}NS";
        printDefinitions($F, \%allAttrs, "attributes", $attrsNamespace);
    }

    print F "}\n\n} }\n\n";
    close F;
}

sub printJSElementIncludes
{
    my $F = shift;

    my %tagsSeen;
    for my $tagName (sort keys %enabledTags) {
        my $JSInterfaceName = $enabledTags{$tagName}{JSInterfaceName};
        next if defined($tagsSeen{$JSInterfaceName}) || usesDefaultJSWrapper($tagName);
        if ($enabledTags{$tagName}{conditional}) {
            # We skip feature-define-specific #includes here since we handle them separately.
            next;
        }
        $tagsSeen{$JSInterfaceName} = 1;

        print F "#include \"JS${JSInterfaceName}.h\"\n";
    }
    print F "#include \"JS$parameters{fallbackInterfaceName}.h\"\n";
}

sub printElementIncludes
{
    my $F = shift;

    my %tagsSeen;
    for my $tagName (sort keys %enabledTags) {
        my $interfaceName = $enabledTags{$tagName}{interfaceName};
        next if defined($tagsSeen{$interfaceName});
        if ($enabledTags{$tagName}{conditional}) {
            # We skip feature-define-specific #includes here since we handle them separately.
            next;
        }
        $tagsSeen{$interfaceName} = 1;

        print F "#include \"${interfaceName}.h\"\n";
    }
    print F "#include \"$parameters{fallbackInterfaceName}.h\"\n";
}

sub printConditionalElementIncludes
{
    my ($F, $wrapperIncludes) = @_;

    my %conditionals;
    my %unconditionalElementIncludes;
    my %unconditionalJSElementIncludes;

    for my $tagName (keys %enabledTags) {
        my $conditional = $enabledTags{$tagName}{conditional};
        my $interfaceName = $enabledTags{$tagName}{interfaceName};
        my $JSInterfaceName = $enabledTags{$tagName}{JSInterfaceName};

        if ($conditional) {
            $conditionals{$conditional}{interfaceNames}{$interfaceName} = 1;
            $conditionals{$conditional}{JSInterfaceNames}{$JSInterfaceName} = 1;
        } else {
            $unconditionalElementIncludes{$interfaceName} = 1;
            $unconditionalJSElementIncludes{$JSInterfaceName} = 1;
        }
    }

    for my $conditional (sort keys %conditionals) {
        print F "\n#if ENABLE($conditional)\n";
        for my $interfaceName (sort keys %{$conditionals{$conditional}{interfaceNames}}) {
            next if $unconditionalElementIncludes{$interfaceName};
            print F "#include \"$interfaceName.h\"\n";
        }
        if ($wrapperIncludes) {
            for my $JSInterfaceName (sort keys %{$conditionals{$conditional}{JSInterfaceNames}}) {
                next if $unconditionalJSElementIncludes{$JSInterfaceName};
                print F "#include \"JS$JSInterfaceName.h\"\n";
            }
        }
        print F "#endif\n";
    }
}

sub printDefinitions
{
    my ($F, $namesRef, $type, $namespaceURI) = @_;
    my $singularType = substr($type, 0, -1);
    my $shortType = substr($singularType, 0, 4);
    my $shortCamelType = ucfirst($shortType);
    my $shortUpperType = uc($shortType);
    
    print F "    // " . ucfirst($type) . "\n";

    for my $name (sort keys %$namesRef) {
        # To generate less code in init(), the common case of nullAtom for the namespace, we call createQualifiedName() without passing $namespaceURI.
        if ($namespaceURI eq "nullAtom") {
            print F "    createQualifiedName((void*)&$name","${shortCamelType}, ${name}Impl);\n";
        } else {
            print F "    createQualifiedName((void*)&$name","${shortCamelType}, ${name}Impl, $namespaceURI);\n";
        }
    }
}

## ElementFactory routines

sub printFactoryCppFile
{
    my $cppPath = shift;
    my $F;
    open F, ">$cppPath";

printLicenseHeader($F);

print F <<END
#include "config.h"
END
;

print F "\n#if $parameters{guardFactoryWith}\n\n" if $parameters{guardFactoryWith};

print F <<END
#include "$parameters{namespace}ElementFactory.h"
#include "$parameters{namespace}Names.h"
END
;

printElementIncludes($F);

print F "\n#include <wtf/HashMap.h>\n";

printConditionalElementIncludes($F, 0);

print F <<END

#include "ContextFeatures.h"
#include "RuntimeEnabledFeatures.h"

#if ENABLE(CUSTOM_ELEMENTS)
#include "CustomElementConstructor.h"
#include "CustomElementRegistry.h"
#endif

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(VIDEO)
#include "Document.h"
#include "Settings.h"
#endif

namespace WebCore {

using namespace $parameters{namespace}Names;

END
;

print F "typedef PassRefPtr<$parameters{namespace}Element> (*ConstructorFunction)(const QualifiedName&, Document*";
print F ", HTMLFormElement*" if $parameters{namespace} eq "HTML";
print F ", bool createdByParser);\n";
print F <<END
typedef HashMap<AtomicStringImpl*, ConstructorFunction> FunctionMap;

static FunctionMap* gFunctionMap = 0;

END
;

my %tagConstructorMap = buildConstructorMap();

printConstructors($F, \%tagConstructorMap);

print F <<END
static void addTag(const QualifiedName& tag, ConstructorFunction func)
{
    gFunctionMap->set(tag.localName().impl(), func);
}

static void createFunctionMap()
{
    ASSERT(!gFunctionMap);

    // Create the table.
    gFunctionMap = new FunctionMap;
    
    // Populate it with constructor functions.
END
;

printFunctionInits($F, \%tagConstructorMap);

print F "}\n";


print F "\nPassRefPtr<$parameters{namespace}Element> $parameters{namespace}ElementFactory::create$parameters{namespace}Element(const QualifiedName& qName, Document* document";
print F ", HTMLFormElement* formElement" if $parameters{namespace} eq "HTML";
print F ", bool createdByParser)\n{\n";

print F <<END
    if (!document)
        return 0;

END
;

if ($parameters{namespace} ne "HTML" and $parameters{namespace} ne "SVG") {
print F <<END
#if ENABLE(DASHBOARD_SUPPORT)
    Settings* settings = document->settings();
    if (settings && settings->usesDashboardBackwardCompatibilityMode())
        return 0;
#endif
END
;

}

print F <<END
#if ENABLE(CUSTOM_ELEMENTS)
    if (document->registry()) {
        if (RefPtr<CustomElementConstructor> constructor = document->registry()->find(nullQName(), qName)) {
            RefPtr<Element> element = constructor->createElement();
            ASSERT(element->is$parameters{namespace}Element());
            return static_pointer_cast<$parameters{namespace}Element>(element.release());
        }
    }
#endif

    if (!gFunctionMap)
        createFunctionMap();
    if (ConstructorFunction function = gFunctionMap->get(qName.localName().impl())) {
END
;

if ($parameters{namespace} eq "HTML") {
    print F "        if (PassRefPtr<$parameters{namespace}Element> element = function(qName, document, formElement, createdByParser))\n";
    print F "            return element;\n";
} else {
    print F "        if (PassRefPtr<$parameters{namespace}Element> element = function(qName, document, createdByParser))\n";
    print F "            return element;\n";
}
print F <<END
    }

    return $parameters{fallbackInterfaceName}::create(qName, document);
}

} // namespace WebCore

END
;

    print F "#endif\n" if $parameters{guardFactoryWith};

    close F;
}

sub printFactoryHeaderFile
{
    my $headerPath = shift;
    my $F;
    open F, ">$headerPath";

    printLicenseHeader($F);

    print F<<END
#ifndef $parameters{namespace}ElementFactory_h
#define $parameters{namespace}ElementFactory_h

#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {
    class Element;
    class Document;
    class QualifiedName;
}

namespace WebCore {

    class $parameters{namespace}Element;
END
;

print F "     class HTMLFormElement;\n" if $parameters{namespace} eq "HTML";

print F<<END
    // The idea behind this class is that there will eventually be a mapping from namespace URIs to ElementFactories that can dispense
    // elements. In a compound document world, the generic createElement function (will end up being virtual) will be called.
    class $parameters{namespace}ElementFactory {
    public:
        PassRefPtr<Element> createElement(const WebCore::QualifiedName&, WebCore::Document*, bool createdByParser = true);
END
;
print F "        static PassRefPtr<$parameters{namespace}Element> create$parameters{namespace}Element(const WebCore::QualifiedName&, WebCore::Document*";
print F ", HTMLFormElement* = 0" if $parameters{namespace} eq "HTML";
print F ", bool createdByParser = true);\n";

printf F<<END
    };
}

#endif // $parameters{namespace}ElementFactory_h

END
;

    close F;
}

## Wrapper Factory routines

sub usesDefaultJSWrapper
{
    my $name = shift;

    # A tag reuses the default wrapper if its JSInterfaceName matches the default namespace Element.
    return $enabledTags{$name}{JSInterfaceName} eq $parameters{namespace} . "Element";
}

sub printWrapperFunctions
{
    my $F = shift;

    my %tagsSeen;
    for my $tagName (sort keys %enabledTags) {
        # Avoid defining the same wrapper method twice.
        my $JSInterfaceName = $enabledTags{$tagName}{JSInterfaceName};
        next if defined($tagsSeen{$JSInterfaceName}) || (usesDefaultJSWrapper($tagName) && ($parameters{fallbackInterfaceName} eq $parameters{namespace} . "Element"));
        $tagsSeen{$JSInterfaceName} = 1;

        my $conditional = $enabledTags{$tagName}{conditional};
        if ($conditional) {
            my $conditionalString = "ENABLE(" . join(") && ENABLE(", split(/&/, $conditional)) . ")";
            print F "#if ${conditionalString}\n\n";
        }

        # Hack for the media tags
        # FIXME: This should have been done via a CustomWrapper attribute and a separate *Custom file.
        if ($enabledTags{$tagName}{wrapperOnlyIfMediaIsAvailable}) {
            print F <<END
static JSDOMWrapper* create${JSInterfaceName}Wrapper(ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<$parameters{namespace}Element> element)
{
    Settings* settings = element->document()->settings();
    if (!MediaPlayer::isAvailable() || (settings && !settings->mediaEnabled()))
        return CREATE_DOM_WRAPPER(exec, globalObject, $parameters{namespace}Element, element.get());
    return CREATE_DOM_WRAPPER(exec, globalObject, ${JSInterfaceName}, element.get());
}

END
    ;
        } elsif ($enabledTags{$tagName}{contextConditional}) {
            my $contextConditional = $enabledTags{$tagName}{contextConditional};
            print F <<END
static JSDOMWrapper* create${JSInterfaceName}Wrapper(ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<$parameters{namespace}Element> element)
{
    if (!ContextFeatures::${contextConditional}Enabled(element->document())) {
        ASSERT(!element || element->is$parameters{fallbackInterfaceName}());
        return CREATE_DOM_WRAPPER(exec, globalObject, $parameters{fallbackInterfaceName}, element.get());
    }

    return CREATE_DOM_WRAPPER(exec, globalObject, ${JSInterfaceName}, element.get());
}
END
    ;
        } elsif ($enabledTags{$tagName}{runtimeConditional}) {
            my $runtimeConditional = $enabledTags{$tagName}{runtimeConditional};
            print F <<END
static JSDOMWrapper* create${JSInterfaceName}Wrapper(ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<$parameters{namespace}Element> element)
{
    if (!RuntimeEnabledFeatures::${runtimeConditional}Enabled()) {
        ASSERT(!element || element->is$parameters{fallbackInterfaceName}());
        return CREATE_DOM_WRAPPER(exec, globalObject, $parameters{fallbackInterfaceName}, element.get());
    }

    return CREATE_DOM_WRAPPER(exec, globalObject, ${JSInterfaceName}, element.get());
}
END
    ;
        } else {
            print F <<END
static JSDOMWrapper* create${JSInterfaceName}Wrapper(ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<$parameters{namespace}Element> element)
{
    return CREATE_DOM_WRAPPER(exec, globalObject, ${JSInterfaceName}, element.get());
}

END
    ;
        }

        if ($conditional) {
            print F "#endif\n\n";
        }
    }
}

sub printWrapperFactoryCppFile
{
    my $outputDir = shift;
    my $wrapperFactoryFileName = shift;
    my $F;
    open F, ">" . $outputDir . "/JS" . $wrapperFactoryFileName . ".cpp";

    printLicenseHeader($F);

    print F "#include \"config.h\"\n";
    print F "#include \"JS$parameters{namespace}ElementWrapperFactory.h\"\n";

    print F "\n#if $parameters{guardFactoryWith}\n\n" if $parameters{guardFactoryWith};

    printJSElementIncludes($F);

    print F "\n#include \"$parameters{namespace}Names.h\"\n\n";

    printElementIncludes($F);

    print F "\n#include <wtf/StdLibExtras.h>\n";

    printConditionalElementIncludes($F, 1);

    print F <<END

#include "ContextFeatures.h"
#include "RuntimeEnabledFeatures.h"

#if ENABLE(VIDEO)
#include "Document.h"
#include "Settings.h"
#endif

END
;

    print F <<END
using namespace JSC;
END
;

    print F <<END

namespace WebCore {

using namespace $parameters{namespace}Names;

END
;
print F <<END
typedef JSDOMWrapper* (*Create$parameters{namespace}ElementWrapperFunction)(ExecState*, JSDOMGlobalObject*, PassRefPtr<$parameters{namespace}Element>);

END
;

    printWrapperFunctions($F);

print F <<END
JSDOMWrapper* createJS$parameters{namespace}Wrapper(ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<$parameters{namespace}Element> element)
{
    typedef HashMap<WTF::AtomicStringImpl*, Create$parameters{namespace}ElementWrapperFunction> FunctionMap;
    DEFINE_STATIC_LOCAL(FunctionMap, map, ());
    if (map.isEmpty()) {
END
;

    for my $tag (sort keys %enabledTags) {
        # Do not add the name to the map if it does not have a JS wrapper constructor or uses the default wrapper.
        next if (usesDefaultJSWrapper($tag, \%enabledTags) && ($parameters{fallbackInterfaceName} eq $parameters{namespace} . "Element"));

        my $conditional = $enabledTags{$tag}{conditional};
        if ($conditional) {
            my $conditionalString = "ENABLE(" . join(") && ENABLE(", split(/&/, $conditional)) . ")";
            print F "#if ${conditionalString}\n";
        }

        my $ucTag = $enabledTags{$tag}{JSInterfaceName};
        print F "       map.set(${tag}Tag.localName().impl(), create${ucTag}Wrapper);\n";

        if ($conditional) {
            print F "#endif\n";
        }
    }

    print F <<END
    }
END
;

    print F <<END
    Create$parameters{namespace}ElementWrapperFunction createWrapperFunction = map.get(element->localName().impl());
    if (createWrapperFunction)
END
;
    print F <<END
        return createWrapperFunction(exec, globalObject, element);
    return CREATE_DOM_WRAPPER(exec, globalObject, $parameters{fallbackInterfaceName}, element.get());
END
;
    print F <<END
}
END
;

    print F "}\n\n";
    print F "#endif\n" if $parameters{guardFactoryWith};

    close F;
}

sub printWrapperFactoryHeaderFile
{
    my $outputDir = shift;
    my $wrapperFactoryFileName = shift;
    my $F;
    open F, ">" . $outputDir . "/JS" . $wrapperFactoryFileName . ".h";

    printLicenseHeader($F);

    print F "#ifndef JS$parameters{namespace}ElementWrapperFactory_h\n";
    print F "#define JS$parameters{namespace}ElementWrapperFactory_h\n\n";

    print F "#if $parameters{guardFactoryWith}\n" if $parameters{guardFactoryWith};

    print F <<END
#include <wtf/Forward.h>

namespace JSC {
    class ExecState;
}                                            
                                             
namespace WebCore {

    class JSDOMWrapper;
    class JSDOMGlobalObject;
    class $parameters{namespace}Element;

    JSDOMWrapper* createJS$parameters{namespace}Wrapper(JSC::ExecState*, JSDOMGlobalObject*, PassRefPtr<$parameters{namespace}Element>);

}
 
END
    ;

    print F "#endif // $parameters{guardFactoryWith}\n\n" if $parameters{guardFactoryWith};

    print F "#endif // JS$parameters{namespace}ElementWrapperFactory_h\n";

    close F;
}
