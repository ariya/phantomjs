/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGPathParserFactory.h"

#include "PathTraversalState.h"
#include "SVGPathBlender.h"
#include "SVGPathBuilder.h"
#include "SVGPathByteStreamBuilder.h"
#include "SVGPathByteStreamSource.h"
#include "SVGPathElement.h"
#include "SVGPathParser.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListSource.h"
#include "SVGPathStringBuilder.h"
#include "SVGPathStringSource.h"
#include "SVGPathTraversalStateBuilder.h"

namespace WebCore {

static SVGPathBuilder* globalSVGPathBuilder(Path& result)
{
    static SVGPathBuilder* s_builder = 0;
    if (!s_builder)
        s_builder = new SVGPathBuilder;

    s_builder->setCurrentPath(&result);
    return s_builder;
}

static SVGPathSegListBuilder* globalSVGPathSegListBuilder(SVGPathElement* element, SVGPathSegRole role, SVGPathSegList& result)
{
    static SVGPathSegListBuilder* s_builder = 0;
    if (!s_builder)
        s_builder = new SVGPathSegListBuilder;

    s_builder->setCurrentSVGPathElement(element);
    s_builder->setCurrentSVGPathSegList(result);
    s_builder->setCurrentSVGPathSegRole(role);
    return s_builder;
}

static SVGPathByteStreamBuilder* globalSVGPathByteStreamBuilder(SVGPathByteStream* result)
{
    static SVGPathByteStreamBuilder* s_builder = 0;
    if (!s_builder)
        s_builder = new SVGPathByteStreamBuilder;

    s_builder->setCurrentByteStream(result);
    return s_builder;
}

static SVGPathStringBuilder* globalSVGPathStringBuilder()
{
    static SVGPathStringBuilder* s_builder = 0;
    if (!s_builder)
        s_builder = new SVGPathStringBuilder;

    return s_builder;
}

static SVGPathTraversalStateBuilder* globalSVGPathTraversalStateBuilder(PathTraversalState& traversalState, float length)
{
    static SVGPathTraversalStateBuilder* s_builder = 0;
    if (!s_builder)
        s_builder = new SVGPathTraversalStateBuilder;

    s_builder->setCurrentTraversalState(&traversalState);
    s_builder->setDesiredLength(length);
    return s_builder;
}

static SVGPathParser* globalSVGPathParser(SVGPathSource* source, SVGPathConsumer* consumer)
{
    static SVGPathParser* s_parser = 0;
    if (!s_parser)
        s_parser = new SVGPathParser;

    s_parser->setCurrentSource(source);
    s_parser->setCurrentConsumer(consumer);
    return s_parser;
}

static SVGPathBlender* globalSVGPathBlender()
{
    static SVGPathBlender* s_blender = 0;
    if (!s_blender)
        s_blender = new SVGPathBlender;

    return s_blender;
}

SVGPathParserFactory* SVGPathParserFactory::self()
{
    static SVGPathParserFactory* s_instance = 0;
    if (!s_instance)
        s_instance = new SVGPathParserFactory;

    return s_instance;
}

SVGPathParserFactory::SVGPathParserFactory()
{
}

SVGPathParserFactory::~SVGPathParserFactory()
{
}

bool SVGPathParserFactory::buildPathFromString(const String& d, Path& result)
{
    if (d.isEmpty())
        return false;

    SVGPathBuilder* builder = globalSVGPathBuilder(result);

    OwnPtr<SVGPathStringSource> source = SVGPathStringSource::create(d);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildSVGPathByteStreamFromSVGPathSegList(const SVGPathSegList& list, OwnPtr<SVGPathByteStream>& result, PathParsingMode parsingMode)
{
    result = SVGPathByteStream::create();
    if (list.isEmpty())
        return false;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result.get());

    OwnPtr<SVGPathSegListSource> source = SVGPathSegListSource::create(list);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildPathFromByteStream(SVGPathByteStream* stream, Path& result)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false;

    SVGPathBuilder* builder = globalSVGPathBuilder(result);

    OwnPtr<SVGPathByteStreamSource> source = SVGPathByteStreamSource::create(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildSVGPathSegListFromByteStream(SVGPathByteStream* stream, SVGPathElement* element, SVGPathSegList& result, PathParsingMode parsingMode)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false; 

    SVGPathSegListBuilder* builder = globalSVGPathSegListBuilder(element, parsingMode == NormalizedParsing ? PathSegNormalizedRole : PathSegUnalteredRole, result);

    OwnPtr<SVGPathByteStreamSource> source = SVGPathByteStreamSource::create(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildStringFromByteStream(SVGPathByteStream* stream, String& result, PathParsingMode parsingMode)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false; 

    SVGPathStringBuilder* builder = globalSVGPathStringBuilder();

    OwnPtr<SVGPathByteStreamSource> source = SVGPathByteStreamSource::create(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    result = builder->result();
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildStringFromSVGPathSegList(const SVGPathSegList& list, String& result, PathParsingMode parsingMode)
{
    result = String();
    if (list.isEmpty())
        return false;

    SVGPathStringBuilder* builder = globalSVGPathStringBuilder();

    OwnPtr<SVGPathSegListSource> source = SVGPathSegListSource::create(list);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    result = builder->result();
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildSVGPathByteStreamFromString(const String& d, OwnPtr<SVGPathByteStream>& result, PathParsingMode parsingMode)
{
    result = SVGPathByteStream::create();
    if (d.isEmpty())
        return false;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result.get());

    OwnPtr<SVGPathStringSource> source = SVGPathStringSource::create(d);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool SVGPathParserFactory::buildAnimatedSVGPathByteStream(SVGPathByteStream* fromStream, SVGPathByteStream* toStream, OwnPtr<SVGPathByteStream>& result, float progress)
{
    ASSERT(fromStream);
    ASSERT(toStream);
    result = SVGPathByteStream::create();
    if (fromStream->isEmpty() || toStream->isEmpty())
        return false;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result.get());

    OwnPtr<SVGPathByteStreamSource> fromSource = SVGPathByteStreamSource::create(fromStream);
    OwnPtr<SVGPathByteStreamSource> toSource = SVGPathByteStreamSource::create(toStream);
    SVGPathBlender* blender = globalSVGPathBlender();
    bool ok = blender->blendAnimatedPath(progress, fromSource.get(), toSource.get(), builder);
    blender->cleanup();
    return ok;
}

bool SVGPathParserFactory::getSVGPathSegAtLengthFromSVGPathByteStream(SVGPathByteStream* stream, float length, unsigned long& pathSeg)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::TraversalSegmentAtLength);
    SVGPathTraversalStateBuilder* builder = globalSVGPathTraversalStateBuilder(traversalState, length);

    OwnPtr<SVGPathByteStreamSource> source = SVGPathByteStreamSource::create(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    pathSeg = builder->pathSegmentIndex();
    parser->cleanup();
    return ok;
}

}

#endif
