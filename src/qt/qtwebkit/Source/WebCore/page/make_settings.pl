#!/usr/bin/perl -w

# Copyright (C) 2012 Tony Chang <tony@chromium.org>
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
# THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. `AS IS'' AND ANY
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

use strict;

use InFilesCompiler;

my %defaultParameters = (
);

my %webcoreTypeToIdlType = (
    'int' => 'long',
    'unsigned' => 'unsigned long',
    'size_t' => 'unsigned long',
    'double' => 'double',
    'float' => 'float',
    'String' => 'DOMString',
    'bool' => 'boolean'
);

sub defaultItemFactory
{
    return (
        'conditional' => 0,
        'initial' => '',
        'type' => 'bool',
        'setNeedsStyleRecalcInAllFrames' => 0,
    );
}

my $InCompiler = InFilesCompiler->new(\%defaultParameters, \&defaultItemFactory);

my $outputDir = $InCompiler->initializeFromCommandLine();
$InCompiler->compile(\&generateCode);

sub generateCode()
{
    my $parsedParametersRef = shift;
    my $parsedItemsRef = shift;

    generateSettingsMacrosHeader($parsedItemsRef);
    generateInternalSettingsIdlFile($parsedItemsRef);
    generateInternalSettingsHeaderFile($parsedItemsRef);
    generateInternalSettingsCppFile($parsedItemsRef);
}

sub generateSettingsMacrosHeader($)
{
    my $parsedItemsRef = shift;

    my %parsedItems = %{ $parsedItemsRef };
    my $outputFile = "$outputDir/SettingsMacros.h";

    my %unconditionalSettings = ();
    my %settingsByConditional = ();

    for my $settingName (sort keys %parsedItems) {
        my $conditional = $parsedItems{$settingName}{"conditional"};

        if ($conditional) {
            if (!defined($settingsByConditional{$conditional})) {
                $settingsByConditional{$conditional} = ();
            }
            $settingsByConditional{$conditional}{$settingName} = 1;
        } else {
            $unconditionalSettings{$settingName} = 1;
        }
    }

    open my $file, ">$outputFile" or die "Failed to open file: $!";

    print $file $InCompiler->license();

    # FIXME: Sort by type so bools come last and are bit packed.

    print $file "#ifndef SettingsMacros_h\n";
    print $file "#define SettingsMacros_h\n\n";

    printConditionalMacros($file, \%settingsByConditional, $parsedItemsRef);

    printGettersAndSetters($file, \%unconditionalSettings, \%settingsByConditional, $parsedItemsRef);
    printMemberVariables($file, \%unconditionalSettings, \%settingsByConditional, $parsedItemsRef);
    printInitializerList($file, \%unconditionalSettings, \%settingsByConditional, $parsedItemsRef);
    printSetterBodies($file, \%unconditionalSettings, \%settingsByConditional, $parsedItemsRef);

    print $file "#endif // SettingsMacros_h\n";

    close $file;
}

sub printConditionalMacros($$$)
{
    my ($file, $settingsByConditionalRef, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };
    my %settingsByConditional = %{ $settingsByConditionalRef };

    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "#if " . $InCompiler->conditionalStringFromAttributeValue($conditional) . "\n";

        print $file "#define ${preferredConditional}_SETTINGS_GETTER_AND_SETTERS \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            printGetterAndSetter($file, $settingName, $parsedItems{$settingName}{"type"}, $parsedItems{$settingName}{"setNeedsStyleRecalcInAllFrames"});
        }
        print $file "// End of ${preferredConditional}_SETTINGS_GETTER_AND_SETTERS\n";

        print $file "#define ${preferredConditional}_SETTINGS_NON_BOOL_MEMBER_VARIABLES \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            my $type = $parsedItems{$settingName}{"type"};
            next if $type eq "bool";
            print $file "    $type m_$settingName; \\\n"
        }
        print $file "// End of ${preferredConditional}_SETTINGS_NON_BOOL_MEMBER_VARIABLES\n";

        print $file "#define ${preferredConditional}_SETTINGS_BOOL_MEMBER_VARIABLES \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            next if $parsedItems{$settingName}{"type"} ne "bool";
            print $file "    bool m_$settingName : 1; \\\n"
        }
        print $file "// End of ${preferredConditional}_SETTINGS_BOOL_MEMBER_VARIABLES\n";

        print $file "#define ${preferredConditional}_SETTINGS_NON_BOOL_INITIALIZERS \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            next if $parsedItems{$settingName}{"type"} eq "bool";
            printInitializer($file, $settingName, $parsedItemsRef);
        }
        print $file "// End of ${preferredConditional}_SETTINGS_NON_BOOL_INITIALIZERS\n";

        print $file "#define ${preferredConditional}_SETTINGS_BOOL_INITIALIZERS \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            next if $parsedItems{$settingName}{"type"} ne "bool";
            printInitializer($file, $settingName, $parsedItemsRef);
        }
        print $file "// End of ${preferredConditional}_SETTINGS_BOOL_INITIALIZERS\n";

        print $file "#define ${preferredConditional}_SETTINGS_SETTER_BODIES \\\n";
        for my $settingName (sort keys %{ $settingsByConditional{$conditional} }) {
            printSetterBody($file, $settingName, $parsedItems{$settingName}{"type"}, $parsedItems{$settingName}{"setNeedsStyleRecalcInAllFrames"});
        }
        print $file "// End of ${preferredConditional}_SETTINGS_SETTER_BODIES\n";

        print $file "#else\n";
        print $file "#define ${preferredConditional}_SETTINGS_GETTER_AND_SETTERS\n";
        print $file "#define ${preferredConditional}_SETTINGS_NON_BOOL_MEMBER_VARIABLES\n";
        print $file "#define ${preferredConditional}_SETTINGS_BOOL_MEMBER_VARIABLES\n";
        print $file "#define ${preferredConditional}_SETTINGS_NON_BOOL_INITIALIZERS\n";
        print $file "#define ${preferredConditional}_SETTINGS_BOOL_INITIALIZERS\n";
        print $file "#define ${preferredConditional}_SETTINGS_SETTER_BODIES\n";
        print $file "#endif\n";
        print $file "\n";
    }
}

sub printGettersAndSetters($$$$)
{
    my ($file, $unconditionalSettingsRef, $settingsByConditionalRef, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };
    my %unconditionalSettings = %{ $unconditionalSettingsRef };
    my %settingsByConditional = %{ $settingsByConditionalRef };

    print $file "#define SETTINGS_GETTERS_AND_SETTERS \\\n";
    for my $settingName (sort keys %unconditionalSettings) {
        printGetterAndSetter($file, $settingName, $parsedItems{$settingName}{"type"}, $parsedItems{$settingName}{"setNeedsStyleRecalcInAllFrames"});
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_GETTER_AND_SETTERS \\\n";
    }
    print $file "// End of SETTINGS_GETTERS_AND_SETTERS.\n\n";
}

sub printMemberVariables($$$$)
{
    my ($file, $unconditionalSettingsRef, $settingsByConditionalRef, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };
    my %unconditionalSettings = %{ $unconditionalSettingsRef };
    my %settingsByConditional = %{ $settingsByConditionalRef };

    print $file "#define SETTINGS_MEMBER_VARIABLES \\\n";
    # We list the bools last so we can bit pack them.
    for my $settingName (sort keys %unconditionalSettings) {
        my $type = $parsedItems{$settingName}{"type"};
        next if $type eq "bool";
        print $file "    $type m_$settingName; \\\n"
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_NON_BOOL_MEMBER_VARIABLES \\\n";
    }
    for my $settingName (sort keys %unconditionalSettings) {
        next if $parsedItems{$settingName}{"type"} ne "bool";
        print $file "    bool m_$settingName : 1; \\\n"
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_BOOL_MEMBER_VARIABLES \\\n";
    }
    print $file "// End of SETTINGS_MEMBER_VARIABLES.\n\n";
}

sub setterFunctionName($)
{
    my $settingName = shift;
    my $setterFunctionName = "set" . $settingName;
    substr($setterFunctionName, 3, 1) = uc(substr($setterFunctionName, 3, 1));
    my @prefixesToUpperCase = ("css", "xss", "ftp", "dom");
    foreach my $prefix (@prefixesToUpperCase) {
        my $prefixLength = length($prefix);
        if (substr($settingName, 0, $prefixLength) eq $prefix) {
            substr($setterFunctionName, $prefixLength, $prefixLength) = uc(substr($setterFunctionName, 3, 3));
        }
    }
    return $setterFunctionName;
}

sub printGetterAndSetter($$$$)
{
    my ($file, $settingName, $type, $setNeedsStyleRecalcInAllFrames) = @_;
    my $setterFunctionName = setterFunctionName($settingName);
    if (lc(substr($type, 0, 1)) eq substr($type, 0, 1)) {
        print $file "    $type $settingName() const { return m_$settingName; } \\\n";
        print $file "    void $setterFunctionName($type $settingName)";
    } else {
        print $file "    const $type& $settingName() { return m_$settingName; } \\\n";
        print $file "    void $setterFunctionName(const $type& $settingName)";
    }
    if ($setNeedsStyleRecalcInAllFrames) {
        print $file "; \\\n";
    } else {
        print $file " { m_$settingName = $settingName; } \\\n";
    }
}

sub printInitializerList($$$$)
{
    my ($file, $unconditionalSettingsRef, $settingsByConditionalRef, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };
    my %unconditionalSettings = %{ $unconditionalSettingsRef };
    my %settingsByConditional = %{ $settingsByConditionalRef };

    print $file "#define SETTINGS_INITIALIZER_LIST \\\n";
    for my $settingName (sort keys %unconditionalSettings) {
        next if $parsedItems{$settingName}{"type"} eq "bool";
        printInitializer($file, $settingName, $parsedItemsRef);
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_NON_BOOL_INITIALIZERS \\\n";
    }
    for my $settingName (sort keys %unconditionalSettings) {
        next if $parsedItems{$settingName}{"type"} ne "bool";
        printInitializer($file, $settingName, $parsedItemsRef);
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_BOOL_INITIALIZERS \\\n";
    }
    print $file "// End of SETTINGS_INITIALIZER_LIST.\n\n";
}

sub printInitializer($$$)
{
    my ($file, $settingName, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };

    my $initialValue = $parsedItems{$settingName}{"initial"};
    my $type = $parsedItems{$settingName}{"type"};
    die "Must provide an initial value for $settingName." if ($initialValue eq '' && lc(substr($type, 0, 1)) eq substr($type, 0, 1));
    return if ($initialValue eq '');
    print $file "    , m_$settingName($initialValue) \\\n"
}

sub printSetterBodies($$$$)
{
    my ($file, $unconditionalSettingsRef, $settingsByConditionalRef, $parsedItemsRef) = @_;
    my %parsedItems = %{ $parsedItemsRef };
    my %unconditionalSettings = %{ $unconditionalSettingsRef };
    my %settingsByConditional = %{ $settingsByConditionalRef };

    print $file "#define SETTINGS_SETTER_BODIES \\\n";
    for my $settingName (sort keys %unconditionalSettings) {
        printSetterBody($file, $settingName, $parsedItems{$settingName}{"type"}, $parsedItems{$settingName}{"setNeedsStyleRecalcInAllFrames"});
    }
    for my $conditional (sort keys %settingsByConditional) {
        my $preferredConditional = $InCompiler->preferredConditional($conditional);
        print $file "    ${preferredConditional}_SETTINGS_SETTER_BODIES \\\n";
    }
    print $file "// End of SETTINGS_SETTER_BODIES.\n\n";
}

sub printSetterBody($$$$)
{
    my ($file, $settingName, $type, $setNeedsStyleRecalcInAllFrames) = @_;
    return if (!$setNeedsStyleRecalcInAllFrames);

    my $setterFunctionName = setterFunctionName($settingName);
    if (lc(substr($type, 0, 1)) eq substr($type, 0, 1)) {
        print $file "void Settings::$setterFunctionName($type $settingName) \\\n";
    } else {
        print $file "void Settings::$setterFunctionName(const $type& $settingName) \\\n";
    }
    print $file "{ \\\n";
    print $file "    if (m_$settingName == $settingName) \\\n";
    print $file "        return; \\\n";
    print $file "    m_$settingName = $settingName; \\\n";
    print $file "    m_page->setNeedsRecalcStyleInAllFrames(); \\\n";
    print $file "} \\\n";
}

sub enumerateParsedItems($$$)
{
    my ($file, $parsedItemsRef, $visitorFunction) = @_;
    my %parsedItems = %{ $parsedItemsRef };

    for my $settingName (sort keys %parsedItems) {
        my $type = $parsedItems{$settingName}{"type"};
        # FIXME: Learn how to auto-generate code for enumerate types.
        next if (!defined($webcoreTypeToIdlType{$type}));

        &$visitorFunction($file, $parsedItemsRef, $settingName)
    }
}

sub generateInternalSettingsIdlFile($)
{
    my $parsedItemsRef = shift;

    my $filename = "$outputDir/InternalSettingsGenerated.idl";
    open my $file, ">$filename" or die "Failed to open file: $!";
    print $file $InCompiler->license();

    print $file "[\n";
    print $file "    NoInterfaceObject,\n";
    print $file "] interface InternalSettingsGenerated {\n";

    sub writeIdlSetter($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $idlType = $webcoreTypeToIdlType{$type};
        my $setterFunctionName = setterFunctionName($settingName);
        print $file "    void $setterFunctionName(in $idlType $settingName);\n";
    };

    enumerateParsedItems($file, $parsedItemsRef, \&writeIdlSetter);

    print $file "};\n";
    close $file;
}

sub generateInternalSettingsHeaderFile($)
{
    my $parsedItemsRef = shift;
    my %parsedItems = %{ $parsedItemsRef };

    my $filename = "$outputDir/InternalSettingsGenerated.h";
    open my $file, ">$filename" or die "Failed to open file: $!";
    print $file $InCompiler->license();

    print $file <<EOF;
#ifndef InternalSettingsGenerated_h
#define InternalSettingsGenerated_h

#include "RefCountedSupplement.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Page;

class InternalSettingsGenerated : public RefCounted<InternalSettingsGenerated> {
public:
    explicit InternalSettingsGenerated(Page*);
    virtual ~InternalSettingsGenerated();
    void resetToConsistentState();
EOF
    sub writeHeaderPrototypes($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $setterFunctionName = setterFunctionName($settingName);
        $type = "const String&" if $type eq "String";
        print $file "    void $setterFunctionName($type $settingName);\n";
    };
    enumerateParsedItems($file, $parsedItemsRef, \&writeHeaderPrototypes);

    print $file <<EOF;

private:
    Page* m_page;

EOF

    sub writeBackupMembers($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $conditional = $parsedItems{$settingName}{"conditional"};
        if ($conditional) {
            print $file "#if " . $InCompiler->conditionalStringFromAttributeValue($conditional) . "\n";
        }
        print $file "    $type m_$settingName;\n";
        if ($conditional) {
            print $file "#endif\n";
        }
    };
    enumerateParsedItems($file, $parsedItemsRef, \&writeBackupMembers);

    print $file "};\n\n";
    print $file "} // namespace WebCore\n";
    print $file "#endif // InternalSettingsGenerated_h\n";

    close $file;
}

sub generateInternalSettingsCppFile($)
{
    my $parsedItemsRef = shift;
    my %parsedItems = %{ $parsedItemsRef };

    my $filename = "$outputDir/InternalSettingsGenerated.cpp";
    open my $file, ">$filename" or die "Failed to open file: $!";
    print $file $InCompiler->license();

    print $file <<EOF;
#include "config.h"
#include "InternalSettingsGenerated.h"

#include "Page.h"
#include "Settings.h"

namespace WebCore {

InternalSettingsGenerated::InternalSettingsGenerated(Page* page)
    : m_page(page)
EOF

    sub writeBackupInitializers($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $conditional = $parsedItems{$settingName}{"conditional"};
        if ($conditional) {
            print $file "#if " . $InCompiler->conditionalStringFromAttributeValue($conditional) . "\n";
        }
        print $file "    , m_$settingName(page->settings()->$settingName())\n";
        if ($conditional) {
            print $file "#endif\n";
        }
    };
    enumerateParsedItems($file, $parsedItemsRef, \&writeBackupInitializers);

    print $file <<EOF;
{
}

InternalSettingsGenerated::~InternalSettingsGenerated()
{
}

void InternalSettingsGenerated::resetToConsistentState()
{
EOF
    sub writeResetToConsistentState($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $setterFunctionName = setterFunctionName($settingName);
        my $conditional = $parsedItems{$settingName}{"conditional"};
        if ($conditional) {
            print $file "#if " . $InCompiler->conditionalStringFromAttributeValue($conditional) . "\n";
        }
        print $file "    m_page->settings()->$setterFunctionName(m_$settingName);\n";
        if ($conditional) {
            print $file "#endif\n";
        }
    };
    enumerateParsedItems($file, $parsedItemsRef, \&writeResetToConsistentState);

    print $file "}\n";

    sub writeSetterFunctions($$$) {
        my ($file, $parsedItemsRef, $settingName) = @_;
        my %parsedItems = %{ $parsedItemsRef };
        my $type = $parsedItems{$settingName}{"type"};
        my $conditional = $parsedItems{$settingName}{"conditional"};
        my $setterFunctionName = setterFunctionName($settingName);
        $type = "const String&" if $type eq "String";

        print $file "void InternalSettingsGenerated::$setterFunctionName($type $settingName)\n";
        print $file "{\n";

        if ($conditional) {
            print $file "#if " . $InCompiler->conditionalStringFromAttributeValue($conditional) . "\n";
        }
        print $file "    m_page->settings()->$setterFunctionName($settingName);\n";
        if ($conditional) {
            print $file "#else\n";
            print $file "    UNUSED_PARAM($settingName);\n";
            print $file "#endif\n";
        }
        print $file "}\n\n";
    };
    enumerateParsedItems($file, $parsedItemsRef, \&writeSetterFunctions);

    print $file "} // namespace WebCore\n";

    close $file;
}
