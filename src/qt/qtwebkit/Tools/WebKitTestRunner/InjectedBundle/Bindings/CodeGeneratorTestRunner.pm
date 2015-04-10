# Copyright (C) 2010 Apple Inc. All rights reserved.
# Copyright (C) 2012 Samsung Electronics
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

use strict;
use warnings;
use File::Spec;

package CodeGeneratorTestRunner;

sub new
{
    my ($class, $codeGenerator, $layerOnTop, $preprocessor, $writeDependencies, $verbose, $idlFilePath) = @_;

    my $reference = {
        codeGenerator => $codeGenerator,
        idlFilePath => $idlFilePath,
    };

    bless($reference, $class);
    return $reference;
}

sub GenerateInterface
{
}

sub WriteData
{
    my ($self, $interface, $outputDir) = @_;

    foreach my $file ($self->_generateHeaderFile($interface), $self->_generateImplementationFile($interface)) {
        open(FILE, ">", File::Spec->catfile($outputDir, $$file{name})) or die "Failed to open $$file{name} for writing: $!";
        print FILE @{$$file{contents}};
        close(FILE) or die "Failed to close $$file{name} after writing: $!";
    }
}

sub _className
{
    my ($idlType) = @_;

    return "JS" . _implementationClassName($idlType);
}

sub _classRefGetter
{
    my ($self, $idlType) = @_;
    return $$self{codeGenerator}->WK_lcfirst(_implementationClassName($idlType)) . "Class";
}

sub _parseLicenseBlock
{
    my ($fileHandle) = @_;

    my ($copyright, $readCount, $buffer, $currentCharacter, $previousCharacter);
    my $startSentinel = "/*";
    my $lengthOfStartSentinel = length($startSentinel);
    $readCount = read($fileHandle, $buffer, $lengthOfStartSentinel);
    return "" if ($readCount < $lengthOfStartSentinel || $buffer ne $startSentinel);
    $copyright = $buffer;

    while ($readCount = read($fileHandle, $currentCharacter, 1)) {
        $copyright .= $currentCharacter;
        return $copyright if $currentCharacter eq "/" && $previousCharacter eq "*";
        $previousCharacter = $currentCharacter;
    }

    return "";
}

sub _parseLicenseBlockFromFile
{
    my ($path) = @_;
    open my $fileHandle, "<", $path or die "Failed to open $path for reading: $!";
    my $licenseBlock = _parseLicenseBlock($fileHandle);
    close($fileHandle);
    return $licenseBlock;
}

sub _defaultLicenseBlock
{
    return <<EOF;
/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
EOF
}

sub _licenseBlock
{
    my ($self) = @_;
    return $self->{licenseBlock} if $self->{licenseBlock};

    my $licenseBlock = _parseLicenseBlockFromFile($self->{idlFilePath}) || _defaultLicenseBlock();
    $self->{licenseBlock} = $licenseBlock;
    return $licenseBlock;
}

sub _generateHeaderFile
{
    my ($self, $interface) = @_;

    my @contents = ();

    my $idlType = $interface->name;
    my $className = _className($idlType);
    my $implementationClassName = _implementationClassName($idlType);
    my $filename = $className . ".h";

    push(@contents, $self->_licenseBlock());

    my $parentClassName = _parentClassName($interface);

    push(@contents, <<EOF);

#ifndef ${className}_h
#define ${className}_h

#include "${parentClassName}.h"
EOF
    push(@contents, <<EOF);

namespace WTR {

class ${implementationClassName};

class ${className} : public ${parentClassName} {
public:
    static JSClassRef @{[$self->_classRefGetter($idlType)]}();

private:
    static const JSStaticFunction* staticFunctions();
    static const JSStaticValue* staticValues();
EOF

    if (my @functions = @{$interface->functions}) {
        push(@contents, "\n    // Functions\n\n");
        foreach my $function (@functions) {
            push(@contents, "    static JSValueRef @{[$function->signature->name]}(JSContextRef, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*);\n");
        }
    }

    if (my @attributes = @{$interface->attributes}) {
        push(@contents, "\n    // Attributes\n\n");
        foreach my $attribute (@attributes) {
            push(@contents, "    static JSValueRef @{[$self->_getterName($attribute)]}(JSContextRef, JSObjectRef, JSStringRef, JSValueRef*);\n");
            push(@contents, "    static bool @{[$self->_setterName($attribute)]}(JSContextRef, JSObjectRef, JSStringRef, JSValueRef, JSValueRef*);\n") unless $attribute->isReadOnly;
        }
    }

    push(@contents, <<EOF);
};
    
${implementationClassName}* to${implementationClassName}(JSContextRef, JSValueRef);

} // namespace WTR

#endif // ${className}_h
EOF

    return { name => $filename, contents => \@contents };
}

sub _generateImplementationFile
{
    my ($self, $interface) = @_;

    my @contentsPrefix = ();
    my %contentsIncludes = ();
    my @contents = ();

    my $idlType = $interface->name;
    my $className = _className($idlType);
    my $implementationClassName = _implementationClassName($idlType);
    my $filename = $className . ".cpp";

    push(@contentsPrefix, $self->_licenseBlock());

    my $classRefGetter = $self->_classRefGetter($idlType);
    my $parentClassName = _parentClassName($interface);

    $contentsIncludes{"${className}.h"} = 1;
    $contentsIncludes{"${implementationClassName}.h"} = 1;

    push(@contentsPrefix, <<EOF);

EOF

    push(@contents, <<EOF);
#include <JavaScriptCore/JSRetainPtr.h>
#include <wtf/GetPtr.h>

namespace WTR {

${implementationClassName}* to${implementationClassName}(JSContextRef context, JSValueRef value)
{
    if (!context || !value || !${className}::${classRefGetter}() || !JSValueIsObjectOfClass(context, value, ${className}::${classRefGetter}()))
        return 0;
    return static_cast<${implementationClassName}*>(JSWrapper::unwrap(context, value));
}

JSClassRef ${className}::${classRefGetter}()
{
    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.className = "${idlType}";
        definition.parentClass = @{[$self->_parentClassRefGetterExpression($interface)]};
        definition.staticValues = staticValues();
        definition.staticFunctions = staticFunctions();
EOF

    push(@contents, "        definition.initialize = initialize;\n") unless _parentInterface($interface);
    push(@contents, "        definition.finalize = finalize;\n") unless _parentInterface($interface);

    push(@contents, <<EOF);
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

EOF

    push(@contents, $self->_staticFunctionsGetterImplementation($interface), "\n");
    push(@contents, $self->_staticValuesGetterImplementation($interface));

    if (my @functions = @{$interface->functions}) {
        push(@contents, "\n// Functions\n");

        foreach my $function (@functions) {
            push(@contents, <<EOF);

JSValueRef ${className}::@{[$function->signature->name]}(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ${implementationClassName}* impl = to${implementationClassName}(context, thisObject);
    if (!impl)
        return JSValueMakeUndefined(context);

EOF
            my $functionCall;
            if ($function->signature->extendedAttributes->{"CustomArgumentHandling"}) {
                $functionCall = "impl->" . $function->signature->name . "(context, argumentCount, arguments, exception)";
            } else {
                my @parameters = ();
                my @specifiedParameters = @{$function->parameters};

                $self->_includeHeaders(\%contentsIncludes, $function->signature->type, $function->signature);

                if ($function->signature->extendedAttributes->{"PassContext"}) {
                    push(@parameters, "context");
                }

                foreach my $i (0..$#specifiedParameters) {
                    my $parameter = $specifiedParameters[$i];

                    $self->_includeHeaders(\%contentsIncludes, $idlType, $parameter);

                    push(@contents, "    " . $self->_platformTypeVariableDeclaration($parameter, $parameter->name, "arguments[$i]", "argumentCount > $i") . "\n");
                    
                    push(@parameters, $self->_parameterExpression($parameter));
                }

                $functionCall = "impl->" . $function->signature->name . "(" . join(", ", @parameters) . ")";
            }
            
            push(@contents, "    ${functionCall};\n\n") if $function->signature->type eq "void";
            push(@contents, "    return " . $self->_returnExpression($function->signature, $functionCall) . ";\n}\n");
        }
    }

    if (my @attributes = @{$interface->attributes}) {
        push(@contents, "\n// Attributes\n");
        foreach my $attribute (@attributes) {
            $self->_includeHeaders(\%contentsIncludes, $attribute->signature->type, $attribute->signature);

            my $getterName = $self->_getterName($attribute);
            my $getterExpression = "impl->${getterName}()";

            push(@contents, <<EOF);

JSValueRef ${className}::${getterName}(JSContextRef context, JSObjectRef object, JSStringRef, JSValueRef* exception)
{
    ${implementationClassName}* impl = to${implementationClassName}(context, object);
    if (!impl)
        return JSValueMakeUndefined(context);

    return @{[$self->_returnExpression($attribute->signature, $getterExpression)]};
}
EOF

            unless ($attribute->isReadOnly) {
                push(@contents, <<EOF);

bool ${className}::@{[$self->_setterName($attribute)]}(JSContextRef context, JSObjectRef object, JSStringRef, JSValueRef value, JSValueRef* exception)
{
    ${implementationClassName}* impl = to${implementationClassName}(context, object);
    if (!impl)
        return false;

EOF

                my $platformValue = $self->_platformTypeConstructor($attribute->signature, "value");

                push(@contents, <<EOF);
    impl->@{[$self->_setterName($attribute)]}(${platformValue});

    return true;
}
EOF
            }
        }
    }

    push(@contents, <<EOF);

} // namespace WTR

EOF

    unshift(@contents, map { "#include \"$_\"\n" } sort keys(%contentsIncludes));
    unshift(@contents, "#include \"config.h\"\n");
    unshift(@contents, @contentsPrefix);

    return { name => $filename, contents => \@contents };
}

sub _getterName
{
    my ($self, $attribute) = @_;

    my $signature = $attribute->signature;
    my $name = $signature->name;

    return $name;
}

sub _includeHeaders
{
    my ($self, $headers, $idlType, $signature) = @_;

    return unless defined $idlType;
    return if $idlType eq "boolean";
    return if $idlType eq "object";
    return if $$self{codeGenerator}->IsNonPointerType($idlType);
    return if $$self{codeGenerator}->IsStringType($idlType);

    $$headers{_className($idlType) . ".h"} = 1;
    $$headers{_implementationClassName($idlType) . ".h"} = 1;
}

sub _implementationClassName
{
    my ($idlType) = @_;

    return $idlType;
}

sub _parentClassName
{
    my ($interface) = @_;

    my $parentInterface = _parentInterface($interface);
    return $parentInterface ? _className($parentInterface) : "JSWrapper";
}

sub _parentClassRefGetterExpression
{
    my ($self, $interface) = @_;

    my $parentInterface = _parentInterface($interface);
    return $parentInterface ? $self->_classRefGetter($parentInterface) . "()" : "0";
}

sub _parentInterface
{
    my ($interface) = @_;
    return $interface->parent;
}

sub _platformType
{
    my ($self, $idlType, $signature) = @_;

    return undef unless defined $idlType;

    return "bool" if $idlType eq "boolean";
    return "JSValueRef" if $idlType eq "object";
    return "JSRetainPtr<JSStringRef>" if $$self{codeGenerator}->IsStringType($idlType);
    return "double" if $$self{codeGenerator}->IsNonPointerType($idlType);
    return _implementationClassName($idlType);
}

sub _platformTypeConstructor
{
    my ($self, $signature, $argumentName) = @_;

    my $idlType = $signature->type;

    return "JSValueToBoolean(context, $argumentName)" if $idlType eq "boolean";
    return "$argumentName" if $idlType eq "object";
    return "JSRetainPtr<JSStringRef>(Adopt, JSValueToStringCopy(context, $argumentName, 0))" if $$self{codeGenerator}->IsStringType($idlType);
    return "JSValueToNumber(context, $argumentName, 0)" if $$self{codeGenerator}->IsNonPointerType($idlType);
    return "to" . _implementationClassName($idlType) . "(context, $argumentName)";
}

sub _platformTypeVariableDeclaration
{
    my ($self, $signature, $variableName, $argumentName, $condition) = @_;

    my $platformType = $self->_platformType($signature->type, $signature);
    my $constructor = $self->_platformTypeConstructor($signature, $argumentName);

    my %nonPointerTypes = (
        "bool" => 1,
        "double" => 1,
        "JSRetainPtr<JSStringRef>" => 1,
        "JSValueRef" => 1,
    );

    my $nullValue = "0";
    if ($platformType eq "JSValueRef") {
        $nullValue = "JSValueMakeUndefined(context)";
    } elsif (defined $nonPointerTypes{$platformType} && $platformType ne "double") {
        $nullValue = "$platformType()";
    }

    $platformType .= "*" unless defined $nonPointerTypes{$platformType};

    return "$platformType $variableName = $condition && $constructor;" if $condition && $platformType eq "bool";
    return "$platformType $variableName = $condition ? $constructor : $nullValue;" if $condition;
    return "$platformType $variableName = $constructor;";
}

sub _returnExpression
{
    my ($self, $signature, $expression) = @_;

    my $returnIDLType = $signature->type;

    return "JSValueMakeUndefined(context)" if $returnIDLType eq "void";
    return "JSValueMakeBoolean(context, ${expression})" if $returnIDLType eq "boolean";
    return "${expression}" if $returnIDLType eq "object";
    return "JSValueMakeNumber(context, ${expression})" if $$self{codeGenerator}->IsNonPointerType($returnIDLType);
    return "JSValueMakeStringOrNull(context, ${expression}.get())" if $$self{codeGenerator}->IsStringType($returnIDLType);
    return "toJS(context, WTF::getPtr(${expression}))";
}

sub _parameterExpression
{
    my ($self, $parameter) = @_;

    my $idlType = $parameter->type;
    my $name = $parameter->name;

    return "${name}.get()" if $$self{codeGenerator}->IsStringType($idlType);
    return $name;
}

sub _setterName
{
    my ($self, $attribute) = @_;

    my $name = $attribute->signature->name;

    return "set" . $$self{codeGenerator}->WK_ucfirst($name);
}

sub _staticFunctionsGetterImplementation
{
    my ($self, $interface) = @_;

    my $mapFunction = sub {
        my $name = $_->signature->name;
        my @attributes = qw(kJSPropertyAttributeDontDelete kJSPropertyAttributeReadOnly);
        push(@attributes, "kJSPropertyAttributeDontEnum") if $_->signature->extendedAttributes->{"DontEnum"};

        return  "{ \"$name\", $name, " . join(" | ", @attributes) . " }";
    };

    return $self->_staticFunctionsOrValuesGetterImplementation($interface, "function", "{ 0, 0, 0 }", $mapFunction, $interface->functions);
}

sub _staticFunctionsOrValuesGetterImplementation
{
    my ($self, $interface, $functionOrValue, $arrayTerminator, $mapFunction, $functionsOrAttributes) = @_;

    my $className = _className($interface->name);
    my $uppercaseFunctionOrValue = $$self{codeGenerator}->WK_ucfirst($functionOrValue);

    my $result = <<EOF;
const JSStatic${uppercaseFunctionOrValue}* ${className}::static${uppercaseFunctionOrValue}s()
{
EOF

    my @initializers = map(&$mapFunction, @{$functionsOrAttributes});
    return $result . "    return 0;\n}\n" unless @initializers;

    $result .= <<EOF
    static const JSStatic${uppercaseFunctionOrValue} ${functionOrValue}s[] = {
        @{[join(",\n        ", @initializers)]},
        ${arrayTerminator}
    };
    return ${functionOrValue}s;
}
EOF
}

sub _staticValuesGetterImplementation
{
    my ($self, $interface) = @_;

    my $mapFunction = sub {
        return if $_->signature->extendedAttributes->{"NoImplementation"};

        my $attributeName = $_->signature->name;
        my $getterName = $self->_getterName($_);
        my $setterName = $_->isReadOnly ? "0" : $self->_setterName($_);
        my @attributes = qw(kJSPropertyAttributeDontDelete);
        push(@attributes, "kJSPropertyAttributeReadOnly") if $_->isReadOnly;
        push(@attributes, "kJSPropertyAttributeDontEnum") if $_->signature->extendedAttributes->{"DontEnum"};

        return "{ \"$attributeName\", $getterName, $setterName, " . join(" | ", @attributes) . " }";
    };

    return $self->_staticFunctionsOrValuesGetterImplementation($interface, "value", "{ 0, 0, 0, 0 }", $mapFunction, $interface->attributes);
}

1;
