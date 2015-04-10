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

#include "config.h"
#include "LevelDBTransaction.h"

#if ENABLE(INDEXED_DATABASE)
#if USE(LEVELDB)

#include "LevelDBDatabase.h"
#include "LevelDBSlice.h"
#include "LevelDBWriteBatch.h"
#include <leveldb/db.h>

namespace WebCore {

PassRefPtr<LevelDBTransaction> LevelDBTransaction::create(LevelDBDatabase* db)
{
    return adoptRef(new LevelDBTransaction(db));
}

LevelDBTransaction::LevelDBTransaction(LevelDBDatabase* db)
    : m_db(db)
    , m_snapshot(db)
    , m_comparator(db->comparator())
    , m_finished(false)
{
    m_tree.abstractor().m_comparator = m_comparator;
}

void LevelDBTransaction::clearTree()
{
    TreeType::Iterator iterator;
    iterator.start_iter_least(m_tree);

    Vector<AVLTreeNode*> nodes;

    while (*iterator) {
        nodes.append(*iterator);
        ++iterator;
    }
    m_tree.purge();

    for (size_t i = 0; i < nodes.size(); ++i)
        delete(nodes[i]);
}

LevelDBTransaction::~LevelDBTransaction()
{
    clearTree();
}

static void initVector(const LevelDBSlice& slice, Vector<char>* vector)
{
    vector->clear();
    vector->append(slice.begin(), slice.end() - slice.begin());
}

void LevelDBTransaction::set(const LevelDBSlice& key, const Vector<char>& value, bool deleted)
{
    ASSERT(!m_finished);
    bool newNode = false;
    AVLTreeNode* node = m_tree.search(key);

    if (!node) {
        node = new AVLTreeNode;
        initVector(key, &node->key);
        m_tree.insert(node);
        newNode = true;
    }
    node->value = value;
    node->deleted = deleted;

    if (newNode)
        notifyIteratorsOfTreeChange();
}

void LevelDBTransaction::put(const LevelDBSlice& key, const Vector<char>& value)
{
    set(key, value, false);
}

void LevelDBTransaction::remove(const LevelDBSlice& key)
{
    set(key, Vector<char>(), true);
}

bool LevelDBTransaction::safeGet(const LevelDBSlice& key, Vector<char>& value, bool& found)
{
    found = false;
    ASSERT(!m_finished);
    AVLTreeNode* node = m_tree.search(key);

    if (node) {
        if (node->deleted)
            return true;

        value = node->value;
        found = true;
        return true;
    }

    bool ok = m_db->safeGet(key, value, found, &m_snapshot);
    if (!ok) {
        ASSERT(!found);
        return false;
    }
    return true;
}

bool LevelDBTransaction::commit()
{
    ASSERT(!m_finished);

    if (m_tree.is_empty()) {
        m_finished = true;
        return true;
    }

    OwnPtr<LevelDBWriteBatch> writeBatch = LevelDBWriteBatch::create();

    TreeType::Iterator iterator;
    iterator.start_iter_least(m_tree);

    while (*iterator) {
        AVLTreeNode* node = *iterator;
        if (!node->deleted)
            writeBatch->put(node->key, node->value);
        else
            writeBatch->remove(node->key);
        ++iterator;
    }

    if (!m_db->write(*writeBatch))
        return false;

    clearTree();
    m_finished = true;
    return true;
}

void LevelDBTransaction::rollback()
{
    ASSERT(!m_finished);
    m_finished = true;
    clearTree();
}

PassOwnPtr<LevelDBIterator> LevelDBTransaction::createIterator()
{
    return TransactionIterator::create(this);
}

PassOwnPtr<LevelDBTransaction::TreeIterator> LevelDBTransaction::TreeIterator::create(LevelDBTransaction* transaction)
{
    return adoptPtr(new TreeIterator(transaction));
}

bool LevelDBTransaction::TreeIterator::isValid() const
{
    return *m_iterator;
}

void LevelDBTransaction::TreeIterator::seekToLast()
{
    m_iterator.start_iter_greatest(*m_tree);
    if (isValid())
        m_key = (*m_iterator)->key;
}

void LevelDBTransaction::TreeIterator::seek(const LevelDBSlice& target)
{
    m_iterator.start_iter(*m_tree, target, TreeType::EQUAL);
    if (!isValid())
        m_iterator.start_iter(*m_tree, target, TreeType::GREATER);

    if (isValid())
        m_key = (*m_iterator)->key;
}

void LevelDBTransaction::TreeIterator::next()
{
    ASSERT(isValid());
    ++m_iterator;
    if (isValid()) {
        ASSERT(m_transaction->m_comparator->compare((*m_iterator)->key, m_key) > 0);
        (void)m_transaction;
        m_key = (*m_iterator)->key;
    }
}

void LevelDBTransaction::TreeIterator::prev()
{
    ASSERT(isValid());
    --m_iterator;
    if (isValid()) {
        ASSERT(m_tree->abstractor().m_comparator->compare((*m_iterator)->key, m_key) < 0);
        m_key = (*m_iterator)->key;
    }
}

LevelDBSlice LevelDBTransaction::TreeIterator::key() const
{
    ASSERT(isValid());
    return m_key;
}

LevelDBSlice LevelDBTransaction::TreeIterator::value() const
{
    ASSERT(isValid());
    ASSERT(!isDeleted());
    return (*m_iterator)->value;
}

bool LevelDBTransaction::TreeIterator::isDeleted() const
{
    ASSERT(isValid());
    return (*m_iterator)->deleted;
}

void LevelDBTransaction::TreeIterator::reset()
{
    ASSERT(isValid());
    m_iterator.start_iter(*m_tree, m_key, TreeType::EQUAL);
    ASSERT(isValid());
}

LevelDBTransaction::TreeIterator::~TreeIterator()
{
}

LevelDBTransaction::TreeIterator::TreeIterator(LevelDBTransaction* transaction)
    : m_tree(&transaction->m_tree)
    , m_transaction(transaction)
{
}

PassOwnPtr<LevelDBTransaction::TransactionIterator> LevelDBTransaction::TransactionIterator::create(PassRefPtr<LevelDBTransaction> transaction)
{
    return adoptPtr(new TransactionIterator(transaction));
}

LevelDBTransaction::TransactionIterator::TransactionIterator(PassRefPtr<LevelDBTransaction> transaction)
    : m_transaction(transaction)
    , m_comparator(m_transaction->m_comparator)
    , m_treeIterator(TreeIterator::create(m_transaction.get()))
    , m_dbIterator(m_transaction->m_db->createIterator(&m_transaction->m_snapshot))
    , m_current(0)
    , m_direction(kForward)
    , m_treeChanged(false)
{
    m_transaction->registerIterator(this);
}

LevelDBTransaction::TransactionIterator::~TransactionIterator()
{
    m_transaction->unregisterIterator(this);
}

bool LevelDBTransaction::TransactionIterator::isValid() const
{
    return m_current;
}

void LevelDBTransaction::TransactionIterator::seekToLast()
{
    m_treeIterator->seekToLast();
    m_dbIterator->seekToLast();
    m_direction = kReverse;

    handleConflictsAndDeletes();
    setCurrentIteratorToLargestKey();
}

void LevelDBTransaction::TransactionIterator::seek(const LevelDBSlice& target)
{
    m_treeIterator->seek(target);
    m_dbIterator->seek(target);
    m_direction = kForward;

    handleConflictsAndDeletes();
    setCurrentIteratorToSmallestKey();
}

void LevelDBTransaction::TransactionIterator::next()
{
    ASSERT(isValid());
    if (m_treeChanged)
        refreshTreeIterator();

    if (m_direction != kForward) {
        // Ensure the non-current iterator is positioned after key().

        LevelDBIterator* nonCurrent = (m_current == m_dbIterator.get()) ? m_treeIterator.get() : m_dbIterator.get();

        nonCurrent->seek(key());
        if (nonCurrent->isValid() && !m_comparator->compare(nonCurrent->key(), key()))
            nonCurrent->next(); // Take an extra step so the non-current key is strictly greater than key().

        ASSERT(!nonCurrent->isValid() || m_comparator->compare(nonCurrent->key(), key()) > 0);

        m_direction = kForward;
    }

    m_current->next();
    handleConflictsAndDeletes();
    setCurrentIteratorToSmallestKey();
}

void LevelDBTransaction::TransactionIterator::prev()
{
    ASSERT(isValid());
    if (m_treeChanged)
        refreshTreeIterator();

    if (m_direction != kReverse) {
        // Ensure the non-current iterator is positioned before key().

        LevelDBIterator* nonCurrent = (m_current == m_dbIterator.get()) ? m_treeIterator.get() : m_dbIterator.get();

        nonCurrent->seek(key());
        if (nonCurrent->isValid()) {
            // Iterator is at first entry >= key().
            // Step back once to entry < key.
            // This is why we don't check for the keys being the same before
            // stepping, like we do in next() above.
            nonCurrent->prev();
        } else
            nonCurrent->seekToLast(); // Iterator has no entries >= key(). Position at last entry.

        ASSERT(!nonCurrent->isValid() || m_comparator->compare(nonCurrent->key(), key()) < 0);

        m_direction = kReverse;
    }

    m_current->prev();
    handleConflictsAndDeletes();
    setCurrentIteratorToLargestKey();
}

LevelDBSlice LevelDBTransaction::TransactionIterator::key() const
{
    ASSERT(isValid());
    if (m_treeChanged)
        refreshTreeIterator();
    return m_current->key();
}

LevelDBSlice LevelDBTransaction::TransactionIterator::value() const
{
    ASSERT(isValid());
    if (m_treeChanged)
        refreshTreeIterator();
    return m_current->value();
}

void LevelDBTransaction::TransactionIterator::treeChanged()
{
    m_treeChanged = true;
}

void LevelDBTransaction::TransactionIterator::refreshTreeIterator() const
{
    ASSERT(m_treeChanged);

    m_treeChanged = false;

    if (m_treeIterator->isValid() && m_treeIterator == m_current) {
        m_treeIterator->reset();
        return;
    }

    if (m_dbIterator->isValid()) {

        // There could be new nodes in the tree that we should iterate over.

        if (m_direction == kForward) {
            // Try to seek tree iterator to something greater than the db iterator.
            m_treeIterator->seek(m_dbIterator->key());
            if (m_treeIterator->isValid() && !m_comparator->compare(m_treeIterator->key(), m_dbIterator->key()))
                m_treeIterator->next(); // If equal, take another step so the tree iterator is strictly greater.
        } else {
            // If going backward, seek to a key less than the db iterator.
            ASSERT(m_direction == kReverse);
            m_treeIterator->seek(m_dbIterator->key());
            if (m_treeIterator->isValid())
                m_treeIterator->prev();
        }
    }
}

bool LevelDBTransaction::TransactionIterator::treeIteratorIsLower() const
{
    return m_comparator->compare(m_treeIterator->key(), m_dbIterator->key()) < 0;
}

bool LevelDBTransaction::TransactionIterator::treeIteratorIsHigher() const
{
    return m_comparator->compare(m_treeIterator->key(), m_dbIterator->key()) > 0;
}

void LevelDBTransaction::TransactionIterator::handleConflictsAndDeletes()
{
    bool loop = true;

    while (loop) {
        loop = false;

        if (m_treeIterator->isValid() && m_dbIterator->isValid() && !m_comparator->compare(m_treeIterator->key(), m_dbIterator->key())) {
            // For equal keys, the tree iterator takes precedence, so move the database iterator another step.
            if (m_direction == kForward)
                m_dbIterator->next();
            else
                m_dbIterator->prev();
        }

        // Skip over delete markers in the tree iterator until it catches up with the db iterator.
        if (m_treeIterator->isValid() && m_treeIterator->isDeleted()) {
            if (m_direction == kForward && (!m_dbIterator->isValid() || treeIteratorIsLower())) {
                m_treeIterator->next();
                loop = true;
            } else if (m_direction == kReverse && (!m_dbIterator->isValid() || treeIteratorIsHigher())) {
                m_treeIterator->prev();
                loop = true;
            }
        }
    }
}

void LevelDBTransaction::TransactionIterator::setCurrentIteratorToSmallestKey()
{
    LevelDBIterator* smallest = 0;

    if (m_treeIterator->isValid())
        smallest = m_treeIterator.get();

    if (m_dbIterator->isValid()) {
        if (!smallest || m_comparator->compare(m_dbIterator->key(), smallest->key()) < 0)
            smallest = m_dbIterator.get();
    }

    m_current = smallest;
}

void LevelDBTransaction::TransactionIterator::setCurrentIteratorToLargestKey()
{
    LevelDBIterator* largest = 0;

    if (m_treeIterator->isValid())
        largest = m_treeIterator.get();

    if (m_dbIterator->isValid()) {
        if (!largest || m_comparator->compare(m_dbIterator->key(), largest->key()) > 0)
            largest = m_dbIterator.get();
    }

    m_current = largest;
}

void LevelDBTransaction::registerIterator(TransactionIterator* iterator)
{
    ASSERT(!m_iterators.contains(iterator));
    m_iterators.add(iterator);
}

void LevelDBTransaction::unregisterIterator(TransactionIterator* iterator)
{
    ASSERT(m_iterators.contains(iterator));
    m_iterators.remove(iterator);
}

void LevelDBTransaction::notifyIteratorsOfTreeChange()
{
    for (HashSet<TransactionIterator*>::iterator i = m_iterators.begin(); i != m_iterators.end(); ++i) {
        TransactionIterator* transactionIterator = *i;
        transactionIterator->treeChanged();
    }
}

PassOwnPtr<LevelDBWriteOnlyTransaction> LevelDBWriteOnlyTransaction::create(LevelDBDatabase* db)
{
    return adoptPtr(new LevelDBWriteOnlyTransaction(db));
}

LevelDBWriteOnlyTransaction::LevelDBWriteOnlyTransaction(LevelDBDatabase* db)
    : m_db(db)
    , m_writeBatch(LevelDBWriteBatch::create())
    , m_finished(false)
{
}

LevelDBWriteOnlyTransaction::~LevelDBWriteOnlyTransaction()
{
    m_writeBatch->clear();
}

void LevelDBWriteOnlyTransaction::remove(const LevelDBSlice& key)
{
    ASSERT(!m_finished);
    m_writeBatch->remove(key);
}

bool LevelDBWriteOnlyTransaction::commit()
{
    ASSERT(!m_finished);

    if (!m_db->write(*m_writeBatch))
        return false;

    m_finished = true;
    m_writeBatch->clear();
    return true;
}

} // namespace WebCore

#endif // USE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)
