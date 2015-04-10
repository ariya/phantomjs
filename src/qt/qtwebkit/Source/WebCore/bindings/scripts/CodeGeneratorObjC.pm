# 
# Copyright (C) 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
# Copyright (C) 2006 Anders Carlsson <andersca@mac.com> 
# Copyright (C) 2006, 2007 Samuel Weinig <sam@webkit.org>
# Copyright (C) 2006 Alexey Proskuryakov <ap@webkit.org>
# Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
# Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
# Copyright (C) 2010 Google Inc.
# Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

package CodeGeneratorObjC;

use constant FileNamePrefix => "DOM";

# Global Variables
my $writeDependencies = 0;
my %publicInterfaces = ();
my $newPublicClass = 0;
my $interfaceAvailabilityVersion = "";
my $isProtocol = 0;
my $noImpl = 0;

my @headerContentHeader = ();
my @headerContent = ();
my %headerForwardDeclarations = ();
my %headerForwardDeclarationsForProtocols = ();

my @privateHeaderContentHeader = ();
my @privateHeaderContent = ();
my %privateHeaderForwardDeclarations = ();
my %privateHeaderForwardDeclarationsForProtocols = ();

my @internalHeaderContent = ();

my @implContentHeader = ();
my @implContent = ();
my %implIncludes = ();
my @depsContent = ();

# Hashes
my %protocolTypeHash = ("XPathNSResolver" => 1, "EventListener" => 1, "EventTarget" => 1, "NodeFilter" => 1,
                        "SVGFilterPrimitiveStandardAttributes" => 1, 
                        "SVGTests" => 1, "SVGLangSpace" => 1, "SVGExternalResourcesRequired" => 1, "SVGURIReference" => 1,
                        "SVGZoomAndPan" => 1, "SVGFitToViewBox" => 1, "SVGAnimatedPathData" => 1);
my %nativeObjCTypeHash = ("URL" => 1, "Color" => 1);

# FIXME: this should be replaced with a function that recurses up the tree
# to find the actual base type.
my %baseTypeHash = ("Object" => 1, "Node" => 1, "NodeList" => 1, "NamedNodeMap" => 1, "DOMImplementation" => 1,
                    "Event" => 1, "CSSRule" => 1, "CSSValue" => 1, "StyleSheet" => 1, "MediaList" => 1,
                    "Counter" => 1, "Rect" => 1, "RGBColor" => 1, "XPathExpression" => 1, "XPathResult" => 1,
                    "NodeIterator" => 1, "TreeWalker" => 1, "AbstractView" => 1, "Blob" => 1,
                    "SVGAngle" => 1, "SVGAnimatedAngle" => 1, "SVGAnimatedBoolean" => 1, "SVGAnimatedEnumeration" => 1,
                    "SVGAnimatedInteger" => 1, "SVGAnimatedLength" => 1, "SVGAnimatedLengthList" => 1,
                    "SVGAnimatedNumber" => 1, "SVGAnimatedNumberList" => 1,
                    "SVGAnimatedPreserveAspectRatio" => 1, "SVGAnimatedRect" => 1, "SVGAnimatedString" => 1,
                    "SVGAnimatedTransformList" => 1, "SVGLength" => 1, "SVGLengthList" => 1, "SVGMatrix" => 1,
                    "SVGNumber" => 1, "SVGNumberList" => 1, "SVGPathSeg" => 1, "SVGPathSegList" => 1, "SVGPoint" => 1,
                    "SVGPointList" => 1, "SVGPreserveAspectRatio" => 1, "SVGRect" => 1, "SVGRenderingIntent" => 1,
                    "SVGStringList" => 1, "SVGTransform" => 1, "SVGTransformList" => 1, "SVGUnitTypes" => 1);

# Constants
my $nullableInit = "bool isNull = false;";
my $exceptionInit = "WebCore::ExceptionCode ec = 0;";
my $jsContextSetter = "WebCore::JSMainThreadNullState state;";
my $exceptionRaiseOnError = "WebCore::raiseOnDOMError(ec);";
my $assertMainThread = "{ DOM_ASSERT_MAIN_THREAD(); WebCoreThreadViolationCheckRoundOne(); }";

my %conflictMethod = (
    # FIXME: Add C language keywords?
    # FIXME: Add other predefined types like "id"?

    "callWebScriptMethod:withArguments:" => "WebScriptObject",
    "evaluateWebScript:" => "WebScriptObject",
    "removeWebScriptKey:" => "WebScriptObject",
    "setException:" => "WebScriptObject",
    "setWebScriptValueAtIndex:value:" => "WebScriptObject",
    "stringRepresentation" => "WebScriptObject",
    "webScriptValueAtIndex:" => "WebScriptObject",

    "autorelease" => "NSObject",
    "awakeAfterUsingCoder:" => "NSObject",
    "class" => "NSObject",
    "classForCoder" => "NSObject",
    "conformsToProtocol:" => "NSObject",
    "copy" => "NSObject",
    "copyWithZone:" => "NSObject",
    "dealloc" => "NSObject",
    "description" => "NSObject",
    "doesNotRecognizeSelector:" => "NSObject",
    "encodeWithCoder:" => "NSObject",
    "finalize" => "NSObject",
    "forwardInvocation:" => "NSObject",
    "hash" => "NSObject",
    "init" => "NSObject",
    "initWithCoder:" => "NSObject",
    "isEqual:" => "NSObject",
    "isKindOfClass:" => "NSObject",
    "isMemberOfClass:" => "NSObject",
    "isProxy" => "NSObject",
    "methodForSelector:" => "NSObject",
    "methodSignatureForSelector:" => "NSObject",
    "mutableCopy" => "NSObject",
    "mutableCopyWithZone:" => "NSObject",
    "performSelector:" => "NSObject",
    "release" => "NSObject",
    "replacementObjectForCoder:" => "NSObject",
    "respondsToSelector:" => "NSObject",
    "retain" => "NSObject",
    "retainCount" => "NSObject",
    "self" => "NSObject",
    "superclass" => "NSObject",
    "zone" => "NSObject",
);

my $fatalError = 0;

# Default License Templates
my $headerLicenseTemplate = << "EOF";
/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig\@gmail.com>
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
EOF

my $implementationLicenseTemplate = << "EOF";
/*
 * This file is part of the WebKit open source project.
 * This file has been generated by generate-bindings.pl. DO NOT MODIFY!
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
EOF

# Default constructor
sub new
{
    my $object = shift;
    my $reference = { };

    $codeGenerator = shift;
    shift; # $useLayerOnTop
    shift; # $preprocessor
    $writeDependencies = shift;

    bless($reference, $object);
    return $reference;
}

sub ReadPublicInterfaces
{
    my $class = shift;
    my $superClass = shift;
    my $defines = shift;
    my $isProtocol = shift;

    my $found = 0;
    my $actualSuperClass;
    %publicInterfaces = ();

    my $fileName = "WebCore/bindings/objc/PublicDOMInterfaces.h";
    my $gccLocation = "";
    if ($ENV{CC}) {
        $gccLocation = $ENV{CC};
    } elsif (($Config::Config{'osname'}) =~ /solaris/i) {
        $gccLocation = "/usr/sfw/bin/gcc";
    } elsif (-x "/usr/bin/clang") {
        $gccLocation = "/usr/bin/clang";
    } else {
        $gccLocation = "/usr/bin/gcc";
    }
    open FILE, "-|", $gccLocation, "-E", "-P", "-x", "objective-c",
        (map { "-D$_" } split(/ +/, $defines)), "-DOBJC_CODE_GENERATION", $fileName or die "Could not open $fileName";
    my @documentContent = <FILE>;
    close FILE;

    foreach $line (@documentContent) {
        if (!$isProtocol && $line =~ /^\s*\@interface\s*$class\s*:\s*(\w+)\s*([A-Z0-9_]*)/) {
            if ($superClass ne $1) {
                warn "Public API change. Superclass for \"$class\" differs ($1 != $superClass)";
                $fatalError = 1;
            }

            $interfaceAvailabilityVersion = $2 if defined $2;
            $found = 1;
            next;
        } elsif ($isProtocol && $line =~ /^\s*\@protocol $class\s*<[^>]+>\s*([A-Z0-9_]*)/) {
            $interfaceAvailabilityVersion = $1 if defined $1;
            $found = 1;
            next;
        }

        last if $found and $line =~ /^\s?\@end\s?$/;

        if ($found) {
            # trim whitspace
            $line =~ s/^\s+//;
            $line =~ s/\s+$//;

            my $availabilityMacro = "";
            $line =~ s/\s([A-Z0-9_]+)\s*;$/;/;
            $availabilityMacro = $1 if defined $1;

            $publicInterfaces{$line} = $availabilityMacro if length $line;
        }
    }

    # If this class was not found in PublicDOMInterfaces.h then it should be considered as an entirely new public class.
    $newPublicClass = !$found;
    $interfaceAvailabilityVersion = "WEBKIT_VERSION_LATEST" if $newPublicClass;
}

sub AddMethodsConstantsAndAttributesFromParentInterfaces
{
    # Add to $interface all of its inherited interface members, except for those
    # inherited through $interface's first listed parent.  If an array reference
    # is passed in as $parents, the names of all ancestor interfaces visited
    # will be appended to the array.  If $collectDirectParents is true, then
    # even the names of $interface's first listed parent and its ancestors will
    # be appended to $parents.

    my $interface = shift;
    my $parents = shift;
    my $collectDirectParents = shift;

    my $first = 1;

    $codeGenerator->ForAllParents($interface, sub {
        my $currentInterface = shift;

        if ($first) {
            # Ignore first parent class, already handled by the generation itself.
            $first = 0;

            if ($collectDirectParents) {
                # Just collect the names of the direct ancestor interfaces,
                # if necessary.
                push(@$parents, $currentInterface->name);
                $codeGenerator->ForAllParents($currentInterface, sub {
                    my $currentInterface = shift;
                    push(@$parents, $currentInterface->name);
                }, undef);
            }

            # Prune the recursion here.
            return 'prune';
        }

        # Collect the name of this additional parent.
        push(@$parents, $currentInterface->name) if $parents;

        print "  |  |>  -> Inheriting "
            . @{$currentInterface->constants} . " constants, "
            . @{$currentInterface->functions} . " functions, "
            . @{$currentInterface->attributes} . " attributes...\n  |  |>\n" if $verbose;

        # Add this parent's members to $interface.
        push(@{$interface->constants}, @{$currentInterface->constants});
        push(@{$interface->functions}, @{$currentInterface->functions});
        push(@{$interface->attributes}, @{$currentInterface->attributes});
    });
}

sub GenerateInterface
{
    my $object = shift;
    my $interface = shift;
    my $defines = shift;

    $fatalError = 0;

    my $name = $interface->name;
    my $className = GetClassName($name);
    my $parentClassName = "DOM" . GetParentImplClassName($interface);
    $isProtocol = $interface->extendedAttributes->{ObjCProtocol};
    $noImpl = $interface->extendedAttributes->{ObjCCustomImplementation} || $isProtocol;

    ReadPublicInterfaces($className, $parentClassName, $defines, $isProtocol);

    # Start actual generation..
    $object->GenerateHeader($interface);
    $object->GenerateImplementation($interface) unless $noImpl;

    # Check for missing public API
    if (keys %publicInterfaces > 0) {
        my $missing = join("\n", keys %publicInterfaces);
        warn "Public API change. There are missing public properties and/or methods from the \"$className\" class.\n$missing\n";
        $fatalError = 1;
    }

    die if $fatalError;
}

sub GetClassName
{
    my $name = shift;

    # special cases
    return "NSString" if $codeGenerator->IsStringType($name) or $name eq "SerializedScriptValue";
    return "NS$name" if IsNativeObjCType($name);
    return "BOOL" if $name eq "boolean";
    return "unsigned char" if $name eq "octet";
    return "char" if $name eq "byte";
    return "unsigned" if $name eq "unsigned long";
    return "int" if $name eq "long";
    return "NSTimeInterval" if $name eq "Date";
    return "DOMAbstractView" if $name eq "DOMWindow";
    return $name if $codeGenerator->IsPrimitiveType($name) or $name eq "DOMImplementation" or $name eq "DOMTimeStamp";

    # Default, assume Objective-C type has the same type name as
    # idl type prefixed with "DOM".
    return "DOM$name";
}

sub GetClassHeaderName
{
    my $name = shift;

    return "DOMDOMImplementation" if $name eq "DOMImplementation";
    return $name;
}

sub GetImplClassName
{
    my $name = shift;

    return "DOMImplementationFront" if $name eq "DOMImplementation";
    return "DOMWindow" if $name eq "AbstractView";
    return $name;
}

sub GetParentImplClassName
{
    my $interface = shift;

    return "Object" if @{$interface->parents} eq 0;

    my $parent = $interface->parents(0);

    # special cases
    return "Object" if $parent eq "HTMLCollection";

    return $parent;
}

sub GetParentAndProtocols
{
    my $interface = shift;
    my $numParents = @{$interface->parents};

    my $parent = "";
    my @protocols = ();
    if ($numParents eq 0) {
        if ($isProtocol) {
            push(@protocols, "NSObject");
            push(@protocols, "NSCopying") if $interface->name eq "EventTarget";
        } else {
            $parent = "DOMObject";
        }
    } elsif ($numParents eq 1) {
        my $parentName = $interface->parents(0);
        if ($isProtocol) {
            die "Parents of protocols must also be protocols." unless IsProtocolType($parentName);
            push(@protocols, "DOM" . $parentName);
        } else {
            if (IsProtocolType($parentName)) {
                push(@protocols, "DOM" . $parentName);
            } elsif ($parentName eq "HTMLCollection") {
                $parent = "DOMObject";
            } else {
                $parent = "DOM" . $parentName;
            }
        }
    } else {
        my @parents = @{$interface->parents};
        my $firstParent = shift(@parents);
        if (IsProtocolType($firstParent)) {
            push(@protocols, "DOM" . $firstParent);
            if (!$isProtocol) {
                $parent = "DOMObject";
            }
        } else {
            $parent = "DOM" . $firstParent;
        }

        foreach my $parentName (@parents) {
            die "Everything past the first class should be a protocol!" unless IsProtocolType($parentName);

            push(@protocols, "DOM" . $parentName);
        }
    }

    return ($parent, @protocols);
}

sub GetBaseClass
{
    $parent = shift;

    return $parent if $parent eq "Object" or IsBaseType($parent);
    return "Event" if $parent eq "UIEvent" or $parent eq "MouseEvent";
    return "CSSValue" if $parent eq "SVGColor" or $parent eq "CSSValueList";
    return "Node";
}

sub IsBaseType
{
    my $type = shift;

    return 1 if $baseTypeHash{$type};
    return 0;
}

sub IsProtocolType
{
    my $type = shift;

    return 1 if $protocolTypeHash{$type};
    return 0;
}

sub IsNativeObjCType
{
    my $type = shift;

    return 1 if $nativeObjCTypeHash{$type};
    return 0;
}

sub SkipFunction
{
    my $function = shift;

    return 1 if $codeGenerator->GetSequenceType($function->signature->type);
    return 1 if $codeGenerator->GetArrayType($function->signature->type);

    foreach my $param (@{$function->parameters}) {
        return 1 if $codeGenerator->GetSequenceType($param->type);
        return 1 if $codeGenerator->GetArrayType($param->type);
        return 1 if $param->extendedAttributes->{"Clamp"};
    }

    return 0;
}

sub SkipAttribute
{
    my $attribute = shift;
    my $type = $attribute->signature->type;

    $codeGenerator->AssertNotSequenceType($type);
    return 1 if $codeGenerator->GetArrayType($type);
    return 1 if $codeGenerator->IsTypedArrayType($type);
    return 1 if $codeGenerator->IsEnumType($type);
    return 1 if $attribute->isStatic;

    # This is for DynamicsCompressorNode.idl
    if ($attribute->signature->name eq "release") {
        return 1;
    }

    return 0;
}


sub GetObjCType
{
    my $type = shift;
    my $name = GetClassName($type);

    return "id <$name>" if IsProtocolType($type);
    return $name if $codeGenerator->IsPrimitiveType($type) or $type eq "DOMTimeStamp";
    return "unsigned short" if $type eq "CompareHow";
    return "$name *";
}

sub GetPropertyAttributes
{
    my $type = shift;
    my $readOnly = shift;

    my @attributes = ();

    push(@attributes, "readonly") if $readOnly;

    # FIXME: <rdar://problem/5049934> Consider using 'nonatomic' on the DOM @property declarations.
    if ($codeGenerator->IsStringType($type) || IsNativeObjCType($type)) {
        push(@attributes, "copy");
    } elsif ($codeGenerator->IsSVGAnimatedType($type)) {
        push(@attributes, "retain");
    } elsif (!$codeGenerator->IsStringType($type) && !$codeGenerator->IsPrimitiveType($type) && $type ne "DOMTimeStamp" && $type ne "CompareHow") {
        push(@attributes, "retain");
    }

    return "" unless @attributes > 0;
    return "(" . join(", ", @attributes) . ")";
}

sub ConversionNeeded
{
    my $type = shift;

    return !$codeGenerator->IsNonPointerType($type) && !$codeGenerator->IsStringType($type) && !IsNativeObjCType($type);
}

sub GetObjCTypeGetter
{
    my $argName = shift;
    my $type = shift;

    return $argName if $codeGenerator->IsPrimitiveType($type) or $codeGenerator->IsStringType($type) or IsNativeObjCType($type);
    return $argName . "Node" if $type eq "EventTarget";
    return "static_cast<WebCore::Range::CompareHow>($argName)" if $type eq "CompareHow";
    return "WTF::getPtr(nativeEventListener)" if $type eq "EventListener";
    return "WTF::getPtr(nativeNodeFilter)" if $type eq "NodeFilter";
    return "WTF::getPtr(nativeResolver)" if $type eq "XPathNSResolver";
    
    if ($type eq "SerializedScriptValue") {
        $implIncludes{"SerializedScriptValue.h"} = 1;
        return "WebCore::SerializedScriptValue::create(WTF::String($argName))";
    }
    return "core($argName)";
}

sub AddForwardDeclarationsForType
{
    my $type = shift;
    my $public = shift;

    return if $codeGenerator->IsNonPointerType($type);
    return if $codeGenerator->GetSequenceType($type);
    return if $codeGenerator->GetArrayType($type);

    my $class = GetClassName($type);

    if (IsProtocolType($type)) {
        $headerForwardDeclarationsForProtocols{$class} = 1 if $public;
        $privateHeaderForwardDeclarationsForProtocols{$class} = 1 if !$public and !$headerForwardDeclarationsForProtocols{$class};
        return;
    }

    $headerForwardDeclarations{$class} = 1 if $public;

    # Private headers include the public header, so only add a forward declaration to the private header
    # if the public header does not already have the same forward declaration.
    $privateHeaderForwardDeclarations{$class} = 1 if !$public and !$headerForwardDeclarations{$class};
}

sub AddIncludesForType
{
    my $type = shift;

    return if $codeGenerator->IsNonPointerType($type);
    return if $codeGenerator->GetSequenceType($type);
    return if $codeGenerator->GetArrayType($type);

    if (IsNativeObjCType($type)) {
        if ($type eq "Color") {
            $implIncludes{"ColorMac.h"} = 1;
        }
        return;
    }

    if ($codeGenerator->IsStringType($type)) {
        $implIncludes{"KURL.h"} = 1;
        return;
    }

    if ($type eq "DOMWindow") {
        $implIncludes{"DOMAbstractViewInternal.h"} = 1;
        $implIncludes{"DOMWindow.h"} = 1;
        return;
    }

    if ($type eq "DOMImplementation") {
        $implIncludes{"DOMDOMImplementationInternal.h"} = 1;
        $implIncludes{"DOMImplementationFront.h"} = 1;
        return;
    }

    if ($type eq "EventTarget") {
        $implIncludes{"Node.h"} = 1;
        $implIncludes{"DOMEventTarget.h"} = 1;
        return;
    }

    if ($codeGenerator->IsSVGAnimatedType($type)) {
        $implIncludes{"${type}.h"} = 1;
        $implIncludes{"DOM${type}Internal.h"} = 1;
        return;
    }

    if ($type =~ /(\w+)(Abs|Rel)$/) {
        $implIncludes{"$1.h"} = 1;
        $implIncludes{"DOM${type}Internal.h"} = 1;
        return;
    }

    if ($type eq "NodeFilter") {
        $implIncludes{"NodeFilter.h"} = 1;
        $implIncludes{"ObjCNodeFilterCondition.h"} = 1;
        return;
    }

    if ($type eq "EventListener") {
        $implIncludes{"EventListener.h"} = 1;
        $implIncludes{"ObjCEventListener.h"} = 1;
        return;
    }

    if ($type eq "XPathNSResolver") {
        $implIncludes{"DOMCustomXPathNSResolver.h"} = 1;
        $implIncludes{"XPathNSResolver.h"} = 1;
        return;
    }

    if ($type eq "SerializedScriptValue") {
        $implIncludes{"SerializedScriptValue.h"} = 1;
        return;
    }

    # FIXME: won't compile without these
    $implIncludes{"CSSImportRule.h"} = 1 if $type eq "CSSRule";
    $implIncludes{"StylePropertySet.h"} = 1 if $type eq "CSSStyleDeclaration";
    $implIncludes{"NameNodeList.h"} = 1 if $type eq "NodeList";

    # Default, include the same named file (the implementation) and the same name prefixed with "DOM". 
    $implIncludes{"$type.h"} = 1 if not $codeGenerator->SkipIncludeHeader($type);
    $implIncludes{"DOM${type}Internal.h"} = 1;
}

sub GetSVGTypeWithNamespace
{
    my $type = shift;
    my $typeWithNamespace = "WebCore::" . $codeGenerator->GetSVGTypeNeedingTearOff($type);

    # Special case for DOMSVGNumber
    $typeWithNamespace =~ s/</\<WebCore::/ unless $type eq "SVGNumber";
    return $typeWithNamespace;
}

sub GetSVGPropertyTypes
{
    my $implType = shift;

    my $svgPropertyType;
    my $svgListPropertyType;
    my $svgNativeType;

    return ($svgPropertyType, $svgListPropertyType, $svgNativeType) if not $implType =~ /SVG/;

    $svgNativeType = $codeGenerator->GetSVGTypeNeedingTearOff($implType);
    return ($svgPropertyType, $svgListPropertyType, $svgNativeType) if not $svgNativeType;

    # Append space to avoid compilation errors when using  PassRefPtr<$svgNativeType>
    $svgNativeType = "WebCore::$svgNativeType ";
    $svgNativeType =~ s/</\<WebCore::/ if not $svgNativeType =~ /float/;

    my $svgWrappedNativeType = $codeGenerator->GetSVGWrappedTypeNeedingTearOff($implType);
    if ($svgNativeType =~ /SVGPropertyTearOff/) {
        if ($svgWrappedNativeType eq "float") {
            # Special case for DOMSVGNumber
            $svgPropertyType = $svgWrappedNativeType;
        } else {
            $svgPropertyType = "WebCore::$svgWrappedNativeType";
            $svgPropertyType =~ s/</\<WebCore::/;
        }
    } elsif ($svgNativeType =~ /SVGListPropertyTearOff/ or $svgNativeType =~ /SVGStaticListPropertyTearOff/) {
        $svgListPropertyType = "WebCore::$svgWrappedNativeType";
        $svgListPropertyType =~ s/</\<WebCore::/;
    } elsif ($svgNativeType =~ /SVGTransformListPropertyTearOff/) {
        $svgListPropertyType = "WebCore::$svgWrappedNativeType";
        $svgListPropertyType =~ s/</\<WebCore::/;
    } elsif ($svgNativeType =~ /SVGPathSegListPropertyTearOff/) {
        $svgListPropertyType = "WebCore::$svgWrappedNativeType";
        $svgListPropertyType =~ s/</\<WebCore::/;
    }

    return ($svgPropertyType, $svgListPropertyType, $svgNativeType);
}

sub GenerateHeader
{
    my $object = shift;
    my $interface = shift;

    my $interfaceName = $interface->name;
    my $className = GetClassName($interfaceName);

    my $parentName = "";
    my @protocolsToImplement = ();
    ($parentName, @protocolsToImplement) = GetParentAndProtocols($interface);

    my $numConstants = @{$interface->constants};
    my $numAttributes = @{$interface->attributes};
    my $numFunctions = @{$interface->functions};

    # - Add default header template
    @headerContentHeader = split("\r", $headerLicenseTemplate);
    push(@headerContentHeader, "\n");

    # - INCLUDES -
    my $includedWebKitAvailabilityHeader = 0;
    unless ($isProtocol) {
        my $parentHeaderName = GetClassHeaderName($parentName);
        push(@headerContentHeader, "#import <WebCore/$parentHeaderName.h>\n");
        $includedWebKitAvailabilityHeader = 1;
    }

    foreach my $parentProtocol (@protocolsToImplement) {
        next if $parentProtocol =~ /^NS/; 
        $parentProtocol = GetClassHeaderName($parentProtocol);
        push(@headerContentHeader, "#import <WebCore/$parentProtocol.h>\n");
        $includedWebKitAvailabilityHeader = 1;
    }

    # Special case needed for legacy support of DOMRange
    if ($interfaceName eq "Range") {
        push(@headerContentHeader, "#import <WebCore/DOMCore.h>\n");
        push(@headerContentHeader, "#import <WebCore/DOMDocument.h>\n");
        push(@headerContentHeader, "#import <WebCore/DOMRangeException.h>\n");
        $includedWebKitAvailabilityHeader = 1;
    }

    push(@headerContentHeader, "#import <JavaScriptCore/WebKitAvailability.h>\n") unless $includedWebKitAvailabilityHeader;

    my $interfaceAvailabilityVersionCheck = "#if WEBKIT_VERSION_MAX_ALLOWED >= $interfaceAvailabilityVersion\n\n";

    push(@headerContentHeader, "\n");
    push(@headerContentHeader, $interfaceAvailabilityVersionCheck) if length $interfaceAvailabilityVersion;

    # - Add constants.
    if ($numConstants > 0) {
        my @headerConstants = ();
        my @constants = @{$interface->constants};
        my $combinedConstants = "";

        # FIXME: we need a way to include multiple enums.
        foreach my $constant (@constants) {
            my $constantName = $constant->name;
            my $constantValue = $constant->value;
            my $conditional = $constant->extendedAttributes->{"Conditional"};
            my $notLast = $constant ne $constants[-1];

            if ($conditional) {
                my $conditionalString = $codeGenerator->GenerateConditionalStringFromAttributeValue($conditional);
                $combinedConstants .= "#if ${conditionalString}\n";
            }
            $combinedConstants .= "    DOM_$constantName = $constantValue";
            $combinedConstants .= "," if $notLast;
            if ($conditional) {
                $combinedConstants .= "\n#endif\n";
            } elsif ($notLast) {
                $combinedConstants .= "\n";
            }
        }

        # FIXME: the formatting of the enums should line up the equal signs.
        # FIXME: enums are unconditionally placed in the public header.
        push(@headerContent, "enum {\n");
        push(@headerContent, $combinedConstants);
        push(@headerContent, "\n};\n\n");        
    }

    # - Begin @interface or @protocol
    my $interfaceDeclaration = ($isProtocol ? "\@protocol $className" : "\@interface $className : $parentName");
    $interfaceDeclaration .= " <" . join(", ", @protocolsToImplement) . ">" if @protocolsToImplement > 0;
    $interfaceDeclaration .= "\n";

    push(@headerContent, $interfaceDeclaration);

    my @headerAttributes = ();
    my @privateHeaderAttributes = ();

    # - Add attribute getters/setters.
    if ($numAttributes > 0) {
        foreach my $attribute (@{$interface->attributes}) {
            next if SkipAttribute($attribute);
            my $attributeName = $attribute->signature->name;

            if ($attributeName eq "id" or $attributeName eq "hash" or $attributeName eq "description") {
                # Special case some attributes (like id and hash) to have a "Name" suffix to avoid ObjC naming conflicts.
                $attributeName .= "Name";
            } elsif ($attributeName eq "frame") {
                # Special case attribute frame to be frameBorders.
                $attributeName .= "Borders";
            }

            my $attributeType = GetObjCType($attribute->signature->type);
            my $property = "\@property" . GetPropertyAttributes($attribute->signature->type, $attribute->isReadOnly);
            $property .= " " . $attributeType . ($attributeType =~ /\*$/ ? "" : " ") . $attributeName;

            my $publicInterfaceKey = $property . ";";

            my $availabilityMacro = "";
            if (defined $publicInterfaces{$publicInterfaceKey} and length $publicInterfaces{$publicInterfaceKey}) {
                $availabilityMacro = $publicInterfaces{$publicInterfaceKey};
            }

            my $declarationSuffix = ";\n";
            $declarationSuffix = " $availabilityMacro;\n" if length $availabilityMacro;

            my $public = (defined $publicInterfaces{$publicInterfaceKey} or $newPublicClass);
            delete $publicInterfaces{$publicInterfaceKey};

            AddForwardDeclarationsForType($attribute->signature->type, $public);

            my $setterName = "set" . ucfirst($attributeName) . ":";

            my $conflict = $conflictMethod{$attributeName};
            if ($conflict) {
                warn "$className conflicts with $conflict method $attributeName\n";
                $fatalError = 1;
            }

            $conflict = $conflictMethod{$setterName};
            if ($conflict) {
                warn "$className conflicts with $conflict method $setterName\n";
                $fatalError = 1;
            }

            $property .= $declarationSuffix;
            push(@headerAttributes, $property) if $public;
            push(@privateHeaderAttributes, $property) unless $public;
        }

        push(@headerContent, @headerAttributes) if @headerAttributes > 0;
    }

    my @headerFunctions = ();
    my @privateHeaderFunctions = ();
    my @deprecatedHeaderFunctions = ();

    # - Add functions.
    if ($numFunctions > 0) {
        foreach my $function (@{$interface->functions}) {
            next if SkipFunction($function);
            next if ($function->signature->name eq "set" and $interface->extendedAttributes->{"TypedArray"});
            my $functionName = $function->signature->name;

            my $returnType = GetObjCType($function->signature->type);
            my $needsDeprecatedVersion = (@{$function->parameters} > 1 and $function->signature->extendedAttributes->{"ObjCLegacyUnnamedParameters"});
            my $numberOfParameters = @{$function->parameters};
            my %typesToForwardDeclare = ($function->signature->type => 1);

            my $parameterIndex = 0;
            my $functionSig = "- ($returnType)$functionName";
            my $methodName = $functionName;
            foreach my $param (@{$function->parameters}) {
                my $paramName = $param->name;
                my $paramType = GetObjCType($param->type);

                $typesToForwardDeclare{$param->type} = 1;

                if ($parameterIndex >= 1) {
                    $functionSig .= " $paramName";
                    $methodName .= $paramName;
                }

                $functionSig .= ":($paramType)$paramName";
                $methodName .= ":";

                $parameterIndex++;
            }

            my $publicInterfaceKey = $functionSig . ";";

            my $conflict = $conflictMethod{$methodName};
            if ($conflict) {
                warn "$className conflicts with $conflict method $methodName\n";
                $fatalError = 1;
            }

            if ($isProtocol && !$newPublicClass && !defined $publicInterfaces{$publicInterfaceKey}) {
                warn "Protocol method $publicInterfaceKey is not in PublicDOMInterfaces.h. Protocols require all methods to be public";
                $fatalError = 1;
            }

            my $availabilityMacro = "";
            if (defined $publicInterfaces{$publicInterfaceKey} and length $publicInterfaces{$publicInterfaceKey}) {
                $availabilityMacro = $publicInterfaces{$publicInterfaceKey};
            }

            my $functionDeclaration = $functionSig;
            $functionDeclaration .= " " . $availabilityMacro if length $availabilityMacro;
            $functionDeclaration .= ";\n";

            my $public = (defined $publicInterfaces{$publicInterfaceKey} or $newPublicClass);
            delete $publicInterfaces{$publicInterfaceKey};

            foreach my $type (keys %typesToForwardDeclare) {
                # add any forward declarations to the public header if a deprecated version will be generated
                AddForwardDeclarationsForType($type, 1) if $needsDeprecatedVersion;
                AddForwardDeclarationsForType($type, $public) unless $public and $needsDeprecatedVersion;
            }

            my $functionConditionalString = $codeGenerator->GenerateConditionalString($function->signature);
            if ($functionConditionalString) {
                push(@headerFunctions, "#if ${functionConditionalString}\n") if $public;
                push(@privateHeaderFunctions, "#if ${functionConditionalString}\n") unless $public;
                push(@deprecatedHeaderFunctions, "#if ${functionConditionalString}\n") if $needsDeprecatedVersion;
            }

            push(@headerFunctions, $functionDeclaration) if $public;
            push(@privateHeaderFunctions, $functionDeclaration) unless $public;

            # generate the old style method names with un-named parameters, these methods are deprecated
            if ($needsDeprecatedVersion) {
                my $deprecatedFunctionSig = $functionSig;
                $deprecatedFunctionSig =~ s/\s\w+:/ :/g; # remove parameter names

                $publicInterfaceKey = $deprecatedFunctionSig . ";";

                my $availabilityMacro = "AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0";
                if (defined $publicInterfaces{$publicInterfaceKey} and length $publicInterfaces{$publicInterfaceKey}) {
                    $availabilityMacro = $publicInterfaces{$publicInterfaceKey};
                }

                $functionDeclaration = "$deprecatedFunctionSig $availabilityMacro;\n";

                push(@deprecatedHeaderFunctions, $functionDeclaration);

                unless (defined $publicInterfaces{$publicInterfaceKey}) {
                    warn "Deprecated method $publicInterfaceKey is not in PublicDOMInterfaces.h. All deprecated methods need to be public, or should have the ObjCLegacyUnnamedParameters IDL attribute removed";
                    $fatalError = 1;
                }

                delete $publicInterfaces{$publicInterfaceKey};
            }

            if ($functionConditionalString) {
                push(@headerFunctions, "#endif\n") if $public;
                push(@privateHeaderFunctions, "#endif\n") unless $public;
                push(@deprecatedHeaderFunctions, "#endif\n") if $needsDeprecatedVersion;
            }
        }

        if (@headerFunctions > 0) {
            push(@headerContent, "\n") if @headerAttributes > 0;
            push(@headerContent, @headerFunctions);
        }
    }

    if (@deprecatedHeaderFunctions > 0 && $isProtocol) {
        push(@headerContent, @deprecatedHeaderFunctions);
    }

    # - End @interface or @protocol
    push(@headerContent, "\@end\n");

    if (@deprecatedHeaderFunctions > 0 && !$isProtocol) {
        # - Deprecated category @interface 
        push(@headerContent, "\n\@interface $className (" . $className . "Deprecated)\n");
        push(@headerContent, @deprecatedHeaderFunctions);
        push(@headerContent, "\@end\n");
    }

    push(@headerContent, "\n#endif\n") if length $interfaceAvailabilityVersion;

    my %alwaysGenerateForNoSVGBuild = map { $_ => 1 } qw(DOMHTMLEmbedElement DOMHTMLObjectElement);

    if (@privateHeaderAttributes > 0 or @privateHeaderFunctions > 0 or exists $alwaysGenerateForNoSVGBuild{$className}) {
        # - Private category @interface
        @privateHeaderContentHeader = split("\r", $headerLicenseTemplate);
        push(@privateHeaderContentHeader, "\n");

        my $classHeaderName = GetClassHeaderName($className);
        push(@privateHeaderContentHeader, "#import <WebCore/$classHeaderName.h>\n\n");
        push(@privateHeaderContentHeader, $interfaceAvailabilityVersionCheck) if length $interfaceAvailabilityVersion;

        @privateHeaderContent = ();
        push(@privateHeaderContent, "\@interface $className (" . $className . "Private)\n");
        push(@privateHeaderContent, @privateHeaderAttributes) if @privateHeaderAttributes > 0;
        push(@privateHeaderContent, "\n") if @privateHeaderAttributes > 0 and @privateHeaderFunctions > 0;
        push(@privateHeaderContent, @privateHeaderFunctions) if @privateHeaderFunctions > 0;
        push(@privateHeaderContent, "\@end\n");

        push(@privateHeaderContent, "\n#endif\n") if length $interfaceAvailabilityVersion;
    }

    unless ($isProtocol) {
        # Generate internal interfaces
        my $implClassName = GetImplClassName($interfaceName);
        my $implClassNameWithNamespace = "WebCore::" . $implClassName;

        my $implType = $implClassNameWithNamespace;
        my ($svgPropertyType, $svgListPropertyType, $svgNativeType) = GetSVGPropertyTypes($implClassName);
        $implType = $svgNativeType if $svgNativeType;

        # Generate interface definitions. 
        @internalHeaderContent = split("\r", $implementationLicenseTemplate);

        push(@internalHeaderContent, "\n#import <WebCore/$className.h>\n\n");
        push(@internalHeaderContent, "#import <WebCore/SVGAnimatedPropertyTearOff.h>\n\n") if $svgPropertyType;
        if ($svgListPropertyType) {
            push(@internalHeaderContent, "#import <WebCore/SVGAnimatedListPropertyTearOff.h>\n\n");
            push(@internalHeaderContent, "#import <WebCore/SVGTransformListPropertyTearOff.h>\n\n") if $svgListPropertyType =~ /SVGTransformList/;
            push(@internalHeaderContent, "#import <WebCore/SVGPathSegListPropertyTearOff.h>\n\n") if $svgListPropertyType =~ /SVGPathSegList/;
        }
        push(@internalHeaderContent, $interfaceAvailabilityVersionCheck) if length $interfaceAvailabilityVersion;

        if ($interfaceName eq "Node") {
            push(@internalHeaderContent, "\@protocol DOMEventTarget;\n\n");
        }

        my $startedNamespace = 0;

        if ($codeGenerator->IsSVGAnimatedType($interfaceName)) {
            push(@internalHeaderContent, "#import <WebCore/$implClassName.h>\n\n");
        } else {
            push(@internalHeaderContent, "namespace WebCore {\n");
            $startedNamespace = 1;
            if ($interfaceName eq "Node") {
                push(@internalHeaderContent, "class EventTarget;\n    class Node;\n");
            } else {
                push(@internalHeaderContent, "class $implClassName;\n");
            }
            push(@internalHeaderContent, "}\n\n");
        }

        push(@internalHeaderContent, "$implType* core($className *);\n");
        push(@internalHeaderContent, "$className *kit($implType*);\n");

        if ($interface->extendedAttributes->{"ObjCPolymorphic"}) {
            push(@internalHeaderContent, "Class kitClass($implType*);\n");
        }

        if ($interfaceName eq "Node") {
            push(@internalHeaderContent, "id <DOMEventTarget> kit(WebCore::EventTarget*);\n");
        }

        push(@internalHeaderContent, "\n#endif\n") if length $interfaceAvailabilityVersion;
    }
}

sub GenerateImplementation
{
    my $object = shift;
    my $interface = shift;

    my @ancestorInterfaceNames = ();

    if (@{$interface->parents} > 1) {
        AddMethodsConstantsAndAttributesFromParentInterfaces($interface, \@ancestorInterfaceNames);
    }

    my $interfaceName = $interface->name;
    my $className = GetClassName($interfaceName);
    my $implClassName = GetImplClassName($interfaceName);
    my $parentImplClassName = GetParentImplClassName($interface);
    my $implClassNameWithNamespace = "WebCore::" . $implClassName;
    my $baseClass = GetBaseClass($parentImplClassName);
    my $classHeaderName = GetClassHeaderName($className);

    my $numAttributes = @{$interface->attributes};
    my $numFunctions = @{$interface->functions};
    my $implType = $implClassNameWithNamespace;

    my ($svgPropertyType, $svgListPropertyType, $svgNativeType) = GetSVGPropertyTypes($implClassName);
    $implType = $svgNativeType if $svgNativeType;

    # - Add default header template.
    @implContentHeader = split("\r", $implementationLicenseTemplate);

    # - INCLUDES -
    push(@implContentHeader, "\n#import \"config.h\"\n");

    my $conditionalString = $codeGenerator->GenerateConditionalString($interface);
    push(@implContentHeader, "\n#if ${conditionalString}\n\n") if $conditionalString;

    push(@implContentHeader, "#import \"DOMInternal.h\"\n\n");
    push(@implContentHeader, "#import \"$classHeaderName.h\"\n\n");

    $implIncludes{"ExceptionHandlers.h"} = 1;
    $implIncludes{"ThreadCheck.h"} = 1;
    $implIncludes{"JSMainThreadExecState.h"} = 1;
    $implIncludes{"WebScriptObjectPrivate.h"} = 1;
    $implIncludes{$classHeaderName . "Internal.h"} = 1;

    # FIXME: These includes are only needed when the class is a subclass of one of these polymorphic classes.
    $implIncludes{"DOMBlobInternal.h"} = 1;
    $implIncludes{"DOMCSSRuleInternal.h"} = 1;
    $implIncludes{"DOMCSSValueInternal.h"} = 1;
    $implIncludes{"DOMEventInternal.h"} = 1;
    $implIncludes{"DOMNodeInternal.h"} = 1;
    $implIncludes{"DOMStyleSheetInternal.h"} = 1;

    $implIncludes{"DOMSVGPathSegInternal.h"} = 1 if $interfaceName =~ /^SVGPathSeg.+/;

    if ($interfaceName =~ /(\w+)(Abs|Rel)$/) {
        $implIncludes{"$1.h"} = 1;
    } else {
        if (!$codeGenerator->SkipIncludeHeader($implClassName)) {
            $implIncludes{"$implClassName.h"} = 1 ;
        } elsif ($codeGenerator->IsSVGTypeNeedingTearOff($implClassName)) {
            my $includeType = $codeGenerator->GetSVGWrappedTypeNeedingTearOff($implClassName);
            $implIncludes{"${includeType}.h"} = 1;
        }
    } 

    @implContent = ();

    push(@implContent, "#import <wtf/GetPtr.h>\n\n");

    # add implementation accessor
    if ($parentImplClassName eq "Object") {
        push(@implContent, "#define IMPL reinterpret_cast<$implType*>(_internal)\n\n");
    } else {
        my $baseClassWithNamespace = "WebCore::$baseClass";
        push(@implContent, "#define IMPL static_cast<$implClassNameWithNamespace*>(reinterpret_cast<$baseClassWithNamespace*>(_internal))\n\n");
    }

    # START implementation
    push(@implContent, "\@implementation $className\n\n");

    # Only generate 'dealloc' and 'finalize' methods for direct subclasses of DOMObject.
    if ($parentImplClassName eq "Object") {
        $implIncludes{"WebCoreObjCExtras.h"} = 1;
        push(@implContent, "- (void)dealloc\n");
        push(@implContent, "{\n");
        push(@implContent, "    if (WebCoreObjCScheduleDeallocateOnMainThread([$className class], self))\n");
        push(@implContent, "        return;\n");
        push(@implContent, "\n");
        if ($interfaceName eq "NodeIterator") {
            push(@implContent, "    if (_internal) {\n");
            push(@implContent, "        [self detach];\n");
            push(@implContent, "        IMPL->deref();\n");
            push(@implContent, "    };\n");
        } else {
            push(@implContent, "    if (_internal)\n");
            push(@implContent, "        IMPL->deref();\n");
        }
        push(@implContent, "    [super dealloc];\n");
        push(@implContent, "}\n\n");

        push(@implContent, "- (void)finalize\n");
        push(@implContent, "{\n");
        if ($interfaceName eq "NodeIterator") {
            push(@implContent, "    if (_internal) {\n");
            push(@implContent, "        [self detach];\n");
            push(@implContent, "        IMPL->deref();\n");
            push(@implContent, "    };\n");
        } else {
            push(@implContent, "    if (_internal)\n");
            push(@implContent, "        IMPL->deref();\n");
        }
        push(@implContent, "    [super finalize];\n");
        push(@implContent, "}\n\n");
        
    }

    %attributeNames = ();

    # - Attributes
    if ($numAttributes > 0) {
        foreach my $attribute (@{$interface->attributes}) {
            next if SkipAttribute($attribute);
            AddIncludesForType($attribute->signature->type);

            my $idlType = $attribute->signature->type;

            my $attributeName = $attribute->signature->name;
            my $attributeType = GetObjCType($attribute->signature->type);
            my $attributeClassName = GetClassName($attribute->signature->type);

            my $attributeInterfaceName = $attributeName;
            if ($attributeName eq "id" or $attributeName eq "hash" or $attributeName eq "description") {
                # Special case some attributes (like id and hash) to have a "Name" suffix to avoid ObjC naming conflicts.
                $attributeInterfaceName .= "Name";
            } elsif ($attributeName eq "frame") {
                # Special case attribute frame to be frameBorders.
                $attributeInterfaceName .= "Borders";
            }

            $attributeNames{$attributeInterfaceName} = 1;

            # - GETTER
            my $getterSig = "- ($attributeType)$attributeInterfaceName\n";

            my ($functionName, @arguments) = $codeGenerator->GetterExpression(\%implIncludes, $interfaceName, $attribute);
            my $getterExpressionPrefix = "$functionName(" . join(", ", @arguments);

            # FIXME: Special case attribute ownerDocument to call document. This makes it return the
            # document when called on the document itself. Legacy behavior, see <https://bugs.webkit.org/show_bug.cgi?id=10889>.
            $getterExpressionPrefix =~ s/\bownerDocument\b/document/;

            my $hasGetterException = $attribute->signature->extendedAttributes->{"GetterRaisesException"};
            my $getterContentHead;
            if ($attribute->signature->extendedAttributes->{"ImplementedBy"}) {
                my $implementedBy = $attribute->signature->extendedAttributes->{"ImplementedBy"};
                $implIncludes{"${implementedBy}.h"} = 1;
                $getterContentHead = "${implementedBy}::${getterExpressionPrefix}IMPL";
            } else {
                $getterContentHead = "IMPL->$getterExpressionPrefix";
            }

            my $getterContentTail = ")";

            if ($svgPropertyType) {
                $getterContentHead = "$getterExpressionPrefix";

                # TODO: Handle special case for DOMSVGLength. We do need Custom code support for this.
                if ($svgPropertyType eq "WebCore::SVGLength" and $attributeName eq "value") {
                    $getterContentHead = "value(WebCore::SVGLengthContext(IMPL->contextElement()), ";
                }
            }

            my $attributeTypeSansPtr = $attributeType;
            $attributeTypeSansPtr =~ s/ \*$//; # Remove trailing " *" from pointer types.

            # special case for EventTarget protocol
            $attributeTypeSansPtr = "DOMNode" if $idlType eq "EventTarget";

            # Special cases
            my @customGetterContent = (); 
            if ($attributeTypeSansPtr eq "DOMImplementation") {
                # FIXME: We have to special case DOMImplementation until DOMImplementationFront is removed
                $getterContentHead = "kit(implementationFront(IMPL";
                $getterContentTail .= ")";
            } elsif ($attributeName =~ /(\w+)DisplayString$/) {
                my $attributeToDisplay = $1;
                $getterContentHead = "WebCore::displayString(IMPL->$attributeToDisplay(), core(self)";
                $implIncludes{"HitTestResult.h"} = 1;
            } elsif ($attributeName =~ /^absolute(\w+)URL$/) {
                my $typeOfURL = $1;
                $getterContentHead = "[self _getURLAttribute:";
                if ($typeOfURL eq "Link") {
                    $getterContentTail = "\@\"href\"]";
                } elsif ($typeOfURL eq "Image") {
                    if ($interfaceName eq "HTMLObjectElement") {
                        $getterContentTail = "\@\"data\"]";
                    } else {
                        $getterContentTail = "\@\"src\"]";
                    }
                    unless ($interfaceName eq "HTMLImageElement") {
                        push(@customGetterContent, "    if (!IMPL->renderer() || !IMPL->renderer()->isImage())\n");
                        push(@customGetterContent, "        return nil;\n");
                        $implIncludes{"RenderObject.h"} = 1;
                    }
                }
                $implIncludes{"DOMPrivate.h"} = 1;
            } elsif ($attribute->signature->extendedAttributes->{"ObjCImplementedAsUnsignedLong"}) {
                $getterContentHead = "WTF::String::number(" . $getterContentHead;
                $getterContentTail .= ")";
            } elsif ($idlType eq "Date") {
                $getterContentHead = "kit($getterContentHead";
                $getterContentTail .= ")";
            } elsif ($svgPropertyType) {
                # Special case for DOMSVGNumber
                if ($svgPropertyType eq "float") {
                    # Intentional leave out closing brace, it's already contained in getterContentTail
                    $getterContentHead = "IMPL->propertyReference(";
                } else {    
                    $getterContentHead = "IMPL->propertyReference().$getterContentHead";
                }

                if ($codeGenerator->IsSVGTypeWithWritablePropertiesNeedingTearOff($idlType) and not defined $attribute->signature->extendedAttributes->{"Immutable"}) {
                    my $getter = $getterContentHead;
                    $getter =~ s/\(\)//;
                    
                    my $tearOffType = GetSVGTypeWithNamespace($idlType);
                    my $selfIsTearOffType = $codeGenerator->IsSVGTypeNeedingTearOff($implClassName);
                    if ($selfIsTearOffType) {
                        $implIncludes{"SVGStaticPropertyWithParentTearOff.h"} = 1;
                        $tearOffType =~ s/SVGPropertyTearOff</SVGStaticPropertyWithParentTearOff<$implClassNameWithNamespace, /;

                        my $getter = $getterExpressionPrefix;
                        $getter =~ s/IMPL->//;
                        $getter =~ s/\(//;
                        my $updateMethod = "&${implClassNameWithNamespace}::update" . $codeGenerator->WK_ucfirst($getter);

                        $getterContentHead = "${tearOffType}::create(IMPL, $getterContentHead$getterContentTail, $updateMethod)";

                        $getterContentHead = "kit(WTF::getPtr($getterContentHead";
                        $getterContentTail = "))";
                    }
                }
            } elsif (($codeGenerator->IsSVGAnimatedType($implClassName) or $implClassName eq "SVGViewSpec") and $codeGenerator->IsSVGTypeNeedingTearOff($idlType)) {
                my $idlTypeWithNamespace = GetSVGTypeWithNamespace($idlType);
                $getterContentHead = "kit(static_cast<$idlTypeWithNamespace*>($getterContentHead)";
                $getterContentTail .= ")";
            } elsif (IsProtocolType($idlType) and $idlType ne "EventTarget") {
                $getterContentHead = "kit($getterContentHead";
                $getterContentTail .= ")";
            } elsif ($idlType eq "Color") {
                $getterContentHead = "WebCore::nsColor($getterContentHead";
                $getterContentTail .= ")";
            } elsif ($attribute->signature->type eq "SerializedScriptValue") {
                $getterContentHead = "$getterContentHead";
                $getterContentTail .= "->toString()";                
            } elsif (ConversionNeeded($attribute->signature->type)) {
                my $type = $attribute->signature->type;
                if ($codeGenerator->IsSVGTypeNeedingTearOff($type) and not $implClassName =~ /List$/) {
                    my $idlTypeWithNamespace = GetSVGTypeWithNamespace($type);
                    $implIncludes{"$type.h"} = 1 if not $codeGenerator->SkipIncludeHeader($type);
                    if ($codeGenerator->IsSVGTypeWithWritablePropertiesNeedingTearOff($type) and not defined $attribute->signature->extendedAttributes->{"Immutable"}) {
                        $idlTypeWithNamespace =~ s/SVGPropertyTearOff</SVGStaticPropertyTearOff<$implClassNameWithNamespace, /;
                        $implIncludes{"SVGStaticPropertyTearOff.h"} = 1;

                        my $getter = $getterContentHead;
                        $getter =~ s/IMPL->//;
                        $getter =~ s/\(//;
                        my $updateMethod = "&${implClassNameWithNamespace}::update" . $codeGenerator->WK_ucfirst($getter);
                        $getterContentHead = "kit(WTF::getPtr(${idlTypeWithNamespace}::create(IMPL, $getterContentHead$getterContentTail, $updateMethod";
                        $getterContentTail .= "))";
                    } elsif ($idlTypeWithNamespace =~ /SVG(Point|PathSeg)List/) {
                        $getterContentHead = "kit(WTF::getPtr($getterContentHead";
                        $getterContentTail .= "))";
                    } elsif ($idlTypeWithNamespace =~ /SVGStaticListPropertyTearOff/) {
                        $getterContentHead = "kit(WTF::getPtr(${idlTypeWithNamespace}::create(IMPL, $getterContentHead";
                        $getterContentTail .= ")))";
                    } else {
                        $getterContentHead = "kit(WTF::getPtr(${idlTypeWithNamespace}::create($getterContentHead";
                        $getterContentTail .= ")))";
                    }
                } else {
                    $getterContentHead = "kit(WTF::getPtr($getterContentHead";
                    $getterContentTail .= "))";
                }
            }

            my $getterContent;
            if ($hasGetterException || $attribute->signature->isNullable) {
                $getterContent = $getterContentHead;
                my $getterWithoutAttributes = $getterContentHead =~ /\($|, $/ ? "ec" : ", ec";
                if ($attribute->signature->isNullable) {
                    $getterContent .= $getterWithoutAttributes ? "isNull" : ", isNull";
                    $getterWithoutAttributes = 0;
                }
                if ($hasGetterException) {
                    $getterContent .= $getterWithoutAttributes ? "ec" : ", ec";
                }
                $getterContent .= $getterContentTail;
            } else {
                $getterContent = $getterContentHead . $getterContentTail;
            }

            my $attributeConditionalString = $codeGenerator->GenerateConditionalString($attribute->signature);
            push(@implContent, "#if ${attributeConditionalString}\n") if $attributeConditionalString;
            push(@implContent, $getterSig);
            push(@implContent, "{\n");
            push(@implContent, "    $jsContextSetter\n");
            push(@implContent, @customGetterContent);

            # FIXME: Should we return a default value when isNull == true?
            if ($attribute->signature->isNullable) {
                push(@implContents, "    $nullableInit\n");
            }

            if ($hasGetterException) {
                # Differentiated between when the return type is a pointer and
                # not for white space issue (ie. Foo *result vs. int result).
                if ($attributeType =~ /\*$/) {
                    $getterContent = $attributeType . "result = " . $getterContent;
                } else {
                    $getterContent = $attributeType . " result = " . $getterContent;
                }

                push(@implContent, "    $exceptionInit\n");
                push(@implContent, "    $getterContent;\n");
                push(@implContent, "    $exceptionRaiseOnError\n");
                push(@implContent, "    return result;\n");
            } else {
                push(@implContent, "    return $getterContent;\n");
            }
            push(@implContent, "}\n");

            # - SETTER
            if (!$attribute->isReadOnly) {
                # Exception handling
                my $hasSetterException = $attribute->signature->extendedAttributes->{"SetterRaisesException"};

                my $coreSetterName = "set" . $codeGenerator->WK_ucfirst($attributeName);
                my $setterName = "set" . ucfirst($attributeInterfaceName);
                my $argName = "new" . ucfirst($attributeInterfaceName);
                my $arg = GetObjCTypeGetter($argName, $idlType);

                # The definition of ObjCImplementedAsUnsignedLong is flipped for the setter
                if ($attribute->signature->extendedAttributes->{"ObjCImplementedAsUnsignedLong"}) {
                    $arg = "WTF::String($arg).toInt()";
                }

                my $setterSig = "- (void)$setterName:($attributeType)$argName\n";

                push(@implContent, "\n");
                push(@implContent, $setterSig);
                push(@implContent, "{\n");
                push(@implContent, "    $jsContextSetter\n");

                unless ($codeGenerator->IsPrimitiveType($idlType) or $codeGenerator->IsStringType($idlType)) {
                    push(@implContent, "    ASSERT($argName);\n\n");
                }

                if ($idlType eq "Date") {
                    $arg = "core(" . $arg . ")";
                }

                if ($svgPropertyType) {
                    $implIncludes{"ExceptionCode.h"} = 1;
                    $getterContentHead = "$getterExpressionPrefix";
                    push(@implContent, "    if (IMPL->isReadOnly()) {\n");
                    push(@implContent, "        WebCore::raiseOnDOMError(WebCore::NO_MODIFICATION_ALLOWED_ERR);\n");
                    push(@implContent, "        return;\n");
                    push(@implContent, "    }\n");
                    push(@implContent, "    $svgPropertyType& podImpl = IMPL->propertyReference();\n");
                    my $ec = $hasSetterException ? ", ec" : "";
                    push(@implContent, "    $exceptionInit\n") if $hasSetterException;

                    # Special case for DOMSVGNumber
                    if ($svgPropertyType eq "float") {
                        push(@implContent, "    podImpl = $arg;\n");
                    } else {
                        # FIXME: Special case for DOMSVGLength. We do need Custom code support for this.
                        if ($svgPropertyType eq "WebCore::SVGLength" and $attributeName eq "value") {
                            push(@implContent, "    podImpl.$coreSetterName($arg, WebCore::SVGLengthContext(IMPL->contextElement())$ec);\n");
                        } else {
                            push(@implContent, "    podImpl.$coreSetterName($arg$ec);\n");
                        }
                    }

                    if ($hasSetterException) {
                        push(@implContent, "    if (!ec)\n");
                        push(@implContent, "        IMPL->commitChange();\n");
                        push(@implContent, "    $exceptionRaiseOnError\n");
                    } else {
                        push(@implContent, "    IMPL->commitChange();\n");
                    }
                } elsif ($svgListPropertyType) {
                    $getterContentHead = "$getterExpressionPrefix";
                    push(@implContent, "    IMPL->$coreSetterName($arg);\n");
                } else {
                    my ($functionName, @arguments) = $codeGenerator->SetterExpression(\%implIncludes, $interfaceName, $attribute);
                    push(@arguments, $arg);
                    push(@arguments, "ec") if $hasSetterException;
                    push(@implContent, "    $exceptionInit\n") if $hasSetterException;
                    if ($attribute->signature->extendedAttributes->{"ImplementedBy"}) {
                        my $implementedBy = $attribute->signature->extendedAttributes->{"ImplementedBy"};
                        $implIncludes{"${implementedBy}.h"} = 1;
                        unshift(@arguments, "IMPL");
                        $functionName = "${implementedBy}::${functionName}";
                    } else {
                        $functionName = "IMPL->${functionName}";
                    }
                    push(@implContent, "    ${functionName}(" . join(", ", @arguments) . ");\n");
                    push(@implContent, "    $exceptionRaiseOnError\n") if $hasSetterException;
                }

                push(@implContent, "}\n");
            }

            push(@implContent, "#endif\n") if $attributeConditionalString;
            push(@implContent, "\n");
        }
    }

    # - Functions
    if ($numFunctions > 0) {
        foreach my $function (@{$interface->functions}) {
            next if SkipFunction($function);
            next if ($function->signature->name eq "set" and $interface->extendedAttributes->{"TypedArray"});
            AddIncludesForType($function->signature->type);

            my $functionName = $function->signature->name;
            my $returnType = GetObjCType($function->signature->type);
            my $hasParameters = @{$function->parameters};
            my $raisesExceptions = $function->signature->extendedAttributes->{"RaisesException"};

            my @parameterNames = ();
            my @needsAssert = ();
            my %needsCustom = ();

            my $parameterIndex = 0;
            my $functionSig = "- ($returnType)$functionName";
            foreach my $param (@{$function->parameters}) {
                my $paramName = $param->name;
                my $paramType = GetObjCType($param->type);

                # make a new parameter name if the original conflicts with a property name
                $paramName = "in" . ucfirst($paramName) if $attributeNames{$paramName};

                AddIncludesForType($param->type);

                my $idlType = $param->type;
                my $implGetter = GetObjCTypeGetter($paramName, $idlType);

                push(@parameterNames, $implGetter);
                $needsCustom{"XPathNSResolver"} = $paramName if $idlType eq "XPathNSResolver";
                $needsCustom{"NodeFilter"} = $paramName if $idlType eq "NodeFilter";
                $needsCustom{"EventListener"} = $paramName if $idlType eq "EventListener";
                $needsCustom{"EventTarget"} = $paramName if $idlType eq "EventTarget";
                $needsCustom{"NodeToReturn"} = $paramName if $param->extendedAttributes->{"CustomReturn"};

                unless ($codeGenerator->IsPrimitiveType($idlType) or $codeGenerator->IsStringType($idlType)) {
                    push(@needsAssert, "    ASSERT($paramName);\n");
                }

                if ($parameterIndex >= 1) {
                    $functionSig .= " " . $param->name;
                }

                $functionSig .= ":($paramType)$paramName";

                $parameterIndex++;
            }

            my @functionContent = ();
            my $caller = "IMPL";

            # special case the XPathNSResolver
            if (defined $needsCustom{"XPathNSResolver"}) {
                my $paramName = $needsCustom{"XPathNSResolver"};
                push(@functionContent, "    WebCore::XPathNSResolver* nativeResolver = 0;\n");
                push(@functionContent, "    RefPtr<WebCore::XPathNSResolver> customResolver;\n");
                push(@functionContent, "    if ($paramName) {\n");
                push(@functionContent, "        if ([$paramName isMemberOfClass:[DOMNativeXPathNSResolver class]])\n");
                push(@functionContent, "            nativeResolver = core(static_cast<DOMNativeXPathNSResolver *>($paramName));\n");
                push(@functionContent, "        else {\n");
                push(@functionContent, "            customResolver = WebCore::DOMCustomXPathNSResolver::create($paramName);\n");
                push(@functionContent, "            nativeResolver = WTF::getPtr(customResolver);\n");
                push(@functionContent, "        }\n");
                push(@functionContent, "    }\n");
            }

            # special case the EventTarget
            if (defined $needsCustom{"EventTarget"}) {
                my $paramName = $needsCustom{"EventTarget"};
                push(@functionContent, "    DOMNode* ${paramName}ObjC = $paramName;\n");
                push(@functionContent, "    WebCore::Node* ${paramName}Node = core(${paramName}ObjC);\n");
                $implIncludes{"DOMNode.h"} = 1;
                $implIncludes{"Node.h"} = 1;
            }

            if ($function->signature->extendedAttributes->{"ObjCUseDefaultView"}) {
                push(@functionContent, "    WebCore::DOMWindow* dv = $caller->defaultView();\n");
                push(@functionContent, "    if (!dv)\n");
                push(@functionContent, "        return nil;\n");
                $implIncludes{"DOMWindow.h"} = 1;
                $caller = "dv";
            }

            # special case the EventListener
            if (defined $needsCustom{"EventListener"}) {
                my $paramName = $needsCustom{"EventListener"};
                push(@functionContent, "    RefPtr<WebCore::EventListener> nativeEventListener = WebCore::ObjCEventListener::wrap($paramName);\n");
            }

            # special case the NodeFilter
            if (defined $needsCustom{"NodeFilter"}) {
                my $paramName = $needsCustom{"NodeFilter"};
                push(@functionContent, "    RefPtr<WebCore::NodeFilter> nativeNodeFilter;\n");
                push(@functionContent, "    if ($paramName)\n");
                push(@functionContent, "        nativeNodeFilter = WebCore::NodeFilter::create(WebCore::ObjCNodeFilterCondition::create($paramName));\n");
            }

            # FIXME! We need [Custom] support for ObjC, to move these hacks into DOMSVGLength/MatrixCustom.mm
            my $svgLengthConvertToSpecifiedUnits = ($svgPropertyType and $svgPropertyType eq "WebCore::SVGLength" and $functionName eq "convertToSpecifiedUnits");

            push(@parameterNames, "WebCore::SVGLengthContext(IMPL->contextElement())") if $svgLengthConvertToSpecifiedUnits; 
            push(@parameterNames, "ec") if $raisesExceptions;

            # Handle arguments that are 'SVGProperty' based (SVGAngle/SVGLength). We need to convert from SVGPropertyTearOff<Type>* to Type,
            # to be able to call the desired WebCore function. If the conversion fails, we can't extract Type and need to raise an exception.
            my $currentParameter = -1;
            foreach my $param (@{$function->parameters}) {
                $currentParameter++;

                my $paramName = $param->name;

                # make a new parameter name if the original conflicts with a property name
                $paramName = "in" . ucfirst($paramName) if $attributeNames{$paramName};

                my $idlType = $param->type;
                next if not $codeGenerator->IsSVGTypeNeedingTearOff($idlType) or $implClassName =~ /List$/;

                my $implGetter = GetObjCTypeGetter($paramName, $idlType);
                my $idlTypeWithNamespace = GetSVGTypeWithNamespace($idlType);

                $implIncludes{"ExceptionCode.h"} = 1;
                push(@functionContent, "    $idlTypeWithNamespace* ${paramName}Core = $implGetter;\n");
                push(@functionContent, "    if (!${paramName}Core) {\n");
                push(@functionContent, "        WebCore::ExceptionCode ec = WebCore::TYPE_MISMATCH_ERR;\n");
                push(@functionContent, "        $exceptionRaiseOnError\n");
                if ($returnType eq "void") { 
                    push(@functionContent, "        return;\n");
                } else {
                    push(@functionContent, "        return nil;\n");
                }
                push(@functionContent, "    }\n");

                # Replace the paramter core() getter, by the cached variable.
                splice(@parameterNames, $currentParameter, 1, "${paramName}Core->propertyReference()");
            }

            my $content;
            if ($function->signature->extendedAttributes->{"ImplementedBy"}) {
                my $implementedBy = $function->signature->extendedAttributes->{"ImplementedBy"};
                $implIncludes{"${implementedBy}.h"} = 1;
                unshift(@parameterNames, $caller);
                $content = "${implementedBy}::" . $codeGenerator->WK_lcfirst($functionName) . "(" . join(", ", @parameterNames) . ")";
            } elsif ($svgPropertyType) {
                $implIncludes{"ExceptionCode.h"} = 1;
                push(@functionContent, "    if (IMPL->isReadOnly()) {\n");
                push(@functionContent, "        WebCore::raiseOnDOMError(WebCore::NO_MODIFICATION_ALLOWED_ERR);\n");
                if ($returnType eq "void") {
                    push(@functionContent, "        return;\n");
                } else {
                    push(@functionContent, "        return nil;\n");
                }
                push(@functionContent, "    }\n");
                push(@functionContent, "    $svgPropertyType& podImpl = IMPL->propertyReference();\n");
                $content = "podImpl." . $codeGenerator->WK_lcfirst($functionName) . "(" . join(", ", @parameterNames) . ")";
            } else {
                $content = "$caller->" . $codeGenerator->WK_lcfirst($functionName) . "(" . join(", ", @parameterNames) . ")";
            }

            if ($returnType eq "void") {
                # Special case 'void' return type.
                if ($raisesExceptions) {
                    push(@functionContent, "    $exceptionInit\n");
                    push(@functionContent, "    $content;\n");
                    if ($svgPropertyType) {
                        push(@functionContent, "    if (!ec)\n");
                        push(@functionContent, "        IMPL->commitChange();\n");
                    }
                    push(@functionContent, "    $exceptionRaiseOnError\n");
                } else {
                    push(@functionContent, "    $content;\n");
                    push(@functionContent, "    IMPL->commitChange();\n") if $svgPropertyType;
                }
            } elsif (defined $needsCustom{"NodeToReturn"}) {
                # Special case the insertBefore, replaceChild, removeChild 
                # and appendChild functions from DOMNode 
                my $toReturn = $needsCustom{"NodeToReturn"};
                if ($raisesExceptions) {
                    push(@functionContent, "    $exceptionInit\n");
                    push(@functionContent, "    if ($content)\n");
                    push(@functionContent, "        return $toReturn;\n");
                    push(@functionContent, "    $exceptionRaiseOnError\n");
                    push(@functionContent, "    return nil;\n");
                } else {
                    push(@functionContent, "    if ($content)\n");
                    push(@functionContent, "        return $toReturn;\n");
                    push(@functionContent, "    return nil;\n");
                }
            } elsif ($returnType eq "SerializedScriptValue") {
                $content = "foo";
            } else {
                if (ConversionNeeded($function->signature->type)) {
                    if ($codeGenerator->IsSVGTypeNeedingTearOff($function->signature->type) and not $implClassName =~ /List$/) {
                        my $idlTypeWithNamespace = GetSVGTypeWithNamespace($function->signature->type);
                        $content = "kit(WTF::getPtr(${idlTypeWithNamespace}::create($content)))";
                    } else {
                        $content = "kit(WTF::getPtr($content))";
                    }
                }

                if ($raisesExceptions) {
                    # Differentiated between when the return type is a pointer and
                    # not for white space issue (ie. Foo *result vs. int result).
                    if ($returnType =~ /\*$/) {
                        $content = $returnType . "result = " . $content;
                    } else {
                        $content = $returnType . " result = " . $content;
                    }

                    push(@functionContent, "    $exceptionInit\n");
                    push(@functionContent, "    $content;\n");
                    push(@functionContent, "    $exceptionRaiseOnError\n");
                    push(@functionContent, "    return result;\n");
                } else {
                    push(@functionContent, "    return $content;\n");
                }
            }

            my $conditionalString = $codeGenerator->GenerateConditionalString($function->signature);
            push(@implContent, "\n#if ${conditionalString}\n") if $conditionalString;

            push(@implContent, "$functionSig\n");
            push(@implContent, "{\n");
            push(@implContent, "    $jsContextSetter\n");
            push(@implContent, @functionContent);
            push(@implContent, "}\n\n");

            push(@implContent, "#endif\n\n") if $conditionalString;

            # generate the old style method names with un-named parameters, these methods are deprecated
            if (@{$function->parameters} > 1 and $function->signature->extendedAttributes->{"ObjCLegacyUnnamedParameters"}) {
                my $deprecatedFunctionSig = $functionSig;
                $deprecatedFunctionSig =~ s/\s\w+:/ :/g; # remove parameter names

                push(@implContent, "$deprecatedFunctionSig\n");
                push(@implContent, "{\n");
                push(@implContent, "    $jsContextSetter\n");
                push(@implContent, @functionContent);
                push(@implContent, "}\n\n");
            }

            # Clear the hash
            %needsCustom = ();
        }
    }

    # END implementation
    push(@implContent, "\@end\n");

    # Generate internal interfaces
    push(@implContent, "\n$implType* core($className *wrapper)\n");
    push(@implContent, "{\n");
    push(@implContent, "    return wrapper ? reinterpret_cast<$implType*>(wrapper->_internal) : 0;\n");
    push(@implContent, "}\n\n");

    if ($parentImplClassName eq "Object") {        
        push(@implContent, "$className *kit($implType* value)\n");
        push(@implContent, "{\n");
        push(@implContent, "    $assertMainThread;\n");
        push(@implContent, "    if (!value)\n");
        push(@implContent, "        return nil;\n");
        push(@implContent, "    if ($className *wrapper = getDOMWrapper(value))\n");
        push(@implContent, "        return [[wrapper retain] autorelease];\n");
        if ($interface->extendedAttributes->{"ObjCPolymorphic"}) {
            push(@implContent, "    $className *wrapper = [[kitClass(value) alloc] _init];\n");
            push(@implContent, "    if (!wrapper)\n");
            push(@implContent, "        return nil;\n");
        } else {
            push(@implContent, "    $className *wrapper = [[$className alloc] _init];\n");
        }
        push(@implContent, "    wrapper->_internal = reinterpret_cast<DOMObjectInternal*>(value);\n");
        push(@implContent, "    value->ref();\n");
        push(@implContent, "    addDOMWrapper(wrapper, value);\n");
        push(@implContent, "    return [wrapper autorelease];\n");
        push(@implContent, "}\n");
    } else {
        push(@implContent, "$className *kit($implType* value)\n");
        push(@implContent, "{\n");
        push(@implContent, "    $assertMainThread;\n");
        push(@implContent, "    return static_cast<$className*>(kit(static_cast<WebCore::$baseClass*>(value)));\n");
        push(@implContent, "}\n");
    }

    # - End the ifdef conditional if necessary
    push(@implContent, "\n#endif // ${conditionalString}\n") if $conditionalString;

    # - Generate dependencies.
    if ($writeDependencies && @ancestorInterfaceNames) {
        push(@depsContent, "$className.h : ", join(" ", map { "$_.idl" } @ancestorInterfaceNames), "\n");
        push(@depsContent, map { "$_.idl :\n" } @ancestorInterfaceNames); 
    }
}

# Internal helper
sub WriteData
{
    my $object = shift;
    my $dataNode = shift;
    my $outputDir = shift;

    # Open files for writing...
    my $name = $dataNode->name;
    my $prefix = FileNamePrefix;
    my $headerFileName = "$outputDir/$prefix$name.h";
    my $privateHeaderFileName = "$outputDir/$prefix${name}Private.h";
    my $implFileName = "$outputDir/$prefix$name.mm";
    my $internalHeaderFileName = "$outputDir/$prefix${name}Internal.h";
    my $depsFileName = "$outputDir/$prefix$name.dep";

    # Write public header.
    my $contents = join "", @headerContentHeader;
    map { $contents .= "\@class $_;\n" } sort keys(%headerForwardDeclarations);
    map { $contents .= "\@protocol $_;\n" } sort keys(%headerForwardDeclarationsForProtocols);

    my $hasForwardDeclarations = keys(%headerForwardDeclarations) + keys(%headerForwardDeclarationsForProtocols);
    $contents .= "\n" if $hasForwardDeclarations;
    $contents .= join "", @headerContent;
    $codeGenerator->UpdateFile($headerFileName, $contents);

    @headerContentHeader = ();
    @headerContent = ();
    %headerForwardDeclarations = ();
    %headerForwardDeclarationsForProtocols = ();

    if (@privateHeaderContent > 0) {
        $contents = join "", @privateHeaderContentHeader;
        map { $contents .= "\@class $_;\n" } sort keys(%privateHeaderForwardDeclarations);
        map { $contents .= "\@protocol $_;\n" } sort keys(%privateHeaderForwardDeclarationsForProtocols);

        $hasForwardDeclarations = keys(%privateHeaderForwardDeclarations) + keys(%privateHeaderForwardDeclarationsForProtocols);
        $contents .= "\n" if $hasForwardDeclarations;
        $contents .= join "", @privateHeaderContent;
        $codeGenerator->UpdateFile($privateHeaderFileName, $contents);

        @privateHeaderContentHeader = ();
        @privateHeaderContent = ();
        %privateHeaderForwardDeclarations = ();
        %privateHeaderForwardDeclarationsForProtocols = ();
    }

    # Write implementation file.
    unless ($noImpl) {
        $contents = join "", @implContentHeader;
        map { $contents .= "#import \"$_\"\n" } sort keys(%implIncludes);
        $contents .= join "", @implContent;
        $codeGenerator->UpdateFile($implFileName, $contents);

        @implContentHeader = ();
        @implContent = ();
        %implIncludes = ();
    }

    if (@internalHeaderContent > 0) {
        $contents = join "", @internalHeaderContent;
        $codeGenerator->UpdateFile($internalHeaderFileName, $contents);

        @internalHeaderContent = ();
    }

    # Write dependency file.
    if (@depsContent) {
        $contents = join "", @depsContent;
        $codeGenerator->UpdateFile($depsFileName, $contents);

        @depsContent = ();
    }
}

1;
