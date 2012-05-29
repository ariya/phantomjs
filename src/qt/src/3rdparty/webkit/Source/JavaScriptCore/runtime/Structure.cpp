/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#include "Identifier.h"
#include "JSObject.h"
#include "JSPropertyNameIterator.h"
#include "Lookup.h"
#include "PropertyNameArray.h"
#include "StructureChain.h"
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/RefPtr.h>

#if ENABLE(JSC_MULTIPLE_THREADS)
#include <wtf/Threading.h>
#endif

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
    return map()->contains(make_pair(rep, attributes));
}

inline Structure* StructureTransitionTable::get(StringImpl* rep, unsigned attributes) const
{
    if (isUsingSingleSlot()) {
        Structure* transition = singleTransition();
        return (transition && transition->m_nameInPrevious == rep && transition->m_attributesInPrevious == attributes) ? transition : 0;
    }
    return map()->get(make_pair(rep, attributes));
}

inline void StructureTransitionTable::remove(Structure* structure)
{
    if (isUsingSingleSlot()) {
        // If more than one transition had been added, then we wouldn't be in
        // single slot mode (even despecifying a from a specific value triggers
        // map mode).
        // As such, the passed structure *must* be the existing transition.
        ASSERT(singleTransition() == structure);
        clearSingleTransition();
    } else {
        // Check whether a mapping exists for structure's key, and whether the
        // entry is structure (the latter check may fail if we initially had a
        // transition with a specific value, and this has been despecified).

        // Newer versions of the STL have an std::make_pair function that takes rvalue references.
        // When either of the parameters are bitfields, the C++ compiler will try to bind them as lvalues, which is invalid. To work around this, use unary "+" to make the parameter an rvalue.
        // See https://bugs.webkit.org/show_bug.cgi?id=59261 for more details
        TransitionMap::iterator entry = map()->find(make_pair(structure->m_nameInPrevious, +structure->m_attributesInPrevious));
        if (entry != map()->end() && structure == entry.get().second)
            map()->remove(entry);
    }
}

inline void StructureTransitionTable::add(JSGlobalData& globalData, Structure* structure)
{
    if (isUsingSingleSlot()) {
        Structure* existingTransition = singleTransition();

        // This handles the first transition being added.
        if (!existingTransition) {
            setSingleTransition(globalData, structure);
            return;
        }

        // This handles the second transition being added
        // (or the first transition being despecified!)
        setMap(new TransitionMap());
        add(globalData, existingTransition);
    }

    // Add the structure to the map.

    // Newer versions of the STL have an std::make_pair function that takes rvalue references.
    // When either of the parameters are bitfields, the C++ compiler will try to bind them as lvalues, which is invalid. To work around this, use unary "+" to make the parameter an rvalue.
    // See https://bugs.webkit.org/show_bug.cgi?id=59261 for more details
    std::pair<TransitionMap::iterator, bool> result = map()->add(globalData, make_pair(structure->m_nameInPrevious, +structure->m_attributesInPrevious), structure);
    if (!result.second) {
        // There already is an entry! - we should only hit this when despecifying.
        ASSERT(result.first.get().second->m_specificValueInPrevious);
        ASSERT(!structure->m_specificValueInPrevious);
        map()->set(result.first, structure);
    }
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
               if (!structure->m_previous)
                    ++numberSingletons;
                break;

            case 1:
                ++numberUsingSingleSlot;
                break;
        }

        if (structure->m_propertyTable) {
            ++numberWithPropertyMaps;
            totalPropertyMapsSize += structure->m_propertyTable->sizeInMemory();
        }
    }

    printf("Number of live Structures: %d\n", liveStructureSet.size());
    printf("Number of Structures using the single item optimization for transition map: %d\n", numberUsingSingleSlot);
    printf("Number of Structures that are leaf nodes: %d\n", numberLeaf);
    printf("Number of Structures that singletons: %d\n", numberSingletons);
    printf("Number of Structures with PropertyMaps: %d\n", numberWithPropertyMaps);

    printf("Size of a single Structures: %d\n", static_cast<unsigned>(sizeof(Structure)));
    printf("Size of sum of all property maps: %d\n", totalPropertyMapsSize);
    printf("Size of average of all property maps: %f\n", static_cast<double>(totalPropertyMapsSize) / static_cast<double>(liveStructureSet.size()));
#else
    printf("Dumping Structure statistics is not enabled.\n");
#endif
}

Structure::Structure(JSGlobalData& globalData, JSValue prototype, const TypeInfo& typeInfo, unsigned anonymousSlotCount, const ClassInfo* classInfo)
    : JSCell(globalData, globalData.structureStructure.get())
    , m_typeInfo(typeInfo)
    , m_prototype(globalData, this, prototype)
    , m_classInfo(classInfo)
    , m_propertyStorageCapacity(typeInfo.isFinal() ? JSFinalObject_inlineStorageCapacity : JSNonFinalObject_inlineStorageCapacity)
    , m_offset(noOffset)
    , m_dictionaryKind(NoneDictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(false)
    , m_hasNonEnumerableProperties(false)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(0)
    , m_anonymousSlotCount(anonymousSlotCount)
    , m_preventExtensions(false)
    , m_didTransition(false)
{
    ASSERT(m_prototype);
    ASSERT(m_prototype.isObject() || m_prototype.isNull());
}

const ClassInfo Structure::s_info = { "Structure", 0, 0, 0 };

Structure::Structure(JSGlobalData& globalData)
    : JSCell(globalData, this)
    , m_typeInfo(CompoundType, OverridesVisitChildren)
    , m_prototype(globalData, this, jsNull())
    , m_classInfo(&s_info)
    , m_propertyStorageCapacity(0)
    , m_offset(noOffset)
    , m_dictionaryKind(NoneDictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(false)
    , m_hasNonEnumerableProperties(false)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(0)
    , m_anonymousSlotCount(0)
    , m_preventExtensions(false)
    , m_didTransition(false)
{
    ASSERT(m_prototype);
    ASSERT(m_prototype.isNull());
    ASSERT(!globalData.structureStructure);
}

Structure::Structure(JSGlobalData& globalData, const Structure* previous)
    : JSCell(globalData, globalData.structureStructure.get())
    , m_typeInfo(previous->typeInfo())
    , m_prototype(globalData, this, previous->storedPrototype())
    , m_classInfo(previous->m_classInfo)
    , m_propertyStorageCapacity(previous->m_propertyStorageCapacity)
    , m_offset(noOffset)
    , m_dictionaryKind(NoneDictionaryKind)
    , m_isPinnedPropertyTable(false)
    , m_hasGetterSetterProperties(previous->m_hasGetterSetterProperties)
    , m_hasNonEnumerableProperties(previous->m_hasNonEnumerableProperties)
    , m_attributesInPrevious(0)
    , m_specificFunctionThrashCount(previous->m_specificFunctionThrashCount)
    , m_anonymousSlotCount(previous->anonymousSlotCount())
    , m_preventExtensions(previous->m_preventExtensions)
    , m_didTransition(true)
{
    ASSERT(m_prototype);
    ASSERT(m_prototype.isObject() || m_prototype.isNull());
}

Structure::~Structure()
{
}

void Structure::materializePropertyMap(JSGlobalData& globalData)
{
    ASSERT(structure()->classInfo() == &s_info);
    ASSERT(!m_propertyTable);

    Vector<Structure*, 8> structures;
    structures.append(this);

    Structure* structure = this;

    // Search for the last Structure with a property table.
    while ((structure = structure->previousID())) {
        if (structure->m_isPinnedPropertyTable) {
            ASSERT(structure->m_propertyTable);
            ASSERT(!structure->m_previous);

            m_propertyTable = structure->m_propertyTable->copy(globalData, 0, m_offset + 1);
            break;
        }

        structures.append(structure);
    }

    if (!m_propertyTable)
        createPropertyMap(m_offset + 1);

    for (ptrdiff_t i = structures.size() - 2; i >= 0; --i) {
        structure = structures[i];
        PropertyMapEntry entry(globalData, this, structure->m_nameInPrevious.get(), m_anonymousSlotCount + structure->m_offset, structure->m_attributesInPrevious, structure->m_specificValueInPrevious.get());
        m_propertyTable->add(entry);
    }
}

void Structure::growPropertyStorageCapacity()
{
    if (isUsingInlineStorage())
        m_propertyStorageCapacity = JSObject::baseExternalStorageCapacity;
    else
        m_propertyStorageCapacity *= 2;
}

void Structure::despecifyDictionaryFunction(JSGlobalData& globalData, const Identifier& propertyName)
{
    StringImpl* rep = propertyName.impl();

    materializePropertyMapIfNecessary(globalData);

    ASSERT(isDictionary());
    ASSERT(m_propertyTable);

    PropertyMapEntry* entry = m_propertyTable->find(rep).first;
    ASSERT(entry);
    entry->specificValue.clear();
}

Structure* Structure::addPropertyTransitionToExistingStructure(Structure* structure, const Identifier& propertyName, unsigned attributes, JSCell* specificValue, size_t& offset)
{
    ASSERT(!structure->isDictionary());
    ASSERT(structure->typeInfo().type() == ObjectType);

    if (Structure* existingTransition = structure->m_transitionTable.get(propertyName.impl(), attributes)) {
        JSCell* specificValueInPrevious = existingTransition->m_specificValueInPrevious.get();
        if (specificValueInPrevious && specificValueInPrevious != specificValue)
            return 0;
        ASSERT(existingTransition->m_offset != noOffset);
        offset = existingTransition->m_offset + existingTransition->m_anonymousSlotCount;
        ASSERT(offset >= structure->m_anonymousSlotCount);
        ASSERT(structure->m_anonymousSlotCount == existingTransition->m_anonymousSlotCount);
        return existingTransition;
    }

    return 0;
}

Structure* Structure::addPropertyTransition(JSGlobalData& globalData, Structure* structure, const Identifier& propertyName, unsigned attributes, JSCell* specificValue, size_t& offset)
{
    // If we have a specific function, we may have got to this point if there is
    // already a transition with the correct property name and attributes, but
    // specialized to a different function.  In this case we just want to give up
    // and despecialize the transition.
    // In this case we clear the value of specificFunction which will result
    // in us adding a non-specific transition, and any subsequent lookup in
    // Structure::addPropertyTransitionToExistingStructure will just use that.
    if (specificValue && structure->m_transitionTable.contains(propertyName.impl(), attributes))
        specificValue = 0;

    ASSERT(!structure->isDictionary());
    ASSERT(structure->typeInfo().type() == ObjectType);
    ASSERT(!Structure::addPropertyTransitionToExistingStructure(structure, propertyName, attributes, specificValue, offset));
    
    if (structure->m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        specificValue = 0;

    if (structure->transitionCount() > s_maxTransitionLength) {
        Structure* transition = toCacheableDictionaryTransition(globalData, structure);
        ASSERT(structure != transition);
        offset = transition->putSpecificValue(globalData, propertyName, attributes, specificValue);
        ASSERT(offset >= structure->m_anonymousSlotCount);
        ASSERT(structure->m_anonymousSlotCount == transition->m_anonymousSlotCount);
        if (transition->propertyStorageSize() > transition->propertyStorageCapacity())
            transition->growPropertyStorageCapacity();
        return transition;
    }

    Structure* transition = create(globalData, structure);

    transition->m_cachedPrototypeChain.set(globalData, transition, structure->m_cachedPrototypeChain.get());
    transition->m_previous.set(globalData, transition, structure);
    transition->m_nameInPrevious = propertyName.impl();
    transition->m_attributesInPrevious = attributes;
    transition->m_specificValueInPrevious.set(globalData, transition, specificValue);

    if (structure->m_propertyTable) {
        if (structure->m_isPinnedPropertyTable)
            transition->m_propertyTable = structure->m_propertyTable->copy(globalData, 0, structure->m_propertyTable->size() + 1);
        else
            transition->m_propertyTable = structure->m_propertyTable.release();
    } else {
        if (structure->m_previous)
            transition->materializePropertyMap(globalData);
        else
            transition->createPropertyMap();
    }

    offset = transition->putSpecificValue(globalData, propertyName, attributes, specificValue);
    ASSERT(offset >= structure->m_anonymousSlotCount);
    ASSERT(structure->m_anonymousSlotCount == transition->m_anonymousSlotCount);
    if (transition->propertyStorageSize() > transition->propertyStorageCapacity())
        transition->growPropertyStorageCapacity();

    transition->m_offset = offset - structure->m_anonymousSlotCount;
    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    structure->m_transitionTable.add(globalData, transition);
    return transition;
}

Structure* Structure::removePropertyTransition(JSGlobalData& globalData, Structure* structure, const Identifier& propertyName, size_t& offset)
{
    ASSERT(!structure->isUncacheableDictionary());

    Structure* transition = toUncacheableDictionaryTransition(globalData, structure);

    offset = transition->remove(propertyName);
    ASSERT(offset >= structure->m_anonymousSlotCount);
    ASSERT(structure->m_anonymousSlotCount == transition->m_anonymousSlotCount);

    return transition;
}

Structure* Structure::changePrototypeTransition(JSGlobalData& globalData, Structure* structure, JSValue prototype)
{
    Structure* transition = create(globalData, structure);

    transition->m_prototype.set(globalData, transition, prototype);

    // Don't set m_offset, as one can not transition to this.

    structure->materializePropertyMapIfNecessary(globalData);
    transition->m_propertyTable = structure->copyPropertyTable(globalData, transition);
    transition->m_isPinnedPropertyTable = true;
    
    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    return transition;
}

Structure* Structure::despecifyFunctionTransition(JSGlobalData& globalData, Structure* structure, const Identifier& replaceFunction)
{
    ASSERT(structure->m_specificFunctionThrashCount < maxSpecificFunctionThrashCount);
    Structure* transition = create(globalData, structure);

    ++transition->m_specificFunctionThrashCount;

    // Don't set m_offset, as one can not transition to this.

    structure->materializePropertyMapIfNecessary(globalData);
    transition->m_propertyTable = structure->copyPropertyTable(globalData, transition);
    transition->m_isPinnedPropertyTable = true;

    if (transition->m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        transition->despecifyAllFunctions(globalData);
    else {
        bool removed = transition->despecifyFunction(globalData, replaceFunction);
        ASSERT_UNUSED(removed, removed);
    }
    
    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    return transition;
}

Structure* Structure::getterSetterTransition(JSGlobalData& globalData, Structure* structure)
{
    Structure* transition = create(globalData, structure);

    // Don't set m_offset, as one can not transition to this.

    structure->materializePropertyMapIfNecessary(globalData);
    transition->m_propertyTable = structure->copyPropertyTable(globalData, transition);
    transition->m_isPinnedPropertyTable = true;
    
    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    return transition;
}

Structure* Structure::toDictionaryTransition(JSGlobalData& globalData, Structure* structure, DictionaryKind kind)
{
    ASSERT(!structure->isUncacheableDictionary());
    
    Structure* transition = create(globalData, structure);

    structure->materializePropertyMapIfNecessary(globalData);
    transition->m_propertyTable = structure->copyPropertyTable(globalData, transition);
    transition->m_isPinnedPropertyTable = true;
    transition->m_dictionaryKind = kind;
    
    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    return transition;
}

Structure* Structure::toCacheableDictionaryTransition(JSGlobalData& globalData, Structure* structure)
{
    return toDictionaryTransition(globalData, structure, CachedDictionaryKind);
}

Structure* Structure::toUncacheableDictionaryTransition(JSGlobalData& globalData, Structure* structure)
{
    return toDictionaryTransition(globalData, structure, UncachedDictionaryKind);
}

// In future we may want to cache this transition.
Structure* Structure::sealTransition(JSGlobalData& globalData, Structure* structure)
{
    Structure* transition = preventExtensionsTransition(globalData, structure);

    if (transition->m_propertyTable) {
        PropertyTable::iterator end = transition->m_propertyTable->end();
        for (PropertyTable::iterator iter = transition->m_propertyTable->begin(); iter != end; ++iter)
            iter->attributes |= DontDelete;
    }

    return transition;
}

// In future we may want to cache this transition.
Structure* Structure::freezeTransition(JSGlobalData& globalData, Structure* structure)
{
    Structure* transition = preventExtensionsTransition(globalData, structure);

    if (transition->m_propertyTable) {
        PropertyTable::iterator end = transition->m_propertyTable->end();
        for (PropertyTable::iterator iter = transition->m_propertyTable->begin(); iter != end; ++iter)
            iter->attributes |= (DontDelete | ReadOnly);
    }

    return transition;
}

// In future we may want to cache this transition.
Structure* Structure::preventExtensionsTransition(JSGlobalData& globalData, Structure* structure)
{
    Structure* transition = create(globalData, structure);

    // Don't set m_offset, as one can not transition to this.

    structure->materializePropertyMapIfNecessary(globalData);
    transition->m_propertyTable = structure->copyPropertyTable(globalData, transition);
    transition->m_isPinnedPropertyTable = true;
    transition->m_preventExtensions = true;

    ASSERT(structure->anonymousSlotCount() == transition->anonymousSlotCount());
    return transition;
}

// In future we may want to cache this property.
bool Structure::isSealed(JSGlobalData& globalData)
{
    if (isExtensible())
        return false;

    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return true;

    PropertyTable::iterator end = m_propertyTable->end();
    for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter) {
        if ((iter->attributes & DontDelete) != DontDelete)
            return false;
    }
    return true;
}

// In future we may want to cache this property.
bool Structure::isFrozen(JSGlobalData& globalData)
{
    if (isExtensible())
        return false;

    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return true;

    PropertyTable::iterator end = m_propertyTable->end();
    for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter) {
        if ((iter->attributes & (DontDelete | ReadOnly)) != (DontDelete | ReadOnly))
            return false;
    }
    return true;
}

Structure* Structure::flattenDictionaryStructure(JSGlobalData& globalData, JSObject* object)
{
    ASSERT(isDictionary());
    if (isUncacheableDictionary()) {
        ASSERT(m_propertyTable);

        unsigned anonymousSlotCount = m_anonymousSlotCount;
        size_t propertyCount = m_propertyTable->size();
        Vector<JSValue> values(propertyCount);

        unsigned i = 0;
        PropertyTable::iterator end = m_propertyTable->end();
        for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter, ++i) {
            values[i] = object->getDirectOffset(iter->offset);
            // Update property table to have the new property offsets
            iter->offset = anonymousSlotCount + i;
        }
        
        // Copy the original property values into their final locations
        for (unsigned i = 0; i < propertyCount; i++)
            object->putDirectOffset(globalData, anonymousSlotCount + i, values[i]);

        m_propertyTable->clearDeletedOffsets();
    }

    m_dictionaryKind = NoneDictionaryKind;
    return this;
}

size_t Structure::addPropertyWithoutTransition(JSGlobalData& globalData, const Identifier& propertyName, unsigned attributes, JSCell* specificValue)
{
    ASSERT(!m_enumerationCache);

    if (m_specificFunctionThrashCount == maxSpecificFunctionThrashCount)
        specificValue = 0;

    materializePropertyMapIfNecessary(globalData);

    m_isPinnedPropertyTable = true;

    size_t offset = putSpecificValue(globalData, propertyName, attributes, specificValue);
    ASSERT(offset >= m_anonymousSlotCount);
    if (propertyStorageSize() > propertyStorageCapacity())
        growPropertyStorageCapacity();
    return offset;
}

size_t Structure::removePropertyWithoutTransition(JSGlobalData& globalData, const Identifier& propertyName)
{
    ASSERT(isUncacheableDictionary());
    ASSERT(!m_enumerationCache);

    materializePropertyMapIfNecessary(globalData);

    m_isPinnedPropertyTable = true;
    size_t offset = remove(propertyName);
    ASSERT(offset >= m_anonymousSlotCount);
    return offset;
}

#if DUMP_PROPERTYMAP_STATS

struct PropertyMapStatisticsExitLogger {
    ~PropertyMapStatisticsExitLogger();
};

static PropertyMapStatisticsExitLogger logger;

PropertyMapStatisticsExitLogger::~PropertyMapStatisticsExitLogger()
{
    printf("\nJSC::PropertyMap statistics\n\n");
    printf("%d probes\n", numProbes);
    printf("%d collisions (%.1f%%)\n", numCollisions, 100.0 * numCollisions / numProbes);
    printf("%d rehashes\n", numRehashes);
    printf("%d removes\n", numRemoves);
}

#endif

#if !DO_PROPERTYMAP_CONSTENCY_CHECK

inline void Structure::checkConsistency()
{
}

#endif

PassOwnPtr<PropertyTable> Structure::copyPropertyTable(JSGlobalData& globalData, Structure* owner)
{
    return adoptPtr(m_propertyTable ? new PropertyTable(globalData, owner, *m_propertyTable) : 0);
}

size_t Structure::get(JSGlobalData& globalData, StringImpl* propertyName, unsigned& attributes, JSCell*& specificValue)
{
    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return WTF::notFound;

    PropertyMapEntry* entry = m_propertyTable->find(propertyName).first;
    if (!entry)
        return WTF::notFound;

    attributes = entry->attributes;
    specificValue = entry->specificValue.get();
    ASSERT(entry->offset >= m_anonymousSlotCount);
    return entry->offset;
}

bool Structure::despecifyFunction(JSGlobalData& globalData, const Identifier& propertyName)
{
    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return false;

    ASSERT(!propertyName.isNull());
    PropertyMapEntry* entry = m_propertyTable->find(propertyName.impl()).first;
    if (!entry)
        return false;

    ASSERT(entry->specificValue);
    entry->specificValue.clear();
    return true;
}

void Structure::despecifyAllFunctions(JSGlobalData& globalData)
{
    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return;

    PropertyTable::iterator end = m_propertyTable->end();
    for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter)
        iter->specificValue.clear();
}

size_t Structure::putSpecificValue(JSGlobalData& globalData, const Identifier& propertyName, unsigned attributes, JSCell* specificValue)
{
    ASSERT(!propertyName.isNull());
    ASSERT(get(globalData, propertyName) == notFound);

    checkConsistency();
    if (attributes & DontEnum)
        m_hasNonEnumerableProperties = true;

    StringImpl* rep = propertyName.impl();

    if (!m_propertyTable)
        createPropertyMap();

    unsigned newOffset;

    if (m_propertyTable->hasDeletedOffset())
        newOffset = m_propertyTable->getDeletedOffset();
    else
        newOffset = m_propertyTable->size() + m_anonymousSlotCount;
    ASSERT(newOffset >= m_anonymousSlotCount);

    m_propertyTable->add(PropertyMapEntry(globalData, this, rep, newOffset, attributes, specificValue));

    checkConsistency();
    return newOffset;
}

size_t Structure::remove(const Identifier& propertyName)
{
    ASSERT(!propertyName.isNull());

    checkConsistency();

    StringImpl* rep = propertyName.impl();

    if (!m_propertyTable)
        return notFound;

    PropertyTable::find_iterator position = m_propertyTable->find(rep);
    if (!position.first)
        return notFound;

    size_t offset = position.first->offset;
    ASSERT(offset >= m_anonymousSlotCount);

    m_propertyTable->remove(position);
    m_propertyTable->addDeletedOffset(offset);

    checkConsistency();
    return offset;
}

void Structure::createPropertyMap(unsigned capacity)
{
    ASSERT(!m_propertyTable);

    checkConsistency();
    m_propertyTable = adoptPtr(new PropertyTable(capacity));
    checkConsistency();
}

void Structure::getPropertyNames(JSGlobalData& globalData, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    materializePropertyMapIfNecessary(globalData);
    if (!m_propertyTable)
        return;

    bool knownUnique = !propertyNames.size();

    PropertyTable::iterator end = m_propertyTable->end();
    for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter) {
        ASSERT(m_hasNonEnumerableProperties || !(iter->attributes & DontEnum));
        if (!(iter->attributes & DontEnum) || (mode == IncludeDontEnumProperties)) {
            if (knownUnique)
                propertyNames.addKnownUnique(iter->key);
            else
                propertyNames.add(iter->key);
        }
    }
}

void Structure::visitChildren(SlotVisitor& visitor)
{
    JSCell::visitChildren(visitor);
    if (m_prototype)
        visitor.append(&m_prototype);
    if (m_cachedPrototypeChain)
        visitor.append(&m_cachedPrototypeChain);
    if (m_previous)
        visitor.append(&m_previous);
    if (m_specificValueInPrevious)
        visitor.append(&m_specificValueInPrevious);
    if (m_enumerationCache)
        visitor.append(&m_enumerationCache);
    if (m_propertyTable) {
        PropertyTable::iterator end = m_propertyTable->end();
        for (PropertyTable::iterator ptr = m_propertyTable->begin(); ptr != end; ++ptr) {
            if (ptr->specificValue)
                visitor.append(&ptr->specificValue);
        }
    }
}

#if DO_PROPERTYMAP_CONSTENCY_CHECK

void PropertyTable::checkConsistency()
{
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
    if (!m_propertyTable)
        return;

    if (!m_hasNonEnumerableProperties) {
        PropertyTable::iterator end = m_propertyTable->end();
        for (PropertyTable::iterator iter = m_propertyTable->begin(); iter != end; ++iter) {
            ASSERT(!(iter->attributes & DontEnum));
            ASSERT(iter->offset >= m_anonymousSlotCount);
        }
    }

    m_propertyTable->checkConsistency();
}

#endif // DO_PROPERTYMAP_CONSTENCY_CHECK

} // namespace JSC
