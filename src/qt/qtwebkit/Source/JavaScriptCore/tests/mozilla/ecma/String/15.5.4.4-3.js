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
    File Name:          15.5.4.4-3.js
    ECMA Section:       15.5.4.4 String.prototype.charAt(pos)
    Description:        Returns a string containing the character at position
                        pos in the string.  If there is no character at that
                        string, the result is the empty string.  The result is
                        a string value, not a String object.

                        When the charAt method is called with one argument,
                        pos, the following steps are taken:
                        1. Call ToString, with this value as its argument
                        2. Call ToInteger pos
                        3. Compute the number of characters  in Result(1)
                        4. If Result(2) is less than 0 is or not less than
                           Result(3), return the empty string
                        5. Return a string of length 1 containing one character
                           from result (1), the character at position Result(2).

                        Note that the charAt function is intentionally generic;
                        it does not require that its this value be a String
                        object.  Therefore it can be transferred to other kinds
                        of objects for use as a method.

                        This tests assiging charAt to a user-defined function.

    Author:             christine@netscape.com
    Date:               2 october 1997
*/
    var SECTION = "15.5.4.4-3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.charAt";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function MyObject (v) {
    this.value      = v;
    this.toString   = new Function( "return this.value +'';" );
    this.valueOf    = new Function( "return this.value" );
    this.charAt     = String.prototype.charAt;
}
function getTestCases() {
    var array = new Array();
    var item = 0;

    var foo = new MyObject('hello');


    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "h", foo.charAt(0)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "e", foo.charAt(1)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "l", foo.charAt(2)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "l", foo.charAt(3)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "o", foo.charAt(4)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "",  foo.charAt(-1)  );
    array[item++] = new TestCase( SECTION, "var foo = new MyObject('hello'); ", "",  foo.charAt(5)  );

    var boo = new MyObject(true);

    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true); ", "t", boo.charAt(0)  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true); ", "r", boo.charAt(1)  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true); ", "u", boo.charAt(2)  );
    array[item++] = new TestCase( SECTION, "var boo = new MyObject(true); ", "e", boo.charAt(3)  );

    var noo = new MyObject( Math.PI );

    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "3", noo.charAt(0)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", ".", noo.charAt(1)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "1", noo.charAt(2)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "4", noo.charAt(3)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "1", noo.charAt(4)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "5", noo.charAt(5)  );
    array[item++] = new TestCase( SECTION, "var noo = new MyObject(Math.PI); ", "9", noo.charAt(6)  );

    return array;
}

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+
                    testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed )
                                    ? ""
                                    : "wrong value "
        }
        stopTest();

    //  all tests must return a boolean value
        return ( testcases );
}
