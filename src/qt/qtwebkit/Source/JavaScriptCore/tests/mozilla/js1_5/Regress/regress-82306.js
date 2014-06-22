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
* Contributor(s): pschwartau@netscape.com, epstein@tellme.com
* Date: 23 May 2001
*
* SUMMARY: Regression test for Bugzilla bug 82306
* See http://bugzilla.mozilla.org/show_bug.cgi?id=82306
*
* This test used to crash the JS engine. This was discovered
* by Mike Epstein <epstein@tellme.com>
*/
//-------------------------------------------------------------------------------------------------
var bug = 82306;
var summary = "Testing we don't crash on encodeURI()";
var URI = '';


//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  URI += '<?xml version="1.0"?>';
  URI += '<zcti application="xxxx_demo">';
  URI += '<pstn_data>';
  URI += '<ani>650-930-xxxx</ani>';
  URI += '<dnis>877-485-xxxx</dnis>';
  URI += '</pstn_data>';
  URI += '<keyvalue key="name" value="xxx"/>';
  URI += '<keyvalue key="phone" value="6509309000"/>';
  URI += '</zcti>';

  // Just testing that we don't crash on this
  encodeURI(URI);

  exitFunc ('test');
}
