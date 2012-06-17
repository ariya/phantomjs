!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!
!! Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
!! All rights reserved.
!! Contact: Nokia Corporation (qt-info@nokia.com)
!!
!! This file is part of the QtGui module of the Qt Toolkit.
!!
!! $QT_BEGIN_LICENSE:LGPL$
!! GNU Lesser General Public License Usage
!! This file may be used under the terms of the GNU Lesser General Public
!! License version 2.1 as published by the Free Software Foundation and
!! appearing in the file LICENSE.LGPL included in the packaging of this
!! file. Please review the following information to ensure the GNU Lesser
!! General Public License version 2.1 requirements will be met:
!! http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
!!
!! In addition, as a special exception, Nokia gives you certain additional
!! rights. These rights are described in the Nokia Qt LGPL Exception
!! version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
!!
!! GNU General Public License Usage
!! Alternatively, this file may be used under the terms of the GNU General
!! Public License version 3.0 as published by the Free Software Foundation
!! and appearing in the file LICENSE.GPL included in the packaging of this
!! file. Please review the following information to ensure the GNU General
!! Public License version 3.0 requirements will be met:
!! http://www.gnu.org/copyleft/gpl.html.
!!
!! Other Usage
!! Alternatively, this file may be used in accordance with the terms and
!! conditions contained in a signed written agreement between you and Nokia.
!!
!!
!!
!!
!!
!! $QT_END_LICENSE$
!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	.section ".text"

	.align 4
	.type q_atomic_trylock_int,#function
	.global q_atomic_trylock_int
q_atomic_trylock_int:
        sethi %hi(-2147483648),%o2
        swap [%o0],%o2
        retl
        mov %o2,%o0
        .size q_atomic_trylock_int,.-q_atomic_trylock_int




        .align 4
        .type q_atomic_trylock_ptr,#function
        .global q_atomic_trylock_ptr
q_atomic_trylock_ptr:
        mov -1, %o2
        swap [%o0], %o2
        retl
        mov %o2, %o0
        .size q_atomic_trylock_ptr,.-q_atomic_trylock_ptr




	.align 4
	.type q_atomic_unlock,#function
	.global q_atomic_unlock
q_atomic_unlock:
	stbar
	retl
	st %o1,[%o0]
	.size q_atomic_unlock,.-q_atomic_unlock




	.align 4
	.type q_atomic_set_int,#function
	.global q_atomic_set_int
q_atomic_set_int:
	swap [%o0],%o1
        stbar
	retl
	mov %o1,%o0
	.size q_atomic_set_int,.-q_atomic_set_int




	.align 4
	.type q_atomic_set_ptr,#function
	.global q_atomic_set_ptr
q_atomic_set_ptr:
	swap [%o0],%o1
        stbar
	retl
	mov %o1,%o0
	.size q_atomic_set_ptr,.-q_atomic_set_ptr

