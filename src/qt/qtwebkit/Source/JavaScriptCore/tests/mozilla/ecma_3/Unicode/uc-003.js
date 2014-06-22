/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
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
 * Rob Ginda rginda@netscape.com
 */

test();

function test()
{
    enterFunc ("test");

    var \u0041 = 5;
    var A\u03B2 = 15;
    var c\u0061se = 25;

    printStatus ("Escapes in identifiers test.");
    printBugNumber (23608);
    printBugNumber (23607);

    reportCompare (5, eval("\u0041"),
                   "Escaped ASCII Identifier test.");
    reportCompare (6, eval("++\u0041"),
                   "Escaped ASCII Identifier test");
    reportCompare (15, eval("A\u03B2"),
                   "Escaped non-ASCII Identifier test");
    reportCompare (16, eval("++A\u03B2"),
                   "Escaped non-ASCII Identifier test");
    reportCompare (25, eval("c\\u00" + "61se"),
                   "Escaped keyword Identifier test");
    reportCompare (26, eval("++c\\u00" + "61se"),
                   "Escaped keyword Identifier test");
    
    exitFunc ("test");
}
