#include "config.h"
#include "SlotVisitor.h"
#include "SlotVisitorInlines.h"

#include "ConservativeRoots.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlines.h"
#include "GCThread.h"
#include "JSArray.h"
#include "JSDestructibleObject.h"
#include "VM.h"
#include "JSObject.h"
#include "JSString.h"
#include "Operations.h"
#include <wtf/StackStats.h>

namespace JSC {

SlotVisitor::SlotVisitor(GCThreadSharedData& shared)
    : m_stack(shared.m_vm->heap.blockAllocator())
    , m_visitCount(0)
    , m_isInParallelMode(false)
    , m_shared(shared)
    , m_shouldHashCons(false)
#if !ASSERT_DISABLED
    , m_isCheckingForDefaultMarkViolation(false)
    , m_isDraining(false)
#endif
{
}

SlotVisitor::~SlotVisitor()
{
    ASSERT(m_stack.isEmpty());
}

void SlotVisitor::setup()
{
    m_shared.m_shouldHashCons = m_shared.m_vm->haveEnoughNewStringsToHashCons();
    m_shouldHashCons = m_shared.m_shouldHashCons;
#if ENABLE(PARALLEL_GC)
    for (unsigned i = 0; i < m_shared.m_gcThreads.size(); ++i)
        m_shared.m_gcThreads[i]->slotVisitor()->m_shouldHashCons = m_shared.m_shouldHashCons;
#endif
}

void SlotVisitor::reset()
{
    m_visitCount = 0;
    ASSERT(m_stack.isEmpty());
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty()); // Should have merged by now.
#else
    m_opaqueRoots.clear();
#endif
    if (m_shouldHashCons) {
        m_uniqueStrings.clear();
        m_shouldHashCons = false;
    }
}

void SlotVisitor::append(ConservativeRoots& conservativeRoots)
{
    StackStats::probe();
    JSCell** roots = conservativeRoots.roots();
    size_t size = conservativeRoots.size();
    for (size_t i = 0; i < size; ++i)
        internalAppend(roots[i]);
}

ALWAYS_INLINE static void visitChildren(SlotVisitor& visitor, const JSCell* cell)
{
    StackStats::probe();
#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_visitedTypeCounts.count(cell);
#endif

    ASSERT(Heap::isMarked(cell));
    
    if (isJSString(cell)) {
        JSString::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSFinalObject(cell)) {
        JSFinalObject::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSArray(cell)) {
        JSArray::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    cell->methodTable()->visitChildren(const_cast<JSCell*>(cell), visitor);
}

void SlotVisitor::donateKnownParallel()
{
    StackStats::probe();
    // NOTE: Because we re-try often, we can afford to be conservative, and
    // assume that donating is not profitable.

    // Avoid locking when a thread reaches a dead end in the object graph.
    if (m_stack.size() < 2)
        return;

    // If there's already some shared work queued up, be conservative and assume
    // that donating more is not profitable.
    if (m_shared.m_sharedMarkStack.size())
        return;

    // If we're contending on the lock, be conservative and assume that another
    // thread is already donating.
    MutexTryLocker locker(m_shared.m_markingLock);
    if (!locker.locked())
        return;

    // Otherwise, assume that a thread will go idle soon, and donate.
    m_stack.donateSomeCellsTo(m_shared.m_sharedMarkStack);

    if (m_shared.m_numberOfActiveParallelMarkers < Options::numberOfGCMarkers())
        m_shared.m_markingCondition.broadcast();
}

void SlotVisitor::drain()
{
    StackStats::probe();
    ASSERT(m_isInParallelMode);
   
#if ENABLE(PARALLEL_GC)
    if (Options::numberOfGCMarkers() > 1) {
        while (!m_stack.isEmpty()) {
            m_stack.refill();
            for (unsigned countdown = Options::minimumNumberOfScansBetweenRebalance(); m_stack.canRemoveLast() && countdown--;)
                visitChildren(*this, m_stack.removeLast());
            donateKnownParallel();
        }
        
        mergeOpaqueRootsIfNecessary();
        return;
    }
#endif
    
    while (!m_stack.isEmpty()) {
        m_stack.refill();
        while (m_stack.canRemoveLast())
            visitChildren(*this, m_stack.removeLast());
    }
}

void SlotVisitor::drainFromShared(SharedDrainMode sharedDrainMode)
{
    StackStats::probe();
    ASSERT(m_isInParallelMode);
    
    ASSERT(Options::numberOfGCMarkers());
    
    bool shouldBeParallel;

#if ENABLE(PARALLEL_GC)
    shouldBeParallel = Options::numberOfGCMarkers() > 1;
#else
    ASSERT(Options::numberOfGCMarkers() == 1);
    shouldBeParallel = false;
#endif
    
    if (!shouldBeParallel) {
        // This call should be a no-op.
        ASSERT_UNUSED(sharedDrainMode, sharedDrainMode == MasterDrain);
        ASSERT(m_stack.isEmpty());
        ASSERT(m_shared.m_sharedMarkStack.isEmpty());
        return;
    }
    
#if ENABLE(PARALLEL_GC)
    {
        MutexLocker locker(m_shared.m_markingLock);
        m_shared.m_numberOfActiveParallelMarkers++;
    }
    while (true) {
        {
            MutexLocker locker(m_shared.m_markingLock);
            m_shared.m_numberOfActiveParallelMarkers--;

            // How we wait differs depending on drain mode.
            if (sharedDrainMode == MasterDrain) {
                // Wait until either termination is reached, or until there is some work
                // for us to do.
                while (true) {
                    // Did we reach termination?
                    if (!m_shared.m_numberOfActiveParallelMarkers && m_shared.m_sharedMarkStack.isEmpty()) {
                        // Let any sleeping slaves know it's time for them to return;
                        m_shared.m_markingCondition.broadcast();
                        return;
                    }
                    
                    // Is there work to be done?
                    if (!m_shared.m_sharedMarkStack.isEmpty())
                        break;
                    
                    // Otherwise wait.
                    m_shared.m_markingCondition.wait(m_shared.m_markingLock);
                }
            } else {
                ASSERT(sharedDrainMode == SlaveDrain);
                
                // Did we detect termination? If so, let the master know.
                if (!m_shared.m_numberOfActiveParallelMarkers && m_shared.m_sharedMarkStack.isEmpty())
                    m_shared.m_markingCondition.broadcast();
                
                while (m_shared.m_sharedMarkStack.isEmpty() && !m_shared.m_parallelMarkersShouldExit)
                    m_shared.m_markingCondition.wait(m_shared.m_markingLock);
                
                // Is the current phase done? If so, return from this function.
                if (m_shared.m_parallelMarkersShouldExit)
                    return;
            }
           
            size_t idleThreadCount = Options::numberOfGCMarkers() - m_shared.m_numberOfActiveParallelMarkers;
            m_stack.stealSomeCellsFrom(m_shared.m_sharedMarkStack, idleThreadCount);
            m_shared.m_numberOfActiveParallelMarkers++;
        }
        
        drain();
    }
#endif
}

void SlotVisitor::mergeOpaqueRoots()
{
    StackStats::probe();
    ASSERT(!m_opaqueRoots.isEmpty()); // Should only be called when opaque roots are non-empty.
    {
        MutexLocker locker(m_shared.m_opaqueRootsLock);
        HashSet<void*>::iterator begin = m_opaqueRoots.begin();
        HashSet<void*>::iterator end = m_opaqueRoots.end();
        for (HashSet<void*>::iterator iter = begin; iter != end; ++iter)
            m_shared.m_opaqueRoots.add(*iter);
    }
    m_opaqueRoots.clear();
}

ALWAYS_INLINE bool JSString::tryHashConsLock()
{
#if ENABLE(PARALLEL_GC)
    unsigned currentFlags = m_flags;

    if (currentFlags & HashConsLock)
        return false;

    unsigned newFlags = currentFlags | HashConsLock;

    if (!WTF::weakCompareAndSwap(&m_flags, currentFlags, newFlags))
        return false;

    WTF::memoryBarrierAfterLock();
    return true;
#else
    if (isHashConsSingleton())
        return false;

    m_flags |= HashConsLock;

    return true;
#endif
}

ALWAYS_INLINE void JSString::releaseHashConsLock()
{
#if ENABLE(PARALLEL_GC)
    WTF::memoryBarrierBeforeUnlock();
#endif
    m_flags &= ~HashConsLock;
}

ALWAYS_INLINE bool JSString::shouldTryHashCons()
{
    return ((length() > 1) && !isRope() && !isHashConsSingleton());
}

ALWAYS_INLINE void SlotVisitor::internalAppend(JSValue* slot)
{
    // This internalAppend is only intended for visits to object and array backing stores.
    // as it can change the JSValue pointed to be the argument when the original JSValue
    // is a string that contains the same contents as another string.

    StackStats::probe();
    ASSERT(slot);
    JSValue value = *slot;
    ASSERT(value);
    if (!value.isCell())
        return;

    JSCell* cell = value.asCell();
    if (!cell)
        return;

    validate(cell);

    if (m_shouldHashCons && cell->isString()) {
        JSString* string = jsCast<JSString*>(cell);
        if (string->shouldTryHashCons() && string->tryHashConsLock()) {
            UniqueStringMap::AddResult addResult = m_uniqueStrings.add(string->string().impl(), value);
            if (addResult.isNewEntry)
                string->setHashConsSingleton();
            else {
                JSValue existingJSValue = addResult.iterator->value;
                if (value != existingJSValue)
                    jsCast<JSString*>(existingJSValue.asCell())->clearHashConsSingleton();
                *slot = existingJSValue;
                string->releaseHashConsLock();
                return;
            }
            string->releaseHashConsLock();
        }
    }

    internalAppend(cell);
}

void SlotVisitor::harvestWeakReferences()
{
    StackStats::probe();
    for (WeakReferenceHarvester* current = m_shared.m_weakReferenceHarvesters.head(); current; current = current->next())
        current->visitWeakReferences(*this);
}

void SlotVisitor::finalizeUnconditionalFinalizers()
{
    StackStats::probe();
    while (m_shared.m_unconditionalFinalizers.hasNext())
        m_shared.m_unconditionalFinalizers.removeNext()->finalizeUnconditionally();
}

#if ENABLE(GC_VALIDATION)
void SlotVisitor::validate(JSCell* cell)
{
    RELEASE_ASSERT(cell);

    if (!cell->structure()) {
        dataLogF("cell at %p has a null structure\n" , cell);
        CRASH();
    }

    // Both the cell's structure, and the cell's structure's structure should be the Structure Structure.
    // I hate this sentence.
    if (cell->structure()->structure()->JSCell::classInfo() != cell->structure()->JSCell::classInfo()) {
        const char* parentClassName = 0;
        const char* ourClassName = 0;
        if (cell->structure()->structure() && cell->structure()->structure()->JSCell::classInfo())
            parentClassName = cell->structure()->structure()->JSCell::classInfo()->className;
        if (cell->structure()->JSCell::classInfo())
            ourClassName = cell->structure()->JSCell::classInfo()->className;
        dataLogF("parent structure (%p <%s>) of cell at %p doesn't match cell's structure (%p <%s>)\n",
                cell->structure()->structure(), parentClassName, cell, cell->structure(), ourClassName);
        CRASH();
    }

    // Make sure we can walk the ClassInfo chain
    const ClassInfo* info = cell->classInfo();
    do { } while ((info = info->parentClass));
}
#else
void SlotVisitor::validate(JSCell*)
{
}
#endif

} // namespace JSC
