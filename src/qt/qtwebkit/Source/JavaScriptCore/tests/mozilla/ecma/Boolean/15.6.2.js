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
    File Name:          15.6.2.js
    ECMA Section:       15.6.2 The Boolean Constructor
                        15.6.2.1 new Boolean( value )
                        15.6.2.2 new Boolean()

                        This test verifies that the Boolean constructor
                        initializes a new object (typeof should return
                        "object").  The prototype of the new object should
                        be Boolean.prototype.  The value of the object
                        should be ToBoolean( value ) (a boolean value).

    Description:
    Author:             christine@netscape.com
    Date:               june 27, 1997

*/
    var SECTION = "15.6.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "15.6.2 The Boolean Constructor; 15.6.2.1 new Boolean( value ); 15.6.2.2 new Boolean()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "typeof (new Boolean(1))",         "object",            typeof (new Boolean(1)) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(1)).constructor",    Boolean.prototype.constructor,   (new Boolean(1)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(1);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(1);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(1)).valueOf()",   true,       (new Boolean(1)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(1)",         "object",   typeof new Boolean(1) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(0)).constructor",    Boolean.prototype.constructor,   (new Boolean(0)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(0);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(0);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(0)).valueOf()",   false,       (new Boolean(0)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(0)",         "object",   typeof new Boolean(0) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(-1)).constructor",    Boolean.prototype.constructor,   (new Boolean(-1)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(-1);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(-1);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(-1)).valueOf()",   true,       (new Boolean(-1)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(-1)",         "object",   typeof new Boolean(-1) );
    array[item++] = new TestCase( SECTION,   "(new Boolean('1')).constructor",    Boolean.prototype.constructor,   (new Boolean('1')).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean('1');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean('1');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean('1')).valueOf()",   true,       (new Boolean('1')).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean('1')",         "object",   typeof new Boolean('1') );
    array[item++] = new TestCase( SECTION,   "(new Boolean('0')).constructor",    Boolean.prototype.constructor,   (new Boolean('0')).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean('0');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean('0');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean('0')).valueOf()",   true,       (new Boolean('0')).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean('0')",         "object",   typeof new Boolean('0') );
    array[item++] = new TestCase( SECTION,   "(new Boolean('-1')).constructor",    Boolean.prototype.constructor,   (new Boolean('-1')).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean('-1');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean('-1');TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean('-1')).valueOf()",   true,       (new Boolean('-1')).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean('-1')",         "object",   typeof new Boolean('-1') );
    array[item++] = new TestCase( SECTION,   "(new Boolean(new Boolean(true))).constructor",    Boolean.prototype.constructor,   (new Boolean(new Boolean(true))).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(new Boolean(true));TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(new Boolean(true));TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(new Boolean(true))).valueOf()",   true,       (new Boolean(new Boolean(true))).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(new Boolean(true))",         "object",   typeof new Boolean(new Boolean(true)) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.NaN)).constructor",    Boolean.prototype.constructor,   (new Boolean(Number.NaN)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(Number.NaN);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(Number.NaN);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.NaN)).valueOf()",   false,       (new Boolean(Number.NaN)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(Number.NaN)",         "object",   typeof new Boolean(Number.NaN) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(null)).constructor",    Boolean.prototype.constructor,   (new Boolean(null)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(null);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(null);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(null)).valueOf()",   false,       (new Boolean(null)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(null)",         "object",   typeof new Boolean(null) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(void 0)).constructor",    Boolean.prototype.constructor,   (new Boolean(void 0)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(void 0);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(void 0);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(void 0)).valueOf()",   false,       (new Boolean(void 0)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(void 0)",         "object",   typeof new Boolean(void 0) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.POSITIVE_INFINITY)).constructor",    Boolean.prototype.constructor,   (new Boolean(Number.POSITIVE_INFINITY)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(Number.POSITIVE_INFINITY);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(Number.POSITIVE_INFINITY);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.POSITIVE_INFINITY)).valueOf()",   true,       (new Boolean(Number.POSITIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(Number.POSITIVE_INFINITY)",         "object",   typeof new Boolean(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.NEGATIVE_INFINITY)).constructor",    Boolean.prototype.constructor,   (new Boolean(Number.NEGATIVE_INFINITY)).constructor );
    array[item++] = new TestCase( SECTION,
                                  "TESTBOOL=new Boolean(Number.NEGATIVE_INFINITY);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean(Number.NEGATIVE_INFINITY);TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.NEGATIVE_INFINITY)).valueOf()",   true,       (new Boolean(Number.NEGATIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase( SECTION,   "typeof new Boolean(Number.NEGATIVE_INFINITY)",         "object",   typeof new Boolean(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "(new Boolean(Number.NEGATIVE_INFINITY)).constructor",    Boolean.prototype.constructor,   (new Boolean(Number.NEGATIVE_INFINITY)).constructor );
    array[item++] = new TestCase( "15.6.2.2",
                                  "TESTBOOL=new Boolean();TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()",
                                  "[object Boolean]",
                                  eval("TESTBOOL=new Boolean();TESTBOOL.toString=Object.prototype.toString;TESTBOOL.toString()") );
    array[item++] = new TestCase( "15.6.2.2",   "(new Boolean()).valueOf()",   false,       (new Boolean()).valueOf() );
    array[item++] = new TestCase( "15.6.2.2",   "typeof new Boolean()",        "object",    typeof new Boolean() );

    return ( array );
}

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );
            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
        }
        stopTest();
        return ( testcases );
}
