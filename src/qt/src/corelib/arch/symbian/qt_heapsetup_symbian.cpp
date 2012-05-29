/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qt_hybridheap_symbian_p.h"

#ifdef QT_USE_NEW_SYMBIAN_ALLOCATOR

extern const TInt KHeapShrinkHysRatio = 0x800;

/*
 * \internal
 * Called from the qtmain.lib application wrapper.
 * Create a new heap as requested, but use the new allocator
 */
Q_CORE_EXPORT TInt qt_symbian_SetupThreadHeap(TBool aNotFirst, SStdEpocThreadCreateInfo& aInfo)
{
    TInt r = KErrNone;
    if (!aInfo.iAllocator && aInfo.iHeapInitialSize>0)
    {
        // new heap required
        RHeap* pH = NULL;
        r = UserHeap::CreateThreadHeap(aInfo, pH);
    }
    else if (aInfo.iAllocator)
    {
        // sharing a heap
        RAllocator* pA = aInfo.iAllocator;
        pA->Open();
        User::SwitchAllocator(pA);
    }
    return r;
}

#ifndef NO_NAMED_LOCAL_CHUNKS
void TChunkCreateInfo::SetThreadHeap(TInt aInitialSize, TInt aMaxSize, const TDesC& aName)
{
    iType = TChunkCreate::ENormal | TChunkCreate::EData;
    iMaxSize = aMaxSize;
    iInitialBottom = 0;
    iInitialTop = aInitialSize;
    iAttributes |= TChunkCreate::ELocalNamed;
    iName = &aName;
    iOwnerType = EOwnerThread;
}
#endif // NO_NAMED_LOCAL_CHUNKS

void Panic(TCdtPanic reason)
{
    _LIT(KCat, "QtHybridHeap");
    User::Panic(KCat, reason);
}

#else /* QT_USE_NEW_SYMBIAN_ALLOCATOR */

#include <e32std.h>

/*
 * \internal
 * Called from the qtmain.lib application wrapper.
 * Create a new heap as requested, using the default system allocator
 */
Q_CORE_EXPORT TInt qt_symbian_SetupThreadHeap(TBool aNotFirst, SStdEpocThreadCreateInfo& aInfo)
{
    return UserHeap::SetupThreadHeap(aNotFirst, aInfo);
}

#endif /* QT_USE_NEW_SYMBIAN_ALLOCATOR */
