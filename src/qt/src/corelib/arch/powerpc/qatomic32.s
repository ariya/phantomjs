############################################################################
##
## Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: Nokia Corporation (qt-info@nokia.com)
##
## This file is part of the QtGui module of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## GNU Lesser General Public License Usage
## This file may be used under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation and
## appearing in the file LICENSE.LGPL included in the packaging of this
## file. Please review the following information to ensure the GNU Lesser
## General Public License version 2.1 requirements will be met:
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain additional
## rights. These rights are described in the Nokia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU General
## Public License version 3.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of this
## file. Please review the following information to ensure the GNU General
## Public License version 3.0 requirements will be met:
## http://www.gnu.org/copyleft/gpl.html.
##
## Other Usage
## Alternatively, this file may be used in accordance with the terms and
## conditions contained in a signed written agreement between you and Nokia.
##
##
##
##
##
## $QT_END_LICENSE$
##
############################################################################
	.machine "ppc"
	.toc
	.csect .text[PR]

	.align 2
	.globl q_atomic_test_and_set_int
	.globl .q_atomic_test_and_set_int
	.csect q_atomic_test_and_set_int[DS],3
q_atomic_test_and_set_int:
	.long .q_atomic_test_and_set_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_int:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_int-.q_atomic_test_and_set_int
	.short 25
	.byte "q_atomic_test_and_set_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_acquire_int
	.globl .q_atomic_test_and_set_acquire_int
	.csect q_atomic_test_and_set_acquire_int[DS],3
q_atomic_test_and_set_acquire_int:
	.long .q_atomic_test_and_set_acquire_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_acquire_int:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+16
	stwcx. 5,0,3
	bne-   $-16
        isync
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_acquire_int-.q_atomic_test_and_set_acquire_int
	.short 33
	.byte "q_atomic_test_and_set_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_release_int
	.globl .q_atomic_test_and_set_release_int
	.csect q_atomic_test_and_set_release_int[DS],3
q_atomic_test_and_set_release_int:
	.long .q_atomic_test_and_set_release_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_release_int:
        eieio
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_release_int:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_release_int-.q_atomic_test_and_set_release_int
	.short 33
	.byte "q_atomic_test_and_set_release_int"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_ptr
	.globl .q_atomic_test_and_set_ptr
	.csect q_atomic_test_and_set_ptr[DS],3
q_atomic_test_and_set_ptr:
	.long .q_atomic_test_and_set_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_ptr:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_ptr-.q_atomic_test_and_set_ptr
	.short 25
	.byte "q_atomic_test_and_set_ptr"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_acquire_ptr
	.globl .q_atomic_test_and_set_acquire_ptr
	.csect q_atomic_test_and_set_acquire_ptr[DS],3
q_atomic_test_and_set_acquire_ptr:
	.long .q_atomic_test_and_set_acquire_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_acquire_ptr:
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+16
	stwcx. 5,0,3
	bne-   $-16
        isync
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_acquire_ptr-.q_atomic_test_and_set_acquire_ptr
	.short 25
	.byte "q_atomic_test_and_set_acquire_ptr"
	.align 2

	.align 2
	.globl q_atomic_test_and_set_release_ptr
	.globl .q_atomic_test_and_set_release_ptr
	.csect q_atomic_test_and_set_release_ptr[DS],3
q_atomic_test_and_set_release_ptr:
	.long .q_atomic_test_and_set_release_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_test_and_set_release_ptr:
        eieio
	lwarx  6,0,3
        xor.   6,6,4
        bne    $+12
	stwcx. 5,0,3
	bne-   $-16
        subfic 3,6,0
        adde   3,3,6
	blr
LT..q_atomic_test_and_set_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,3,0
	.long 0
	.long LT..q_atomic_test_and_set_release_ptr-.q_atomic_test_and_set_release_ptr
	.short 33
	.byte "q_atomic_test_and_set_release_ptr"
	.align 2

	.align 2
	.globl q_atomic_increment
	.globl .q_atomic_increment
	.csect q_atomic_increment[DS],3
q_atomic_increment:
	.long .q_atomic_increment,TOC[tc0],0
	.csect .text[PR]
.q_atomic_increment:
	lwarx  4,0,3
	addi   4,4,1
	stwcx. 4,0,3
	bne-   $-12
	mr     3,4
	blr
LT..q_atomic_increment:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_increment-.q_atomic_increment
	.short 18
	.byte "q_atomic_increment"
	.align 2

	.align 2
	.globl q_atomic_decrement
	.globl .q_atomic_decrement
	.csect q_atomic_decrement[DS],3
q_atomic_decrement:
	.long .q_atomic_decrement,TOC[tc0],0
	.csect .text[PR]
.q_atomic_decrement:
	lwarx  4,0,3
	subi   4,4,1
	stwcx. 4,0,3
	bne-   $-12
	mr     3,4
	blr
LT..q_atomic_decrement:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_decrement-.q_atomic_decrement
	.short 18
	.byte "q_atomic_decrement"
	.align 2

	.align 2
	.globl q_atomic_set_int
	.globl .q_atomic_set_int
	.csect q_atomic_set_int[DS],3
q_atomic_set_int:
	.long .q_atomic_set_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_set_int:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_set_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_set_int-.q_atomic_set_int
	.short 16
	.byte "q_atomic_set_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_acquire_int
	.globl .q_atomic_fetch_and_store_acquire_int
	.csect q_atomic_fetch_and_store_acquire_int[DS],3
q_atomic_fetch_and_store_acquire_int:
	.long .q_atomic_fetch_and_store_acquire_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_store_acquire_int:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_acquire_int-.q_atomic_fetch_and_store_acquire_int
	.short 16
	.byte "q_atomic_fetch_and_store_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_release_int
	.globl .q_atomic_fetch_and_store_release_int
	.csect q_atomic_fetch_and_store_release_int[DS],3
q_atomic_fetch_and_store_release_int:
	.long .q_atomic_fetch_and_store_release_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_store_release_int:
        eieio
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_release_int:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_release_int-.q_atomic_fetch_and_store_release_int
	.short 16
	.byte "q_atomic_fetch_and_store_release_int"
	.align 2

        .align 2
	.globl q_atomic_set_ptr
	.globl .q_atomic_set_ptr
	.csect q_atomic_set_ptr[DS],3
q_atomic_set_ptr:
	.long .q_atomic_set_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_set_ptr:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_set_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_set_ptr-.q_atomic_set_ptr
	.short 16
	.byte "q_atomic_set_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_store_acquire_ptr
	.globl .q_atomic_fetch_and_store_acquire_ptr
	.csect q_atomic_fetch_and_store_acquire_ptr[DS],3
q_atomic_fetch_and_store_acquire_ptr:
	.long .q_atomic_fetch_and_store_acquire_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_store_acquire_ptr:
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_acquire_ptr-.q_atomic_fetch_and_store_acquire_ptr
	.short 16
	.byte "q_atomic_fetch_and_store_acquire_ptr"
	.align 2

        .align 2
	.globl q_atomic_fetch_and_store_release_ptr
	.globl .q_atomic_fetch_and_store_release_ptr
	.csect q_atomic_fetch_and_store_release_ptr[DS],3
q_atomic_fetch_and_store_release_ptr:
	.long .q_atomic_fetch_and_store_release_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_store_release_ptr:
        eieio
	lwarx  5,0,3
	stwcx. 4,0,3
	bne-   $-8
	mr     3,5
	blr
LT..q_atomic_fetch_and_store_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,2,0
	.long 0
	.long LT..q_atomic_fetch_and_store_release_ptr-.q_atomic_fetch_and_store_release_ptr
	.short 16
	.byte "q_atomic_fetch_and_store_release_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_int
	.globl .q_atomic_fetch_and_add_int
	.csect q_atomic_fetch_and_add_int[DS],3
q_atomic_fetch_and_add_int:
	.long .q_atomic_fetch_and_add_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_int:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_int-.q_atomic_fetch_and_add_int
	.short 18
	.byte "q_atomic_fetch_and_add_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_acquire_int
	.globl .q_atomic_fetch_and_add_acquire_int
	.csect q_atomic_fetch_and_add_acquire_int[DS],3
q_atomic_fetch_and_add_acquire_int:
	.long .q_atomic_fetch_and_add_acquire_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_acquire_int:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_acquire_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_acquire_int-.q_atomic_fetch_and_add_acquire_int
	.short 18
	.byte "q_atomic_fetch_and_add_acquire_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_release_int
	.globl .q_atomic_fetch_and_add_release_int
	.csect q_atomic_fetch_and_add_release_int[DS],3
q_atomic_fetch_and_add_release_int:
	.long .q_atomic_fetch_and_add_release_int,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_release_int:
        eieio
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_release_int:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_release_int-.q_atomic_fetch_and_add_release_int
	.short 34
	.byte "q_atomic_fetch_and_add_release_int"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_ptr
	.globl .q_atomic_fetch_and_add_ptr
	.csect q_atomic_fetch_and_add_ptr[DS],3
q_atomic_fetch_and_add_ptr:
	.long .q_atomic_fetch_and_add_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_ptr:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_ptr-.q_atomic_fetch_and_add_ptr
	.short 26
	.byte "q_atomic_fetch_and_add_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_acquire_ptr
	.globl .q_atomic_fetch_and_add_acquire_ptr
	.csect q_atomic_fetch_and_add_acquire_ptr[DS],3
q_atomic_fetch_and_add_acquire_ptr:
	.long .q_atomic_fetch_and_add_acquire_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_acquire_ptr:
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
        isync
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_acquire_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_acquire_ptr-.q_atomic_fetch_and_add_acquire_ptr
	.short 34
	.byte "q_atomic_fetch_and_add_acquire_ptr"
	.align 2

	.align 2
	.globl q_atomic_fetch_and_add_release_ptr
	.globl .q_atomic_fetch_and_add_release_ptr
	.csect q_atomic_fetch_and_add_release_ptr[DS],3
q_atomic_fetch_and_add_release_ptr:
	.long .q_atomic_fetch_and_add_release_ptr,TOC[tc0],0
	.csect .text[PR]
.q_atomic_fetch_and_add_release_ptr:
        eieio
	lwarx  5,0,3
	add    6,4,5
	stwcx. 6,0,3
	bne-   $-12
	mr     3,5
	blr
LT..q_atomic_fetch_and_add_release_ptr:
	.long 0
	.byte 0,9,32,64,0,0,1,0
	.long 0
	.long LT..q_atomic_fetch_and_add_release_ptr-.q_atomic_fetch_and_add_release_ptr
	.short 34
	.byte "q_atomic_fetch_and_add_release_ptr"
	.align 2

_section_.text:
	.csect .data[RW],3
	.long _section_.text
