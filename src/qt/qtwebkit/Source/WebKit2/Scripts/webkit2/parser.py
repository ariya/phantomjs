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

import re

from webkit2 import model


def parse(file):
    receiver_attributes = None
    destination = None
    messages = []
    condition = None
    master_condition = None
    for line in file:
        match = re.search(r'messages -> (?P<destination>[A-Za-z_0-9]+) \s*(?:(?P<attributes>.*?)\s+)?{', line)
        if match:
            receiver_attributes = parse_attributes_string(match.group('attributes'))

            if condition:
                master_condition = condition
                condition = None
            destination = match.group('destination')
            continue
        if line.startswith('#'):
            if line.startswith('#if '):
                condition = line.rstrip()[4:]
            elif line.startswith('#endif'):
                condition = None
            continue
        match = re.search(r'([A-Za-z_0-9]+)\((.*?)\)(?:(?:\s+->\s+)\((.*?)\))?(?:\s+(.*))?', line)
        if match:
            name, parameters_string, reply_parameters_string, attributes_string = match.groups()
            if parameters_string:
                parameters = parse_parameters_string(parameters_string)
                for parameter in parameters:
                    parameter.condition = condition
            else:
                parameters = []

            attributes = parse_attributes_string(attributes_string)

            if reply_parameters_string:
                reply_parameters = parse_parameters_string(reply_parameters_string)
                for reply_parameter in reply_parameters:
                    reply_parameter.condition = condition
            elif reply_parameters_string == '':
                reply_parameters = []
            else:
                reply_parameters = None

            messages.append(model.Message(name, parameters, reply_parameters, attributes, condition))
    return model.MessageReceiver(destination, receiver_attributes, messages, master_condition)


def parse_attributes_string(attributes_string):
    if not attributes_string:
        return None
    return attributes_string.split()


def split_parameters_string(parameters_string):
    parameters = []
    current_parameter_string = ''

    nest_level = 0
    for character in parameters_string:
        if character == ',' and nest_level == 0:
            parameters.append(current_parameter_string)
            current_parameter_string = ''
            continue

        if character == '<':
            nest_level += 1
        elif character == '>':
            nest_level -= 1

        current_parameter_string += character

    parameters.append(current_parameter_string)
    return parameters

def parse_parameters_string(parameters_string):
    parameters = []

    for parameter_string in split_parameters_string(parameters_string):
        match = re.search(r'\s*(?:\[(?P<attributes>.*?)\]\s+)?(?P<type_and_name>.*)', parameter_string)
        attributes_string, type_and_name_string = match.group('attributes', 'type_and_name')
        parameter_type, parameter_name = type_and_name_string.rsplit(' ', 1)
        parameters.append(model.Parameter(type=parameter_type, name=parameter_name, attributes=parse_attributes_string(attributes_string)))
    return parameters
