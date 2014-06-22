/**
 *  File Name:          RegExp/properties-002.js
 *  ECMA Section:       15.7.6.js
 *  Description:        Based on ECMA 2 Draft 7 February 1999
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */
 //-----------------------------------------------------------------------------
var SECTION = "RegExp/properties-002.js";
var VERSION = "ECMA_2";
var TITLE   = "Properties of RegExp Instances";
var BUGNUMBER ="http://scopus/bugsplat/show_bug.cgi?id=346032";
// ALSO SEE http://bugzilla.mozilla.org/show_bug.cgi?id=124339


startTest();

re_1 = /\cA?/g;
re_1.lastIndex = Math.pow(2,31);
AddRegExpCases( re_1, "\\cA?", true, false, false, Math.pow(2,31) );

re_2 = /\w*/i;
re_2.lastIndex = Math.pow(2,32) -1;
AddRegExpCases( re_2, "\\w*", false, true, false, Math.pow(2,32)-1 );

re_3 = /\*{0,80}/m;
re_3.lastIndex = Math.pow(2,31) -1;
AddRegExpCases( re_3, "\\*{0,80}", false, false, true, Math.pow(2,31) -1 );

re_4 = /^./gim;
re_4.lastIndex = Math.pow(2,30) -1;
AddRegExpCases( re_4, "^.", true, true, true, Math.pow(2,30) -1 );

re_5 = /\B/;
re_5.lastIndex = Math.pow(2,30);
AddRegExpCases( re_5, "\\B", false, false, false, Math.pow(2,30) );

/*
 * Brendan: "need to test cases Math.pow(2,32) and greater to see
 * whether they round-trip." Reason: thanks to the work done in
 * http://bugzilla.mozilla.org/show_bug.cgi?id=124339, lastIndex
 * is now stored as a double instead of a uint32 (unsigned integer).
 *
 * Note 2^32 -1 is the upper bound for uint32's, but doubles can go
 * all the way up to Number.MAX_VALUE. So that's why we need cases
 * between those two numbers.
 *
 */ 
re_6 = /\B/;
re_6.lastIndex = Math.pow(2,32);
AddRegExpCases( re_6, "\\B", false, false, false, Math.pow(2,32) );

re_7 = /\B/;
re_7.lastIndex = Math.pow(2,32) + 1;
AddRegExpCases( re_7, "\\B", false, false, false, Math.pow(2,32) + 1 );

re_8 = /\B/;
re_8.lastIndex = Math.pow(2,32) * 2;
AddRegExpCases( re_8, "\\B", false, false, false, Math.pow(2,32) * 2 );

re_9 = /\B/;
re_9.lastIndex = Math.pow(2,40);
AddRegExpCases( re_9, "\\B", false, false, false, Math.pow(2,40) );

re_10 = /\B/;
re_10.lastIndex = Number.MAX_VALUE;
AddRegExpCases( re_10, "\\B", false, false, false, Number.MAX_VALUE );



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function AddRegExpCases( re, s, g, i, m, l ){

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

    AddTestCase( re + ".source",
                 s,
                 re.source );

    AddTestCase( re + ".toString()",
                 "/" + s +"/" + (g?"g":"") + (i?"i":"") +(m?"m":""),
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
