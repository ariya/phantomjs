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
* Contributor(s): pschwartau@netscape.com
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
* Date:    12 Feb 2002
* SUMMARY: Don't crash on invalid regexp literals /  \\/  /
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=122076
* The function checkURL() below sometimes caused a compile-time error:
*
*         SyntaxError: unterminated parenthetical (:
*
* However, sometimes it would cause a crash instead. The presence of
* other functions below is merely fodder to help provoke the crash.
* The constant |STRESS| is number of times we'll try to crash on this.
*
*/
//-----------------------------------------------------------------------------
var bug = 122076;
var summary = "Don't crash on invalid regexp literals /  \\/  /";
var STRESS = 10;
var sEval = '';

printBugNumber(bug);
printStatus(summary);


sEval += 'function checkDate()'
sEval += '{'
sEval += 'return (this.value.search(/^[012]?\d\/[0123]?\d\/[0]\d$/) != -1);'
sEval += '}'

sEval += 'function checkDNSName()'
sEval += '{'
sEval += '  return (this.value.search(/^([\w\-]+\.)+([\w\-]{2,3})$/) != -1);'
sEval += '}'

sEval += 'function checkEmail()'
sEval += '{'
sEval += '  return (this.value.search(/^([\w\-]+\.)*[\w\-]+@([\w\-]+\.)+([\w\-]{2,3})$/) != -1);'
sEval += '}'

sEval += 'function checkHostOrIP()'
sEval += '{'
sEval += '  if (this.value.search(/^([\w\-]+\.)+([\w\-]{2,3})$/) == -1)'
sEval += '    return (this.value.search(/^[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}$/) != -1);'
sEval += '  else'
sEval += '    return true;'
sEval += '}'

sEval += 'function checkIPAddress()'
sEval += '{'
sEval += '  return (this.value.search(/^[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}$/) != -1);'
sEval += '}'

sEval += 'function checkURL()'
sEval += '{'
sEval += '  return (this.value.search(/^(((https?)|(ftp)):\/\/([\-\w]+\.)+\w{2,4}(\/[%\-\w]+(\.\w{2,})?)*(([\w\-\.\?\\/\*\$+@&#;`~=%!]*)(\.\w{2,})?)*\/?)$/) != -1);'
sEval += '}'


for (var i=0; i<STRESS; i++)
{
  try
  {
    eval(sEval);
  }
  catch(e)
  {
  }
}
