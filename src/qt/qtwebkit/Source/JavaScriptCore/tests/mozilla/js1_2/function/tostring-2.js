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
    File Name:          tostring-1.js
    Section:            Function.toString
    Description:

    Since the behavior of Function.toString() is implementation-dependent,
    toString tests for function are not in the ECMA suite.

    Currently, an attempt to parse the toString output for some functions
    and verify that the result is something reasonable.

    This verifies
    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=99212

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

// These test cases should not be testing for a particular
// whitespace formatting (this is implementation defined).
// Strip out whitespace, or in the case of whitespace
// abutting a word character reduce to a single space.
function simplify(str)
{
    return str.replace(/\s+/g, " ").replace(/ (\W)/g, "$1").replace(/(\W) /g, "$1").trim();
}

    var SECTION = "tostring-2";
    var VERSION = "JS1_2";
    startTest();
    var TITLE   = "Function.toString()";
    var BUGNUMBER="123444";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var tab = "    ";


var equals = new TestFunction( "Equals", "a, b", tab+ "return a == b;" );
function Equals (a, b) {
    return a == b;
}

var reallyequals = new TestFunction( "ReallyEquals", "a, b",
    ( version() <= 120 )  ? tab +"return a == b;" : tab +"return a === b;" );
function ReallyEquals( a, b ) {
    return a === b;
}

var doesntequal = new TestFunction( "DoesntEqual", "a, b", tab + "return a != b;" );
function DoesntEqual( a, b ) {
    return a != b;
}

var reallydoesntequal = new TestFunction( "ReallyDoesntEqual", "a, b",
    ( version() <= 120 ) ? tab +"return a != b;"  : tab +"return a !== b;" );
function ReallyDoesntEqual( a, b ) {
    return a !== b;
}

// Modified to match expected results; JSC won't automatically insert redundant braces into the result.
var testor = new TestFunction( "TestOr", "a",  tab+"if (a == null || a == void 0) {\n"+
    tab +tab+"return 0;\n"+tab+"} else {\n"+tab+tab+"return a;\n"+tab+"}" );
function TestOr( a ) {
 if ( a == null || a == void 0 ) {
    return 0;
 } else {
    return a;
 }
}

// Modified to match expected results; JSC won't automatically insert redundant braces into the result.
var testand = new TestFunction( "TestAnd", "a", tab+"if (a != null && a != void 0) {\n"+
    tab+tab+"return a;\n" + tab+ "} else {\n"+tab+tab+"return 0;\n"+tab+"}" );
function TestAnd( a ) {
 if ( a != null && a != void 0 ) {
    return a;
 } else {
    return 0;
 }
}

var or = new TestFunction( "Or", "a, b", tab + "return a | b;" );
function Or( a, b ) {
    return a | b;
}

var and = new TestFunction( "And", "a, b", tab + "return a & b;" );
function And( a, b ) {
    return a & b;
}

var xor = new TestFunction( "XOr", "a, b", tab + "return a ^ b;" );
function XOr( a, b ) {
    return a ^ b;
}

    testcases[testcases.length] = new TestCase( SECTION,
        "Equals.toString()",
        simplify(equals.valueOf()),
        simplify(Equals.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "ReallyEquals.toString()",
        simplify(reallyequals.valueOf()),
        simplify(ReallyEquals.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "DoesntEqual.toString()",
        simplify(doesntequal.valueOf()),
        simplify(DoesntEqual.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "ReallyDoesntEqual.toString()",
        simplify(reallydoesntequal.valueOf()),
        simplify(ReallyDoesntEqual.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "TestOr.toString()",
        simplify(testor.valueOf()),
        simplify(TestOr.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "TestAnd.toString()",
        simplify(testand.valueOf()),
        simplify(TestAnd.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "Or.toString()",
        simplify(or.valueOf()),
        simplify(Or.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "And.toString()",
        simplify(and.valueOf()),
        simplify(And.toString()) );

    testcases[testcases.length] = new TestCase( SECTION,
        "XOr.toString()",
        simplify(xor.valueOf()),
        simplify(XOr.toString()) );

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
function TestFunction( name, args, body ) {
    this.name = name;
    this.arguments = args.toString();
    this.body = body;

    /* the format of Function.toString() in JavaScript 1.2 is:
    /n
    function name ( arguments ) {
        body
    }
    */
    this.value = "\nfunction " + (name ? name : "anonymous" )+
    "("+args+") {\n"+ (( body ) ? body +"\n" : "") + "}\n";

    this.toString = new Function( "return this.value" );
    this.valueOf = new Function( "return this.value" );
    return this;
}
