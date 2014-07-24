# Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import collections
import re
from webkit2 import parser

WANTS_CONNECTION_ATTRIBUTE = 'WantsConnection'
LEGACY_RECEIVER_ATTRIBUTE = 'LegacyReceiver'
DELAYED_ATTRIBUTE = 'Delayed'
VARIADIC_ATTRIBUTE = 'Variadic'

_license_header = """/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

"""


def messages_header_filename(receiver):
    return '%sMessages.h' % receiver.name


def surround_in_condition(string, condition):
    if not condition:
        return string
    return '#if %s\n%s#endif\n' % (condition, string)


def function_parameter_type(type):
    # Don't use references for built-in types.
    builtin_types = frozenset([
        'bool',
        'float',
        'double',
        'uint8_t',
        'uint16_t',
        'uint32_t',
        'uint64_t',
        'int8_t',
        'int16_t',
        'int32_t',
        'int64_t',
    ])

    if type in builtin_types:
        return type

    return 'const %s&' % type


def reply_parameter_type(type):
    return '%s&' % type


def arguments_type(parameters, parameter_type_function):
    arguments_type = 'CoreIPC::Arguments%d' % len(parameters)
    if len(parameters):
        arguments_type = '%s<%s>' % (arguments_type, ', '.join(parameter_type_function(parameter.type) for parameter in parameters))
    return arguments_type


def base_class(message):
    return arguments_type(message.parameters, function_parameter_type)


def reply_type(message):
    return arguments_type(message.reply_parameters, reply_parameter_type)


def decode_type(message):
    if message.has_attribute(VARIADIC_ATTRIBUTE):
        return arguments_type(message.parameters[:-1], reply_parameter_type)
    return base_class(message)


def delayed_reply_type(message):
    return arguments_type(message.reply_parameters, function_parameter_type)


def message_to_struct_declaration(message):
    result = []
    function_parameters = [(function_parameter_type(x.type), x.name) for x in message.parameters]
    result.append('struct %s : %s' % (message.name, base_class(message)))
    result.append(' {\n')
    result.append('    static CoreIPC::StringReference receiverName() { return messageReceiverName(); }\n')
    result.append('    static CoreIPC::StringReference name() { return CoreIPC::StringReference("%s"); }\n' % message.name)
    result.append('    static const bool isSync = %s;\n' % ('false', 'true')[message.reply_parameters != None])
    result.append('\n')
    if message.reply_parameters != None:
        if message.has_attribute(DELAYED_ATTRIBUTE):
            send_parameters = [(function_parameter_type(x.type), x.name) for x in message.reply_parameters]
            result.append('    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {\n')
            result.append('        DelayedReply(PassRefPtr<CoreIPC::Connection>, PassOwnPtr<CoreIPC::MessageEncoder>);\n')
            result.append('        ~DelayedReply();\n')
            result.append('\n')
            result.append('        bool send(%s);\n' % ', '.join([' '.join(x) for x in send_parameters]))
            result.append('\n')
            result.append('    private:\n')
            result.append('        RefPtr<CoreIPC::Connection> m_connection;\n')
            result.append('        OwnPtr<CoreIPC::MessageEncoder> m_encoder;\n')
            result.append('    };\n\n')

        result.append('    typedef %s Reply;\n' % reply_type(message))

    result.append('    typedef %s DecodeType;\n' % decode_type(message))
    if len(function_parameters):
        result.append('    %s%s(%s)' % (len(function_parameters) == 1 and 'explicit ' or '', message.name, ', '.join([' '.join(x) for x in function_parameters])))
        result.append('\n        : %s(%s)\n' % (base_class(message), ', '.join([x[1] for x in function_parameters])))
        result.append('    {\n')
        result.append('    }\n')
    result.append('};\n')
    return surround_in_condition(''.join(result), message.condition)


def struct_or_class(namespace, type):
    structs = frozenset([
        'WebCore::Animation',
        'WebCore::EditorCommandsForKeyEvent',
        'WebCore::CompositionUnderline',
        'WebCore::Cookie',
        'WebCore::DragSession',
        'WebCore::FloatPoint3D',
        'WebCore::FileChooserSettings',
        'WebCore::GrammarDetail',
        'WebCore::IdentityTransformOperation',
        'WebCore::KeypressCommand',
        'WebCore::Length',
        'WebCore::MatrixTransformOperation',
        'WebCore::Matrix3DTransformOperation',
        'WebCore::NotificationContents',
        'WebCore::PerspectiveTransformOperation',
        'WebCore::PluginInfo',
        'WebCore::PrintInfo',
        'WebCore::RotateTransformOperation',
        'WebCore::ScaleTransformOperation',
        'WebCore::SkewTransformOperation',
        'WebCore::TimingFunction',
        'WebCore::TransformationMatrix',
        'WebCore::TransformOperation',
        'WebCore::TransformOperations',
        'WebCore::TranslateTransformOperation',
        'WebCore::ViewportAttributes',
        'WebCore::WindowFeatures',
        'WebKit::AttributedString',
        'WebKit::ColorSpaceData',
        'WebKit::ContextMenuState',
        'WebKit::DictionaryPopupInfo',
        'WebKit::DrawingAreaInfo',
        'WebKit::EditorState',
        'WebKit::NetworkProcessCreationParameters',
        'WebKit::OfflineStorageProcessCreationParameters',
        'WebKit::PlatformPopupMenuData',
        'WebKit::PluginCreationParameters',
        'WebKit::PluginProcessCreationParameters',
        'WebKit::PrintInfo',
        'WebKit::SecurityOriginData',
        'WebKit::SharedWorkerProcessCreationParameters',
        'WebKit::StatisticsData',
        'WebKit::TextCheckerState',
        'WebKit::WebNavigationDataStore',
        'WebKit::WebPageCreationParameters',
        'WebKit::WebPreferencesStore',
        'WebKit::WebProcessCreationParameters',
        'WebKit::WindowGeometry',
    ])

    qualified_name = '%s::%s' % (namespace, type)
    if qualified_name in structs:
        return 'struct %s' % type

    return 'class %s' % type

def forward_declarations_for_namespace(namespace, types):
    result = []
    result.append('namespace %s {\n' % namespace)
    result += ['    %s;\n' % struct_or_class(namespace, x) for x in types]
    result.append('}\n')
    return ''.join(result)


def forward_declarations_and_headers(receiver):
    types_by_namespace = collections.defaultdict(set)

    headers = set([
        '"Arguments.h"',
        '"MessageEncoder.h"',
        '"StringReference.h"',
    ])

    for message in receiver.messages:
        if message.reply_parameters != None and message.has_attribute(DELAYED_ATTRIBUTE):
            headers.add('<wtf/ThreadSafeRefCounted.h>')
            types_by_namespace['CoreIPC'].update(['Connection'])

    for parameter in receiver.iterparameters():
        type = parameter.type

        if type.find('<') != -1:
            # Don't forward declare class templates.
            headers.update(headers_for_type(type))
            continue

        split = type.split('::')

        if len(split) == 2:
            namespace = split[0]
            inner_type = split[1]
            types_by_namespace[namespace].add(inner_type)
        elif len(split) > 2:
            # We probably have a nested struct, which means we can't forward declare it.
            # Include its header instead.
            headers.update(headers_for_type(type))

    forward_declarations = '\n'.join([forward_declarations_for_namespace(namespace, types) for (namespace, types) in sorted(types_by_namespace.items())])
    headers = ['#include %s\n' % header for header in sorted(headers)]

    return (forward_declarations, headers)

def generate_messages_header(file):
    receiver = parser.parse(file)
    header_guard = messages_header_filename(receiver).replace('.', '_')

    result = []

    result.append(_license_header)

    result.append('#ifndef %s\n' % header_guard)
    result.append('#define %s\n\n' % header_guard)

    if receiver.condition:
        result.append('#if %s\n\n' % receiver.condition)

    forward_declarations, headers = forward_declarations_and_headers(receiver)

    result += headers
    result.append('\n')

    result.append(forward_declarations)
    result.append('\n')

    result.append('namespace Messages {\nnamespace %s {\n' % receiver.name)
    result.append('\n')
    result.append('static inline CoreIPC::StringReference messageReceiverName()\n')
    result.append('{\n')
    result.append('    return CoreIPC::StringReference("%s");\n' % receiver.name)
    result.append('}\n')
    result.append('\n')
    result.append('\n'.join([message_to_struct_declaration(x) for x in receiver.messages]))
    result.append('\n')
    result.append('} // namespace %s\n} // namespace Messages\n' % receiver.name)

    if receiver.condition:
        result.append('\n#endif // %s\n' % receiver.condition)

    result.append('\n#endif // %s\n' % header_guard)

    return ''.join(result)


def handler_function(receiver, message):
    if message.name.find('URL') == 0:
        return '%s::%s' % (receiver.name, 'url' + message.name[3:])
    return '%s::%s' % (receiver.name, message.name[0].lower() + message.name[1:])


def async_message_statement(receiver, message):
    dispatch_function_args = ['decoder', 'this', '&%s' % handler_function(receiver, message)]

    dispatch_function = 'handleMessage'
    if message.has_attribute(VARIADIC_ATTRIBUTE):
        dispatch_function += 'Variadic'

    if message.has_attribute(WANTS_CONNECTION_ATTRIBUTE):
        dispatch_function_args.insert(0, 'connection')

    result = []
    result.append('    if (decoder.messageName() == Messages::%s::%s::name()) {\n' % (receiver.name, message.name))
    result.append('        CoreIPC::%s<Messages::%s::%s>(%s);\n' % (dispatch_function, receiver.name, message.name, ', '.join(dispatch_function_args)))
    result.append('        return;\n')
    result.append('    }\n')
    return surround_in_condition(''.join(result), message.condition)


def sync_message_statement(receiver, message):
    dispatch_function = 'handleMessage'
    if message.has_attribute(DELAYED_ATTRIBUTE):
        dispatch_function += 'Delayed'
    if message.has_attribute(VARIADIC_ATTRIBUTE):
        dispatch_function += 'Variadic'

    wants_connection = message.has_attribute(DELAYED_ATTRIBUTE) or message.has_attribute(WANTS_CONNECTION_ATTRIBUTE)

    result = []
    result.append('    if (decoder.messageName() == Messages::%s::%s::name()) {\n' % (receiver.name, message.name))
    result.append('        CoreIPC::%s<Messages::%s::%s>(%sdecoder, %sreplyEncoder, this, &%s);\n' % (dispatch_function, receiver.name, message.name, 'connection, ' if wants_connection else '', '' if message.has_attribute(DELAYED_ATTRIBUTE) else '*', handler_function(receiver, message)))
    result.append('        return;\n')
    result.append('    }\n')
    return surround_in_condition(''.join(result), message.condition)


def class_template_headers(template_string):
    template_string = template_string.strip()

    class_template_types = {
        # FIXME: Remove the unprefixed versions.
        'Vector': {'headers': ['<wtf/Vector.h>'], 'argument_coder_headers': ['"ArgumentCoders.h"']},
        'HashMap': {'headers': ['<wtf/HashMap.h>'], 'argument_coder_headers': ['"ArgumentCoders.h"']},

        'WTF::Vector': {'headers': ['<wtf/Vector.h>'], 'argument_coder_headers': ['"ArgumentCoders.h"']},
        'WTF::HashMap': {'headers': ['<wtf/HashMap.h>'], 'argument_coder_headers': ['"ArgumentCoders.h"']},
        'std::pair': {'headers': ['<utility>'], 'argument_coder_headers': ['"ArgumentCoders.h"']},
    }

    match = re.match('(?P<template_name>.+?)<(?P<parameter_string>.+)>', template_string)
    if not match:
        return {'header_infos':[], 'types':[template_string]}
    header_infos = [class_template_types[match.groupdict()['template_name']]]
    types = []

    for parameter in parser.split_parameters_string(match.groupdict()['parameter_string']):
        parameter_header_infos_and_types = class_template_headers(parameter)

        header_infos += parameter_header_infos_and_types['header_infos'];
        types += parameter_header_infos_and_types['types']

    return {'header_infos':header_infos, 'types':types}


def argument_coder_headers_for_type(type):
    header_infos_and_types = class_template_headers(type)

    special_cases = {
        'WTF::String': '"ArgumentCoders.h"',
        'WebKit::InjectedBundleUserMessageEncoder': '"InjectedBundleUserMessageCoders.h"',
        'WebKit::WebContextUserMessageEncoder': '"WebContextUserMessageCoders.h"',
    }

    headers = []
    for header_info in header_infos_and_types['header_infos']:
        headers += header_info['argument_coder_headers']

    for type in header_infos_and_types['types']:
        if type in special_cases:
            headers.append(special_cases[type])
            continue

        split = type.split('::')
        if len(split) < 2:
            continue
        if split[0] == 'WebCore':
            headers.append('"WebCoreArgumentCoders.h"')

    return headers

def headers_for_type(type):
    header_infos_and_types = class_template_headers(type)

    special_cases = {
        'WTF::String': ['<wtf/text/WTFString.h>'],
        'WebCore::CompositionUnderline': ['<WebCore/Editor.h>'],
        'WebCore::GrammarDetail': ['<WebCore/TextCheckerClient.h>'],
        'WebCore::GraphicsLayerAnimations': ['<WebCore/GraphicsLayerAnimation.h>'],
        'WebCore::KeyframeValueList': ['<WebCore/GraphicsLayer.h>'],
        'WebCore::KeypressCommand': ['<WebCore/KeyboardEvent.h>'],
        'WebCore::FileChooserSettings': ['<WebCore/FileChooser.h>'],
        'WebCore::PluginInfo': ['<WebCore/PluginData.h>'],
        'WebCore::TextCheckingRequestData': ['<WebCore/TextChecking.h>'],
        'WebCore::TextCheckingResult': ['<WebCore/TextCheckerClient.h>'],
        'WebCore::ViewportAttributes': ['<WebCore/ViewportArguments.h>'],
        'WebKit::InjectedBundleUserMessageEncoder': [],
        'WebKit::WebContextUserMessageEncoder': [],
        'WebKit::WebGestureEvent': ['"WebEvent.h"'],
        'WebKit::WebKeyboardEvent': ['"WebEvent.h"'],
        'WebKit::WebMouseEvent': ['"WebEvent.h"'],
        'WebKit::WebTouchEvent': ['"WebEvent.h"'],
        'WebKit::WebWheelEvent': ['"WebEvent.h"'],
    }

    headers = []
    for header_info in header_infos_and_types['header_infos']:
        headers += header_info['headers']

    for type in header_infos_and_types['types']:
        if type in special_cases:
            headers += special_cases[type]
            continue

        # We assume that we must include a header for a type iff it has a scope
        # resolution operator (::).
        split = type.split('::')
        if len(split) < 2:
            continue

        if split[0] == 'WebKit' or split[0] == 'CoreIPC':
            headers.append('"%s.h"' % split[1])
        else:
            headers.append('<%s/%s.h>' % tuple(split))

    return headers

def generate_message_handler(file):
    receiver = parser.parse(file)
    headers = {
        '"%s"' % messages_header_filename(receiver): [None],
        '"HandleMessage.h"': [None],
        '"MessageDecoder.h"': [None],
    }

    type_conditions = {}
    for parameter in receiver.iterparameters():
        if not parameter.type in type_conditions:
            type_conditions[parameter.type] = []

        if not parameter.condition in type_conditions[parameter.type]:
            type_conditions[parameter.type].append(parameter.condition)

    for parameter in receiver.iterparameters():
        type = parameter.type
        conditions = type_conditions[type]

        argument_encoder_headers = argument_coder_headers_for_type(type)
        if argument_encoder_headers:
            for header in argument_encoder_headers:
                if header not in headers:
                    headers[header] = []
                headers[header].extend(conditions)

        type_headers = headers_for_type(type)
        for header in type_headers:
            if header not in headers:
                headers[header] = []
            headers[header].extend(conditions)

    for message in receiver.messages:
        if message.reply_parameters is not None:
            for reply_parameter in message.reply_parameters:
                type = reply_parameter.type
                argument_encoder_headers = argument_coder_headers_for_type(type)
                if argument_encoder_headers:
                    for header in argument_encoder_headers:
                        if header not in headers:
                            headers[header] = []
                        headers[header].append(message.condition)

                type_headers = headers_for_type(type)
                for header in type_headers:
                    if header not in headers:
                        headers[header] = []
                    headers[header].append(message.condition)


    result = []

    result.append(_license_header)
    result.append('#include "config.h"\n')
    result.append('\n')

    if receiver.condition:
        result.append('#if %s\n\n' % receiver.condition)

    result.append('#include "%s.h"\n\n' % receiver.name)
    for headercondition in sorted(headers):
        if headers[headercondition] and not None in headers[headercondition]:
            result.append('#if %s\n' % ' || '.join(set(headers[headercondition])))
            result += ['#include %s\n' % headercondition]
            result.append('#endif\n')
        else:
            result += ['#include %s\n' % headercondition]
    result.append('\n')

    sync_delayed_messages = []
    for message in receiver.messages:
        if message.reply_parameters != None and message.has_attribute(DELAYED_ATTRIBUTE):
            sync_delayed_messages.append(message)

    if sync_delayed_messages:
        result.append('namespace Messages {\n\nnamespace %s {\n\n' % receiver.name)

        for message in sync_delayed_messages:
            send_parameters = [(function_parameter_type(x.type), x.name) for x in message.reply_parameters]

            if message.condition:
                result.append('#if %s\n\n' % message.condition)
            
            result.append('%s::DelayedReply::DelayedReply(PassRefPtr<CoreIPC::Connection> connection, PassOwnPtr<CoreIPC::MessageEncoder> encoder)\n' % message.name)
            result.append('    : m_connection(connection)\n')
            result.append('    , m_encoder(encoder)\n')
            result.append('{\n')
            result.append('}\n')
            result.append('\n')
            result.append('%s::DelayedReply::~DelayedReply()\n' % message.name)
            result.append('{\n')
            result.append('    ASSERT(!m_connection);\n')
            result.append('}\n')
            result.append('\n')
            result.append('bool %s::DelayedReply::send(%s)\n' % (message.name, ', '.join([' '.join(x) for x in send_parameters])))
            result.append('{\n')
            result.append('    ASSERT(m_encoder);\n')
            result += ['    *m_encoder << %s;\n' % x.name for x in message.reply_parameters]
            result.append('    bool result = m_connection->sendSyncReply(m_encoder.release());\n')
            result.append('    m_connection = nullptr;\n')
            result.append('    return result;\n')
            result.append('}\n')
            result.append('\n')

            if message.condition:
                result.append('#endif\n\n')

        result.append('} // namespace %s\n\n} // namespace Messages\n\n' % receiver.name)

    result.append('namespace WebKit {\n\n')

    async_messages = []
    sync_messages = []
    for message in receiver.messages:
        if message.reply_parameters is not None:
            sync_messages.append(message)
        else:
            async_messages.append(message)

    if async_messages:
        if receiver.has_attribute(LEGACY_RECEIVER_ATTRIBUTE):
            result.append('void %s::didReceive%sMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder& decoder)\n' % (receiver.name, receiver.name))
        else:
            result.append('void %s::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)\n' % (receiver.name))

        result.append('{\n')
        result += [async_message_statement(receiver, message) for message in async_messages]
        if not receiver.has_attribute(LEGACY_RECEIVER_ATTRIBUTE):
            result.append('    UNUSED_PARAM(connection);\n')
        result.append('    ASSERT_NOT_REACHED();\n')
        result.append('}\n')

    if sync_messages:
        result.append('\n')
        if receiver.has_attribute(LEGACY_RECEIVER_ATTRIBUTE):
            result.append('void %s::didReceiveSync%sMessage(CoreIPC::Connection*%s, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)\n' % (receiver.name, receiver.name, ' connection' if sync_delayed_messages else ''))
        else:
            result.append('void %s::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)\n' % (receiver.name))
        result.append('{\n')
        result += [sync_message_statement(receiver, message) for message in sync_messages]
        if not receiver.has_attribute(LEGACY_RECEIVER_ATTRIBUTE):
            result.append('    UNUSED_PARAM(connection);\n')
        result.append('    ASSERT_NOT_REACHED();\n')
        result.append('}\n')

    result.append('\n} // namespace WebKit\n')

    if receiver.condition:
        result.append('\n#endif // %s\n' % receiver.condition)

    return ''.join(result)
