/*
 * Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XPathPath_h
#define XPathPath_h

#include "XPathExpressionNode.h"
#include "XPathNodeSet.h"

namespace WebCore {

    namespace XPath {

        class Predicate;
        class Step;

        class Filter : public Expression {
        public:
            Filter(Expression*, const Vector<Predicate*>& = Vector<Predicate*>());
            virtual ~Filter();

            virtual Value evaluate() const;

        private:
            virtual Value::Type resultType() const { return Value::NodeSetValue; }

            Expression* m_expr;
            Vector<Predicate*> m_predicates;
        };

        class LocationPath : public Expression {
        public:
            LocationPath();
            virtual ~LocationPath();
            void setAbsolute(bool value) { m_absolute = value; setIsContextNodeSensitive(!m_absolute); }

            virtual Value evaluate() const;
            void evaluate(NodeSet& nodes) const; // nodes is an input/output parameter

            void appendStep(Step* step);
            void insertFirstStep(Step* step);

        private:
            virtual Value::Type resultType() const { return Value::NodeSetValue; }

            Vector<Step*> m_steps;
            bool m_absolute;
        };

        class Path : public Expression {
        public:
            Path(Filter*, LocationPath*);
            virtual ~Path();

            virtual Value evaluate() const;

        private:
            virtual Value::Type resultType() const { return Value::NodeSetValue; }

            Filter* m_filter;
            LocationPath* m_path;
        };

    }
}

#endif // XPath_Path_H
