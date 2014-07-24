/*

   HTML manglizer
   --------------
   Copyright (C) 2004 by Michal Zalewski <lcamtuf@coredump.cx>

   Tag and parameter list: guesstimating / reference compilation.

 */


#define MAXTAGS 80
#define MAXPARS 20

static char* tags[MAXTAGS][MAXPARS] = {
  { "A", "NAME", "HREF", "REF", "REV", "TITLE", "TARGET", "SHAPE", "onLoad", "STYLE", 0 },
  { "APPLET", "CODEBASE", "CODE", "NAME", "ALIGN", "ALT", "HEIGHT", "WIDTH", "HSPACE", "VSPACE", "DOWNLOAD", "HEIGHT", "NAME", "TITLE", "onLoad", "STYLE", 0 }, 
  { "AREA", "SHAPE", "ALT", "CO-ORDS", "HREF", "onLoad", "STYLE", 0 }, 
  { "B", "onLoad", "STYLE", 0 }, 
  { "BANNER", "onLoad", "STYLE", 0 }, 
  { "BASE", "HREF", "TARGET", "onLoad", "STYLE", 0 }, 
  { "BASEFONT", "SIZE", "onLoad", "STYLE", 0 }, 
  { "BGSOUND", "SRC", "LOOP", "onLoad", "STYLE", 0 }, 
  { "BQ", "CLEAR", "NOWRAP", "onLoad", "STYLE", 0 }, 
  { "BODY", "BACKGROUND", "BGCOLOR", "TEXT", "LINK", "ALINK", "VLINK", "LEFTMARGIN", "TOPMARGIN", "BGPROPERTIES", "onLoad", "STYLE", 0 }, 
  { "CAPTION", "ALIGN", "VALIGN", "onLoad", "STYLE", 0 }, 
  { "CENTER", "onLoad", "STYLE", 0 }, 
  { "COL", "ALIGN", "SPAN", "onLoad", "STYLE", 0 }, 
  { "COLGROUP", "ALIGN", "VALIGN", "HALIGN", "WIDTH", "SPAN", "onLoad", "STYLE", 0 }, 
  { "DIV", "ALIGN", "CLASS", "LANG", "onLoad", "STYLE", 0 }, 
  { "EMBED", "SRC", "HEIGHT", "WIDTH", "UNITS", "NAME", "PALETTE", "onLoad", "STYLE", 0 }, 
  { "FIG", "SRC", "ALIGN", "HEIGHT", "WIDTH", "UNITS", "IMAGEMAP", "onLoad", "STYLE", 0 }, 
  { "FN", "ID", "onLoad", "STYLE", 0 }, 
  { "FONT", "SIZE", "COLOR", "FACE", "onLoad", "STYLE", 0 }, 
  { "FORM", "ACTION", "METHOD", "ENCTYPE", "TARGET", "SCRIPT", "onLoad", "STYLE", 0 }, 
  { "FRAME", "SRC", "NAME", "MARGINWIDTH", "MARGINHEIGHT", "SCROLLING", "FRAMESPACING", "onLoad", "STYLE", 0 }, 
  { "FRAMESET", "ROWS", "COLS", "onLoad", "STYLE", 0 }, 
  { "H1", "SRC", "DINGBAT", "onLoad", "STYLE", 0 }, 
  { "HEAD", "onLoad", "STYLE", 0 }, 
  { "HR", "SRC", "SIZE", "WIDTH", "ALIGN", "COLOR", "onLoad", "STYLE", 0 }, 
  { "HTML", "onLoad", "STYLE", 0 }, 
  { "IFRAME", "ALIGN", "FRAMEBORDER", "HEIGHT", "MARGINHEIGHT", "MARGINWIDTH", "NAME", "SCROLLING", "SRC", "ADDRESS", "WIDTH", "onLoad", "STYLE", 0 }, 
  { "IMG", "ALIGN", "ALT", "SRC", "BORDER", "DYNSRC", "HEIGHT", "HSPACE", "ISMAP", "LOOP", "LOWSRC", "START", "UNITS", "USEMAP", "WIDTH", "VSPACE", "onLoad", "STYLE", 0 }, 
  { "INPUT", "TYPE", "NAME", "VALUE", "onLoad", "STYLE", 0 }, 
  { "ISINDEX", "HREF", "PROMPT", "onLoad", "STYLE", 0 }, 
  { "LI", "SRC", "DINGBAT", "SKIP", "TYPE", "VALUE", "onLoad", "STYLE", 0 }, 
  { "LINK", "REL", "REV", "HREF", "TITLE", "onLoad", "STYLE", 0 }, 
  { "MAP", "NAME", "onLoad", "STYLE", 0 }, 
  { "MARQUEE", "ALIGN", "BEHAVIOR", "BGCOLOR", "DIRECTION", "HEIGHT", "HSPACE", "LOOP", "SCROLLAMOUNT", "SCROLLDELAY", "WIDTH", "VSPACE", "onLoad", "STYLE", 0 }, 
  { "MENU", "onLoad", "STYLE", 0 }, 
  { "META", "HTTP-EQUIV", "CONTENT", "NAME", "onLoad", "STYLE", 0 }, 
  { "MULTICOL", "COLS", "GUTTER", "WIDTH", "onLoad", "STYLE", 0 }, 
  { "NOFRAMES", "onLoad", "STYLE", 0 }, 
  { "NOTE", "CLASS", "SRC", "onLoad", "STYLE", 0 }, 
  { "OVERLAY", "SRC", "X", "Y", "HEIGHT", "WIDTH", "UNITS", "IMAGEMAP", "onLoad", "STYLE", 0 }, 
  { "PARAM", "NAME", "VALUE", "onLoad", "STYLE", 0 }, 
  { "RANGE", "FROM", "UNTIL", "onLoad", "STYLE", 0 }, 
  { "SCRIPT", "LANGUAGE", "onLoad", "STYLE", 0 }, 
  { "SELECT", "NAME", "SIZE", "MULTIPLE", "WIDTH", "HEIGHT", "UNITS", "onLoad", "STYLE", 0 },
  { "OPTION", "VALUE", "SHAPE", "onLoad", "STYLE", 0 }, 
  { "SPACER", "TYPE", "SIZE", "WIDTH", "HEIGHT", "ALIGN", "onLoad", "STYLE", 0 }, 
  { "SPOT", "ID", "onLoad", "STYLE", 0 }, 
  { "TAB", "INDENT", "TO", "ALIGN", "DP", "onLoad", "STYLE", 0 }, 
  { "TABLE", "ALIGN", "WIDTH", "BORDER", "CELLPADDING", "CELLSPACING", "BGCOLOR", "VALIGN", "COLSPEC", "UNITS", "DP", "onLoad", "STYLE", 0 }, 
  { "TBODY", "CLASS", "ID", "onLoad", "STYLE", 0 },
  { "TD", "COLSPAN", "ROWSPAN", "ALIGN", "VALIGN", "BGCOLOR", "onLoad", "STYLE", 0 }, 
  { "TEXTAREA", "NAME", "COLS", "ROWS", "onLoad", "STYLE", 0 }, 
  { "TEXTFLOW", "CLASS", "ID", "onLoad", "STYLE", 0 },
  { "TFOOT", "COLSPAN", "ROWSPAN", "ALIGN", "VALIGN", "BGCOLOR", "onLoad", "STYLE", 0 }, 
  { "TH", "ALIGN", "CLASS", "ID", "onLoad", "STYLE", 0 },
  { "TITLE", "onLoad", "STYLE", 0 }, 
  { "TR", "ALIGN", "VALIGN", "BGCOLOR", "CLASS", "onLoad", "STYLE", 0 }, 
  { "UL", "SRC", "DINGBAT", "WRAP", "TYPE", "PLAIN", "onLoad", "STYLE", 0 }, 
  { 0 }
};

