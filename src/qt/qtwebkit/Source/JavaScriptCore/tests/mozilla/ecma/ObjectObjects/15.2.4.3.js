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
    File Name:          15.2.4.3.js
    ECMA Section:       15.2.4.3 Object.prototype.valueOf()

    Description:        As a rule, the valueOf method for an object simply
                        returns the object; but if the object is a "wrapper"
                        for a host object, as may perhaps be created by the
                        Object constructor, then the contained host object
                        should be returned.

                        This only covers native objects.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.2.4.3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Object.prototype.valueOf()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var myarray = new Array();
    myarray.valueOf = Object.prototype.valueOf;
    var myboolean = new Boolean();
    myboolean.valueOf = Object.prototype.valueOf;
    var myfunction = new Function();
    myfunction.valueOf = Object.prototype.valueOf;
    var myobject = new Object();
    myobject.valueOf = Object.prototype.valueOf;
    var mymath = Math;
    mymath.valueOf = Object.prototype.valueOf;
    var mydate = new Date();
    mydate.valueOf = Object.prototype.valueOf;
    var mynumber = new Number();
    mynumber.valueOf = Object.prototype.valueOf;
    var mystring = new String();
    mystring.valueOf = Object.prototype.valueOf;

    array[item++] = new TestCase( SECTION,  "Object.prototype.valueOf.length",      0,      Object.prototype.valueOf.length );

    array[item++] = new TestCase( SECTION,
                                 "myarray = new Array(); myarray.valueOf = Object.prototype.valueOf; myarray.valueOf()",
                                 myarray,
                                 myarray.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "myboolean = new Boolean(); myboolean.valueOf = Object.prototype.valueOf; myboolean.valueOf()",
                                 myboolean,
                                 myboolean.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "myfunction = new Function(); myfunction.valueOf = Object.prototype.valueOf; myfunction.valueOf()",
                                 myfunction,
                                 myfunction.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "myobject = new Object(); myobject.valueOf = Object.prototype.valueOf; myobject.valueOf()",
                                 myobject,
                                 myobject.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "mymath = Math; mymath.valueOf = Object.prototype.valueOf; mymath.valueOf()",
                                 mymath,
                                 mymath.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "mynumber = new Number(); mynumber.valueOf = Object.prototype.valueOf; mynumber.valueOf()",
                                 mynumber,
                                 mynumber.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "mystring = new String(); mystring.valueOf = Object.prototype.valueOf; mystring.valueOf()",
                                 mystring,
                                 mystring.valueOf() );
    array[item++] = new TestCase( SECTION,
                                 "mydate = new Date(); mydate.valueOf = Object.prototype.valueOf; mydate.valueOf()",
                                 mydate,
                                 mydate.valueOf() );
    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
