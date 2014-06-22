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

    printStatus ("Unicode Characters 1C-1F with regexps test.");
    printBugNumber (23612);
    
    var ary = ["\u001Cfoo", "\u001Dfoo", "\u001Efoo", "\u001Ffoo"];
    
    for (var i in ary)
    {       
        reportCompare (0, ary[Number(i)].search(/^\Sfoo$/),
                       "Unicode characters 1C-1F in regexps, ary[" +
                       i + "] did not match \\S test (it should not.)");
        reportCompare (-1, ary[Number(i)].search(/^\sfoo$/),
                       "Unicode characters 1C-1F in regexps, ary[" +
                       i + "] matched \\s test (it should not.)");
    }

    exitFunc ("test");
}
