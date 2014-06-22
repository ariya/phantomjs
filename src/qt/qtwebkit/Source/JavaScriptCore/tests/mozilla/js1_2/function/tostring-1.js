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

    var SECTION = "tostring-1";
    var VERSION = "JS1_2";
    startTest();
    var TITLE   = "Function.toString()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var tab = "    ";

    t1 = new TestFunction( "stub", "value", tab + "return value;" );

    t2 = new TestFunction( "ToString", "object", tab+"return object + \"\";" );

    t3 = new TestFunction( "Add", "a, b, c, d, e",  tab +"var s = a + b + c + d + e;\n" +
                        tab + "return s;" );

    t4 = new TestFunction( "noop", "value" );

    t5 = new TestFunction( "anonymous", "", tab+"return \"hello!\";" );

    var f = new Function( "return \"hello!\"");

    testcases[tc++] = new TestCase( SECTION,
                                    "stub.toString()",
                                    simplify(t1.valueOf()),
                                    simplify(stub.toString()) );

    testcases[tc++] = new TestCase( SECTION,
                                    "ToString.toString()",
                                    simplify(t2.valueOf()),
                                    simplify(ToString.toString()) );

    testcases[tc++] = new TestCase( SECTION,
                                    "Add.toString()",
                                    simplify(t3.valueOf()),
                                    simplify(Add.toString()) );

    testcases[tc++] = new TestCase( SECTION,
                                    "noop.toString()",
                                    simplify(t4.toString()),
                                    simplify(noop.toString()) );

    testcases[tc++] = new TestCase( SECTION,
                                    "f.toString()",
                                    simplify(t5.toString()),
                                    simplify(f.toString()) );
    test();

function noop( value ) {
}
function Add( a, b, c, d, e ) {
    var s = a + b + c + d + e;
    return s;
}
function stub( value ) {
    return value;
}
function ToString( object ) {
    return object + "";
}

function ToBoolean( value ) {
    if ( value == 0 || value == NaN || value == false ) {
        return false;
    } else {
        return true;
    }
}

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
    if ( name == "anonymous" && version() == 120 ) {
        name = "";
    }

    this.name = name;
    this.arguments = args.toString();
    this.body = body;

    /* the format of Function.toString() in JavaScript 1.2 is:
    /n
    function name ( arguments ) {
        body
    }
    */
    this.value = "\nfunction " + (name ? name : "" )+
    "("+args+") {\n"+ (( body ) ? body +"\n" : "") + "}\n";

    this.toString = new Function( "return this.value" );
    this.valueOf = new Function( "return this.value" );
    return this;
}
