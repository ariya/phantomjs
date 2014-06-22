#include "config.h"
#include "DumpRenderTree.h"

#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class CommandTokenizer {
public:
    explicit CommandTokenizer(const std::string& input)
        : m_input(input)
        , m_posNextSeparator(0)
    {
        pump();
    }

    bool hasNext() const;
    std::string next();

private:
    void pump();
    static const char kSeparator = '\'';
    const std::string& m_input;
    std::string m_next;
    size_t m_posNextSeparator;
};

void CommandTokenizer::pump()
{
    if (m_posNextSeparator == std::string::npos || m_posNextSeparator == m_input.size()) {
        m_next = std::string();
        return;
    }
    size_t start = m_posNextSeparator ? m_posNextSeparator + 1 : 0;
    m_posNextSeparator = m_input.find(kSeparator, start);
    size_t size = m_posNextSeparator == std::string::npos ? std::string::npos : m_posNextSeparator - start;
    m_next = std::string(m_input, start, size);
}

std::string CommandTokenizer::next()
{
    ASSERT(hasNext());

    std::string oldNext = m_next;
    pump();
    return oldNext;
}

bool CommandTokenizer::hasNext() const
{
    return !m_next.empty();
}

NO_RETURN static void die(const std::string& inputLine)
{
    fprintf(stderr, "Unexpected input line: %s\n", inputLine.c_str());
    exit(1);
}

TestCommand parseInputLine(const std::string& inputLine)
{
    TestCommand result;
    CommandTokenizer tokenizer(inputLine);
    if (!tokenizer.hasNext())
        die(inputLine);

    std::string arg = tokenizer.next();
    result.pathOrURL = arg;
    while (tokenizer.hasNext()) {
        arg = tokenizer.next();
        if (arg == std::string("--timeout")) {
            std::string timeoutToken = tokenizer.next();
            result.timeout = atoi(timeoutToken.c_str());
        } else if (arg == std::string("-p") || arg == std::string("--pixel-test")) {
            result.shouldDumpPixels = true;
            if (tokenizer.hasNext())
                result.expectedPixelHash = tokenizer.next();
        } else
            die(inputLine);
    }

    return result;
}
