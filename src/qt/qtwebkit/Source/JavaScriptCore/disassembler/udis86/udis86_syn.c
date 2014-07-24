/* udis86 - libudis86/syn.c
 *
 * Copyright (c) 2002-2009 Vivek Thampi
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice, 
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, 
 *       this list of conditions and the following disclaimer in the documentation 
 *       and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(UDIS86)

/* -----------------------------------------------------------------------------
 * Intel Register Table - Order Matters (types.h)!
 * -----------------------------------------------------------------------------
 */
const char* ud_reg_tab[] = 
{
  "al",		"cl",		"dl",		"bl",
  "ah",		"ch",		"dh",		"bh",
  "spl",	"bpl",		"sil",		"dil",
  "r8b",	"r9b",		"r10b",		"r11b",
  "r12b",	"r13b",		"r14b",		"r15b",

  "ax",		"cx",		"dx",		"bx",
  "sp",		"bp",		"si",		"di",
  "r8w",	"r9w",		"r10w",		"r11w",
  "r12w",	"r13W"	,	"r14w",		"r15w",
	
  "eax",	"ecx",		"edx",		"ebx",
  "esp",	"ebp",		"esi",		"edi",
  "r8d",	"r9d",		"r10d",		"r11d",
  "r12d",	"r13d",		"r14d",		"r15d",
	
  "rax",	"rcx",		"rdx",		"rbx",
  "rsp",	"rbp",		"rsi",		"rdi",
  "r8",		"r9",		"r10",		"r11",
  "r12",	"r13",		"r14",		"r15",

  "es",		"cs",		"ss",		"ds",
  "fs",		"gs",	

  "cr0",	"cr1",		"cr2",		"cr3",
  "cr4",	"cr5",		"cr6",		"cr7",
  "cr8",	"cr9",		"cr10",		"cr11",
  "cr12",	"cr13",		"cr14",		"cr15",
	
  "dr0",	"dr1",		"dr2",		"dr3",
  "dr4",	"dr5",		"dr6",		"dr7",
  "dr8",	"dr9",		"dr10",		"dr11",
  "dr12",	"dr13",		"dr14",		"dr15",

  "mm0",	"mm1",		"mm2",		"mm3",
  "mm4",	"mm5",		"mm6",		"mm7",

  "st0",	"st1",		"st2",		"st3",
  "st4",	"st5",		"st6",		"st7", 

  "xmm0",	"xmm1",		"xmm2",		"xmm3",
  "xmm4",	"xmm5",		"xmm6",		"xmm7",
  "xmm8",	"xmm9",		"xmm10",	"xmm11",
  "xmm12",	"xmm13",	"xmm14",	"xmm15",

  "rip"
};

#endif // USE(UDIS86)

