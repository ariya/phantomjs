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
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): rogerl@netscape.com, pschwartau@netscape.com
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
* Date:    15 July 2002
* SUMMARY: Testing functions with double-byte names
* See http://bugzilla.mozilla.org/show_bug.cgi?id=58274
*
* Here is a sample of the problem:
*
*    js> function f\u02B1 () {}
*
*    js> f\u02B1.toSource();
*    function f¦() {}
*
*    js> f\u02B1.toSource().toSource();
*    (new String("function f\xB1() {}"))
*
*
* See how the high-byte information (the 02) has been lost?
* The same thing was happening with the toString() method:
*
*    js> f\u02B1.toString();
*
*    function f¦() {
*    }
*
*    js> f\u02B1.toString().toSource();
*    (new String("\nfunction f\xB1() {\n}\n"))
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 58274;
var summary = 'Testing functions with double-byte names';
var ERR = 'UNEXPECTED ERROR! \n';
var ERR_MALFORMED_NAME = ERR + 'Could not find function name in: \n\n';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var sEval;
var sName;


sEval = "function f\u02B2() {return 42;}";
eval(sEval);
sName = getFunctionName(f\u02B2);

// Test function call -
status = inSection(1);
actual = f\u02B2();
expect = 42;
addThis();

// Test both characters of function name -
status = inSection(2);
actual = sName[0];
expect = sEval[9];
addThis();

status = inSection(3);
actual = sName[1];
expect = sEval[10];
addThis();



sEval = "function f\u02B2\u0AAA () {return 84;}";
eval(sEval);
sName = getFunctionName(f\u02B2\u0AAA);

// Test function call -
status = inSection(4);
actual = f\u02B2\u0AAA();
expect = 84;
addThis();

// Test all three characters of function name -
status = inSection(5);
actual = sName[0];
expect = sEval[9];
addThis();

status = inSection(6);
actual = sName[1];
expect = sEval[10];
addThis();

status = inSection(7);
actual = sName[2];
expect = sEval[11];
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



/*
 * Goal: test that f.toString() contains the proper function name.
 *
 * Note, however, f.toString() is implementation-independent. For example,
 * it may begin with '\nfunction' instead of 'function'. Therefore we use
 * a regexp to make sure we extract the name properly.
 *
 * Here we assume that f has been defined by means of a function statement,
 * and not a function expression (where it wouldn't have to have a name).
 *
 * Rhino uses a Unicode representation for f.toString(); whereas
 * SpiderMonkey uses an ASCII representation, putting escape sequences
 * for non-ASCII characters. For example, if a function is called f\u02B1,
 * then in Rhino the toString() method will present a 2-character Unicode
 * string for its name, whereas SpiderMonkey will present a 7-character
 * ASCII string for its name: the string literal 'f\u02B1'.
 *
 * So we force the lexer to condense the string before using it.
 * This will give uniform results in Rhino and SpiderMonkey.
 */
function getFunctionName(f)
{
  var s = condenseStr(f.toString());
  var re = /\s*function\s+(\S+)\s*\(/;
  var arr = s.match(re);

  if (!(arr && arr[1]))
    return ERR_MALFORMED_NAME + s;
  return arr[1];
}


/*
 * This function is the opposite of functions like escape(), which take
 * Unicode characters and return escape sequences for them. Here, we force
 * the lexer to turn escape sequences back into single characters.
 *
 * Note we can't simply do |eval(str)|, since in practice |str| will be an
 * identifier somewhere in the program (e.g. a function name); thus |eval(str)|
 * would return the object that the identifier represents: not what we want.
 *
 * So we surround |str| lexicographically with quotes to force the lexer to
 * evaluate it as a string. Have to strip out any linefeeds first, however -
 */
function condenseStr(str)
{
  /*
   * You won't be able to do the next step if |str| has
   * any carriage returns or linefeeds in it. For example:
   *
   *  js> eval("'" + '\nHello' + "'");
   *  1: SyntaxError: unterminated string literal:
   *  1: '
   *  1: ^
   *
   * So replace them with the empty string -
   */
  str = str.replace(/[\r\n]/g, '') 
  return eval("'" + str + "'");
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
