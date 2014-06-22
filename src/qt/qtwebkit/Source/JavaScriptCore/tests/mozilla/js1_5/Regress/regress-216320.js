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
* Contributor(s): brendan@mozilla.org, pschwartau@netscape.com
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
* Date:    09 September 2003
* SUMMARY: Just seeing we don't crash on this code
* See http://bugzilla.mozilla.org/show_bug.cgi?id=216320
*
*/
//-----------------------------------------------------------------------------
var bug = 216320;
var summary = "Just seeing we don't crash on this code";

printBugNumber(bug);
printStatus(summary);


/* TESTCASE BEGINS HERE */
status=0;
ism='NO';
scf='N';

function vol(){
if(navigator.appName!="Netscape"){	if(!window.navigator.onLine){	alert(pbc0430);	return false;	} }
return true; }

function vnid(formfield){
nid=formfield.value;
if(!nid.match(/^\s*$/)){
nl=nid.split('/').length;
if(nl!=2&&nl!=3){
alert(pbc0420);
formfield.focus();
return false;
}}}

function vnull(formfield){
text=formfield.value;
if(text.match(/^\s*$/)){
alert(pbc0425);
formfield.focus();
return false;
}
return true;
}

function vdt(formfield){
date=formfield.value;
//MM/DD/YYYY
//YYYY/MM/DD
year=date.substring(0,4);
hy1=date.charAt(4);
month=date.substring(5,7);
hy2=date.charAt(7);
day=date.substring(8,10);
today=new Date();
tdy=today.getDate();
tmn=today.getMonth()+1;
if(today.getYear()<2000)tyr=today.getYear()+1900;
else tyr=today.getYear();
if(date.match(/^\s*$/))	{return true;	}

if(hy1!="/"||hy2!="/"){
alert(pbc0409);
formfield.focus();
return false;
}
if(month>12||day>31||month<=0||day<=0||(isNaN(month)==true)||(isNaN(day)==true)||(isNaN(year)==true)){
alert(pbc0409);
formfield.focus();
return false;
}

if(((month==1||month==3||month==5||month==7||month==8||month==10||month==12)&&day>31)||(year%4==0&&month==2&&day>29)||(year%4!=0&&month==2&&day>28)||((month==4||month==6||month==9||month==11)&&day>30)){
alert(pbc0409);
formfield.focus();
return false;
}
return true;
}

function vkdt(formfield){
date=formfield.value;
year=date.substring(0,4);
hy1=date.charAt(4);
month=date.substring(5,7);
hy2=date.charAt(7);
day=date.substring(8,10);
today=new Date();
tdy=today.getDate();
tmn=today.getMonth()+1;
if(today.getYear()<2000)tyr=today.getYear()+1900;
else tyr=today.getYear();
if(date.match(/^\s*$/)){
alert(pbc0425);
formfield.focus();
return false;
}
if(hy1!="/"||hy2!="/"){
alert(pbc0409);
formfield.focus();
return false;
}

if(month>12||day>31||month<=0||day<=0||(isNaN(month)==true)||(isNaN(day)==true)||(isNaN(year)==true)){
alert(pbc0409);
formfield.focus();
return false;
}

if(((month==1||month==3||month==5||month==7||month==8||month==10||month==12)&&day>31)||(year%4==0&&month==2&&day>29)||(year%4!=0&&month==2&&day>28)||((month==4||month==6||month==9||month==11)&&day>30)){
alert(pbc0409);
formfield.focus();
return false;
}
return true;
}

function ddif(month1,day1,year1,month2,day2,year2){
start = new Date();
start.setYear(year1);
start.setMonth(month1-1);
start.setDate(day1);
start.setMinutes(0);
start.setHours(0);
start.setSeconds(0);
end = new Date();
end.setYear(year2);
end.setMonth(month2-1);
end.setDate(day2);
end.setMinutes(0);
end.setHours(0);
end.setSeconds(0);
current =(end.getTime() - start.getTime());
days = Math.floor(current /(1000 * 60 * 60 * 24));
return(days);
}

function vsub(form,status,ism,action){
if(!vol()){ return false; }
if(status<9||status==12){
band=form.BAND.options[form.BAND.selectedIndex].value;
if(band=="00"){
alert(pbc0425);
form.BAND.focus();
return false;
}
}

if((status>=0&&status<5)||(status==7)||(status>=5&&status<9&&ism=="YES")||(status==12&&ism=="YES")){
if(!vnull(form.PT))	{	return false;	}
adt1=form.STD;
adt2=form.END;
stdt=adt1.value;
etdt=adt2.value;
syr=stdt.substring(0,4);
start_hy1=stdt.charAt(4);
smon=stdt.substring(5,7);
start_hy2=stdt.charAt(7);
sdy=stdt.substring(8,10);
eyr=etdt.substring(0,4);
end_hy1=etdt.charAt(4);
emon=etdt.substring(5,7);
end_hy2=etdt.charAt(7);
edy=etdt.substring(8,10);
today=new Date();
date=today.getDate();
month=today.getMonth()+1;
if(today.getYear()<2000)year=today.getYear()+1900;	else year=today.getYear();
nextYear=year+1;
if(!vnull(form.STD)){	return false;	}
if(!vnull(form.END)){	return false;	}
if(start_hy1!="/"||start_hy2!="/"){
alert(pbc0409);
form.STD.focus();
return false;
}
if(end_hy1!="/"||end_hy2!="/"){
alert(pbc0409);
form.END.focus();
return false;
}
if(smon>12||sdy>31||smon<=0||sdy<=0||(isNaN(smon)==true)||(isNaN(sdy)==true)||(isNaN(syr)==true)){
alert(pbc0409);
form.STD.focus();
return false;
}
if(emon>12||edy>31||emon<=0||edy<=0||(isNaN(emon)==true)||(isNaN(edy)==true)||(isNaN(eyr)==true)){
alert(pbc0409);
form.END.focus();
return false;
}
if(((smon==1||smon==3||smon==5||smon==7||smon==8||smon==10||smon==12)&&sdy>31)||(syr%4==0&&smon==2&&sdy>29)||(syr%4!=0&&smon==2&&sdy>28)||((smon==4||smon==6||smon==9||smon==11)&&sdy>30)){
alert(pbc0409);
form.STD.focus();
return false;
}
if(((emon==1||emon==3||emon==5||emon==7||emon==8||emon==10||emon==12)&&edy>31)||(eyr%4==0&&emon==2&&edy>29)||(eyr%4!=0&&emon==2&&edy>28)||((emon==4||emon==6||emon==9||emon==11)&&edy>30)){
alert(pbc0409);
form.END.focus();
return false;
}
if ((eyr==nextYear)&&(syr==year)) {
if ((emon>1)||(edy >31)) {
alert(pbc0401);
form.END.focus();
return false;
}
} else {

if ((syr!=eyr)){
alert(pbc0406);
form.STD.focus();
return false;
}
if(smon>emon||(smon==emon&&sdy>=edy)){
alert(pbc0402);
form.STD.focus();
return false;
}
if((eyr!=year)&&(eyr!=year-1)){
alert(pbc0405);
form.END.focus();
return false;
}
}
if(ism=='YES'&&(status==5||status==6||status==12)){
if(ddif(month,date,year,emon,edy,eyr)>31){
alert(pbc0421);
form.END.focus();
return false;
}
}
if((status>2&&status<5)||(status==7)||((status>=5&&status<9||status==12)&&ism=="YES")){
if(status!=5){
if(!vdt(form.IRD1)){
return false;
}
if(!vdt(form.IRD2)){
return false;
}
if(!vdt(form.IRD3)){
return false;
}
ird1=form.IRD1.value;
ird2=form.IRD2.value;
ird3=form.IRD3.value;
if(((ird1==ird2)&&(!ird1.match(/^\s*$/)))||((ird1==ird3)&&(!ird1.match(/^\s*$/)))){
alert(pbc0417);
form.IRD1.focus();
return false;
}
else if((ird2==ird3)&&(!ird2.match(/^\s*$/))){
alert(pbc0417);
form.IRD2.focus();
return false;
}
if(!vdt(form.FRD1)){ return false;}
}
if(status==5){
if(!vdt(form.IRD1)){return false;}
if(!vdt(form.IRD2)){return false;}
if(!vdt(form.IRD3)){return false;}
ird1=form.IRD1.value;
ird2=form.IRD2.value;
ird3=form.IRD3.value;
if(((ird1==ird2)&&(!ird1.match(/^\s*$/)))||((ird1==ird3)&&(!ird1.match(/^\s*$/)))){
alert(pbc0417);
form.IRD1.focus();
return false;
}
else if((ird2==ird3)&&(!ird2.match(/^\s*$/))){
alert(pbc0417);
form.IRD2.focus();
return false;
}
if(!vkdt(form.FRD1)){
return false;
}
}
}
}
if((status>=0&&status<2)||(status==3)||(status==7)||(status>=2&&status<9&&ism=="YES")||(status==12&&ism=="YES")){
if(!vnull(form.WO)){
return false;
}
if(!vnull(form.EO)){
return false;
}
if(!vnull(form.TO)){
return false;
}
}
if((status==2||status==4)||(status>=5&&status<9&&ism=="YES")||(status==12&&ism=="YES")){
if(!vnull(form.WR)){return false;}
if(!vnull(form.ER)){return false;}
if(!vnull(form.TR)){return false;}
}
if((status==5||status==6||status==12)&&ism=="YES"){
if(!vkdt(form.FRD1)){return false;}
frdt=form.FRD1.value;
fryr=frdt.substring(0,4);
frmn=frdt.substring(5,7);
frdy=frdt.substring(8,10);
if(fryr<syr||(fryr==syr&&frmn<smon)||(fryr==syr&&frmn==smon&&frdy<=sdy)){
alert(pbc0410);
form.FRD1.focus();
return false;
}
if((status==5||status==6||status==12)&&ism=="YES"){
isnh="";
for(i=0; i<form.INH.length; i++){
if(form.INH[i].checked==true){ isnh=form.INH[i].value; }
}
if(isnh==""){
alert(pbc0424);
form.INH[1].focus();
return false;
}
if(isnh=="Y"){
beh="";
for(i=0; i<form.NHB.length; i++){
if(form.NHB[i].checked==true){ beh=form.NHB[i].value; }
}
skl="";
for(i=0; i<form.NHS.length; i++){
if(form.NHS[i].checked==true){ skl=form.NHS[i].value; }
}
if(beh==""){
alert(pbc0408);
form.NHB[0].focus();
return false;
}
if(skl==""){
alert(pbc0426);
form.NHS[0].focus();
return false;
}
if((beh=="N"||skl=="N")&&status!=12){
if(form.RCD[3].checked==false){
if(confirm(pbc0455))srdb(form.RCD,"4");
else {
form.NHB[0].focus();
return false;
}}}}}
rating="";
if(status!=12){ for(i=0; i<form.RCD.length; i++){ if(form.RCD[i].checked==true)rating=form.RCD[i].value; } }
else if(status==12){ rating="4"; }
if(rating==""){
alert(pbc0428);
form.RCD[0].focus();
return false;
}
if(rating=="4"){
if(!vkdt(form.SID)){ return false; }
idt=form.SID.value;
iyr=idt.substring(0,4);
imon=idt.substring(5,7);
idy=idt.substring(8,10);
frdt=form.FRD1.value;
fryr=frdt.substring(0,4);
frmn=frdt.substring(5,7);
frdy=frdt.substring(8,10);
if(iyr<eyr||(iyr==eyr&&imon<emon)||(iyr==eyr&&imon==emon&&idy<=edy)){
alert(pbc0415);
form.SID.focus();
return false;
}
if(iyr<fryr||(iyr==fryr&&imon<frmn)||(iyr==fryr&&imon==frmn&&idy<=frdy)){
alert(pbc0427);
form.SID.focus();
return false;
}
if(ddif(emon,edy,eyr,imon,idy,iyr)<30){
alert(pbc0416);
form.SID.focus();
return false;
}
if(ddif(emon,edy,eyr,imon,idy,iyr)>90){
if(!confirm(pbc0439+" "+pbc0442)){
form.SID.focus();
return false;
}}} else {
// MK/06-20-01 = If Rating Not equals to 4 blank out the sustained improve Date
form.SID.value="";
}
if(!vnull(form.OAT)){ return false; }
if(form.MSRQ.checked==true){
if(form.NEW_SIGN_MGR_ID.value.match(/^\s*$/)){
alert(pbc0418);
form.NEW_SIGN_MGR_ID.focus();
return false;
}
if(vnid(form.NEW_SIGN_MGR_ID)==false){	return false; }
} else {
if(!form.NEW_SIGN_MGR_ID.value.match(/^\s*$/)){
alert(pbc0422);
form.NEW_SIGN_MGR_ID.focus();
return false;
}
if ( (form.TOC.value=="YES") && (form.RSRQ.checked==true) ) {
alert(pbc0429);
form.NEW_SEC_LINE_REV_ID.focus();
return false;
}
}
if(form.RSRQ.checked==true){
if(form.NEW_SEC_LINE_REV_ID.value.match(/^\s*$/)){
alert(pbc0418);
form.NEW_SEC_LINE_REV_ID.focus();
return false;
}
if(vnid(form.NEW_SEC_LINE_REV_ID)==false){	return false; }
} else {
if(!form.NEW_SEC_LINE_REV_ID.value.match(/^\s*$/)) {
alert(pbc0423);
form.NEW_SEC_LINE_REV_ID.focus();
return false;
}
if ( (form.TOC.value=="YES") && (form.MSRQ.checked==true) ) {
alert(pbc0431);
form.NEW_SEC_LINE_REV_ID.focus();
return false;
}}}
if(status!=9){
/**for returned objectives **/
if(status==3){
if(conf(pbc0466) == false) return false;
}

if(ism=='NO'){
if(status==0||status==1||status==3||status==7){
if(conf(pbc0456) == false) return false;
}

if(status==2||status==4||status==8){
if(conf(pbc0457) == false) return false;
}
} else if(ism=='YES'){
if(status==0||status==1||status==3||status==7){
if(conf(pbc0458) == false)return false;
}
if(status==2||status==4||status==8){
if(conf(pbc0459) == false)return false;
}
if(status==5||status==6){
if(form.ESRQ.checked==false){
if(conf(pbc0460) == false)return false;
} else {
if(conf(pbc0461) == false)return false;
}}}}
if(status==9){
if(ism=='NO'){
if(conf(pbc0462) == false)return false;
} else if(ism=='YES'){
if(conf(pbc0463) == false)return false;
} else if(ism=='REVIEWER'){
if(conf(pbc0464) == false)return false;
}}
sact(action);
if(status>=9&&status<=11){ snul(); }
form.submit();
return true;
}

function vsav(form,status,ism,action) {
if(!vol()){ return false; }
adt1=form.STD;
adt2=form.END;
stdt=adt1.value;
etdt=adt2.value;
syr=stdt.substring(0,4);
start_hy1=stdt.charAt(4);
smon=stdt.substring(5,7);
start_hy2=stdt.charAt(7);
sdy=stdt.substring(8,10);
eyr=etdt.substring(0,4);
end_hy1=etdt.charAt(4);
emon=etdt.substring(5,7);
end_hy2=etdt.charAt(7);
edy=etdt.substring(8,10);
today=new Date();
date=today.getDate();
month=today.getMonth()+1;
if(today.getYear()<2000) year=today.getYear()+1900; else year=today.getYear();
nextYear=year+1;
if(!vnull(form.STD)) return false;
if(!vnull(form.END)) return false;
if(start_hy1!="/"||start_hy2!="/"){
alert(pbc0409);
form.STD.focus();
return false;
}
if(end_hy1!="/"||end_hy2!="/"){
alert(pbc0409);
form.END.focus();
return false;
}
if(smon>12||sdy>31||smon<=0||sdy<=0||(isNaN(smon)==true)||(isNaN(sdy)==true)||(isNaN(syr)==true)){
alert(pbc0409);
form.STD.focus();
return false;
}
if(emon>12||edy>31||emon<=0||edy<=0||(isNaN(emon)==true)||(isNaN(edy)==true)||(isNaN(eyr)==true)){
alert(pbc0409);
form.END.focus();
return false;
}
if(((smon==1||smon==3||smon==5||smon==7||smon==8||smon==10||smon==12)&&sdy>31)||(syr%4==0&&smon==2&&sdy>29)||(syr%4!=0&&smon==2&&sdy>28)||((smon==4||smon==6||smon==9||smon==11)&&sdy>30)){
alert(pbc0409);
form.STD.focus();
return false;
}
if(((emon==1||emon==3||emon==5||emon==7||emon==8||emon==10||emon==12)&&edy>31)||(eyr%4==0&&emon==2&&edy>29)||(eyr%4!=0&&emon==2&&edy>28)||((emon==4||emon==6||emon==9||emon==11)&&edy>30)){
alert(pbc0409);
form.END.focus();
return false;
}
if ((eyr==nextYear)&&(syr==year)) {
if ((emon>1)||(edy >31)) {
alert(pbc0401);
form.END.focus();
return false;
}
} else {
if ((syr<year-1) || (syr>year)) {
alert(pbc0407);
form.STD.focus();
return false;
}
if((eyr!=year)&&(eyr!=year-1)){
alert(pbc0405);
form.END.focus();
return false;
}
if(smon>emon||(smon==emon&&sdy>=edy)){
alert(pbc0403);
form.STD.focus();
return false;
}
}
if((status>2&&status<5)||(status>=5&&status<9&&ism=="YES")||(status==12&&ism=="YES")){
if(!vdt(form.IRD1)){return false;}
if(!vdt(form.IRD2)){return false;}
if(!vdt(form.IRD3)){	return false;	}
ird1=form.IRD1.value;
ird2=form.IRD2.value;
ird3=form.IRD3.value;
if(((ird1==ird2)&&(!ird1.match(/^\s*$/)))||((ird1==ird3)&&(!ird1.match(/^\s*$/)))){
alert(pbc0417);
form.IRD1.focus();
return false;
}
else if((ird2==ird3)&&(!ird2.match(/^\s*$/))){
alert(pbc0417);
form.IRD2.focus();
return false;
}
if(!vdt(form.FRD1)){return false;}
if(ism=="YES"){
if(!vdt(form.FRD1)){return false;}
}
}
if((status==5||status==6)&&ism=="YES"){
rating="";
for(i=0;i<form.RCD.length;i++){
if(form.RCD[i].checked==true)rating=form.RCD[i].value;
}
isnh="";
for(i=0; i<form.INH.length; i++){
if(form.INH[i].checked==true){
isnh=form.INH[i].value;
}
}
if(isnh=="Y"){
beh="";
for(i=0; i<form.NHB.length;i++){
if(form.NHB[i].checked==true){
beh=form.NHB[i].value;
}
}
skl="";
for(i=0; i<form.NHS.length;i++){
if(form.NHS[i].checked==true){
skl=form.NHS[i].value;
}
}
if((beh=="N"||skl=="N")&&rating!=""){
if(form.RCD[3].checked==false){
if(confirm(pbc0455))srdb(form.RCD,"4");
else {
form.NHB[0].focus();
return false;
}
}
}
if(!vdt(form.SID)){	return false;}
}
}
if((status==2||status==4 || status==8 || status==5 || status==6 || status==10)&&ism=='YES')
{
if(!confirm(pbc0436)){	return false;}
if(form.OBJECTIVE_CHANGED.value=='Y') {
    if(confirm(pbc0452+" "+pbc0453+" "+pbc0454)){form.MRQ.value=4; } else { form.MRQ.value=0; }
}else if (( status==5 || status==6 || status==10) && (form.RESULTS_CHANGED.value=='Y')) {
   if(confirm(pbc0470+" "+pbc0453+" "+pbc0454)){form.MRQ.value=8; } else { form.MRQ.value=0; }
}
}
sact(action);
if(status>=9&&status<=11){
snul();
}
form.submit();
return true;
}
function cft(formfield){
nid=formfield.value;
if(nid.match(/^\s*$/)){
alert(pbc0419);
formfield.focus();
return false;
}
nl=nid.split('/').length;
if(nl!=2&&nl!=3){
alert(pbc0420);
formfield.focus();
return false;
}
return true;
}
function dcf(form,pbcId,cnum,sequence,status,atyp,ver){
if(!vol()){}
dflg=confirm("\n\n<====================== " + pbc0468 + " ======================>\n\n" + pbc0469 + "\n\n<==================================================================>");
if(dflg==true) {
form.ATYP.value=atyp;
form.PID.value=pbcId;
form.CNUM.value=cnum;
form.SEQ.value=sequence;
form.ST.value=status;
form.VER.value=ver;
form.submit();
}

}



function lop(){
//if(confirm(pbc0447+" "+pbc0451)){
sck("timer","");
sck("PBC_AUTH4","");
sck("IBM004","");
this.close();
//}

}

function csrlop(){
   top.location="logoff.jsp";
}
function lof(){
csr=gck("IBM004");
if(csr==null){ top.location="logoff.jsp";  }
else if(csr.charAt(0)==3){ window.location="csrlogoff.jsp"; }
else{ top.location="logoff.jsp";  }
}

function goToHome(){
 top.location="pbcmain.jsp";
 }

function docsr(){
sck("IBM004","1^NONE^1");
window.location="pbcmain.jsp"
}

function ccd(){
if(confirm(pbc0434)){
if(navigator.appName!="Netscape"){
if(!window.navigator.onLine){
window.close();
}
else {
window.location='pbcmain.jsp';
}
}
else {
window.location='pbcmain.jsp';
}
}
}

function crt(form,action){
if(!vol()){return false;}
band=form.BAND.options[form.BAND.selectedIndex].value;
if(band=="00"){
alert(pbc0425);
form.BAND.focus();
return false;
}
if(!confirm(pbc0450)){return false;}
sact(action);
form.submit();
return true;
}
function cusat(form,action){
if(!vol()){return false;}
sact(action);
form.action="unsatreq.jsp";
form.submit();
return true;
}
function cfrt(form,ism,action){
if(!vol()){return false;}
sact(action);
if(ism=="NO"){
if(confirm(pbc0449+" "+pbc0432)){
snul();
form.submit();
return true;
}
}
if(ism=="REVIEWER"){
if(confirm(pbc0449+" "+pbc0448)){
snul();
form.submit();
return true;
}
}
if(ism=="YES"){
if(confirm(pbc0440)){
snul();
form.submit();
return true;
}
}
}

function cces(form){
if(form.ESRQ.checked==true){
if(!confirm(pbc0435+" "+pbc0443))form.ESRQ.checked=false;
else {form.ESRQ.checked=true;}
}
}

function ccms(form){
if(form.MSRQ.checked==true){
if(!confirm(pbc0441+" "+pbc0438+" "+pbc0444+" "+pbc0445))form.MSRQ.checked=false;
else {
form.MSRQ.checked=true;
}
}
}

function ccrs(form){
if(form.RSRQ.checked==true){
if(!confirm(pbc0441+" "+pbc0438+" "+pbc0444+" "+pbc0446))form.RSRQ.checked=false;
else {
form.RSRQ.checked=true;
}
}
}

function seo(){
alert(pbc0412+" "+pbc0413+" "+pbc0414);
}
function cows(form,action){
if(!vol()){
return false;
}
if(confirm(pbc0437)){
sact(action);
form.submit();
return true;
}
}

function srdb(rdb,value) {
for(i=0; i<rdb.length;i++) {
if(rdb[i].value == value) {
rdb[i].checked = true;
return true;
}
}
return true;
}

function slop(lbx,value) {
if(lbx.options.length > 0)  {
for(i=0;i < lbx.options.length;i++) {
if(lbx.options[i].value == value) {
lbx.options[i].selected = true;
return true;
}
}
}
return true;
}

function ourl(URL,WIN_NAME){
if(!vol()){ return; }
var emp_win;
if(document.layers) {
child_screenX=window.screenX+50;
child_width=window.innerWidth-75;
child_height=window.innerHeight-75;
emp_win=window.open(URL,WIN_NAME,"screenX="+ child_screenX +",screenY=75,height="+ child_height +",width="+ child_width +",resizable,status,scrollbars");
} else{
child_width = screen.width-160;
child_height = screen.height-200;
emp_win=window.open(URL,WIN_NAME,"height="+ child_height +",width="+ child_width +",resizable=yes,status=no,scrollbars=yes");
//emp_win.moveTo(110,0);
}
//if (URL.indexOf("pbcsitehelp")==-1) { alert("Opened new window."); }
emp_win.focus();
}

function dnh(form){
form.NHS[0].checked=false;
form.NHS[1].checked=false;
form.NHB[0].checked=false;
form.NHB[1].checked=false;
}

function cnh(form){
isnh="";
for(i=0; i<form.INH.length;i++)
{
if(form.INH[i].checked==true){isnh=form.INH[i].value; }
}
if(isnh != 'Y'){
form.NHS[0].checked=false;
form.NHS[1].checked=false;
form.NHB[0].checked=false;
form.NHB[1].checked=false;
return false;
}
else
{
  //if ((form.NHS[0].checked || form.NHS[1].checked) && (form.NHB[0].checked || form.NHB[1].checked))
  if (form.NHS[1].checked || form.NHB[1].checked )
  {
    form.RCD[3].checked=true;
    return true;
  }
  return false;
}
}

function err(errMsg) {
alert(getEncodedText(errMsg));
}

function getEncodedText(txtValue) {
if (txtValue.match(/^\s*$/)) return txtValue;
var txtValue1 = txtValue.replace((/&quot;/g),'"');
var txtValue2 = txtValue1.replace((/&gt;/g),">");
var txtValue3 = txtValue2.replace((/&lt;/g),"<");
return txtValue3;
}

function encodeText(txtValue) {
if (txtValue.match(/^\s*$/)) return txtValue;
var txtValue0 = txtValue.replace((/\r\n/g),'&lf;');
var txtValue1 = txtValue0.replace((/"/g),'&quot;');
var txtValue2 = txtValue1.replace((/>/g),'&gt;');
var txtValue3 = txtValue2.replace((/</g),'&lt;');
return txtValue3;
}


function gck(name){
result = null;
mck = " " + document.cookie + ";";
srcnm = " " + name + "=";
scok = mck.indexOf(srcnm);
if(scok != -1){
scok += srcnm.length;
eofck = mck.indexOf(";",scok);
result = unescape(mck.substring(scok,eofck));
}
return(result);
}

function sck(name,value){
ckpth="path=/;domain=.ibm.com";
document.cookie = name + "=" + value + ";" + ckpth;
}


function testForCookie(){
	sck("PBCTest","test");
	if(gck("PBCTest") == "test") {
		//	alert("Cookie test is good");
			return true;
			}
 		else {
		//	alert("Cookie test is bad");
			return false;	
			}
	}


function prn(form,l_status,l_ism,l_scf,l_locale){
status = l_status;
ism = l_ism;
scf = l_scf;
pwin=window.open("printvw.jsp?nls="+l_locale + "ISNEWWIN=TRUE","pwin","resizable=yes,width=560,height=400,scrollbars=yes,toolbar,screenX=5,screenY=5");
}

function gsno(form){
unum=form.UNUM.value;
eofsn=unum.length-3;
cnum=unum.substring(0,eofsn);
return(cnum);
}

function conf(msg){
return top.confirm(msg);
}

function sact(action){
document.PBC_FORM.ATYP.value=action;
}

function snul(){
document.PBC_FORM.WO.value="";
document.PBC_FORM.WR.value="";
document.PBC_FORM.EO.value="";
document.PBC_FORM.ER.value="";
document.PBC_FORM.TO.value="";
document.PBC_FORM.TR.value="";
document.PBC_FORM.OAT.value="";
}

function gcnum(){
unum=document.PBC_FORM.UNUM.value;
eofsn=unum.length-3;
cnum=unum.substring(0,eofsn);
return(cnum);
}
function checkForEditPage() {
 if(true==checkForm()){
   if(!confirm(pbc0465)) return false;
 }
 return true;
}

function checkForm() {
  var frms=document.forms["PBC_FORM"];
  if (navigator.appName=="Netscape") {
    if (frms==undefined) return false;
    if (frms.IS_EDIT==undefined) return false;
  } else {
    if(frms==null) return false;
    if (frms.IS_EDIT==null) return false;
  }
  return true;
}



function removeAnchor(link){
link2 = link;
indx = link.indexOf('#');
while (indx!=-1)
{
link2 = link.substring(0,indx);
indx=link2.indexOf("#");


}
return link2;
}

function gotoHREF(link){
if(document.layers){
var documentURL = removeAnchor(document.URL);
location.href=documentURL+link;
return true;

}else{
var documentURL = removeAnchor(document.URL);
document.URL=documentURL+link;


}


}

function init_resize_event(){
}

function putVal2ck()
{
}

function setValuesFromCookie()
{
}
