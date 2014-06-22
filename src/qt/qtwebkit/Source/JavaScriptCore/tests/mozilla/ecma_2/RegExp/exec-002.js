/**
 *  File Name:          RegExp/exec-002.js
 *  ECMA Section:       15.7.5.3
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *
 *  Test cases provided by rogerl@netscape.com
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */
    var SECTION = "RegExp/exec-002";
    var VERSION = "ECMA_2";
    var TITLE   = "RegExp.prototype.exec(string)";

    startTest();

    /*
     * for each test case, verify:
     * - type of object returned
     * - length of the returned array
     * - value of lastIndex
     * - value of index
     * - value of input
     * - value of the array indices
     */

    AddRegExpCases(
        /(a|d|q|)x/i,
        "bcaDxqy",
        3,
        ["Dx", "D"] );

    AddRegExpCases(
        /(a|(e|q))(x|y)/,
        "bcaddxqy",
        6,
        ["qy","q","q","y"] );


    AddRegExpCases(
        /a+b+d/,
        "aabbeeaabbs",
        0,
        null );

    AddRegExpCases(
        /a*b/,
        "aaadaabaaa",
        4,
        ["aab"] );

    AddRegExpCases(
        /a*b/,
        "dddb",
        3,
        ["b"] );

    AddRegExpCases(
        /a*b/,
        "xxx",
        0,
        null );

    AddRegExpCases(
        /x\d\dy/,
        "abcx45ysss235",
        3,
        ["x45y"] );

    AddRegExpCases(
        /[^abc]def[abc]+/,
        "abxdefbb",
        2,
        ["xdefbb"] );

    AddRegExpCases(
        /(a*)baa/,
        "ccdaaabaxaabaa",
        9,
        ["aabaa", "aa"] );

    AddRegExpCases(
        /(a*)baa/,
        "aabaa",
        0,
        ["aabaa", "aa"] );

    AddRegExpCases(
        /q(a|b)*q/,
        "xxqababqyy",
        2,
        ["qababq", "b"] );

    AddRegExpCases(
        /(a(.|[^d])c)*/,
        "adcaxc",
        0,
        ["adcaxc", "axc", "x"] );

    AddRegExpCases(
        /(a*)b\1/,
        "abaaaxaabaayy",
        0,
        ["aba", "a"] );

    AddRegExpCases(
        /(a*)b\1/,
        "abaaaxaabaayy",
        0,
        ["aba", "a"] );

    AddRegExpCases(
        /(a*)b\1/,
        "cccdaaabaxaabaayy",
        6,
        ["aba", "a"] );

    AddRegExpCases(
        /(a*)b\1/,
        "cccdaaabqxaabaayy",
        7,
        ["b", ""] );

    AddRegExpCases(
        /"(.|[^"\\\\])*"/,
        'xx\"makudonarudo\"yy',
        2,
        ["\"makudonarudo\"", "o"] );

    AddRegExpCases(
        /"(.|[^"\\\\])*"/,
        "xx\"ma\"yy",
        2,
        ["\"ma\"", "a"] );

    test();

function AddRegExpCases(
    regexp, pattern, index, matches_array ) {

    // prevent a runtime error

    if ( regexp.exec(pattern) == null || matches_array == null ) {
        AddTestCase(
            regexp + ".exec(" + pattern +")",
            matches_array,
            regexp.exec(pattern) );

        return;
    }
    AddTestCase(
        regexp + ".exec(" + pattern +").length",
        matches_array.length,
        regexp.exec(pattern).length );

    AddTestCase(
        regexp + ".exec(" + pattern +").index",
        index,
        regexp.exec(pattern).index );

    AddTestCase(
        regexp + ".exec(" + pattern +").input",
        pattern,
        regexp.exec(pattern).input );

    AddTestCase(
        regexp + ".exec(" + pattern +").toString()",
        matches_array.toString(),
        regexp.exec(pattern).toString() );
/*
    var limit = matches_array.length > regexp.exec(pattern).length
                ? matches_array.length
                : regexp.exec(pattern).length;

    for ( var matches = 0; matches < limit; matches++ ) {
        AddTestCase(
            regexp + ".exec(" + pattern +")[" + matches +"]",
            matches_array[matches],
            regexp.exec(pattern)[matches] );
    }
*/
}
