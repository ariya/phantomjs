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
 *  File Name:   eval-002.js
 *  Description:  (SEE REVISED DESCRIPTION FURTHER BELOW)
 *
 *  The global eval function may not be accessed indirectly and then called.
 *  This feature will continue to work in JavaScript 1.3 but will result in an
 *  error in JavaScript 1.4. This restriction is also in place for the With and
 *  Closure constructors.
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=324451
 *
 *  Author:          christine@netscape.com
 *  Date:               11 August 1998
 *
 *
 *  REVISION:    05  February 2001
 *  Author:          pschwartau@netscape.com
 *  
 *  Indirect eval IS NOT ILLEGAL per ECMA3!!!  See
 *
 *  http://bugzilla.mozilla.org/show_bug.cgi?id=38512
 *
 *  ------- Additional Comments From Brendan Eich 2001-01-30 17:12 ------- 
 * ECMA-262 Edition 3 doesn't require implementations to throw EvalError, 
 * see the short, section-less Chapter 16.  It does say an implementation that
 * doesn't throw EvalError must allow assignment to eval and indirect calls 
 * of the evalnative method.
 *
 */
    var SECTION = "eval-002.js";
    var VERSION = "JS1_4";
    var TITLE   = "Calling eval indirectly should NOT fail in version 140";
    var BUGNUMBER="38512";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var MY_EVAL = eval;
    var RESULT = "";
    var EXPECT = 1 + "testString"

    EvalTest();

    test();


function EvalTest() 
{    
   MY_EVAL( "RESULT = EXPECT" );
 
    testcases[tc++] = new TestCase(
        SECTION,
        "Call eval indirectly",
        EXPECT,
        RESULT );
}

