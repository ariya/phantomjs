/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ProfileTreeNode_h
#define ProfileTreeNode_h

namespace JSC {

class ProfileTreeNode {
    typedef HashMap<String, ProfileTreeNode> Map;
    typedef Map::ValueType MapEntry;

public:
    ProfileTreeNode()
        : m_count(0)
        , m_children(0)
    {
    }

    ~ProfileTreeNode()
    {
        delete m_children;
    }

    ProfileTreeNode* sampleChild(const char* name)
    {
        if (!m_children)
            m_children = new Map();
    
        ProfileTreeNode newEntry;
        Map::AddResult result = m_children->add(String(name), newEntry);
        ProfileTreeNode* childInMap = &result.iterator->value;
        ++childInMap->m_count;
        return childInMap;
    }

    void dump()
    {
        dumpInternal(0);
    }

    uint64_t count()
    {
        return m_count;
    }

    uint64_t childCount()
    {
        if (!m_children)
            return 0;
        uint64_t childCount = 0;
        for (Map::iterator it = m_children->begin(); it != m_children->end(); ++it)
            childCount += it->value.count();
        return childCount;
    }
    
private:
    void dumpInternal(unsigned indent)
    {
        if (!m_children)
            return;

        // Copy pointers to all children into a vector, and sort the vector by sample count.
        Vector<MapEntry*> entries;
        for (Map::iterator it = m_children->begin(); it != m_children->end(); ++it)
            entries.append(&*it);
        qsort(entries.begin(), entries.size(), sizeof(MapEntry*), compareEntries);

        // Iterate over the children in sample-frequency order.
        for (size_t e = 0; e < entries.size(); ++e) {
            MapEntry* entry = entries[e];

            // Print the number of samples, the name of this node, and the number of samples that are stack-top
            // in this node (samples directly within this node, excluding samples in children.
            for (unsigned i = 0; i < indent; ++i)
                dataLogF("    ");
            dataLogF("% 8lld: %s (%lld stack top)\n",
                static_cast<long long>(entry->value.count()),
                entry->key.utf8().data(),
                static_cast<long long>(entry->value.count() - entry->value.childCount()));

            // Recursively dump the child nodes.
            entry->value.dumpInternal(indent + 1);
        }
    }

    static int compareEntries(const void* a, const void* b)
    {
        uint64_t da = (*static_cast<MapEntry* const *>(a))->value.count();
        uint64_t db = (*static_cast<MapEntry* const *>(b))->value.count();
        return (da < db) - (da > db);
    }

    uint64_t m_count;
    Map* m_children;
};

}

#endif // ProfileTreeNode_h

