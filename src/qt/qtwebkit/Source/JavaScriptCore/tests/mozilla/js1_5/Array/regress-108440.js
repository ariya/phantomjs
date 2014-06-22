/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2001
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com, Liorean@user.bip.net
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date: 30 October 2001
* SUMMARY: Regression test for bug 108440
* See http://bugzilla.mozilla.org/show_bug.cgi?id=108440
*
* We shouldn't crash trying to add an array as an element of itself (!)
*
* Brendan: "...it appears that Array.prototype.toString is unsafe,
* and what's more, ECMA-262 Edition 3 has no helpful words about
* avoiding recursive death on a cycle."
*/
//-----------------------------------------------------------------------------
var bug = 108440;
var summary = "Shouldn't crash trying to add an array as an element of itself";
var self = this;
var temp = '';

printBugNumber(bug);
printStatus(summary);

/*
 * Explicit test:
 */
var a=[];
temp = (a[a.length]=a);

/*
 * Implicit test (one of the properties of |self| is |a|)
 */
a=[];
for(var prop in self)
{
  temp = prop;
  temp = (a[a.length] = self[prop]);
}

/*
 * Stressful explicit test
 */
a=[];
for (var i=0; i<10; i++)
{
  a[a.length] = a;
}

/*
 * Test toString()
 */
a=[];
for (var i=0; i<10; i++)
{
  a[a.length] = a.toString();
}

/*
 * Test toSource() - but Rhino doesn't have this, so try...catch it
 */
a=[];
try
{
  for (var i=0; i<10; i++)
  {
    a[a.length] = a.toSource();
  }
}
catch(e)
{
}
