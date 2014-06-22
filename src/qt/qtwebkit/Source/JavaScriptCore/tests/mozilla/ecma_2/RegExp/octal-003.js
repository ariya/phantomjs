/**
 *  File Name:          RegExp/octal-003.js
 *  ECMA Section:       15.7.1
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *  Simple test cases for matching OctalEscapeSequences.
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 *
 *  Revised:            02 August 2002
 *  Author:             pschwartau@netscape.com
 *
 *  WHY:  the original test expected the regexp /.\011/ 
 *        to match 'a' + String.fromCharCode(0) + '11'
 *
 *  This is incorrect: the string is a 4-character string consisting of
 *  the characters <'a'>, <nul>, <'1'>, <'1'>. By contrast, the \011 in the
 *  regexp should be parsed as a single token: it is the octal escape sequence
 *  for the horizontal tab character '\t' === '\u0009' === '\x09' === '\011'. 
 *
 *  So the regexp consists of 2 characters: <any-character>, <'\t'>.
 *  There is no match between the regexp and the string.
 *
 *  See the testcase ecma_3/RegExp/octal-002.js for an elaboration.
 *
 */
    var SECTION = "RegExp/octal-003.js";
    var VERSION = "ECMA_2";
    var TITLE   = "RegExp patterns that contain OctalEscapeSequences";
    var BUGNUMBER="http://scopus/bugsplat/show_bug.cgi?id=346132";

    startTest();

    AddRegExpCases( /.\011/, "/\\011/", "a" + String.fromCharCode(0) + "11", "a\\011", 0, null );

    test();

function AddRegExpCases(
    regexp, str_regexp, pattern, str_pattern, index, matches_array ) {

    // prevent a runtime error

    if ( regexp.exec(pattern) == null || matches_array == null ) {
        AddTestCase(
            regexp + ".exec(" + str_pattern +")",
            matches_array,
            regexp.exec(pattern) );

        return;
    }
    AddTestCase(
        str_regexp + ".exec(" + str_pattern +").length",
        matches_array.length,
        regexp.exec(pattern).length );

    AddTestCase(
        str_regexp + ".exec(" + str_pattern +").index",
        index,
        regexp.exec(pattern).index );

    AddTestCase(
        str_regexp + ".exec(" + str_pattern +").input",
        escape(pattern),
        escape(regexp.exec(pattern).input) );

    AddTestCase(
        str_regexp + ".exec(" + str_pattern +").toString()",
        matches_array.toString(),
        escape(regexp.exec(pattern).toString()) );

    var limit = matches_array.length > regexp.exec(pattern).length
                ? matches_array.length
                : regexp.exec(pattern).length;

    for ( var matches = 0; matches < limit; matches++ ) {
        AddTestCase(
            str_regexp + ".exec(" + str_pattern +")[" + matches +"]",
            matches_array[matches],
            escape(regexp.exec(pattern)[matches]) );
    }

}
