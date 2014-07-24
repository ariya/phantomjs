/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef LALR_H
#define LALR_H

#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/qmap.h>
#include <QtCore/qlinkedlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qpair.h>

#include <algorithm>
#include <functional>

class Rule;
class State;
class Grammar;
class Item;
class State;
class Arrow;
class Automaton;

template <typename _Tp >
class OrderedSet : protected QMap<_Tp, bool>
{
  typedef QMap<_Tp, bool> _Base;

public:
  class const_iterator
  {
    typename _Base::const_iterator _M_iterator;

  public:
    const_iterator () {}

    const_iterator (const typename _Base::const_iterator &it):
      _M_iterator (it) {}

    const _Tp &operator * () const
    { return _M_iterator.key (); }

    const _Tp *operator -> () const
    { return &_M_iterator.key (); }

    const_iterator &operator ++ ()
    { ++_M_iterator; return *this; }

    const_iterator operator ++ (int) const
    {
      const_iterator me (*this);
      ++_M_iterator;
      return me;
    }

    bool operator == (const const_iterator &other) const
    { return _M_iterator == other._M_iterator; }

    bool operator != (const const_iterator &other) const
    { return _M_iterator != other._M_iterator; }
  };

  typedef const_iterator iterator;

public:
  OrderedSet () {}

  const_iterator begin () const
  { return const_iterator (_Base::begin ()); }

  const_iterator end () const
  { return const_iterator (_Base::end ()); }

  bool isEmpty () const
  { return _Base::isEmpty (); }

  int size () const
  { return _Base::size (); }

  const_iterator find (const _Tp &elt) const
  { return const_iterator (_Base::find (elt)); }

  QPair<const_iterator, bool> insert (const _Tp &elt)
  {
    int elts = _Base::size ();
    const_iterator it (_Base::insert (typename _Base::key_type (elt), true));
    return qMakePair (it, elts != _Base::size ());
  }

  QPair<const_iterator, bool> insert (const_iterator, const _Tp &elt)
  {
    int elts = _Base::size ();
    const_iterator it (_Base::insert (typename _Base::key_type (elt), true));
    return qMakePair (it, elts != _Base::size ());
  }

  const _Tp &operator [] (const _Tp &elt)
  { return *insert (elt)->first; }

  template <typename _InputIterator>
  void insert (_InputIterator first, _InputIterator last)
  {
    for (; first != last; ++first)
      insert (*first);
  }
};

// names
typedef QLinkedList<QString>::iterator Name;
typedef QLinkedList<Name> NameList;
typedef OrderedSet<Name> NameSet;

// items
typedef QLinkedList<Item> ItemList;
typedef ItemList::iterator ItemPointer;

// rules
typedef QLinkedList<Rule> debug_infot;
typedef debug_infot::iterator RulePointer;
typedef QMultiMap<Name, RulePointer> RuleMap;

// states
typedef QLinkedList<State> StateList;
typedef StateList::iterator StatePointer;

// arrows
typedef QMap<Name, StatePointer> Bundle;

class Rule
{
public:
  void clear ()
  {
    lhs = Name ();
    rhs.clear ();
    prec = Name ();
  }

public:
  Name lhs;
  NameList rhs;
  Name prec;
};

class Lookback
{
public:
  Lookback (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Lookback &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Lookback &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Lookback &other) const;

public:
  StatePointer state;
  Name nt;
};

class Item
{
public:
  inline NameList::iterator begin_rhs () const
  { return rule->rhs.begin (); }

  inline NameList::iterator end_rhs () const
  { return rule->rhs.end (); }

  inline bool operator == (const Item &other) const
  { return rule == other.rule && dot == other.dot; }

  inline bool operator != (const Item &other) const
  { return rule != other.rule || dot != other.dot; }

  inline bool isReduceItem () const
  { return dot == rule->rhs.end (); }

  Item next () const;

public:
  RulePointer rule;
  NameList::iterator dot;
};

class State
{
public:
  State (Grammar *grammar);

  inline bool operator == (const State &other) const
  { return kernel == other.kernel; }

  inline bool operator != (const State &other) const
  { return kernel != other.kernel; }

  QPair<ItemPointer, bool> insert (const Item &item);
  QPair<ItemPointer, bool> insertClosure (const Item &item);

public: // attributes
  ItemList kernel;
  ItemList closure;
  Bundle bundle;
  QMap<Name, NameSet> reads;
  QMap<Name, NameSet> follows;
  RulePointer defaultReduce;
};

/////////////////////////////////////////////////////////////
// digraph
/////////////////////////////////////////////////////////////
template <typename _Tp>
class Node
{
public:
  typedef OrderedSet<Node<_Tp> > Repository;
  typedef typename Repository::iterator iterator;
  typedef typename QLinkedList<iterator>::iterator edge_iterator;

public:
  static iterator get (_Tp data);

  QPair<edge_iterator, bool> insertEdge (iterator other) const;

  inline edge_iterator begin () const
  { return outs.begin (); }

  inline edge_iterator end () const
  { return outs.end (); }

  inline bool operator == (const Node<_Tp> &other) const
  { return data == other.data; }

  inline bool operator != (const Node<_Tp> &other) const
  { return data != other.data; }

  inline bool operator < (const Node<_Tp> &other) const
  { return data < other.data; }

  static inline iterator begin_nodes ()
  { return repository ().begin (); }

  static inline iterator end_nodes ()
  { return repository ().end (); }

  static Repository &repository ()
  {
    static Repository r;
    return r;
  }

public: // attributes
  mutable bool root;
  mutable int dfn;
  mutable _Tp data;
  mutable QLinkedList<iterator> outs;

protected:
  inline Node () {}

  inline Node (_Tp d):
    root (true), dfn (0), data (d) {}
};

template <typename _Tp>
typename Node<_Tp>::iterator Node<_Tp>::get (_Tp data)
{
  Node<_Tp> tmp (data);
  iterator it = repository ().find (tmp);

  if (it != repository ().end ())
    return it;

  return repository ().insert (tmp).first;
}

template <typename _Tp>
QPair<typename QLinkedList<typename Node<_Tp>::iterator>::iterator, bool> Node<_Tp>::insertEdge (typename Node<_Tp>::iterator other) const
{
  edge_iterator it = std::find (outs.begin (), outs.end (), other);

  if (it != outs.end ())
    return qMakePair (it, false);

  other->root = false;
  return qMakePair (outs.insert (outs.end (), other), true);
}

/////////////////////////////////////////////////////////////
// Grammar
/////////////////////////////////////////////////////////////
class Grammar
{
public:
  Grammar ();

  Name intern (const QString &id);

  inline bool isTerminal (Name name) const
  { return terminals.find (name) != terminals.end (); }

  inline bool isNonTerminal (Name name) const
  { return non_terminals.find (name) != non_terminals.end (); }

  void buildRuleMap ();
  void buildExtendedGrammar ();

public:
  QString merged_output;
  QString table_name;
  QString decl_file_name;
  QString impl_file_name;
  QString token_prefix;
  QLinkedList<QString> names;
  Name start;
  NameSet terminals;
  NameSet non_terminals;
  QMap<Name, QString> spells;
  debug_infot rules;
  RuleMap rule_map;
  RulePointer goal;
  Name tk_end;
  Name accept_symbol;
  NameSet declared_lhs;
  int expected_shift_reduce;
  int expected_reduce_reduce;

  enum Assoc {
    NonAssoc,
    Left,
    Right
  };

  struct TokenInfo {
    Assoc assoc;
    int prec;
  };

  QMap<Name, TokenInfo> token_info;
  Assoc current_assoc;
  int current_prec;
};

class Read
{
public:
  inline Read () {}

  inline Read (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Read &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Read &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Read &other) const;

public:
  StatePointer state;
  Name nt;
};

class Include
{
public:
  inline Include () {}

  inline Include (StatePointer s, Name n):
    state (s), nt (n) {}

  inline bool operator == (const Include &other) const
  { return state == other.state && nt == other.nt; }

  inline bool operator != (const Include &other) const
  { return state != other.state || nt != other.nt; }

  bool operator < (const Include &other) const;

public:
  StatePointer state;
  Name nt;
};

class Automaton
{
public:
  Automaton (Grammar *g);

  QPair<StatePointer, bool> internState (const State &state);

  typedef Node<Read> ReadsGraph;
  typedef ReadsGraph::iterator ReadNode;

  typedef Node<Include> IncludesGraph;
  typedef IncludesGraph::iterator IncludeNode;

  void build ();
  void buildNullables ();

  void buildLookbackSets ();

  void buildDirectReads ();
  void buildReadsDigraph ();
  void buildReads ();
  void visitReadNode (ReadNode node);

  void buildIncludesAndFollows ();
  void buildIncludesDigraph ();
  void visitIncludeNode (IncludeNode node);

  void buildLookaheads ();

  void buildDefaultReduceActions ();

  void closure (StatePointer state);

  int id (RulePointer rule);
  int id (StatePointer state);
  int id (Name name);

  void dump (QTextStream &out, IncludeNode incl);
  void dump (QTextStream &out, ReadNode rd);
  void dump (QTextStream &out, const Lookback &lp);

public: // ### private
  Grammar *_M_grammar;
  StateList states;
  StatePointer start;
  NameSet nullables;
  QMultiMap<ItemPointer, Lookback> lookbacks;
  QMap<ItemPointer, NameSet> lookaheads;

private:
  QStack<ReadsGraph::iterator> _M_reads_stack;
  int _M_reads_dfn;

  QStack<IncludesGraph::iterator> _M_includes_stack;
  int _M_includes_dfn;
};

QT_BEGIN_NAMESPACE
bool operator < (Name a, Name b);
bool operator < (StatePointer a, StatePointer b);
bool operator < (ItemPointer a, ItemPointer b);
QT_END_NAMESPACE

QTextStream &operator << (QTextStream &out, const Name &n);
QTextStream &operator << (QTextStream &out, const Rule &r);
QTextStream &operator << (QTextStream &out, const Item &item);
QTextStream &operator << (QTextStream &out, const NameSet &ns);

QT_BEGIN_NAMESPACE
// ... hmm
extern QTextStream qerr;
extern QTextStream qout;
QT_END_NAMESPACE

#endif // LALR_H
