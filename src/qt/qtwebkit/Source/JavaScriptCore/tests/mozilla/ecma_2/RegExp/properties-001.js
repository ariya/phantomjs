/**
 *  File Name:          RegExp/properties-001.js
 *  ECMA Section:       15.7.6.js
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */
    var SECTION = "RegExp/properties-001.js";
    var VERSION = "ECMA_2";
    var TITLE   = "Properties of RegExp Instances";
    var BUGNUMBER ="http://scopus/bugsplat/show_bug.cgi?id=346000";

    startTest();

    AddRegExpCases( new RegExp, "",   false, false, false, 0 );
    AddRegExpCases( /.*/,       ".*", false, false, false, 0 );
    AddRegExpCases( /[\d]{5}/g, "[\\d]{5}", true, false, false, 0 );
    AddRegExpCases( /[\S]?$/i,  "[\\S]?$", false, true, false, 0 );
    AddRegExpCases( /^([a-z]*)[^\w\s\f\n\r]+/m,  "^([a-z]*)[^\\w\\s\\f\\n\\r]+", false, false, true, 0 );
    AddRegExpCases( /[\D]{1,5}[\ -][\d]/gi,      "[\\D]{1,5}[\\ -][\\d]", true, true, false, 0 );
    AddRegExpCases( /[a-zA-Z0-9]*/gm, "[a-zA-Z0-9]*", true, false, true, 0 );
    AddRegExpCases( /x|y|z/gim, "x|y|z", true, true, true, 0 );

    AddRegExpCases( /\u0051/im, "\\u0051", false, true, true, 0 );
    AddRegExpCases( /\x45/gm, "\\x45", true, false, true, 0 );
    AddRegExpCases( /\097/gi, "\\097", true, true, false, 0 );

    test();

function AddRegExpCases( re, s, g, i, m, l ) {

    AddTestCase( re + ".test == RegExp.prototype.test",
                 true,
                 re.test == RegExp.prototype.test );

    AddTestCase( re + ".toString == RegExp.prototype.toString",
                 true,
                 re.toString == RegExp.prototype.toString );

    AddTestCase( re + ".contructor == RegExp.prototype.constructor",
                 true,
                 re.constructor == RegExp.prototype.constructor );

    AddTestCase( re + ".compile == RegExp.prototype.compile",
                 true,
                 re.compile == RegExp.prototype.compile );

    AddTestCase( re + ".exec == RegExp.prototype.exec",
                 true,
                 re.exec == RegExp.prototype.exec );

    // properties

/*
 * http://bugzilla.mozilla.org/show_bug.cgi?id=225550 changed
 * the behavior of toString() and toSource() on empty regexps.
 * So branch if |s| is the empty string -
 */
    var S = s? s : '(?:)';

    AddTestCase( re + ".source",
                 S,
                 re.source );

    AddTestCase( re + ".toString()",
                 "/" + S +"/" + (g?"g":"") + (i?"i":"") +(m?"m":""),
                 re.toString() );

    AddTestCase( re + ".global",
                 g,
                 re.global );

    AddTestCase( re + ".ignoreCase",
                 i,
                 re.ignoreCase );

    AddTestCase( re + ".multiline",
                 m,
                 re.multiline);

    AddTestCase( re + ".lastIndex",
                 l,
                 re.lastIndex  );
}
