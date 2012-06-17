/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef __KERNEL_MODE__

class slab;
class slabhdr;
#define MAXSLABSIZE		56
#define	PAGESHIFT		12
#define PAGESIZE		(1<<PAGESHIFT)
#define	SLABSHIFT		10
#define	SLABSIZE		(1 << SLABSHIFT)
#define CELLALIGN		8


const unsigned slabfull  = 0;
const TInt	slabsperpage = (int)(PAGESIZE/SLABSIZE);
#define HIBIT(bits) (((unsigned)bits & 0xc) ? 2 + ((unsigned)bits>>3) : ((unsigned) bits>>1))

#define LOWBIT(bits)	(((unsigned) bits&3) ? 1 - ((unsigned)bits&1) : 3 - (((unsigned)bits>>2)&1))

#define ZEROBITS(header)	(((unsigned)header & 0x70000000) ? 0 : 1)

class slabhdr
{
	public:
		unsigned iHeader;
		// made up of
		// bits   |    31    | 30..28 | 27..18 | 17..12 |  11..8  |   7..0   |
		//        +----------+--------+--------+--------+---------+----------+
		// field  | floating |  zero  | used-4 |  size  | pagemap | free pos |
		//
		slab** iParent;		// reference to iParent's pointer to this slab in tree
		slab* iChild1;		// 1st iChild in tree
		slab* iChild2;		// 2nd iChild in tree
};

const TInt KMaxSlabPayload = SLABSIZE - sizeof(slabhdr);
#define MAXUSEDM4BITS   0x0fc00000
#define FLOATING_BIT    0x80000000

inline unsigned HeaderFloating(unsigned h)
{return (h&0x80000000);}
const unsigned maxuse = (SLABSIZE - sizeof(slabhdr))>>2;
const unsigned firstpos = sizeof(slabhdr)>>2;

#ifdef _DEBUG
#define CHECKTREE(x) DoCheckSlabTree(x,EFalse)
#define CHECKSLAB(s,t,p) DoCheckSlab(s,t,p)
#define CHECKSLABBFR(s,p) {TUint32 b[4]; BuildPartialSlabBitmap(b,s,p);}
#else
#define CHECKTREE(x) (void)0
#define CHECKSLAB(s,t,p) (void)0
#define CHECKSLABBFR(s,p) (void)0
#endif

class slabset
{
	public:
		slab* iPartial;
};

class slab : public slabhdr
{
	public:
		void Init(unsigned clz);
		//static slab* SlabFor( void* p);
		static slab* SlabFor(const void* p) ;
		unsigned char iPayload[SLABSIZE-sizeof(slabhdr)];
};

class page
{
	public:
		inline static page* PageFor(slab* s);
		//slab iSlabs;
		slab iSlabs[slabsperpage];
};


inline page* page::PageFor(slab* s)
{
	return reinterpret_cast<page*>((unsigned(s))&~(PAGESIZE-1));
}


#endif   // __KERNEL_MODE__
