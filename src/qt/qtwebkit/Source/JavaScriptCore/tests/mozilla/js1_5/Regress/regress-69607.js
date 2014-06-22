/*
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
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com  
* Date: 21 Feb 2001
* See http://bugzilla.mozilla.org/show_bug.cgi?id=69607
*
* SUMMARY:  testing that we don't crash on trivial JavaScript
*
*/
//-------------------------------------------------------------------------------------------------
var bug = 69607;
var summary = "Testing that we don't crash on trivial JavaScript";
var var1;
var var2;
var var3;

printBugNumber (bug);
printStatus (summary);

/*
 * The crash this bug reported was caused by precisely these lines
 * placed in top-level code (i.e. not wrapped inside a function) - 
*/
if(false) 
{
  var1 = 0;
}
else
{
  var2 = 0;
}

if(false)
{
  var3 = 0;
}

