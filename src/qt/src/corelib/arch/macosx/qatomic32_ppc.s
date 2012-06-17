;/****************************************************************************
;**
;** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
;** All rights reserved.
;** Contact: Nokia Corporation (qt-info@nokia.com)
;**
;** This file is part of the QtGui module of the Qt Toolkit.
;**
;** $QT_BEGIN_LICENSE:LGPL$
;** GNU Lesser General Public License Usage
;** This file may be used under the terms of the GNU Lesser General Public
;** License version 2.1 as published by the Free Software Foundation and
;** appearing in the file LICENSE.LGPL included in the packaging of this
;** file. Please review the following information to ensure the GNU Lesser
;** General Public License version 2.1 requirements will be met:
;** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
;**
;** In addition, as a special exception, Nokia gives you certain additional
;** rights. These rights are described in the Nokia Qt LGPL Exception
;** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
;**
;** GNU General Public License Usage
;** Alternatively, this file may be used under the terms of the GNU General
;** Public License version 3.0 as published by the Free Software Foundation
;** and appearing in the file LICENSE.GPL included in the packaging of this
;** file. Please review the following information to ensure the GNU General
;** Public License version 3.0 requirements will be met:
;** http://www.gnu.org/copyleft/gpl.html.
;**
;** Other Usage
;** Alternatively, this file may be used in accordance with the terms and
;** conditions contained in a signed written agreement between you and Nokia.
;**
;**
;**
;**
;**
;** $QT_END_LICENSE$
;**
;****************************************************************************/
	.section __TEXT,__text,regular,pure_instructions
	.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
        .section __TEXT,__text,regular,pure_instructions
	.align 2
	.align 2
	.globl _q_atomic_test_and_set_int
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_test_and_set_int:
	lwarx  r6,0,r3
        cmpw   r6,r4
        bne-   $+20
        stwcx. r5,0,r3
        bne-   $-16
        addi   r3,0,1
        blr
        addi   r3,0,0
	blr

	.align 2
	.globl _q_atomic_test_and_set_acquire_int
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_test_and_set_acquire_int:
	lwarx  r6,0,r3
        cmpw   r6,r4
        bne-   $+20
        stwcx. r5,0,r3
        bne-   $-16
        addi   r3,0,1
        b      $+8
        addi   r3,0,0
        eieio
	blr

	.align 2
	.globl _q_atomic_test_and_set_release_int
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_test_and_set_release_int:
        eieio
	lwarx  r6,0,r3
        cmpw   r6,r4
        bne-   $+20
        stwcx. r5,0,r3
        bne-   $-16
        addi   r3,0,1
        blr
        addi   r3,0,0
	blr

	.align 2
	.globl _q_atomic_test_and_set_ptr
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_test_and_set_ptr:
	lwarx  r6,0,r3
        cmpw   r6,r4
        bne-   $+20
        stwcx. r5,0,r3
        bne-   $-16
        addi   r3,0,1
        blr
        addi   r3,0,0
	blr

	.align 2
	.globl _q_atomic_increment
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_increment:
	lwarx  r4,0,r3
        addi   r4,r4,1
        stwcx. r4,0,r3
        bne-   $-12
	mr     r3,r4
	blr

	.align 2
	.globl _q_atomic_decrement
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_decrement:
	lwarx  r4,0,r3
        subi   r4,r4,1
        stwcx. r4,0,r3
        bne-   $-12
	mr     r3,r4
	blr

	.align 2
	.globl _q_atomic_set_int
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_set_int:
	lwarx  r5,0,r3
        stwcx. r4,0,r3
        bne-   $-8
	mr     r3,r5
	blr

	.align 2
	.globl _q_atomic_set_ptr
        .section __TEXT,__text,regular,pure_instructions
	.align 2
_q_atomic_set_ptr:
	lwarx  r5,0,r3
        stwcx. r4,0,r3
        bne-   $-8
	mr     r3,r5
	blr

.globl q_atomic_test_and_set_int.eh
	q_atomic_test_and_set_int.eh = 0
.globl q_atomic_test_and_set_ptr.eh
	q_atomic_test_and_set_ptr.eh = 0
.globl q_atomic_increment.eh
	q_atomic_increment.eh = 0
.globl q_atomic_decrement.eh
	q_atomic_decrement.eh = 0
.globl q_atomic_set_int.eh
	q_atomic_set_int.eh = 0
.globl q_atomic_set_ptr.eh
	q_atomic_set_ptr.eh = 0
.data
.constructor
.data
.destructor
.align 1
