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
* Date: 26 November 2000
*
*
* SUMMARY:  This test arose from Bugzilla bug 57631: 
* "RegExp with invalid pattern or invalid flag causes segfault"
*
* Either error should throw an exception of type SyntaxError, 
* and we check to see that it does...
*/
//-------------------------------------------------------------------------------------------------
var bug = '57631';
var summary = 'Testing new RegExp(pattern,flag) with illegal pattern or flag';
var statprefix = 'Testing for error creating illegal RegExp object on pattern ';
var statsuffix =  'and flag ';
var cnSUCCESS = 'SyntaxError';
var cnFAILURE = 'not a SyntaxError';
var singlequote = "'";
var i = -1; var j = -1; var s = ''; var f = '';
var obj = {};
var status = ''; var actual = ''; var expect = ''; var msg = '';
var legalpatterns = new Array(); var illegalpatterns = new Array();
var legalflags = new Array();  var illegalflags = new Array();


// valid regular expressions to try - 
legalpatterns[0] = '';
legalpatterns[1] = 'abc';
legalpatterns[2] = '(.*)(3-1)\s\w';
legalpatterns[3] = '(.*)(...)\\s\\w';
legalpatterns[4] = '[^A-Za-z0-9_]';
legalpatterns[5] = '[^\f\n\r\t\v](123.5)([4 - 8]$)';

// invalid regular expressions to try - 
illegalpatterns[0] = '()';
illegalpatterns[1] = '(a';
illegalpatterns[2] = '( ]';
illegalpatterns[3] = '\d{s}';

// valid flags to try -
legalflags[0] = 'i';
legalflags[1] = 'g';
legalflags[2] = 'm';
legalflags[3] = undefined;

// invalid flags to try -
illegalflags[0] = 'a';
illegalflags[1] = 123;
illegalflags[2] = new RegExp();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);
  
  testIllegalRegExps(legalpatterns, illegalflags);
  testIllegalRegExps(illegalpatterns, legalflags);
  testIllegalRegExps(illegalpatterns, illegalflags);

  exitFunc ('test');
}


// This function will only be called where all the patterns are illegal, or all the flags
function testIllegalRegExps(patterns, flags)
{
  for (i in patterns)
    {
      s = patterns[i];
  
      for (j in flags)
      {
        f = flags[j];
        status = getStatus(s, f);
  
        try 
        {
          // This should cause an exception if either s or f is illegal -         
          eval('obj = new RegExp(s, f);');  
        }  
        catch(e) 
        {
          // We expect to get a SyntaxError - test for this:
          actual = (e instanceof SyntaxError)? cnSUCCESS : cnFAILURE; 
          expect = cnSUCCESS;
          reportCompare(expect, actual, status);   
        }
      }
    }
}


function getStatus(regexp, flag)
{ 
  return (statprefix  +  quote(regexp) +  statsuffix  +   quote(flag)); 
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}