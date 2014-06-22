#
# WebKit IDL parser
#
# Copyright (C) 2005 Nikolas Zimmermann <wildfox@kde.org>
# Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
# Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
# Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
# Copyright (C) Research In Motion Limited 2010. All rights reserved.
# Copyright (C) 2013 Samsung Electronics. All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#

package CodeGenerator;

use strict;

use File::Find;

my $useDocument = "";
my $useGenerator = "";
my $useOutputDir = "";
my $useOutputHeadersDir = "";
my $useDirectories = "";
my $useLayerOnTop = 0;
my $preprocessor;
my $writeDependencies = 0;
my $defines = "";
my $targetIdlFilePath = "";

my $codeGenerator = 0;

my $verbose = 0;

my %numericTypeHash = ("int" => 1, "short" => 1, "long" => 1, "long long" => 1,
                       "unsigned int" => 1, "unsigned short" => 1,
                       "unsigned long" => 1, "unsigned long long" => 1,
                       "float" => 1, "double" => 1, "byte" => 1,
                       "octet" => 1);

my %primitiveTypeHash = ( "boolean" => 1, "void" => 1, "Date" => 1);

my %stringTypeHash = ("DOMString" => 1, "AtomicString" => 1);

# WebCore types used directly in IDL files.
my %webCoreTypeHash = (
    "CompareHow" => 1,
    "SerializedScriptValue" => 1,
    "Dictionary" => 1
);

my %enumTypeHash = ();

my %nonPointerTypeHash = ("DOMTimeStamp" => 1, "CompareHow" => 1);

my %svgAttributesInHTMLHash = ("class" => 1, "id" => 1, "onabort" => 1, "onclick" => 1,
                               "onerror" => 1, "onload" => 1, "onmousedown" => 1,
                               "onmouseenter" => 1, "onmouseleave" => 1,
                               "onmousemove" => 1, "onmouseout" => 1, "onmouseover" => 1,
                               "onmouseup" => 1, "onresize" => 1, "onscroll" => 1,
                               "onunload" => 1);

my %svgTypeNeedingTearOff = (
    "SVGAngle" => "SVGPropertyTearOff<SVGAngle>",
    "SVGLength" => "SVGPropertyTearOff<SVGLength>",
    "SVGLengthList" => "SVGListPropertyTearOff<SVGLengthList>",
    "SVGMatrix" => "SVGPropertyTearOff<SVGMatrix>",
    "SVGNumber" => "SVGPropertyTearOff<float>",
    "SVGNumberList" => "SVGListPropertyTearOff<SVGNumberList>",
    "SVGPathSegList" => "SVGPathSegListPropertyTearOff",
    "SVGPoint" => "SVGPropertyTearOff<SVGPoint>",
    "SVGPointList" => "SVGListPropertyTearOff<SVGPointList>",
    "SVGPreserveAspectRatio" => "SVGPropertyTearOff<SVGPreserveAspectRatio>",
    "SVGRect" => "SVGPropertyTearOff<FloatRect>",
    "SVGStringList" => "SVGStaticListPropertyTearOff<SVGStringList>",
    "SVGTransform" => "SVGPropertyTearOff<SVGTransform>",
    "SVGTransformList" => "SVGTransformListPropertyTearOff"
);

my %svgTypeWithWritablePropertiesNeedingTearOff = (
    "SVGPoint" => 1,
    "SVGMatrix" => 1
);

# Cache of IDL file pathnames.
my $idlFiles;
my $cachedInterfaces = {};

# Default constructor
sub new
{
    my $object = shift;
    my $reference = { };

    $useDirectories = shift;
    $useGenerator = shift;
    $useOutputDir = shift;
    $useOutputHeadersDir = shift;
    $useLayerOnTop = shift;
    $preprocessor = shift;
    $writeDependencies = shift;
    $verbose = shift;
    $targetIdlFilePath = shift;

    bless($reference, $object);
    return $reference;
}

sub ProcessDocument
{
    my $object = shift;
    $useDocument = shift;
    $defines = shift;

    my $ifaceName = "CodeGenerator" . $useGenerator;
    require $ifaceName . ".pm";

    %enumTypeHash = map { $_->name => $_->values } @{$useDocument->enumerations};

    # Dynamically load external code generation perl module
    $codeGenerator = $ifaceName->new($object, $useLayerOnTop, $preprocessor, $writeDependencies, $verbose, $targetIdlFilePath);
    unless (defined($codeGenerator)) {
        my $interfaces = $useDocument->interfaces;
        foreach my $interface (@$interfaces) {
            print "Skipping $useGenerator code generation for IDL interface \"" . $interface->name . "\".\n" if $verbose;
        }
        return;
    }

    my $interfaces = $useDocument->interfaces;
    foreach my $interface (@$interfaces) {
        print "Generating $useGenerator bindings code for IDL interface \"" . $interface->name . "\"...\n" if $verbose;
        $codeGenerator->GenerateInterface($interface, $defines);
        $codeGenerator->WriteData($interface, $useOutputDir, $useOutputHeadersDir);
    }
}

sub FileNamePrefix
{
    my $object = shift;

    my $ifaceName = "CodeGenerator" . $useGenerator;
    require $ifaceName . ".pm";

    # Dynamically load external code generation perl module
    $codeGenerator = $ifaceName->new($object, $useLayerOnTop, $preprocessor, $writeDependencies, $verbose);
    return $codeGenerator->FileNamePrefix();
}

sub UpdateFile
{
    my $object = shift;
    my $fileName = shift;
    my $contents = shift;

    open FH, "> $fileName" or die "Couldn't open $fileName: $!\n";
    print FH $contents;
    close FH;
}

sub ForAllParents
{
    my $object = shift;
    my $interface = shift;
    my $beforeRecursion = shift;
    my $afterRecursion = shift;

    my $recurse;
    $recurse = sub {
        my $currentInterface = shift;

        for (@{$currentInterface->parents}) {
            my $interfaceName = $_;
            my $parentInterface = $object->ParseInterface($interfaceName);

            if ($beforeRecursion) {
                &$beforeRecursion($parentInterface) eq 'prune' and next;
            }
            &$recurse($parentInterface);
            &$afterRecursion($parentInterface) if $afterRecursion;
        }
    };

    &$recurse($interface);
}

sub FindSuperMethod
{
    my ($object, $interface, $functionName) = @_;
    my $indexer;
    $object->ForAllParents($interface, undef, sub {
        my $currentInterface = shift;
        foreach my $function (@{$currentInterface->functions}) {
            if ($function->signature->name eq $functionName) {
                $indexer = $function->signature;
                return 'prune';
            }
        }
    });
    return $indexer;
}

sub IDLFileForInterface
{
    my $object = shift;
    my $interfaceName = shift;

    unless ($idlFiles) {
        my $sourceRoot = $ENV{SOURCE_ROOT};
        my @directories = map { $_ = "$sourceRoot/$_" if $sourceRoot && -d "$sourceRoot/$_"; $_ } @$useDirectories;
        push(@directories, ".");

        $idlFiles = { };

        my $wanted = sub {
            $idlFiles->{$1} = $File::Find::name if /^([A-Z].*)\.idl$/;
            $File::Find::prune = 1 if /^\../;
        };
        find($wanted, @directories);
    }

    return $idlFiles->{$interfaceName};
}

sub ParseInterface
{
    my $object = shift;
    my $interfaceName = shift;

    return undef if $interfaceName eq 'Object';

    if (exists $cachedInterfaces->{$interfaceName}) {
        return $cachedInterfaces->{$interfaceName};
    }

    # Step #1: Find the IDL file associated with 'interface'
    my $filename = $object->IDLFileForInterface($interfaceName)
        or die("Could NOT find IDL file for interface \"$interfaceName\"!\n");

    print "  |  |>  Parsing parent IDL \"$filename\" for interface \"$interfaceName\"\n" if $verbose;

    # Step #2: Parse the found IDL file (in quiet mode).
    my $parser = IDLParser->new(1);
    my $document = $parser->Parse($filename, $defines, $preprocessor);

    foreach my $interface (@{$document->interfaces}) {
        if ($interface->name eq $interfaceName) {
            $cachedInterfaces->{$interfaceName} = $interface;
            return $interface;
        }
    }

    die("Could NOT find interface definition for $interfaceName in $filename");
}

# Helpers for all CodeGenerator***.pm modules

sub SkipIncludeHeader
{
    my $object = shift;
    my $type = shift;

    return 1 if $object->IsPrimitiveType($type);

    # Special case: SVGNumber.h does not exist.
    return 1 if $type eq "SVGNumber";
    return 0;
}

sub IsConstructorTemplate
{
    my $object = shift;
    my $interface = shift;
    my $template = shift;

    return $interface->extendedAttributes->{"ConstructorTemplate"} && $interface->extendedAttributes->{"ConstructorTemplate"} eq $template;
}

sub IsNumericType
{
    my $object = shift;
    my $type = shift;

    return 1 if $numericTypeHash{$type};
    return 0;
}

sub IsPrimitiveType
{
    my $object = shift;
    my $type = shift;

    return 1 if $primitiveTypeHash{$type};
    return 1 if $numericTypeHash{$type};
    return 0;
}

sub IsStringType
{
    my $object = shift;
    my $type = shift;

    return 1 if $stringTypeHash{$type};
    return 0;
}

sub IsEnumType
{
    my $object = shift;
    my $type = shift;

    return 1 if exists $enumTypeHash{$type};
    return 0;
}

sub ValidEnumValues
{
    my $object = shift;
    my $type = shift;

    return @{$enumTypeHash{$type}};
}

sub IsNonPointerType
{
    my $object = shift;
    my $type = shift;

    return 1 if $nonPointerTypeHash{$type} or $primitiveTypeHash{$type} or $numericTypeHash{$type};
    return 0;
}

sub IsSVGTypeNeedingTearOff
{
    my $object = shift;
    my $type = shift;

    return 1 if exists $svgTypeNeedingTearOff{$type};
    return 0;
}

sub IsSVGTypeWithWritablePropertiesNeedingTearOff
{
    my $object = shift;
    my $type = shift;

    return 1 if $svgTypeWithWritablePropertiesNeedingTearOff{$type};
    return 0;
}

sub IsTypedArrayType
{
    my $object = shift;
    my $type = shift;
    return 1 if (($type eq "ArrayBuffer") or ($type eq "ArrayBufferView"));
    return 1 if (($type eq "Uint8Array") or ($type eq "Uint8ClampedArray") or ($type eq "Uint16Array") or ($type eq "Uint32Array"));
    return 1 if (($type eq "Int8Array") or ($type eq "Int16Array") or ($type eq "Int32Array"));
    return 1 if (($type eq "Float32Array") or ($type eq "Float64Array"));
    return 0;
}

sub IsRefPtrType
{
    my $object = shift;
    my $type = shift;

    return 0 if $object->IsPrimitiveType($type);
    return 0 if $object->GetArrayType($type);
    return 0 if $object->GetSequenceType($type);
    return 0 if $type eq "DOMString";
    return 0 if $object->IsEnumType($type);

    return 1;
}

sub GetSVGTypeNeedingTearOff
{
    my $object = shift;
    my $type = shift;

    return $svgTypeNeedingTearOff{$type} if exists $svgTypeNeedingTearOff{$type};
    return undef;
}

sub GetSVGWrappedTypeNeedingTearOff
{
    my $object = shift;
    my $type = shift;

    my $svgTypeNeedingTearOff = $object->GetSVGTypeNeedingTearOff($type);
    return $svgTypeNeedingTearOff if not $svgTypeNeedingTearOff;

    if ($svgTypeNeedingTearOff =~ /SVGPropertyTearOff/) {
        $svgTypeNeedingTearOff =~ s/SVGPropertyTearOff<//;
    } elsif ($svgTypeNeedingTearOff =~ /SVGListPropertyTearOff/) {
        $svgTypeNeedingTearOff =~ s/SVGListPropertyTearOff<//;
    } elsif ($svgTypeNeedingTearOff =~ /SVGStaticListPropertyTearOff/) {
        $svgTypeNeedingTearOff =~ s/SVGStaticListPropertyTearOff<//;
    }  elsif ($svgTypeNeedingTearOff =~ /SVGTransformListPropertyTearOff/) {
        $svgTypeNeedingTearOff =~ s/SVGTransformListPropertyTearOff<//;
    } 

    $svgTypeNeedingTearOff =~ s/>//;
    return $svgTypeNeedingTearOff;
}

sub IsSVGAnimatedType
{
    my $object = shift;
    my $type = shift;

    return $type =~ /^SVGAnimated/;
}

sub GetSequenceType
{
    my $object = shift;
    my $type = shift;

    return $1 if $type =~ /^sequence<([\w\d_\s]+)>.*/;
    return "";
}

sub GetArrayType
{
    my $object = shift;
    my $type = shift;

    return $1 if $type =~ /^([\w\d_\s]+)\[\]/;
    return "";
}

sub AssertNotSequenceType
{
    my $object = shift;
    my $type = shift;
    die "Sequences must not be used as the type of an attribute, constant or exception field." if $object->GetSequenceType($type);
}

# Uppercase the first letter while respecting WebKit style guidelines.
# E.g., xmlEncoding becomes XMLEncoding, but xmlllang becomes Xmllang.
sub WK_ucfirst
{
    my ($object, $param) = @_;
    my $ret = ucfirst($param);
    $ret =~ s/Xml/XML/ if $ret =~ /^Xml[^a-z]/;
    $ret =~ s/Svg/SVG/ if $ret =~ /^Svg/;

    return $ret;
}

# Lowercase the first letter while respecting WebKit style guidelines.
# URL becomes url, but SetURL becomes setURL.
sub WK_lcfirst
{
    my ($object, $param) = @_;
    my $ret = lcfirst($param);
    $ret =~ s/hTML/html/ if $ret =~ /^hTML/;
    $ret =~ s/uRL/url/ if $ret =~ /^uRL/;
    $ret =~ s/jS/js/ if $ret =~ /^jS/;
    $ret =~ s/xML/xml/ if $ret =~ /^xML/;
    $ret =~ s/xSLT/xslt/ if $ret =~ /^xSLT/;
    $ret =~ s/cSS/css/ if $ret =~ /^cSS/;

    # For HTML5 FileSystem API Flags attributes.
    # (create is widely used to instantiate an object and must be avoided.)
    $ret =~ s/^create/isCreate/ if $ret =~ /^create$/;
    $ret =~ s/^exclusive/isExclusive/ if $ret =~ /^exclusive$/;

    return $ret;
}

# Return the C++ namespace that a given attribute name string is defined in.
sub NamespaceForAttributeName
{
    my ($object, $interfaceName, $attributeName) = @_;
    return "SVGNames" if $interfaceName =~ /^SVG/ && !$svgAttributesInHTMLHash{$attributeName};
    return "HTMLNames";
}

# Identifies overloaded functions and for each function adds an array with
# links to its respective overloads (including itself).
sub LinkOverloadedFunctions
{
    my ($object, $interface) = @_;

    my %nameToFunctionsMap = ();
    foreach my $function (@{$interface->functions}) {
        my $name = $function->signature->name;
        $nameToFunctionsMap{$name} = [] if !exists $nameToFunctionsMap{$name};
        push(@{$nameToFunctionsMap{$name}}, $function);
        $function->{overloads} = $nameToFunctionsMap{$name};
        $function->{overloadIndex} = @{$nameToFunctionsMap{$name}};
    }
}

sub AttributeNameForGetterAndSetter
{
    my ($generator, $attribute) = @_;

    my $attributeName = $attribute->signature->name;
    if ($attribute->signature->extendedAttributes->{"ImplementedAs"}) {
        $attributeName = $attribute->signature->extendedAttributes->{"ImplementedAs"};
    }
    my $attributeType = $attribute->signature->type;

    # SVG animated types need to use a special attribute name.
    # The rest of the special casing for SVG animated types is handled in the language-specific code generators.
    $attributeName .= "Animated" if $generator->IsSVGAnimatedType($attributeType);

    return $attributeName;
}

sub ContentAttributeName
{
    my ($generator, $implIncludes, $interfaceName, $attribute) = @_;

    my $contentAttributeName = $attribute->signature->extendedAttributes->{"Reflect"};
    return undef if !$contentAttributeName;

    $contentAttributeName = lc $generator->AttributeNameForGetterAndSetter($attribute) if $contentAttributeName eq "VALUE_IS_MISSING";

    my $namespace = $generator->NamespaceForAttributeName($interfaceName, $contentAttributeName);

    $implIncludes->{"${namespace}.h"} = 1;
    return "WebCore::${namespace}::${contentAttributeName}Attr";
}

sub GetterExpression
{
    my ($generator, $implIncludes, $interfaceName, $attribute) = @_;

    my $contentAttributeName = $generator->ContentAttributeName($implIncludes, $interfaceName, $attribute);

    if (!$contentAttributeName) {
        return ($generator->WK_lcfirst($generator->AttributeNameForGetterAndSetter($attribute)));
    }

    my $functionName;
    if ($attribute->signature->extendedAttributes->{"URL"}) {
        $functionName = "getURLAttribute";
    } elsif ($attribute->signature->type eq "boolean") {
        $functionName = "fastHasAttribute";
    } elsif ($attribute->signature->type eq "long") {
        $functionName = "getIntegralAttribute";
    } elsif ($attribute->signature->type eq "unsigned long") {
        $functionName = "getUnsignedIntegralAttribute";
    } else {
        if ($contentAttributeName eq "WebCore::HTMLNames::idAttr") {
            $functionName = "getIdAttribute";
            $contentAttributeName = "";
        } elsif ($contentAttributeName eq "WebCore::HTMLNames::nameAttr") {
            $functionName = "getNameAttribute";
            $contentAttributeName = "";
        } elsif ($generator->IsSVGAnimatedType($attribute)) {
            $functionName = "getAttribute";
        } else {
            $functionName = "fastGetAttribute";
        }
    }

    return ($functionName, $contentAttributeName);
}

sub SetterExpression
{
    my ($generator, $implIncludes, $interfaceName, $attribute) = @_;

    my $contentAttributeName = $generator->ContentAttributeName($implIncludes, $interfaceName, $attribute);

    if (!$contentAttributeName) {
        return ("set" . $generator->WK_ucfirst($generator->AttributeNameForGetterAndSetter($attribute)));
    }

    my $functionName;
    if ($attribute->signature->type eq "boolean") {
        $functionName = "setBooleanAttribute";
    } elsif ($attribute->signature->type eq "long") {
        $functionName = "setIntegralAttribute";
    } elsif ($attribute->signature->type eq "unsigned long") {
        $functionName = "setUnsignedIntegralAttribute";
    } else {
        $functionName = "setAttribute";
    }

    return ($functionName, $contentAttributeName);
}

sub IsWrapperType
{
    my $object = shift;
    my $type = shift;

    return 0 if $object->IsPrimitiveType($type);
    return 0 if $object->GetArrayType($type);
    return 0 if $object->GetSequenceType($type);
    return 0 if $object->IsEnumType($type);
    return 0 if $object->IsStringType($type);
    return 0 if $webCoreTypeHash{$type};
    return 0 if $type eq "any";

    return 1;
}

sub IsCallbackInterface
{
  my $object = shift;
  my $type = shift;

  return 0 unless $object->IsWrapperType($type);

  my $idlFile = $object->IDLFileForInterface($type)
      or die("Could NOT find IDL file for interface \"$type\"!\n");

  open FILE, "<", $idlFile;
  my @lines = <FILE>;
  close FILE;

  my $fileContents = join('', @lines);
  return ($fileContents =~ /callback\s+interface\s+(\w+)/gs);
}

sub GenerateConditionalString
{
    my $generator = shift;
    my $node = shift;

    my $conditional = $node->extendedAttributes->{"Conditional"};
    if ($conditional) {
        return $generator->GenerateConditionalStringFromAttributeValue($conditional);
    } else {
        return "";
    }
}

sub GenerateConstructorConditionalString
{
    my $generator = shift;
    my $node = shift;

    my $conditional = $node->extendedAttributes->{"ConstructorConditional"};
    if ($conditional) {
        return $generator->GenerateConditionalStringFromAttributeValue($conditional);
    } else {
        return "";
    }
}

sub GenerateConditionalStringFromAttributeValue
{
    my $generator = shift;
    my $conditional = shift;

    my $operator = ($conditional =~ /&/ ? '&' : ($conditional =~ /\|/ ? '|' : ''));
    if ($operator) {
        # Avoid duplicated conditions.
        my %conditions;
        map { $conditions{$_} = 1 } split('\\' . $operator, $conditional);
        return "ENABLE(" . join(") $operator$operator ENABLE(", sort keys %conditions) . ")";
    } else {
        return "ENABLE(" . $conditional . ")";
    }
}

sub GenerateCompileTimeCheckForEnumsIfNeeded
{
    my ($generator, $interface) = @_;
    my $interfaceName = $interface->name;
    my @checks = ();
    # If necessary, check that all constants are available as enums with the same value.
    if (!$interface->extendedAttributes->{"DoNotCheckConstants"} && @{$interface->constants}) {
        push(@checks, "\n");
        foreach my $constant (@{$interface->constants}) {
            my $reflect = $constant->extendedAttributes->{"Reflect"};
            my $name = $reflect ? $reflect : $constant->name;
            my $value = $constant->value;
            my $conditional = $constant->extendedAttributes->{"Conditional"};

            if ($conditional) {
                my $conditionalString = $generator->GenerateConditionalStringFromAttributeValue($conditional);
                push(@checks, "#if ${conditionalString}\n");
            }

            if ($constant->extendedAttributes->{"ImplementedBy"}) {
                push(@checks, "COMPILE_ASSERT($value == " . $constant->extendedAttributes->{"ImplementedBy"} . "::$name, ${interfaceName}Enum${name}IsWrongUseDoNotCheckConstants);\n");
            } else {
                push(@checks, "COMPILE_ASSERT($value == ${interfaceName}::$name, ${interfaceName}Enum${name}IsWrongUseDoNotCheckConstants);\n");
            }

            if ($conditional) {
                push(@checks, "#endif\n");
            }
        }
        push(@checks, "\n");
    }
    return @checks;
}

sub ExtendedAttributeContains
{
    my $object = shift;
    my $callWith = shift;
    return 0 unless $callWith;
    my $keyword = shift;

    my @callWithKeywords = split /\s*\&\s*/, $callWith;
    return grep { $_ eq $keyword } @callWithKeywords;
}

# FIXME: This is backwards. We currently name the interface and the IDL files with the implementation name. We
# should use the real interface name in the IDL files and then use ImplementedAs to map this to the implementation name.
sub GetVisibleInterfaceName
{
    my $object = shift;
    my $interface = shift;
    my $interfaceName = $interface->extendedAttributes->{"InterfaceName"};
    return $interfaceName ? $interfaceName : $interface->name;
}

sub InheritsInterface
{
    my $object = shift;
    my $interface = shift;
    my $interfaceName = shift;
    my $found = 0;

    return 1 if $interfaceName eq $interface->name;
    $object->ForAllParents($interface, sub {
        my $currentInterface = shift;
        if ($currentInterface->name eq $interfaceName) {
            $found = 1;
        }
        return 1 if $found;
    }, 0);

    return $found;
}

sub InheritsExtendedAttribute
{
    my $object = shift;
    my $interface = shift;
    my $extendedAttribute = shift;
    my $found = 0;

    return 1 if $interface->extendedAttributes->{$extendedAttribute};
    $object->ForAllParents($interface, sub {
        my $currentInterface = shift;
        if ($currentInterface->extendedAttributes->{$extendedAttribute}) {
            $found = 1;
        }
        return 1 if $found;
    }, 0);

    return $found;
}

1;
