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
    File Name:          15.2.2.1.js
    ECMA Section:       15.2.2.1 The Object Constructor:  new Object( value )

    1.If the type of the value is not Object, go to step 4.
    2.If the value is a native ECMAScript object, do not create a new object; simply return value.
    3.If the value is a host object, then actions are taken and a result is returned in an
      implementation-dependent manner that may depend on the host object.
    4.If the type of the value is String, return ToObject(value).
    5.If the type of the value is Boolean, return ToObject(value).
    6.If the type of the value is Number, return ToObject(value).
    7.(The type of the value must be Null or Undefined.) Create a new native ECMAScript object.
      The [[Prototype]] property of the newly constructed object is set to the Object prototype object.
      The [[Class]] property of the newly constructed object is set to "Object".
      The newly constructed object has no [[Value]] property.
      Return the newly created native object.

    Description:        This does not test cases where the object is a host object.
    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.2.2.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "new Object( value )";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "typeof new Object(null)",      "object",           typeof new Object(null) );
    array[item++] = new TestCase( SECTION,  "MYOB = new Object(null); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Object]",   eval("MYOB = new Object(null); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

    array[item++] = new TestCase( SECTION,  "typeof new Object(void 0)",      "object",           typeof new Object(void 0) );
    array[item++] = new TestCase( SECTION,  "MYOB = new Object(new Object(void 0)); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Object]",   eval("MYOB = new Object(new Object(void 0)); MYOB.toString = Object.prototype.toString; MYOB.toString()") );

    array[item++] = new TestCase( SECTION,  "typeof new Object('string')",      "object",           typeof new Object('string') );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object('string'); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object String]",   eval("MYOB = new Object('string'); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object('string').valueOf()",  "string",           (new Object('string')).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object('')",            "object",           typeof new Object('') );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object String]",   eval("MYOB = new Object(''); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object('').valueOf()",        "",                 (new Object('')).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(Number.NaN)",      "object",                 typeof new Object(Number.NaN) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(Number.NaN); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(Number.NaN).valueOf()",  Number.NaN,               (new Object(Number.NaN)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(0)",      "object",                 typeof new Object(0) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(0).valueOf()",  0,               (new Object(0)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(-0)",      "object",                 typeof new Object(-0) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(-0); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(-0).valueOf()",  -0,               (new Object(-0)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(1)",      "object",                 typeof new Object(1) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(1).valueOf()",  1,               (new Object(1)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(-1)",      "object",                 typeof new Object(-1) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Number]",   eval("MYOB = new Object(-1); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(-1).valueOf()",  -1,               (new Object(-1)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(true)",      "object",                 typeof new Object(true) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(true); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(true).valueOf()",  true,               (new Object(true)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(false)",      "object",              typeof new Object(false) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(false); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(false).valueOf()",  false,                 (new Object(false)).valueOf() );

    array[item++] = new TestCase( SECTION,  "typeof new Object(Boolean())",         "object",               typeof new Object(Boolean()) );
    array[item++] = new TestCase( SECTION,  "MYOB = (new Object(Boolean()); MYOB.toString = Object.prototype.toString; MYOB.toString()",  "[object Boolean]",   eval("MYOB = new Object(Boolean()); MYOB.toString = Object.prototype.toString; MYOB.toString()") );
    array[item++] = new TestCase( SECTION,  "(new Object(Boolean()).valueOf()",     Boolean(),              (new Object(Boolean())).valueOf() );


    var myglobal    = this;
    var myobject    = new Object( "my new object" );
    var myarray     = new Array();
    var myboolean   = new Boolean();
    var mynumber    = new Number();
    var mystring    = new String();
    var myobject    = new Object();
    var myfunction  = new Function( "x", "return x");
    var mymath      = Math;

    array[item++] = new TestCase( SECTION, "myglobal = new Object( this )",                     myglobal,       new Object(this) );
    array[item++] = new TestCase( SECTION, "myobject = new Object('my new object'); new Object(myobject)",            myobject,       new Object(myobject) );
    array[item++] = new TestCase( SECTION, "myarray = new Array(); new Object(myarray)",        myarray,        new Object(myarray) );
    array[item++] = new TestCase( SECTION, "myboolean = new Boolean(); new Object(myboolean)",  myboolean,      new Object(myboolean) );
    array[item++] = new TestCase( SECTION, "mynumber = new Number(); new Object(mynumber)",     mynumber,       new Object(mynumber) );
    array[item++] = new TestCase( SECTION, "mystring = new String9); new Object(mystring)",     mystring,       new Object(mystring) );
    array[item++] = new TestCase( SECTION, "myobject = new Object(); new Object(mynobject)",    myobject,       new Object(myobject) );
    array[item++] = new TestCase( SECTION, "myfunction = new Function(); new Object(myfunction)", myfunction,   new Object(myfunction) );
    array[item++] = new TestCase( SECTION, "mymath = Math; new Object(mymath)",                 mymath,         new Object(mymath) );

    return ( array );
}
function test() {
    for (tc = 0 ; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";

    }

    stopTest();
    return ( testcases );
}
