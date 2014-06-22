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
    File Name:          15.8.2.7.js
    ECMA Section:       15.8.2.7 cos( x )
    Description:        return an approximation to the cosine of the
                        argument.  argument is expressed in radians
    Author:             christine@netscape.com
    Date:               7 july 1997

*/

    var SECTION = "15.8.2.7";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.cos(x)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "Math.cos.length",       1,                      Math.cos.length );

    array[item++] = new TestCase( SECTION, "Math.cos()",            Number.NaN,             Math.cos() );
    array[item++] = new TestCase( SECTION, "Math.cos(void 0)",      Number.NaN,             Math.cos(void 0) );
    array[item++] = new TestCase( SECTION, "Math.cos(false)",       1,                      Math.cos(false) );
    array[item++] = new TestCase( SECTION, "Math.cos(null)",        1,                      Math.cos(null) );

    array[item++] = new TestCase( SECTION, "Math.cos('0')",         1,                      Math.cos('0') );
    array[item++] = new TestCase( SECTION, "Math.cos('Infinity')",  Number.NaN,             Math.cos("Infinity") );
    array[item++] = new TestCase( SECTION, "Math.cos('3.14159265359')",  -1,                 Math.cos('3.14159265359') );

    array[item++] = new TestCase( SECTION, "Math.cos(NaN)",          Number.NaN,             Math.cos(Number.NaN)        );
    array[item++] = new TestCase( SECTION, "Math.cos(0)",            1,                      Math.cos(0)                 );
    array[item++] = new TestCase( SECTION, "Math.cos(-0)",           1,                      Math.cos(-0)                );
    array[item++] = new TestCase( SECTION, "Math.cos(Infinity)",     Number.NaN,             Math.cos(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "Math.cos(-Infinity)",    Number.NaN,             Math.cos(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "Math.cos(0.7853981633974)",	0.7071067811865,    Math.cos(0.7853981633974)   );
    array[item++] = new TestCase( SECTION, "Math.cos(1.570796326795)",   0,                  Math.cos(1.570796326795)    );
    array[item++] = new TestCase( SECTION, "Math.cos(2.356194490192)",	-0.7071067811865,   Math.cos(2.356194490192)    );
    array[item++] = new TestCase( SECTION, "Math.cos(3.14159265359)",	-1,                 Math.cos(3.14159265359)     );
    array[item++] = new TestCase( SECTION, "Math.cos(3.926990816987)",	-0.7071067811865,   Math.cos(3.926990816987)    );
    array[item++] = new TestCase( SECTION, "Math.cos(4.712388980385)",	0,                  Math.cos(4.712388980385)    );
    array[item++] = new TestCase( SECTION, "Math.cos(5.497787143782)",	0.7071067811865,    Math.cos(5.497787143782)    );
    array[item++] = new TestCase( SECTION, "Math.cos(Math.PI*2)",	    1,                  Math.cos(Math.PI*2)         );
    array[item++] = new TestCase( SECTION, "Math.cos(Math.PI/4)",	    Math.SQRT2/2,       Math.cos(Math.PI/4)         );
    array[item++] = new TestCase( SECTION, "Math.cos(Math.PI/2)",	    0,                  Math.cos(Math.PI/2)         );
    array[item++] = new TestCase( SECTION, "Math.cos(3*Math.PI/4)",	    -Math.SQRT2/2,      Math.cos(3*Math.PI/4)       );
    array[item++] = new TestCase( SECTION, "Math.cos(Math.PI)",	        -1,                 Math.cos(Math.PI)           );
    array[item++] = new TestCase( SECTION, "Math.cos(5*Math.PI/4)",	    -Math.SQRT2/2,      Math.cos(5*Math.PI/4)       );
    array[item++] = new TestCase( SECTION, "Math.cos(3*Math.PI/2)",	    0,                  Math.cos(3*Math.PI/2)       );
    array[item++] = new TestCase( SECTION, "Math.cos(7*Math.PI/4)",	    Math.SQRT2/2,       Math.cos(7*Math.PI/4)       );
    array[item++] = new TestCase( SECTION, "Math.cos(Math.PI*2)",	    1,                  Math.cos(2*Math.PI)         );
    array[item++] = new TestCase( SECTION, "Math.cos(-0.7853981633974)",	0.7071067811865,    Math.cos(-0.7853981633974)  );
    array[item++] = new TestCase( SECTION, "Math.cos(-1.570796326795)",	0,                  Math.cos(-1.570796326795)   );
    array[item++] = new TestCase( SECTION, "Math.cos(-2.3561944901920)",	-.7071067811865,    Math.cos(2.3561944901920)   );
    array[item++] = new TestCase( SECTION, "Math.cos(-3.14159265359)",	-1,                 Math.cos(3.14159265359)     );
    array[item++] = new TestCase( SECTION, "Math.cos(-3.926990816987)",	-0.7071067811865,   Math.cos(3.926990816987)    );
    array[item++] = new TestCase( SECTION, "Math.cos(-4.712388980385)",	0,                  Math.cos(4.712388980385)    );
    array[item++] = new TestCase( SECTION, "Math.cos(-5.497787143782)",	0.7071067811865,    Math.cos(5.497787143782)    );
    array[item++] = new TestCase( SECTION, "Math.cos(-6.28318530718)",	1,                  Math.cos(6.28318530718)     );
    array[item++] = new TestCase( SECTION, "Math.cos(-Math.PI/4)",	    Math.SQRT2/2,       Math.cos(-Math.PI/4)        );
    array[item++] = new TestCase( SECTION, "Math.cos(-Math.PI/2)",	    0,                  Math.cos(-Math.PI/2)        );
    array[item++] = new TestCase( SECTION, "Math.cos(-3*Math.PI/4)",	    -Math.SQRT2/2,      Math.cos(-3*Math.PI/4)      );
    array[item++] = new TestCase( SECTION, "Math.cos(-Math.PI)",	        -1,                 Math.cos(-Math.PI)          );
    array[item++] = new TestCase( SECTION, "Math.cos(-5*Math.PI/4)",	    -Math.SQRT2/2,      Math.cos(-5*Math.PI/4)      );
    array[item++] = new TestCase( SECTION, "Math.cos(-3*Math.PI/2)",	    0,                  Math.cos(-3*Math.PI/2)      );
    array[item++] = new TestCase( SECTION, "Math.cos(-7*Math.PI/4)",	    Math.SQRT2/2,       Math.cos(-7*Math.PI/4)      );
    array[item++] = new TestCase( SECTION, "Math.cos(-Math.PI*2)",	    1,                  Math.cos(-Math.PI*2)        );

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
