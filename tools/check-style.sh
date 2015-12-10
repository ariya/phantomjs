#!/usr/bin/env bash
cwd=$(pwd)

CONFIG=''
CONFIG+=' --indent=spaces=4'
CONFIG+=' --style=otbs'
CONFIG+=' --indent-labels'
CONFIG+=' --pad-header'
CONFIG+=' --pad-oper'
CONFIG+=' --unpad-paren'
CONFIG+=' --keep-one-line-blocks'
CONFIG+=' --keep-one-line-statements'
CONFIG+=' --convert-tabs'
CONFIG+=' --indent-preprocessor'
CONFIG+=' --align-pointer=type'
CONFIG+=' --suffix=none'

astyle -n -Q $CONFIG $cwd/src/*.cpp
astyle -n -Q $CONFIG $cwd/src/*.h
