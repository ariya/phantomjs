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
    File Name:          15.5.2.js
    ECMA Section:       15.5.2 The String Constructor
            			15.5.2.1 new String(value)
			            15.5.2.2 new String()

    Description:	When String is called as part of a new expression, it
                    is a constructor; it initializes the newly constructed
                    object.

        			- The prototype property of the newly constructed
		        	object is set to the original String prototype object,
		        	the one that is the intial value of String.prototype
        			- The internal [[Class]] property of the object is "String"
        			- The value of the object is ToString(value).
        			- If no value is specified, its value is the empty string.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/

    var SECTION = "15.5.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The String Constructor";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,	"typeof new String('string primitive')",	    "object",	        typeof new String('string primitive') );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String('string primitive'); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String('string primitive'); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String('string primitive')).valueOf()",   'string primitive', (new String('string primitive')).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String('string primitive')).substring",   String.prototype.substring,   (new String('string primitive')).substring );

    array[item++] = new TestCase( SECTION,	"typeof new String(void 0)",	                "object",	        typeof new String(void 0) );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(void 0); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(void 0); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String(void 0)).valueOf()",               "undefined", (new String(void 0)).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String(void 0)).toString",               String.prototype.toString,   (new String(void 0)).toString );

    array[item++] = new TestCase( SECTION,	"typeof new String(null)",	            "object",	        typeof new String(null) );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(null); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(null); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String(null)).valueOf()",         "null",             (new String(null)).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String(null)).valueOf",         String.prototype.valueOf,   (new String(null)).valueOf );

    array[item++] = new TestCase( SECTION,	"typeof new String(true)",	            "object",	        typeof new String(true) );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(true); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(true); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String(true)).valueOf()",         "true",             (new String(true)).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String(true)).charAt",         String.prototype.charAt,   (new String(true)).charAt );

    array[item++] = new TestCase( SECTION,	"typeof new String(false)",	            "object",	        typeof new String(false) );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(false); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(false); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String(false)).valueOf()",        "false",            (new String(false)).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String(false)).charCodeAt",        String.prototype.charCodeAt,   (new String(false)).charCodeAt );

    array[item++] = new TestCase( SECTION,	"typeof new String(new Boolean(true))",	       "object",	        typeof new String(new Boolean(true)) );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(new Boolean(true)); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(new Boolean(true)); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String(new Boolean(true))).valueOf()",   "true",              (new String(new Boolean(true))).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String(new Boolean(true))).indexOf",   String.prototype.indexOf,    (new String(new Boolean(true))).indexOf );

    array[item++] = new TestCase( SECTION,	"typeof new String()",	                        "object",	        typeof new String() );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String()).valueOf()",   '',                 (new String()).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String()).lastIndexOf",   String.prototype.lastIndexOf,   (new String()).lastIndexOf );

    array[item++] = new TestCase( SECTION,	"typeof new String('')",	    "object",	        typeof new String('') );
    array[item++] = new TestCase( SECTION,	"var TESTSTRING = new String(''); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()", "[object String]",   eval("var TESTSTRING = new String(''); TESTSTRING.toString=Object.prototype.toString;TESTSTRING.toString()") );
    array[item++] = new TestCase( SECTION,  "(new String('')).valueOf()",   '',                 (new String('')).valueOf() );
    array[item++] = new TestCase( SECTION,  "(new String('')).split",   String.prototype.split,   (new String('')).split );

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
