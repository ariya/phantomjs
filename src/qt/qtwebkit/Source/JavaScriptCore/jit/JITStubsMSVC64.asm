;/*
; Copyright (C) 2013 Digia Plc. and/or its subsidiary(-ies)
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
; OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*/

EXTERN cti_vm_throw : near
PUBLIC ctiTrampoline
PUBLIC ctiVMThrowTrampoline
PUBLIC ctiOpThrowNotCaught

_TEXT   SEGMENT

ctiTrampoline PROC
    ; Dump register parameters to their home address
    mov qword ptr[rsp+20h], r9
    mov qword ptr[rsp+18h], r8
    mov qword ptr[rsp+10h], rdx
    mov qword ptr[rsp+8h], rcx

    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    push rbx

    ; Decrease rsp to point to the start of our JITStackFrame
    sub rsp, 58h
    mov r12, 512
    mov r14, 0FFFF000000000000h
    mov r15, 0FFFF000000000002h
    mov r13, r8
    call rcx
    add rsp, 58h
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret
ctiTrampoline ENDP

ctiVMThrowTrampoline PROC
    mov rcx, rsp
    call cti_vm_throw
    int 3
ctiVMThrowTrampoline ENDP

ctiOpThrowNotCaught PROC
    add rsp, 58h
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret
ctiOpThrowNotCaught ENDP

_TEXT   ENDS

END