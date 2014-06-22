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

    printStatus ("Unicode Characters 1C-1F negative test.");
    printBugNumber (23612);
    
    reportCompare ("error", eval ("'no'\u001C+' error'"),
                   "Unicode whitespace test (1C.)");
    reportCompare ("error", eval ("'no'\u001D+' error'"),
                   "Unicode whitespace test (1D.)");
    reportCompare ("error", eval ("'no'\u001E+' error'"),
                   "Unicode whitespace test (1E.)");
    reportCompare ("error", eval ("'no'\u001F+' error'"),
                   "Unicode whitespace test (1F.)");

    exitFunc ("test");
}
