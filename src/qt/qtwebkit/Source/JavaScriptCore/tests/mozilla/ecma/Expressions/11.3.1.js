/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          11.3.1.js
    ECMA Section:       11.3.1 Postfix increment operator
    Description:
    The production MemberExpression : MemberExpression ++ is evaluated as
    follows:

    1.  Evaluate MemberExpression.
    2.  Call GetValue(Result(1)).
    3.  Call ToNumber(Result(2)).
    4.  Add the value 1 to Result(3), using the same rules as for the +
        operator (section 0).
    5.  Call PutValue(Result(1), Result(4)).
    6.  Return Result(3).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.3.1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Postfix increment operator");

    testcases = getTestCases();
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    // special numbers
    array[item++] = new TestCase( SECTION,  "var MYVAR; MYVAR++",                       NaN,                            eval("var MYVAR; MYVAR++") );
    array[item++] = new TestCase( SECTION,  "var MYVAR= void 0; MYVAR++",               NaN,                            eval("var MYVAR=void 0; MYVAR++") );
    array[item++] = new TestCase( SECTION,  "var MYVAR=null; MYVAR++",                  0,                            eval("var MYVAR=null; MYVAR++") );
    array[item++] = new TestCase( SECTION,  "var MYVAR=true; MYVAR++",                  1,                            eval("var MYVAR=true; MYVAR++") );
    array[item++] = new TestCase( SECTION,  "var MYVAR=false; MYVAR++",                 0,                            eval("var MYVAR=false; MYVAR++") );

    // verify return value

    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.POSITIVE_INFINITY;MYVAR++", Number.POSITIVE_INFINITY,   eval("var MYVAR=Number.POSITIVE_INFINITY;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.NEGATIVE_INFINITY;MYVAR++", Number.NEGATIVE_INFINITY,   eval("var MYVAR=Number.NEGATIVE_INFINITY;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.NaN;MYVAR++",               Number.NaN,                 eval("var MYVAR=Number.NaN;MYVAR++") );

    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.POSITIVE_INFINITY;MYVAR++;MYVAR", Number.POSITIVE_INFINITY,   eval("var MYVAR=Number.POSITIVE_INFINITY;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.NEGATIVE_INFINITY;MYVAR++;MYVAR", Number.NEGATIVE_INFINITY,   eval("var MYVAR=Number.NEGATIVE_INFINITY;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=Number.NaN;MYVAR++;MYVAR",               Number.NaN,                 eval("var MYVAR=Number.NaN;MYVAR++;MYVAR") );

    // number primitives
    array[item++] = new TestCase( SECTION,    "var MYVAR=0;MYVAR++",            0,          eval("var MYVAR=0;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=0.2345;MYVAR++",       0.2345,     eval("var MYVAR=0.2345;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=-0.2345;MYVAR++",      -0.2345,     eval("var MYVAR=-0.2345;MYVAR++") );

    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR=0;MYVAR++;MYVAR",      1,          eval("var MYVAR=0;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=0.2345;MYVAR++;MYVAR", 1.2345,     eval("var MYVAR=0.2345;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=-0.2345;MYVAR++;MYVAR", 0.7655,   eval("var MYVAR=-0.2345;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=0;MYVAR++;MYVAR",      1,   eval("var MYVAR=0;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=0;MYVAR++;MYVAR",      1,   eval("var MYVAR=0;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=0;MYVAR++;MYVAR",      1,   eval("var MYVAR=0;MYVAR++;MYVAR") );

    // boolean values
    // verify return value

    array[item++] = new TestCase( SECTION,    "var MYVAR=true;MYVAR++",         1,       eval("var MYVAR=true;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=false;MYVAR++",        0,      eval("var MYVAR=false;MYVAR++") );
    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR=true;MYVAR++;MYVAR",   2,   eval("var MYVAR=true;MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=false;MYVAR++;MYVAR",  1,   eval("var MYVAR=false;MYVAR++;MYVAR") );

    // boolean objects
    // verify return value

    array[item++] = new TestCase( SECTION,    "var MYVAR=new Boolean(true);MYVAR++",         1,     eval("var MYVAR=true;MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new Boolean(false);MYVAR++",        0,     eval("var MYVAR=false;MYVAR++") );
    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR=new Boolean(true);MYVAR++;MYVAR",   2,     eval("var MYVAR=new Boolean(true);MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new Boolean(false);MYVAR++;MYVAR",  1,     eval("var MYVAR=new Boolean(false);MYVAR++;MYVAR") );

    // string primitives
    array[item++] = new TestCase( SECTION,    "var MYVAR='string';MYVAR++",         Number.NaN,     eval("var MYVAR='string';MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='12345';MYVAR++",          12345,          eval("var MYVAR='12345';MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='-12345';MYVAR++",         -12345,         eval("var MYVAR='-12345';MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='0Xf';MYVAR++",            15,             eval("var MYVAR='0Xf';MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='077';MYVAR++",            77,             eval("var MYVAR='077';MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=''; MYVAR++",              0,              eval("var MYVAR='';MYVAR++") );

    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR='string';MYVAR++;MYVAR",   Number.NaN,     eval("var MYVAR='string';MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='12345';MYVAR++;MYVAR",    12346,          eval("var MYVAR='12345';MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='-12345';MYVAR++;MYVAR",   -12344,          eval("var MYVAR='-12345';MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='0xf';MYVAR++;MYVAR",      16,             eval("var MYVAR='0xf';MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='077';MYVAR++;MYVAR",      78,             eval("var MYVAR='077';MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR='';MYVAR++;MYVAR",         1,              eval("var MYVAR='';MYVAR++;MYVAR") );

    // string objects
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('string');MYVAR++",         Number.NaN,     eval("var MYVAR=new String('string');MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('12345');MYVAR++",          12345,          eval("var MYVAR=new String('12345');MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('-12345');MYVAR++",         -12345,         eval("var MYVAR=new String('-12345');MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('0Xf');MYVAR++",            15,             eval("var MYVAR=new String('0Xf');MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('077');MYVAR++",            77,             eval("var MYVAR=new String('077');MYVAR++") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String(''); MYVAR++",              0,              eval("var MYVAR=new String('');MYVAR++") );

    // verify value of variable

    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('string');MYVAR++;MYVAR",   Number.NaN,     eval("var MYVAR=new String('string');MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('12345');MYVAR++;MYVAR",    12346,          eval("var MYVAR=new String('12345');MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('-12345');MYVAR++;MYVAR",   -12344,          eval("var MYVAR=new String('-12345');MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('0xf');MYVAR++;MYVAR",      16,             eval("var MYVAR=new String('0xf');MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('077');MYVAR++;MYVAR",      78,             eval("var MYVAR=new String('077');MYVAR++;MYVAR") );
    array[item++] = new TestCase( SECTION,    "var MYVAR=new String('');MYVAR++;MYVAR",         1,              eval("var MYVAR=new String('');MYVAR++;MYVAR") );

    return ( array );
}
