/**
 *  File Name:          String/match-004.js
 *  ECMA Section:       15.6.4.9
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */

/*
 *  String.match( regexp )
 *
 *  If regexp is not an object of type RegExp, it is replaced with result
 *  of the expression new RegExp(regexp). Let string denote the result of
 *  converting the this value to a string.  If regexp.global is false,
 *  return the result obtained by invoking RegExp.prototype.exec (see
 *  section 15.7.5.3) on regexp with string as parameter.
 *
 *  Otherwise, set the regexp.lastIndex property to 0 and invoke
 *  RegExp.prototype.exec repeatedly until there is no match. If there is a
 *  match with an empty string (in other words, if the value of
 *  regexp.lastIndex is left unchanged) increment regexp.lastIndex by 1.
 *  The value returned is an array with the properties 0 through n-1
 *  corresponding to the first element of the result of each matching
 *  invocation of RegExp.prototype.exec.
 *
 *  Note that the match function is intentionally generic; it does not
 *  require that its this value be a string object.  Therefore, it can be
 *  transferred to other kinds of objects for use as a method.
 *
 *
 *  The match function should be intentionally generic, and not require
 *  this to be a string.
 *
 */

    var SECTION = "String/match-004.js";
    var VERSION = "ECMA_2";
    var TITLE   = "String.prototype.match( regexp )";

    var BUGNUMBER="http://scopus/bugsplat/show_bug.cgi?id=345818";

    startTest();

    // set the value of lastIndex
    re = /0./;
    s = 10203040506070809000;

    Number.prototype.match = String.prototype.match;

    AddRegExpCases(  re,
                     "re = " + re ,
                     s,
                     String(s),
                     1,
                     ["02"]);


    re.lastIndex = 0;
    AddRegExpCases(  re,
                     "re = " + re +" [lastIndex is " + re.lastIndex+"]",
                     s,
                     String(s),
                     1,
                     ["02"]);
/*

    re.lastIndex = s.length;

    AddRegExpCases( re,
                    "re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
                    s.length,
                    s,
                    s.lastIndexOf("0"),
                    null );

    re.lastIndex = s.lastIndexOf("0");

    AddRegExpCases( re,
                    "re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
                    s.lastIndexOf("0"),
                    s,
                    s.lastIndexOf("0"),
                    ["02134"]);

    re.lastIndex = s.lastIndexOf("0") + 1;

    AddRegExpCases( re,
                    "re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
                    s.lastIndexOf("0") +1,
                    s,
                    0,
                    null);
*/
    test();

function AddRegExpCases(
    regexp, str_regexp, string, str_string, index, matches_array ) {

  // prevent a runtime error

    if ( regexp.exec(string) == null || matches_array == null ) {
        AddTestCase(
          string + ".match(" + regexp +")",
          matches_array,
          string.match(regexp) );

        return;
    }

    AddTestCase(
        "( " + string  + " ).match(" + str_regexp +").length",
        matches_array.length,
        string.match(regexp).length );

    AddTestCase(
        "( " + string + " ).match(" + str_regexp +").index",
        index,
        string.match(regexp).index );

	AddTestCase(
        "( " + string + " ).match(" + str_regexp +").input",
        str_string,
        string.match(regexp).input );

    var limit = matches_array.length > string.match(regexp).length ?
                matches_array.length :
                string.match(regexp).length;

    for ( var matches = 0; matches < limit; matches++ ) {
        AddTestCase(
            "( " + string + " ).match(" + str_regexp +")[" + matches +"]",
            matches_array[matches],
            string.match(regexp)[matches] );
    }
}

function AddGlobalRegExpCases(
    regexp, str_regexp, string, matches_array ) {

  // prevent a runtime error

    if ( regexp.exec(string) == null || matches_array == null ) {
        AddTestCase(
          regexp + ".exec(" + string +")",
          matches_array,
          regexp.exec(string) );

        return;
    }

    AddTestCase(
        "( " + string  + " ).match(" + str_regexp +").length",
        matches_array.length,
        string.match(regexp).length );

    var limit = matches_array.length > string.match(regexp).length ?
                matches_array.length :
                string.match(regexp).length;

    for ( var matches = 0; matches < limit; matches++ ) {
        AddTestCase(
            "( " + string + " ).match(" + str_regexp +")[" + matches +"]",
            matches_array[matches],
            string.match(regexp)[matches] );
    }
}
