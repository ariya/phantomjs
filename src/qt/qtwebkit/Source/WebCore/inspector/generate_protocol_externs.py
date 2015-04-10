#!/usr/bin/env python
# Copyright (c) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re

type_traits = {
    "any": "*",
    "string": "string",
    "integer": "number",
    "number": "number",
    "boolean": "boolean",
    "array": "Array.<*>",
    "object": "Object",
}

ref_types = {}


def full_qualified_type_id(domain_name, type_id):
    if type_id.find(".") == -1:
        return "%s.%s" % (domain_name, type_id)
    return type_id


def fix_camel_case(name):
    refined = re.sub(r'-(\w)', lambda pat: pat.group(1).upper(), name)
    refined = to_title_case(refined)
    return re.sub(r'(?i)HTML|XML|WML|API', lambda pat: pat.group(0).upper(), refined)


def to_title_case(name):
    return name[:1].upper() + name[1:]


def generate_enum(name, json):
    enum_members = []
    for member in json["enum"]:
        enum_members.append("    %s: \"%s\"" % (fix_camel_case(member), member))
    return "\n/** @enum {string} */\n%s = {\n%s\n};\n" % (name, (",\n".join(enum_members)))


def param_type(domain_name, param):
    if "type" in param:
        if param["type"] == "array":
            items = param["items"]
            return "Array.<%s>" % param_type(domain_name, items)
        else:
            return type_traits[param["type"]]
    if "$ref" in param:
        type_id = full_qualified_type_id(domain_name, param["$ref"])
        if type_id in ref_types:
            return ref_types[type_id]
        else:
            print "Type not found: " + type_id
            return "!! Type not found: " + type_id


def generate_protocol_externs(output_path, input_path):
    input_file = open(input_path, "r")
    json_string = input_file.read()
    json_string = json_string.replace(": true", ": True")
    json_string = json_string.replace(": false", ": False")
    json_api = eval(json_string)["domains"]

    output_file = open(output_path, "w")

    output_file.write(
"""
var Protocol = {};
/** @typedef {string}*/
Protocol.Error;
""")

    for domain in json_api:
        domain_name = domain["domain"]
        if "types" in domain:
            for type in domain["types"]:
                type_id = full_qualified_type_id(domain_name, type["id"])
                ref_types[type_id] = "%sAgent.%s" % (domain_name, type["id"])

    for domain in json_api:
        domain_name = domain["domain"]
        output_file.write("\n\n\nvar %sAgent = {};\n" % domain_name)
        if "types" in domain:
            for type in domain["types"]:
                if type["type"] == "object":
                    typedef_args = []
                    if "properties" in type:
                        for property in type["properties"]:
                            suffix = ""
                            if ("optional" in property):
                                suffix = "|undefined"
                            if "enum" in property:
                                enum_name = "%sAgent.%s%s" % (domain_name, type["id"], to_title_case(property["name"]))
                                output_file.write(generate_enum(enum_name, property))
                                typedef_args.append("%s:(%s%s)" % (property["name"], enum_name, suffix))
                            else:
                                typedef_args.append("%s:(%s%s)" % (property["name"], param_type(domain_name, property), suffix))
                    if (typedef_args):
                        output_file.write("\n/** @typedef {{%s}|null} */\n%sAgent.%s;\n" % (", ".join(typedef_args), domain_name, type["id"]))
                    else:
                        output_file.write("\n/** @typedef {Object} */\n%sAgent.%s;\n" % (domain_name, type["id"]))
                elif type["type"] == "string" and "enum" in type:
                    output_file.write(generate_enum("%sAgent.%s" % (domain_name, type["id"]), type))
                elif type["type"] == "array":
                    suffix = ""
                    if ("optional" in property):
                        suffix = "|undefined"
                    output_file.write("\n/** @typedef {Array.<%s>%s} */\n%sAgent.%s;\n" % (param_type(domain_name, type["items"]), suffix, domain_name, type["id"]))
                else:
                    output_file.write("\n/** @typedef {%s} */\n%sAgent.%s;\n" % (type_traits[type["type"]], domain_name, type["id"]))

        if "commands" in domain:
            for command in domain["commands"]:
                output_file.write("\n/**\n")
                params = []
                if ("parameters" in command):
                    for in_param in command["parameters"]:
                        if ("optional" in in_param):
                            params.append("opt_%s" % in_param["name"])
                            output_file.write(" * @param {%s=} opt_%s\n" % (param_type(domain_name, in_param), in_param["name"]))
                        else:
                            params.append(in_param["name"])
                            output_file.write(" * @param {%s} %s\n" % (param_type(domain_name, in_param), in_param["name"]))
                returns = ["?Protocol.Error"]
                if ("returns" in command):
                    for out_param in command["returns"]:
                        if ("optional" in out_param):
                            returns.append("%s=" % param_type(domain_name, out_param))
                        else:
                            returns.append("%s" % param_type(domain_name, out_param))
                output_file.write(" * @param {function(%s):void=} opt_callback\n" % ", ".join(returns))
                output_file.write(" */\n")
                params.append("opt_callback")
                output_file.write("%sAgent.%s = function(%s) {}\n" % (domain_name, command["name"], ", ".join(params)))
                output_file.write("/** @param {function(%s):void=} opt_callback */\n" % ", ".join(returns))
                output_file.write("%sAgent.%s.invoke = function(obj, opt_callback) {}\n" % (domain_name, command["name"]))

        output_file.write("/** @interface */\n")
        output_file.write("%sAgent.Dispatcher = function() {};\n" % domain_name)
        if "events" in domain:
            for event in domain["events"]:
                params = []
                if ("parameters" in event):
                    output_file.write("/**\n")
                    for param in event["parameters"]:
                        if ("optional" in param):
                            params.append("opt_%s" % param["name"])
                            output_file.write(" * @param {%s=} opt_%s\n" % (param_type(domain_name, param), param["name"]))
                        else:
                            params.append(param["name"])
                            output_file.write(" * @param {%s} %s\n" % (param_type(domain_name, param), param["name"]))
                    output_file.write(" */\n")
                output_file.write("%sAgent.Dispatcher.prototype.%s = function(%s) {};\n" % (domain_name, event["name"], ", ".join(params)))
        output_file.write("/**\n * @param {%sAgent.Dispatcher} dispatcher\n */\n" % domain_name)
        output_file.write("InspectorBackend.register%sDispatcher = function(dispatcher) {}\n" % domain_name)
    output_file.close()

if __name__ == "__main__":
    import sys
    import os.path
    program_name = os.path.basename(__file__)
    if len(sys.argv) < 4 or sys.argv[1] != "-o":
        sys.stderr.write("Usage: %s -o OUTPUT_FILE INPUT_FILE\n" % program_name)
        exit(1)
    output_path = sys.argv[2]
    input_path = sys.argv[3]
    generate_protocol_externs(output_path, input_path)
