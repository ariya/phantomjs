#!/bin/bash
# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generates various components of GLSL ES preprocessor.

run_flex()
{
input_file=$script_dir/$1
output_source=$script_dir/$2
flex --noline --nounistd --outfile=$output_source $input_file
}

run_bison()
{
input_file=$script_dir/$1
output_source=$script_dir/$2
bison --no-lines --skeleton=yacc.c --output=$output_source $input_file
}

script_dir=$(dirname $0)

# Generate preprocessor
run_flex Tokenizer.l Tokenizer.cpp
run_bison ExpressionParser.y ExpressionParser.cpp
