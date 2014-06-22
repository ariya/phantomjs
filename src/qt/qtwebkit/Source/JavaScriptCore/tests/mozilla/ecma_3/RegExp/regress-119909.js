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
* Contributor(s): 1010mozilla@Ostermiller.com, pschwartau@netscape.com
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
* Date:    14 Jan 2002
* SUMMARY: Shouldn't crash on regexps with many nested parentheses
* See http://bugzilla.mozilla.org/show_bug.cgi?id=119909
*
*/
//-----------------------------------------------------------------------------
var bug = 119909;
var summary = "Shouldn't crash on regexps with many nested parentheses";
var NO_BACKREFS = false;
var DO_BACKREFS = true;


//--------------------------------------------------
test();
//--------------------------------------------------


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  // Changed the parameter from 500 to 200 for WebKit, because PCRE reports an error for more parentheses.
  testThis(200, NO_BACKREFS, 'hello', 'goodbye');
  testThis(200, DO_BACKREFS, 'hello', 'goodbye');

  exitFunc('test');
}


/*
 * Creates a regexp pattern like (((((((((hello)))))))))
 * and tests str.search(), str.match(), str.replace()
 */
function testThis(numParens, doBackRefs, strOriginal, strReplace)
{
  var openParen = doBackRefs? '(' : '(?:';
  var closeParen = ')';
  var pattern = '';
  
  for (var i=0; i<numParens; i++) {pattern += openParen;}
  pattern += strOriginal;
  for (i=0; i<numParens; i++) {pattern += closeParen;}
  var re = new RegExp(pattern);

  var res = strOriginal.search(re);
  res = strOriginal.match(re);
  res = strOriginal.replace(re, strReplace);
}
