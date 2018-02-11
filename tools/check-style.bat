@echo off

set ROOT_DIR=%CD%

set CONFIG=
set CONFIG=%CONFIG% --indent=spaces=4
set CONFIG=%CONFIG% --style=otbs
set CONFIG=%CONFIG% --indent-labels
set CONFIG=%CONFIG% --pad-header
set CONFIG=%CONFIG% --pad-oper
set CONFIG=%CONFIG% --unpad-paren
set CONFIG=%CONFIG% --keep-one-line-blocks
set CONFIG=%CONFIG% --keep-one-line-statements
set CONFIG=%CONFIG% --convert-tabs
set CONFIG=%CONFIG% --indent-preprocessor
set CONFIG=%CONFIG% --align-pointer=type
set CONFIG=%CONFIG% --suffix=none

call astyle %CONFIG% !ROOT_DIR!\..\src\*.cpp
call astyle %CONFIG% !ROOT_DIR!\..\src\*.h
