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
*                            26 November 2000
*
*
*SUMMARY: Testing numeric literals that begin with 0. 
*This test arose from Bugzilla bug 49233.
*The best explanation is from jsscan.c:                
*
*     "We permit 08 and 09 as decimal numbers, which makes 
*     our behaviour a superset of the ECMA numeric grammar.  
*     We might not always be so permissive, so we warn about it."
*
*Thus an expression 010 will evaluate, as always, as an octal (to 8).
*However, 018 will evaluate as a decimal, to 18. Even though the
*user began the expression as an octal, he later used a non-octal
*digit. We forgive this and assume he intended a decimal. If the
*JavaScript "strict" option is set though, we will give a warning.
*/
//-------------------------------------------------------------------------------------------------
var bug = '49233';
var summary = 'Testing numeric literals that begin with 0';
var statprefix = 'Testing ';
var quote = "'";
var status = new Array();
var actual = new Array();
var expect = new Array();


status[0]=showStatus('01')
actual[0]=01
expect[0]=1

status[1]=showStatus('07')
actual[1]=07
expect[1]=7

status[2]=showStatus('08')
actual[2]=08
expect[2]=8

status[3]=showStatus('09')
actual[3]=09
expect[3]=9

status[4]=showStatus('010')
actual[4]=010
expect[4]=8

status[5]=showStatus('017')
actual[5]=017
expect[5]=15

status[6]=showStatus('018')
actual[6]=018
expect[6]=18

status[7]=showStatus('019')
actual[7]=019
expect[7]=19

status[8]=showStatus('079')
actual[8]=079
expect[8]=79

status[9]=showStatus('0079')
actual[9]=0079
expect[9]=79

status[10]=showStatus('099')
actual[10]=099
expect[10]=99

status[11]=showStatus('0099')
actual[11]=0099
expect[11]=99

status[12]=showStatus('000000000077')
actual[12]=000000000077
expect[12]=63

status[13]=showStatus('000000000078')
actual[13]=000000000078
expect[13]=78

status[14]=showStatus('0000000000770000')
actual[14]=0000000000770000
expect[14]=258048

status[15]=showStatus('0000000000780000')
actual[15]=0000000000780000
expect[15]=780000

status[16]=showStatus('0765432198')
actual[16]=0765432198
expect[16]=765432198

status[17]=showStatus('00076543219800')
actual[17]=00076543219800
expect[17]=76543219800

status[18]=showStatus('0000001001007')
actual[18]=0000001001007
expect[18]=262663

status[19]=showStatus('0000001001009')
actual[19]=0000001001009
expect[19]=1001009

status[20]=showStatus('070')
actual[20]=070
expect[20]=56

status[21]=showStatus('080')
actual[21]=080
expect[21]=80



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function showStatus(msg)
{
  return (statprefix  + quote  +  msg  + quote);
}


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);


  for (i=0; i !=status.length; i++)
  {
    reportCompare (expect[i], actual[i], status[i]);
  }

  exitFunc ('test');
}