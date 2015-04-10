/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QGRAPH_P_H
#define QGRAPH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QDebug>

#include <float.h>

QT_BEGIN_NAMESPACE

template <typename Vertex, typename EdgeData>
class Graph
{
public:
    Graph() {}

    class const_iterator {
    public:
        const_iterator(const Graph *graph, bool begin) : g(graph){
            if (begin) {
                row = g->m_graph.constBegin();
                //test if the graph is empty
                if (row != g->m_graph.constEnd())
                {
                    column = (*row)->constBegin();
                }
            } else {
                row = g->m_graph.constEnd();
            }
        }

        inline Vertex *operator*() {
            return column.key();
        }

        inline Vertex *from() const {
            return row.key();
        }

        inline Vertex *to() const {
            return column.key();
        }

        inline bool operator==(const const_iterator &o) const { return !(*this != o); }
        inline bool operator!=(const const_iterator &o) const {
           if (row ==  g->m_graph.end()) {
                return row != o.row;
           } else {
                return row != o.row || column != o.column;
           }
        }
        inline const_iterator& operator=(const const_iterator &o) const { row = o.row; column = o.column; return *this;}

        // prefix
        const_iterator &operator++() {
            if (row != g->m_graph.constEnd()) {
                ++column;
                if (column == (*row)->constEnd()) {
                    ++row;
                    if (row != g->m_graph.constEnd()) {
                        column = (*row)->constBegin();
                    }
                }
            }
            return *this;
        }

    private:
        const Graph *g;
        typename QHash<Vertex *, QHash<Vertex *, EdgeData *> * >::const_iterator row;
        typename QHash<Vertex *, EdgeData *>::const_iterator column;
    };

    const_iterator constBegin() const {
        return const_iterator(this,true);
    }

    const_iterator constEnd() const {
        return const_iterator(this,false);
    }

    /*!
     * \internal
     *
     * If there is an edge between \a first and \a second, it will return a structure
     * containing the data associated with the edge, otherwise it will return 0.
     *
     */
    EdgeData *edgeData(Vertex* first, Vertex* second) {
        QHash<Vertex *, EdgeData *> *row = m_graph.value(first);
        return row ? row->value(second) : 0;
    }

    void createEdge(Vertex *first, Vertex *second, EdgeData *data)
    {
        // Creates a bidirectional edge
#if defined(QT_DEBUG) && 0
        qDebug("Graph::createEdge(): %s",
               qPrintable(QString::fromLatin1("%1-%2")
                          .arg(first->toString()).arg(second->toString())));
#endif
        if (edgeData(first, second)) {
#ifdef QT_DEBUG
            qWarning("%s-%s already has an edge", qPrintable(first->toString()), qPrintable(second->toString()));
#endif
        }
        createDirectedEdge(first, second, data);
        createDirectedEdge(second, first, data);
    }

    void removeEdge(Vertex *first, Vertex *second)
    {
        // Removes a bidirectional edge
#if defined(QT_DEBUG) && 0
        qDebug("Graph::removeEdge(): %s",
               qPrintable(QString::fromLatin1("%1-%2")
                          .arg(first->toString()).arg(second->toString())));
#endif
        EdgeData *data = edgeData(first, second);
        removeDirectedEdge(first, second);
        removeDirectedEdge(second, first);
        if (data) delete data;
    }

    EdgeData *takeEdge(Vertex* first, Vertex* second)
    {
#if defined(QT_DEBUG) && 0
        qDebug("Graph::takeEdge(): %s",
               qPrintable(QString::fromLatin1("%1-%2")
                          .arg(first->toString()).arg(second->toString())));
#endif
        // Removes a bidirectional edge
        EdgeData *data = edgeData(first, second);
        if (data) {
            removeDirectedEdge(first, second);
            removeDirectedEdge(second, first);
        }
        return data;
    }

    QList<Vertex *> adjacentVertices(Vertex *vertex) const
    {
        QHash<Vertex *, EdgeData *> *row = m_graph.value(vertex);
        QList<Vertex *> l;
        if (row)
            l = row->keys();
        return l;
    }

    QSet<Vertex*> vertices() const {
        QSet<Vertex *> setOfVertices;
        for (const_iterator it = constBegin(); it != constEnd(); ++it) {
            setOfVertices.insert(*it);
        }
        return setOfVertices;
    }

    QList<QPair<Vertex*, Vertex*> > connections() const {
        QList<QPair<Vertex*, Vertex*> > conns;
        for (const_iterator it = constBegin(); it != constEnd(); ++it) {
            Vertex *from = it.from();
            Vertex *to = it.to();
            // do not return (from,to) *and* (to,from)
            if (from < to) {
                conns.append(qMakePair(from, to));
            }
        }
        return conns;
    }

#if defined(QT_DEBUG)
    QString serializeToDot() {   // traversal
        QString strVertices;
        QString edges;

        QSet<Vertex *> setOfVertices = vertices();
        for (typename QSet<Vertex*>::const_iterator it = setOfVertices.begin(); it != setOfVertices.end(); ++it) {
            Vertex *v = *it;
            QList<Vertex*> adjacents = adjacentVertices(v);
            for (int i = 0; i < adjacents.count(); ++i) {
                Vertex *v1 = adjacents.at(i);
                EdgeData *data = edgeData(v, v1);
                bool forward = data->from == v;
                if (forward) {
                    edges += QString::fromLatin1("\"%1\"->\"%2\" [label=\"[%3,%4,%5,%6,%7]\" color=\"#000000\"] \n")
                        .arg(v->toString())
                        .arg(v1->toString())
                        .arg(data->minSize)
                        .arg(data->minPrefSize)
                        .arg(data->prefSize)
                        .arg(data->maxPrefSize)
                        .arg(data->maxSize)
                        ;
                }
            }
            strVertices += QString::fromLatin1("\"%1\" [label=\"%2\"]\n").arg(v->toString()).arg(v->toString());
        }
        return QString::fromLatin1("%1\n%2\n").arg(strVertices).arg(edges);
    }
#endif

protected:
    void createDirectedEdge(Vertex *from, Vertex *to, EdgeData *data)
    {
        QHash<Vertex *, EdgeData *> *adjacentToFirst = m_graph.value(from);
        if (!adjacentToFirst) {
            adjacentToFirst = new QHash<Vertex *, EdgeData *>();
            m_graph.insert(from, adjacentToFirst);
        }
        adjacentToFirst->insert(to, data);
    }

    void removeDirectedEdge(Vertex *from, Vertex *to)
    {
        QHash<Vertex *, EdgeData *> *adjacentToFirst = m_graph.value(from);
        Q_ASSERT(adjacentToFirst);

        adjacentToFirst->remove(to);
        if (adjacentToFirst->isEmpty()) {
            //nobody point to 'from' so we can remove it from the graph
            m_graph.remove(from);
            delete adjacentToFirst;
        }
    }

private:
    QHash<Vertex *, QHash<Vertex *, EdgeData *> *> m_graph;
};

QT_END_NAMESPACE

#endif
