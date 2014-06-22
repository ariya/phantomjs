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
* Portions created by the Initial Developer are Copyright (C) 2003
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): briang@tonic.com, igor@fastmail.fm, pschwartau@netscape.com
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
* Date:    29 April 2003
* SUMMARY: eval() is not a constructor, but don't crash on |new eval();|
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=204210
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 204210;
var summary = "eval() is not a constructor, but don't crash on |new eval();|";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

printBugNumber(bug);
printStatus(summary);

/*
 * Just testing that we don't crash on any of these constructs -
 */


/*
 * global scope -
 */
try
{
  var x = new eval();
  new eval();
}
catch(e)
{
}


/*
 * function scope -
 */
f();
function f()
{
  try
  {
    var x = new eval();
    new eval();
  }
  catch(e)
  {
  }
}


/*
 * eval scope -
 */
var s = '';
s += 'try';
s += '{';
s += '  var x = new eval();';
s += '  new eval();';
s += '}';
s += 'catch(e)';
s += '{';
s += '}';
eval(s);


/*
 * some combinations of scope -
 */
s = '';
s += 'function g()';
s += '{';
s += '  try';
s += '  {';
s += '    var x = new eval();';
s += '    new eval();';
s += '  }';
s += '  catch(e)';
s += '  {';
s += '  }';
s += '}';
s += 'g();';
eval(s);


function h()
{
  var s = '';
  s += 'function f()';
  s += '{';
  s += '  try';
  s += '  {';
  s += '    var x = new eval();';
  s += '    new eval();';
  s += '  }';
  s += '  catch(e)';
  s += '  {';
  s += '  }';
  s += '}';
  s += 'f();';
  eval(s);
}
h();
