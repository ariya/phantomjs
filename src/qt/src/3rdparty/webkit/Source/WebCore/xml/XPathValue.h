/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef XPathValue_h
#define XPathValue_h

#if ENABLE(XPATH)

#include "PlatformString.h"
#include "XPathNodeSet.h"

namespace WebCore {

    namespace XPath {
    
        class ValueData : public RefCounted<ValueData> {
        public:            
            static PassRefPtr<ValueData> create() { return adoptRef(new ValueData); }
            static PassRefPtr<ValueData> create(const NodeSet& nodeSet) { return adoptRef(new ValueData(nodeSet)); }
            static PassRefPtr<ValueData> create(const String& string) { return adoptRef(new ValueData(string)); }

            NodeSet m_nodeSet;
            String m_string;
            
        private:
            ValueData() { }
            ValueData(const NodeSet& nodeSet) : m_nodeSet(nodeSet) { }
            ValueData(const String& string) : m_string(string) { }            
        };

        // Copying Value objects makes their data partially shared, so care has to be taken when dealing with copies.
        class Value {
        public:
            enum Type { NodeSetValue, BooleanValue, NumberValue, StringValue };
            
            Value(unsigned value) : m_type(NumberValue), m_bool(false), m_number(value) {}
            Value(unsigned long value) : m_type(NumberValue), m_bool(false), m_number(value) {}
            Value(double value) : m_type(NumberValue), m_bool(false), m_number(value) {}

            Value(const char* value) : m_type(StringValue), m_bool(false), m_number(0), m_data(ValueData::create(value)) {}
            Value(const String& value) : m_type(StringValue), m_bool(false), m_number(0), m_data(ValueData::create(value)) {}
            Value(const NodeSet& value) : m_type(NodeSetValue), m_bool(false), m_number(0), m_data(ValueData::create(value)) {}
            Value(Node* value) : m_type(NodeSetValue), m_bool(false), m_number(0), m_data(ValueData::create()) { m_data->m_nodeSet.append(value); }

            // This is needed to safely implement constructing from bool - with normal function overloading, any pointer type would match.
            template<typename T> Value(T);

            static const struct AdoptTag {} adopt;
            Value(NodeSet& value, const AdoptTag&) : m_type(NodeSetValue), m_bool(false), m_number(0),  m_data(ValueData::create()) { value.swap(m_data->m_nodeSet); }

            Type type() const { return m_type; }

            bool isNodeSet() const { return m_type == NodeSetValue; }
            bool isBoolean() const { return m_type == BooleanValue; }
            bool isNumber() const { return m_type == NumberValue; }
            bool isString() const { return m_type == StringValue; }

            const NodeSet& toNodeSet() const;
            NodeSet& modifiableNodeSet();
            bool toBoolean() const;
            double toNumber() const;
            String toString() const;

        private:
            Type m_type;
            bool m_bool;
            double m_number;
            RefPtr<ValueData> m_data;
        };

        template<>
        inline Value::Value(bool value)
            : m_type(BooleanValue)
            , m_bool(value)
            , m_number(0)
        {
        }
    }
}

#endif // ENABLE(XPATH)

#endif // XPath_Value_H
