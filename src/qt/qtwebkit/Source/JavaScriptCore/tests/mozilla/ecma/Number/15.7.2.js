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
    File Name:          15.7.2.js
    ECMA Section:       15.7.2 The Number Constructor
                        15.7.2.1
                        15.7.2.2

    Description:        15.7.2 When Number is called as part of a new
                        expression, it is a constructor:  it initializes
                        the newly created object.

                        15.7.2.1 The [[Prototype]] property of the newly
                        constructed object is set to othe original Number
                        prototype object, the one that is the initial value
                        of Number.prototype(0).  The [[Class]] property is
                        set to "Number".  The [[Value]] property of the
                        newly constructed object is set to ToNumber(value)

                        15.7.2.2 new Number().  same as in 15.7.2.1, except
                        the [[Value]] property is set to +0.

                        need to add more test cases.  see the testcases for
                        TypeConversion ToNumber.

    Author:             christine@netscape.com
    Date:               29 september 1997
*/

    var SECTION = "15.7.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Number Constructor";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    //  To verify that the object's prototype is the Number.prototype, check to see if the object's
    //  constructor property is the same as Number.prototype.constructor.

    array[item++] = new TestCase(SECTION, "(new Number()).constructor",      Number.prototype.constructor,   (new Number()).constructor );

    array[item++] = new TestCase(SECTION, "typeof (new Number())",         "object",           typeof (new Number()) );
    array[item++] = new TestCase(SECTION,  "(new Number()).valueOf()",     0,                   (new Number()).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(0)).constructor",     Number.prototype.constructor,    (new Number(0)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(0))",         "object",           typeof (new Number(0)) );
    array[item++] = new TestCase(SECTION,  "(new Number(0)).valueOf()",     0,                   (new Number(0)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(0);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(0);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(1)).constructor",     Number.prototype.constructor,    (new Number(1)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(1))",         "object",           typeof (new Number(1)) );
    array[item++] = new TestCase(SECTION,  "(new Number(1)).valueOf()",     1,                   (new Number(1)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(1);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(1);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(-1)).constructor",     Number.prototype.constructor,    (new Number(-1)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(-1))",         "object",           typeof (new Number(-1)) );
    array[item++] = new TestCase(SECTION,  "(new Number(-1)).valueOf()",     -1,                   (new Number(-1)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(-1);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(-1);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(Number.NaN)).constructor",     Number.prototype.constructor,    (new Number(Number.NaN)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(Number.NaN))",         "object",           typeof (new Number(Number.NaN)) );
    array[item++] = new TestCase(SECTION,  "(new Number(Number.NaN)).valueOf()",     Number.NaN,                   (new Number(Number.NaN)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(Number.NaN);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(Number.NaN);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number('string')).constructor",     Number.prototype.constructor,    (new Number('string')).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number('string'))",         "object",           typeof (new Number('string')) );
    array[item++] = new TestCase(SECTION,  "(new Number('string')).valueOf()",     Number.NaN,                   (new Number('string')).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number('string');NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number('string');NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(new String())).constructor",     Number.prototype.constructor,    (new Number(new String())).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(new String()))",         "object",           typeof (new Number(new String())) );
    array[item++] = new TestCase(SECTION,  "(new Number(new String())).valueOf()",     0,                   (new Number(new String())).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(new String());NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(new String());NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number('')).constructor",     Number.prototype.constructor,    (new Number('')).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(''))",         "object",           typeof (new Number('')) );
    array[item++] = new TestCase(SECTION,  "(new Number('')).valueOf()",     0,                   (new Number('')).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number('');NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number('');NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(Number.POSITIVE_INFINITY)).constructor",     Number.prototype.constructor,    (new Number(Number.POSITIVE_INFINITY)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(Number.POSITIVE_INFINITY))",         "object",           typeof (new Number(Number.POSITIVE_INFINITY)) );
    array[item++] = new TestCase(SECTION,  "(new Number(Number.POSITIVE_INFINITY)).valueOf()",     Number.POSITIVE_INFINITY,    (new Number(Number.POSITIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(Number.POSITIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(Number.POSITIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    array[item++] = new TestCase(SECTION, "(new Number(Number.NEGATIVE_INFINITY)).constructor",     Number.prototype.constructor,    (new Number(Number.NEGATIVE_INFINITY)).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number(Number.NEGATIVE_INFINITY))",         "object",           typeof (new Number(Number.NEGATIVE_INFINITY)) );
    array[item++] = new TestCase(SECTION,  "(new Number(Number.NEGATIVE_INFINITY)).valueOf()",     Number.NEGATIVE_INFINITY,                   (new Number(Number.NEGATIVE_INFINITY)).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number(Number.NEGATIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number(Number.NEGATIVE_INFINITY);NUMB.toString=Object.prototype.toString;NUMB.toString()") );


    array[item++] = new TestCase(SECTION, "(new Number()).constructor",     Number.prototype.constructor,    (new Number()).constructor );
    array[item++] = new TestCase(SECTION, "typeof (new Number())",         "object",           typeof (new Number()) );
    array[item++] = new TestCase(SECTION,  "(new Number()).valueOf()",     0,                   (new Number()).valueOf() );
    array[item++] = new TestCase(SECTION,
                                "NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()",
                                "[object Number]",
                                eval("NUMB = new Number();NUMB.toString=Object.prototype.toString;NUMB.toString()") );

    return ( array );
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