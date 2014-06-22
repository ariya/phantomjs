/*
 * Copyright (C) 2008, 2009, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Structure.h"

#include "CodeBlock.h"
#include "JSObject.h"
#include "JSPropertyNameIterator.h"
#include "Lookup.h"
#include "PropertyNameArray.h"
#include "StructureChain.h"
#include "StructureRareDataInlines.h"
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

#define DUMP_STRUCTURE_ID_STATISTICS 0

#ifndef NDEBUG
#define DO_PROPERTYMAP_CONSTENCY_CHECK 0
#else
#define DO_PROPERTYMAP_CONSTENCY_CHECK 0
#endif

using namespace std;
using namespace WTF;

#if DUMP_PROPERTYMAP_STATS

int numProbes;
int numCollisions;
int numRehashes;
int numRemoves;

#endif

namespace JSC {

#if DUMP_STRUCTURE_ID_STATISTICS
static HashSet<Structure*>& liveStructureSet = *(new HashSet<Structure*>);
#endif

bool StructureTransitionTable::contains(StringImpl* rep, unsigned attributes) const
{
    if (isUsingSingleSlot()) {
        Structure* transition = singleTransition();
        return transition && transition->m_nameInPrevious == rep && transition->m_attributesInPrevious == attributes;
    }
    return map()->get(make_pair(rep, attributes));
}

inline Structure* StructureTransitionTable::get(StringImpl* rep, unsigned attributes) const
{
    if (isUsingSingleSlot()) {
        Structure* transition = singleTransition();
        return (transition && transition->m_nameInPrevious == rep && transition->m_attributesInPrevious == attributes) ? transition : 0;
    }
    return map()->get(make_pair(rep, attributes));
}

inline void StructureTransitionTable::add(VM& vm, Structure* structure)
{
    if (isUsingSingleSlot()) {
        Structure* existingTransition = singleTransition();

        // This handles the first transition being added.
        if (!existingTransition) {
            setSingleTransition(vm, structure);
            return;
        }

        // This handles the second transition being added
        // (or the first transition being despecified!)
        setMap(new TransitionMap());
        add(vm, existingTransition);
    }

    // Add the structure to the map.

    // Newer versions of the STL have an std::make_pair function that takes rvalue references.
    // When either of the parameters are bitfields, the C++ compiler will try to bind them as lvalues, which is invalid. To work around this, use unary "+" to make the parameter an rvalue.
    // See https://bugs.webkit.org/show_bug.cgi?id=59261 for more details
    map()->set(make_pair(structure->m_nameInPrevious, +structure->m_attributesInPrevious), structure);
}

void Structure::dumpStatistics()
{
#if DUMP_STRUCTURE_ID_STATISTICS
    unsigned numberLeaf = 0;
    unsigned numberUsingSingleSlot = 0;
    unsigned numberSingletons = 0;
    unsigned numberWithPropertyMaps = 0;
    unsigned totalPropertyMapsSize = 0;

    HashSet<Structure*>::const_iterator end = liveStructureSet.end();
    for (HashSet<Structure*>::const_iterator it = liveStructureSet.begin(); it != end; ++it) {
        Structure* structure = *it;

        switch (structure->m_transitionTable.size()) {
            case 0:
                ++numberLeaf;
                if (!structure->previousID())
                    ++numberSingletons;
                break;

            case 1:
                ++numberUsingSingleSlot;
                break;
        }

        if (structure->propertyTable()) {
            ++numberWithPropertyMaps;
            totalPropertyMapsSize += structure->propertyTable()->sizeInMemory();
        }
    }

    dataLogF("Number of live Structures: %d\n", liveStructureSet.size());
    dataLogF("Number of Structures using the single item optimization for transition map: %d\n", numberUsingSingleSlot);
    dataLogF("Number of Structures that are leaf nodes: %d\n", numberLeaf);
    dataLogF("Number of Structures that singletons: %d\n", numberSingletons);
    dataLogF("Number of Structures with PropertyMaps: %d\n", numberWithPropertyMaps);

    dataLogF("Size of a single Structures: %d\n", static_cast<unsigned>(sizeof(Structure)));
    dataLogF("Size of sum of all property maps: %d\n", totalPropertyMapsSize);
    dataLogF("Size of average of all property maps: %f\n", static_cast<double>(totalPropertyMapsSize) / static_cast<double>(liveStructureSet.size()));
#else
    dataLogF("Dumping Structure statistics is not enabled.\n");
#endif
}

Structure::Structure(VM& vm, JSGlobalObject* globalObject, JSValue prototype, const TypeInfo& typeInfo, const ClassInfo* classInfo, IndexingType indexingType, unsigned inlineCapacity)
    : JSCell(vm, vm.structureStructure.get())
    , m_globalObject(vm, this, globalObject, WriteBarrier<JSGlobalObject>::MayBeNull)
    , m_prototype(vm, this, prototype)
    , m_classInfo(classInfo)
    , m_transitionWatchpointSet(InitializedWatching)
    , m_offset(invalidOffset)
    , m_typeInfo(typeInfo)
    , m_indexingType(indexingType)
    , m_inlineCapacity(inlineCapacity)
    , m_dictionaryKind(NoneDictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(false)
    , m_hasReadOnlyOrGetterSetterPropertiesExcludingProto(false)
    , m_hasNonEnumerableProperties(false)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(0)
    , m_preventExtensions(false)
    , m_didTransition(false)
    , m_staticFunctionReified(false)
{
    ASSERT(inlineCapacity <= JSFinalObject::maxInlineCapacity());
    ASSERT(static_cast<PropertyOffset>(inlineCapacity) < firstOutOfLineOffset);
    ASSERT(!typeInfo.structureHasRareData());
}

const ClassInfo Structure::s_info = { "Structure", 0, 0, 0, CREATE_METHOD_TABLE(Structure) };

Structure::Structure(VM& vm)
    : JSCell(CreatingEarlyCell)
    , m_prototype(vm, this, jsNull())
    , m_classInfo(&s_info)
    , m_transitionWatchpointSet(InitializedWatching)
    , m_offset(invalidOffset)
    , m_typeInfo(CompoundType, OverridesVisitChildren)
    , m_indexingType(0)
    , m_inlineCapacity(0)
    , m_dictionaryKind(NoneDictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(false)
    , m_hasReadOnlyOrGetterSetterPropertiesExcludingProto(false)
    , m_hasNonEnumerableProperties(false)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(0)
    , m_preventExtensions(false)
    , m_didTransition(false)
    , m_staticFunctionReified(false)
{
}

Structure::Structure(VM& vm, const Structure* previous)
    : JSCell(vm, vm.structureStructure.get())
    , m_prototype(vm, this, previous->storedPrototype())
    , m_classInfo(previous->m_classInfo)
    , m_transitionWatchpointSet(InitializedWatching)
    , m_offset(invalidOffset)
    , m_typeInfo(previous->typeInfo().type(), previous->typeInfo().flags() & ~StructureHasRareData)
    , m_indexingType(previous->indexingTypeIncludingHistory())
    , m_inlineCapacity(previous->m_inlineCapacity)
    , m_dictionaryKind(previous->m_dictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(previous->m_hasGetterSetterProperties)
    , m_hasReadOnlyOrGetterSetterPropertiesExcludingProto(previous->m_hasReadOnlyOrGetterSetterPropertiesExcludingProto)
    , m_hasNonEnumerableProperties(previous->m_hasNonEnumerableProperties)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(previous->m_specificFunctionThrashCount)
    , m_preventExtensions(previous->m_preventExtensions)
    , m_didTransition(true)
    , m_staticFunctionReified(previous->m_staticFunctionReified)
{
    if (previous->typeInfo().structureHasRareData() && previous->rareData()->needsCloning())
        cloneRareDataFrom(vm, previous);
    else if (previous->previousID())
        m_previousOrRareData.set(vm, this, previous->previousID());

    previous->notifyTransitionFromThisStructure();
    if (previous->m_globalObject)
        m_globalObject.set(vm, this, previous->m_globalObject.get());
}

void Structure::destroy(JSCell* cell)
{
    static_cast<Structure*>(cell)->Structure::~Structure();
}

void Structure::materializePropertyMap(VM& vm)
{
    ASSERT(structure()->classInfo() == &s_info);
    ASSERT(!propertyTable());

    Vector<Structure*, 8> structures;
    structures.append(this);

    Structure* structure = this;

    // Search for the last Structure with a property table.
    while ((structure = structure->previousID())) {
        if (structure->m_isPinnedPropertyTable) {
            ASSERT(structure->propertyTable());
            ASSERT(!structure->previousID());

            propertyTable().set(vm, this, structure->propertyTable()->copy(vm, 0, numberOfSlotsForLastOffset(m_offset, m_inlineCapacity)));
            break;
        }

        structures.append(structure);
    }

    if (!propertyTable())
        createPropertyMap(vm, numberOfSlotsForLastOffset(m_offset, m_inlineCapacity));

    for (ptrdiff_t i = structures.size() - 1; i >= 0; --i) {
        structure = structures[i];
        if (!structure->m_nameInPrevious)
            continue;
        PropertyMapEntry entry(vm, this, structure->m_nameInPrevious.get(), structure->m_offset, structure->m_attributesInPrevious, structure->m_specificValueInPrevious.get());
        propertyTable()->add(entry, m_offset, PropertyTable::PropertyOffsetMustNotChange);
    }
    
    checkOffsetConsistency();
}

inline size_t nextOutOfLineStorageCapacity(size_t currentCapacity)
{
    if (!currentCapacity)
        return initialOutOfLineCapacity;
    return currentCapacity * outOfLineGrowthFactor;
}

size_t Structure::suggestedNewOutOfLineStorageCapacity()
{
    return nextOutOfLineStorageCapacity(outOfLineCapacity());
}
 
void Structure::despecifyDictionaryFunction(VM& vm, PropertyName propertyName)
{
    StringImpl* rep = propertyName.uid();

    materializePropertyMapIfNecessary(vm);

    ASSERT(isDictionary());
    ASSERT(propertyTable());

    PropertyMapEntry* entry = propertyTable()->find(rep).first;
    ASSERT(entry);
    entry->specificValue.clear();
}

Structure* Structure::addPropertyTransitionToExistingStructure(Structure* structure, PropertyName propertyName, unsigned attributes, JSCell* specificValue, PropertyOffset& offset)
{
    ASSERT(!structure->isDictionary());
    ASSERT(structure->isObject());

    if (Structure* existingTransition = structure->m_transitionTable.get(propertyName.uid(), attributes)) {
        JSCell* specificValueInPrevious = existingTransition->m_specificValueInPrevious.get();
        if (specificValueInPrevious && specificValueInPrevious != specificValue)
            return 0;
        validateOffset(existingTransition->m_offset, existingTransition->inlineCapacity());
        offset = existingTransition->m_offset;
        return existingTransition;
    }

    return 0;
}

bool Structure::anyObjectInChainMayInterceptIndexedAccesses() const
{
    for (const Structure* current = this; ;) {
        if (current->mayInterceptIndexedAccesses())
            return true;
        
        JSValue prototype = current->storedPrototype();
        if (prototype.isNull())
            return false;
        
        current = asObject(prototype)->structure();
    }
}

bool Structure::needsSlowPutIndexing() const
{
    return anyObjectInChainMayInterceptIndexedAccesses()
        || globalObject()->isHavingABadTime();
}

NonPropertyTransition Structure::suggestedArrayStorageTransition() const
{
    if (needsSlowPutIndexing())
        return AllocateSlowPutArrayStorage;
    
    return AllocateArrayStorage;
}

Structure* Structure::addPropertyTransition(VM& vm, Structure* structure, PropertyName propertyName, unsigned attributes, JSCell* specificValue, PropertyOffset& offset)
{
    // If we have a specific function, we may have got to this point if there is
    // already a transition with the correct property name and attributes, but
    // specialized to a different function.  In this case we just want to give up
    // and despecialize the transition.
    // In this case we clear the value of specificFunction which will result
    // in us adding a non-specific transition, and any subsequent lookup in
    // Structure::addPropertyTransitionToExistingStructure will just use that.
    if (specificValue && structure->m_transitionTable.contains(propertyName.uid(), attributes))
        specificValue = 0;

    ASSERT(!structure->isDictionary());
    ASSERT(structure->isObject());
    ASSERT(!Structure::addPropertyTransitionToExistingStructure(structure, propertyName, attributes, specificValue, offset));
    
    if (structure->m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        specificValue = 0;

    if (structure->transitionCount() > s_maxTransitionLength) {
        Structure* transition = toCacheableDictionaryTransition(vm, structure);
        ASSERT(structure != transition);
        offset = transition->putSpecificValue(vm, propertyName, attributes, specificValue);
        return transition;
    }
    
    Structure* transition = create(vm, structure);

    transition->m_cachedPrototypeChain.setMayBeNull(vm, transition, structure->m_cachedPrototypeChain.get());
    transition->setPreviousID(vm, transition, structure);
    transition->m_nameInPrevious = propertyName.uid();
    transition->m_attributesInPrevious = attributes;
    transition->m_specificValueInPrevious.setMayBeNull(vm, transition, specificValue);
    transition->propertyTable().set(vm, transition, structure->takePropertyTableOrCloneIfPinned(vm, transition));
    transition->m_offset = structure->m_offset;

    offset = transition->putSpecificValue(vm, propertyName, attributes, specificValue);

    checkOffset(transition->m_offset, transition->inlineCapacity());
    structure->m_transitionTable.add(vm, transition);
    transition->checkOffsetConsistency();
    structure->checkOffsetConsistency();
    return transition;
}

Structure* Structure::removePropertyTransition(VM& vm, Structure* structure, PropertyName propertyName, PropertyOffset& offset)
{
    ASSERT(!structure->isUncacheableDictionary());

    Structure* transition = toUncacheableDictionaryTransition(vm, structure);

    offset = transition->remove(propertyName);

    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::changePrototypeTransition(VM& vm, Structure* structure, JSValue prototype)
{
    Structure* transition = create(vm, structure);

    transition->m_prototype.set(vm, transition, prototype);

    structure->materializePropertyMapIfNecessary(vm);
    transition->propertyTable().set(vm, transition, structure->copyPropertyTableForPinning(vm, transition));
    transition->m_offset = structure->m_offset;
    transition->pin();

    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::despecifyFunctionTransition(VM& vm, Structure* structure, PropertyName replaceFunction)
{
    ASSERT(structure->m_specificFunctionThrashCount < maxSpecificFunctionThrashCount);
    Structure* transition = create(vm, structure);

    ++transition->m_specificFunctionThrashCount;

    structure->materializePropertyMapIfNecessary(vm);
    transition->propertyTable().set(vm, transition, structure->copyPropertyTableForPinning(vm, transition));
    transition->m_offset = structure->m_offset;
    transition->pin();

    if (transition->m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        transition->despecifyAllFunctions(vm);
    else {
        bool removed = transition->despecifyFunction(vm, replaceFunction);
        ASSERT_UNUSED(removed, removed);
    }

    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::attributeChangeTransition(VM& vm, Structure* structure, PropertyName propertyName, unsigned attributes)
{
    if (!structure->isUncacheableDictionary()) {
        Structure* transition = create(vm, structure);

        structure->materializePropertyMapIfNecessary(vm);
        transition->propertyTable().set(vm, transition, structure->copyPropertyTableForPinning(vm, transition));
        transition->m_offset = structure->m_offset;
        transition->pin();
        
        structure = transition;
    }

    ASSERT(structure->propertyTable());
    PropertyMapEntry* entry = structure->propertyTable()->find(propertyName.uid()).first;
    ASSERT(entry);
    entry->attributes = attributes;

    structure->checkOffsetConsistency();
    return structure;
}

Structure* Structure::toDictionaryTransition(VM& vm, Structure* structure, DictionaryKind kind)
{
    ASSERT(!structure->isUncacheableDictionary());
    
    Structure* transition = create(vm, structure);

    structure->materializePropertyMapIfNecessary(vm);
    transition->propertyTable().set(vm, transition, structure->copyPropertyTableForPinning(vm, transition));
    transition->m_offset = structure->m_offset;
    transition->m_dictionaryKind = kind;
    transition->pin();

    transition->checkOffsetConsistency();
    return transition;
}

Structure* Structure::toCacheableDictionaryTransition(VM& vm, Structure* structure)
{
    return toDictionaryTransition(vm, structure, CachedDictionaryKind);
}

Structure* Structure::toUncacheableDictionaryTransition(VM& vm, Structure* structure)
{
    return toDictionaryTransition(vm, structure, UncachedDictionaryKind);
}

// In future we may want to cache this transition.
Structure* Structure::sealTransition(VM& vm, Structure* structure)
{
    Structure* transition = preventExtensionsTransition(vm, structure);

    if (transition->propertyTable()) {
        PropertyTable::iterator end = transition->propertyTable()->end();
        for (PropertyTable::iterator iter = transition->propertyTable()->begin(); iter != end; ++iter)
            iter->attributes |= DontDelete;
    }

    transition->checkOffsetConsistency();
    return transition;
}

// In future we may want to cache this transition.
Structure* Structure::freezeTransition(VM& vm, Structure* structure)
{
    Structure* transition = preventExtensionsTransition(vm, structure);

    if (transition->propertyTable()) {
        PropertyTable::iterator iter = transition->propertyTable()->begin();
        PropertyTable::iterator end = transition->propertyTable()->end();
        if (iter != end)
            transition->m_hasReadOnlyOrGetterSetterPropertiesExcludingProto = true;
        for (; iter != end; ++iter)
            iter->attributes |= iter->attributes & Accessor ? DontDelete : (DontDelete | ReadOnly);
    }

    transition->checkOffsetConsistency();
    return transition;
}

// In future we may want to cache this transition.
Structure* Structure::preventExtensionsTransition(VM& vm, Structure* structure)
{
    Structure* transition = create(vm, structure);

    // Don't set m_offset, as one can not transition to this.

    structure->materializePropertyMapIfNecessary(vm);
    transition->propertyTable().set(vm, transition, structure->copyPropertyTableForPinning(vm, transition));
    transition->m_offset = structure->m_offset;
    transition->m_preventExtensions = true;
    transition->pin();

    transition->checkOffsetConsistency();
    return transition;
}

PropertyTable* Structure::takePropertyTableOrCloneIfPinned(VM& vm, Structure* owner)
{
    materializePropertyMapIfNecessaryForPinning(vm);
    if (m_isPinnedPropertyTable)
        return propertyTable()->copy(vm, owner, propertyTable()->size() + 1);
    PropertyTable* takenPropertyTable = propertyTable().get();
    propertyTable().clear();
    return takenPropertyTable;
}

Structure* Structure::nonPropertyTransition(VM& vm, Structure* structure, NonPropertyTransition transitionKind)
{
    unsigned attributes = toAttributes(transitionKind);
    IndexingType indexingType = newIndexingType(structure->indexingTypeIncludingHistory(), transitionKind);
    
    if (JSGlobalObject* globalObject = structure->m_globalObject.get()) {
        if (globalObject->isOriginalArrayStructure(structure)) {
            Structure* result = globalObject->originalArrayStructureForIndexingType(indexingType);
            if (result->indexingTypeIncludingHistory() == indexingType) {
                structure->notifyTransitionFromThisStructure();
                return result;
            }
        }
    }
    
    if (Structure* existingTransition = structure->m_transitionTable.get(0, attributes)) {
        ASSERT(existingTransition->m_attributesInPrevious == attributes);
        ASSERT(existingTransition->indexingTypeIncludingHistory() == indexingType);
        return existingTransition;
    }
    
    Structure* transition = create(vm, structure);
    transition->setPreviousID(vm, transition, structure);
    transition->m_attributesInPrevious = attributes;
    transition->m_indexingType = indexingType;
    transition->propertyTable().set(vm, transition, structure->takePropertyTableOrCloneIfPinned(vm, transition));
    transition->m_offset = structure->m_offset;
    checkOffset(transition->m_offset, transition->inlineCapacity());
    
    structure->m_transitionTable.add(vm, transition);
    transition->checkOffsetConsistency();
    return transition;
}

// In future we may want to cache this property.
bool Structure::isSealed(VM& vm)
{
    if (isExtensible())
        return false;

    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return true;

    PropertyTable::iterator end = propertyTable()->end();
    for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter) {
        if ((iter->attributes & DontDelete) != DontDelete)
            return false;
    }
    return true;
}

// In future we may want to cache this property.
bool Structure::isFrozen(VM& vm)
{
    if (isExtensible())
        return false;

    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return true;

    PropertyTable::iterator end = propertyTable()->end();
    for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter) {
        if (!(iter->attributes & DontDelete))
            return false;
        if (!(iter->attributes & (ReadOnly | Accessor)))
            return false;
    }
    return true;
}

Structure* Structure::flattenDictionaryStructure(VM& vm, JSObject* object)
{
    checkOffsetConsistency();
    ASSERT(isDictionary());
    if (isUncacheableDictionary()) {
        ASSERT(propertyTable());

        size_t propertyCount = propertyTable()->size();

        // Holds our values compacted by insertion order.
        Vector<JSValue> values(propertyCount);

        // Copies out our values from their hashed locations, compacting property table offsets as we go.
        unsigned i = 0;
        PropertyTable::iterator end = propertyTable()->end();
        m_offset = invalidOffset;
        for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter, ++i) {
            values[i] = object->getDirect(iter->offset);
            m_offset = iter->offset = offsetForPropertyNumber(i, m_inlineCapacity);
        }
        
        // Copies in our values to their compacted locations.
        for (unsigned i = 0; i < propertyCount; i++)
            object->putDirect(vm, offsetForPropertyNumber(i, m_inlineCapacity), values[i]);

        propertyTable()->clearDeletedOffsets();
        checkOffsetConsistency();
    }

    m_dictionaryKind = NoneDictionaryKind;
    return this;
}

PropertyOffset Structure::addPropertyWithoutTransition(VM& vm, PropertyName propertyName, unsigned attributes, JSCell* specificValue)
{
    ASSERT(!enumerationCache());

    if (m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        specificValue = 0;

    materializePropertyMapIfNecessaryForPinning(vm);
    
    pin();

    return putSpecificValue(vm, propertyName, attributes, specificValue);
}

PropertyOffset Structure::removePropertyWithoutTransition(VM& vm, PropertyName propertyName)
{
    ASSERT(isUncacheableDictionary());
    ASSERT(!enumerationCache());

    materializePropertyMapIfNecessaryForPinning(vm);

    pin();
    return remove(propertyName);
}

void Structure::pin()
{
    ASSERT(propertyTable());
    m_isPinnedPropertyTable = true;
    clearPreviousID();
    m_nameInPrevious.clear();
}

void Structure::allocateRareData(VM& vm)
{
    ASSERT(!typeInfo().structureHasRareData());
    StructureRareData* rareData = StructureRareData::create(vm, previous());
    m_typeInfo = TypeInfo(typeInfo().type(), typeInfo().flags() | StructureHasRareData);
    m_previousOrRareData.set(vm, this, rareData);
}

void Structure::cloneRareDataFrom(VM& vm, const Structure* other)
{
    ASSERT(other->typeInfo().structureHasRareData());
    StructureRareData* newRareData = StructureRareData::clone(vm, other->rareData());
    m_typeInfo = TypeInfo(typeInfo().type(), typeInfo().flags() | StructureHasRareData);
    m_previousOrRareData.set(vm, this, newRareData);
}

#if DUMP_PROPERTYMAP_STATS

struct PropertyMapStatisticsExitLogger {
    ~PropertyMapStatisticsExitLogger();
};

static PropertyMapStatisticsExitLogger logger;

PropertyMapStatisticsExitLogger::~PropertyMapStatisticsExitLogger()
{
    dataLogF("\nJSC::PropertyMap statistics\n\n");
    dataLogF("%d probes\n", numProbes);
    dataLogF("%d collisions (%.1f%%)\n", numCollisions, 100.0 * numCollisions / numProbes);
    dataLogF("%d rehashes\n", numRehashes);
    dataLogF("%d removes\n", numRemoves);
}

#endif

#if !DO_PROPERTYMAP_CONSTENCY_CHECK

inline void Structure::checkConsistency()
{
    checkOffsetConsistency();
}

#endif

PropertyTable* Structure::copyPropertyTable(VM& vm, Structure* owner)
{
    if (!propertyTable())
        return 0;
    return PropertyTable::clone(vm, owner, *propertyTable().get());
}

PropertyTable* Structure::copyPropertyTableForPinning(VM& vm, Structure* owner)
{
    if (propertyTable())
        return PropertyTable::clone(vm, owner, *propertyTable().get());
    return PropertyTable::create(vm, numberOfSlotsForLastOffset(m_offset, m_inlineCapacity));
}

PropertyOffset Structure::get(VM& vm, PropertyName propertyName, unsigned& attributes, JSCell*& specificValue)
{
    ASSERT(structure()->classInfo() == &s_info);

    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return invalidOffset;

    PropertyMapEntry* entry = propertyTable()->find(propertyName.uid()).first;
    if (!entry)
        return invalidOffset;

    attributes = entry->attributes;
    specificValue = entry->specificValue.get();
    return entry->offset;
}

bool Structure::despecifyFunction(VM& vm, PropertyName propertyName)
{
    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return false;

    PropertyMapEntry* entry = propertyTable()->find(propertyName.uid()).first;
    if (!entry)
        return false;

    ASSERT(entry->specificValue);
    entry->specificValue.clear();
    return true;
}

void Structure::despecifyAllFunctions(VM& vm)
{
    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return;

    PropertyTable::iterator end = propertyTable()->end();
    for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter)
        iter->specificValue.clear();
}

PropertyOffset Structure::putSpecificValue(VM& vm, PropertyName propertyName, unsigned attributes, JSCell* specificValue)
{
    ASSERT(!JSC::isValidOffset(get(vm, propertyName)));

    checkConsistency();
    if (attributes & DontEnum)
        m_hasNonEnumerableProperties = true;

    StringImpl* rep = propertyName.uid();

    if (!propertyTable())
        createPropertyMap(vm);

    PropertyOffset newOffset = propertyTable()->nextOffset(m_inlineCapacity);

    propertyTable()->add(PropertyMapEntry(vm, this, rep, newOffset, attributes, specificValue), m_offset, PropertyTable::PropertyOffsetMayChange);
    
    checkConsistency();
    return newOffset;
}

PropertyOffset Structure::remove(PropertyName propertyName)
{
    checkConsistency();

    StringImpl* rep = propertyName.uid();

    if (!propertyTable())
        return invalidOffset;

    PropertyTable::find_iterator position = propertyTable()->find(rep);
    if (!position.first)
        return invalidOffset;

    PropertyOffset offset = position.first->offset;

    propertyTable()->remove(position);
    propertyTable()->addDeletedOffset(offset);

    checkConsistency();
    return offset;
}

void Structure::createPropertyMap(VM& vm, unsigned capacity)
{
    ASSERT(!propertyTable());

    checkConsistency();
    propertyTable().set(vm, this, PropertyTable::create(vm, capacity));
}

void Structure::getPropertyNamesFromStructure(VM& vm, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    materializePropertyMapIfNecessary(vm);
    if (!propertyTable())
        return;

    bool knownUnique = !propertyNames.size();

    PropertyTable::iterator end = propertyTable()->end();
    for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter) {
        ASSERT(m_hasNonEnumerableProperties || !(iter->attributes & DontEnum));
        if (iter->key->isIdentifier() && (!(iter->attributes & DontEnum) || mode == IncludeDontEnumProperties)) {
            if (knownUnique)
                propertyNames.addKnownUnique(iter->key);
            else
                propertyNames.add(iter->key);
        }
    }
}

JSValue Structure::prototypeForLookup(CodeBlock* codeBlock) const
{
    return prototypeForLookup(codeBlock->globalObject());
}

void Structure::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    Structure* thisObject = jsCast<Structure*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    JSCell::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_globalObject);
    if (!thisObject->isObject())
        thisObject->m_cachedPrototypeChain.clear();
    else {
        visitor.append(&thisObject->m_prototype);
        visitor.append(&thisObject->m_cachedPrototypeChain);
    }
    visitor.append(&thisObject->m_previousOrRareData);
    visitor.append(&thisObject->m_specificValueInPrevious);

    if (thisObject->m_isPinnedPropertyTable) {
        ASSERT(thisObject->m_propertyTableUnsafe);
        visitor.append(&thisObject->m_propertyTableUnsafe);
    } else if (thisObject->m_propertyTableUnsafe)
        thisObject->m_propertyTableUnsafe.clear();
}

bool Structure::prototypeChainMayInterceptStoreTo(VM& vm, PropertyName propertyName)
{
    unsigned i = propertyName.asIndex();
    if (i != PropertyName::NotAnIndex)
        return anyObjectInChainMayInterceptIndexedAccesses();
    
    for (Structure* current = this; ;) {
        JSValue prototype = current->storedPrototype();
        if (prototype.isNull())
            return false;
        
        current = prototype.asCell()->structure();
        
        unsigned attributes;
        JSCell* specificValue;
        PropertyOffset offset = current->get(vm, propertyName, attributes, specificValue);
        if (!JSC::isValidOffset(offset))
            continue;
        
        if (attributes & (ReadOnly | Accessor))
            return true;
        
        return false;
    }
}

#if DO_PROPERTYMAP_CONSTENCY_CHECK

void PropertyTable::checkConsistency()
{
    checkOffsetConsistency();
    ASSERT(m_indexSize >= PropertyTable::MinimumTableSize);
    ASSERT(m_indexMask);
    ASSERT(m_indexSize == m_indexMask + 1);
    ASSERT(!(m_indexSize & m_indexMask));

    ASSERT(m_keyCount <= m_indexSize / 2);
    ASSERT(m_keyCount + m_deletedCount <= m_indexSize / 2);
    ASSERT(m_deletedCount <= m_indexSize / 4);

    unsigned indexCount = 0;
    unsigned deletedIndexCount = 0;
    for (unsigned a = 0; a != m_indexSize; ++a) {
        unsigned entryIndex = m_index[a];
        if (entryIndex == PropertyTable::EmptyEntryIndex)
            continue;
        if (entryIndex == deletedEntryIndex()) {
            ++deletedIndexCount;
            continue;
        }
        ASSERT(entryIndex < deletedEntryIndex());
        ASSERT(entryIndex - 1 <= usedCount());
        ++indexCount;

        for (unsigned b = a + 1; b != m_indexSize; ++b)
            ASSERT(m_index[b] != entryIndex);
    }
    ASSERT(indexCount == m_keyCount);
    ASSERT(deletedIndexCount == m_deletedCount);

    ASSERT(!table()[deletedEntryIndex() - 1].key);

    unsigned nonEmptyEntryCount = 0;
    for (unsigned c = 0; c < usedCount(); ++c) {
        StringImpl* rep = table()[c].key;
        if (rep == PROPERTY_MAP_DELETED_ENTRY_KEY)
            continue;
        ++nonEmptyEntryCount;
        unsigned i = rep->existingHash();
        unsigned k = 0;
        unsigned entryIndex;
        while (1) {
            entryIndex = m_index[i & m_indexMask];
            ASSERT(entryIndex != PropertyTable::EmptyEntryIndex);
            if (rep == table()[entryIndex - 1].key)
                break;
            if (k == 0)
                k = 1 | doubleHash(rep->existingHash());
            i += k;
        }
        ASSERT(entryIndex == c + 1);
    }

    ASSERT(nonEmptyEntryCount == m_keyCount);
}

void Structure::checkConsistency()
{
    if (!propertyTable())
        return;

    if (!m_hasNonEnumerableProperties) {
        PropertyTable::iterator end = propertyTable()->end();
        for (PropertyTable::iterator iter = propertyTable()->begin(); iter != end; ++iter) {
            ASSERT(!(iter->attributes & DontEnum));
        }
    }

    propertyTable()->checkConsistency();
}

#endif // DO_PROPERTYMAP_CONSTENCY_CHECK

} // namespace JSC
