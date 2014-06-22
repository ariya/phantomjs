/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LevelDBTransaction_h
#define LevelDBTransaction_h

#if ENABLE(INDEXED_DATABASE)
#if USE(LEVELDB)

#include "LevelDBComparator.h"
#include "LevelDBDatabase.h"
#include "LevelDBIterator.h"
#include "LevelDBSlice.h"
#include <wtf/AVLTree.h>
#include <wtf/HashSet.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class LevelDBWriteBatch;

using WTF::AVLTree;

class LevelDBTransaction : public RefCounted<LevelDBTransaction> {
public:
    static PassRefPtr<LevelDBTransaction> create(LevelDBDatabase*);

    ~LevelDBTransaction();
    void put(const LevelDBSlice& key, const Vector<char>& value);
    void remove(const LevelDBSlice& key);
    // FIXME: Rename safeGet to get.
    bool safeGet(const LevelDBSlice& key, Vector<char>& value, bool& found);
    bool commit();
    void rollback();

    PassOwnPtr<LevelDBIterator> createIterator();

private:
    LevelDBTransaction(LevelDBDatabase*);

    struct AVLTreeNode {
        Vector<char> key;
        Vector<char> value;
        bool deleted;

        AVLTreeNode* less;
        AVLTreeNode* greater;
        int balanceFactor;
    };

    struct AVLTreeAbstractor {
        typedef AVLTreeNode* handle;
        typedef size_t size;
        typedef LevelDBSlice key;

        handle get_less(handle h) { return h->less; }
        void set_less(handle h, handle less) { h->less = less; }
        handle get_greater(handle h) { return h->greater; }
        void set_greater(handle h, handle greater) { h->greater = greater; }

        int get_balance_factor(handle h) { return h->balanceFactor; }
        void set_balance_factor(handle h, int bf) { h->balanceFactor = bf; }

        int compare_key_key(const key& ka, const key& kb) { return m_comparator->compare(ka, kb); }
        int compare_key_node(const key& k, handle h) { return compare_key_key(k, h->key); }
        int compare_node_node(handle ha, handle hb) { return compare_key_key(ha->key, hb->key); }

        static handle null() { return 0; }

        const LevelDBComparator* m_comparator;
    };

    typedef AVLTree<AVLTreeAbstractor> TreeType;

    class TreeIterator : public LevelDBIterator {
    public:
        static PassOwnPtr<TreeIterator> create(LevelDBTransaction*);
        ~TreeIterator();

        virtual bool isValid() const;
        virtual void seekToLast();
        virtual void seek(const LevelDBSlice&);
        virtual void next();
        virtual void prev();
        virtual LevelDBSlice key() const;
        virtual LevelDBSlice value() const;
        bool isDeleted() const;
        void reset();

    private:
        TreeIterator(LevelDBTransaction*);
        mutable TreeType::Iterator m_iterator; // Dereferencing this is non-const.
        TreeType* m_tree;
        LevelDBTransaction* m_transaction;
        Vector<char> m_key;
    };

    class TransactionIterator : public LevelDBIterator {
    public:
        ~TransactionIterator();
        static PassOwnPtr<TransactionIterator> create(PassRefPtr<LevelDBTransaction>);

        virtual bool isValid() const;
        virtual void seekToLast();
        virtual void seek(const LevelDBSlice& target);
        virtual void next();
        virtual void prev();
        virtual LevelDBSlice key() const;
        virtual LevelDBSlice value() const;
        void treeChanged();

    private:
        TransactionIterator(PassRefPtr<LevelDBTransaction>);
        void handleConflictsAndDeletes();
        void setCurrentIteratorToSmallestKey();
        void setCurrentIteratorToLargestKey();
        void refreshTreeIterator() const;
        bool treeIteratorIsLower() const;
        bool treeIteratorIsHigher() const;

        RefPtr<LevelDBTransaction> m_transaction;
        const LevelDBComparator* m_comparator;
        mutable OwnPtr<TreeIterator> m_treeIterator;
        OwnPtr<LevelDBIterator> m_dbIterator;
        LevelDBIterator* m_current;

        enum Direction {
            kForward,
            kReverse
        };
        Direction m_direction;
        mutable bool m_treeChanged;
    };

    void set(const LevelDBSlice& key, const Vector<char>& value, bool deleted);
    void clearTree();
    void registerIterator(TransactionIterator*);
    void unregisterIterator(TransactionIterator*);
    void notifyIteratorsOfTreeChange();

    LevelDBDatabase* m_db;
    const LevelDBSnapshot m_snapshot;
    const LevelDBComparator* m_comparator;
    TreeType m_tree;
    bool m_finished;
    HashSet<TransactionIterator*> m_iterators;
};

class LevelDBWriteOnlyTransaction {
public:
    static PassOwnPtr<LevelDBWriteOnlyTransaction> create(LevelDBDatabase*);

    ~LevelDBWriteOnlyTransaction();
    void remove(const LevelDBSlice& key);
    bool commit();

private:
    LevelDBWriteOnlyTransaction(LevelDBDatabase*);

    LevelDBDatabase* m_db;
    OwnPtr<LevelDBWriteBatch> m_writeBatch;
    bool m_finished;
};

}

#endif // USE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)

#endif // LevelDBTransaction_h
