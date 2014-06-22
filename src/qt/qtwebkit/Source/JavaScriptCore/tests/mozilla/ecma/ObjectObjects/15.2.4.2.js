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
    File Name:          15.2.4.2.js
    ECMA Section:       15.2.4.2 Object.prototype.toString()

    Description:        When the toString method is called, the following
                        steps are taken:
                        1.  Get the [[Class]] property of this object
                        2.  Call ToString( Result(1) )
                        3.  Compute a string value by concatenating the three
                            strings "[object " + Result(2) + "]"
                        4.  Return Result(3).

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.2.4.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Object.prototype.toString()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "(new Object()).toString()",    "[object Object]",  (new Object()).toString() );

    array[item++] = new TestCase( SECTION,  "myvar = this;  myvar.toString = Object.prototype.toString; myvar.toString()",
                                            GLOBAL,
                                            eval("myvar = this;  myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = MyObject; myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Function]",
                                            eval("myvar = MyObject; myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new MyObject( true ); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            '[object Object]',
                                            eval("myvar = new MyObject( true ); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new Number(0); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Number]",
                                            eval("myvar = new Number(0); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new String(''); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object String]",
                                            eval("myvar = new String(''); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = Math; myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Math]",
                                            eval("myvar = Math; myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new Function(); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Function]",
                                            eval("myvar = new Function(); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new Array(); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Array]",
                                            eval("myvar = new Array(); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new Boolean(); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Boolean]",
                                            eval("myvar = new Boolean(); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "myvar = new Date(); myvar.toString = Object.prototype.toString; myvar.toString()",
                                            "[object Date]",
                                            eval("myvar = new Date(); myvar.toString = Object.prototype.toString; myvar.toString()") );

    array[item++] = new TestCase( SECTION,  "var MYVAR = new Object( this ); MYVAR.toString()",
                                            GLOBAL,
                                            eval("var MYVAR = new Object( this ); MYVAR.toString()") );

    array[item++] = new TestCase( SECTION,  "var MYVAR = new Object(); MYVAR.toString()",
                                            "[object Object]",
                                            eval("var MYVAR = new Object(); MYVAR.toString()") );

    array[item++] = new TestCase( SECTION,  "var MYVAR = new Object(void 0); MYVAR.toString()",
                                            "[object Object]",
                                            eval("var MYVAR = new Object(void 0); MYVAR.toString()") );

    array[item++] = new TestCase( SECTION,  "var MYVAR = new Object(null); MYVAR.toString()",
                                            "[object Object]",
                                            eval("var MYVAR = new Object(null); MYVAR.toString()") );

    return ( array );
}
function test( array ) {
    for ( tc=0 ; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}

function MyObject( value ) {
    this.value = new Function( "return this.value" );
    this.toString = new Function ( "return this.value+''");
}