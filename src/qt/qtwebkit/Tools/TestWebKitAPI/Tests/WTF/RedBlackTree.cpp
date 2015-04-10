/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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
#include <wtf/RedBlackTree.h>
#include <wtf/Vector.h>

using namespace WTF;

namespace TestWebKitAPI {

class TestNode : public RedBlackTree<TestNode, char>::Node {
public:
    TestNode(char key, unsigned value)
        : m_key(key)
        , m_value(value)
    {
    }

    char key()
    {
        return m_key;
    }

    char m_key;
    unsigned m_value;
};

class RedBlackTreeTest : public testing::Test {
public:
    unsigned m_counter;
    
    virtual void SetUp()
    {
        m_counter = 0;
    }
    
    virtual void TearDown()
    {
    }
    
    struct Pair {
        char key;
        unsigned value;
        
        Pair() { }
        
        Pair(char key, unsigned value)
            : key(key)
            , value(value)
        {
        }
        
        bool operator==(const Pair& other) const
        {
            return key == other.key;
        }
        
        bool operator<(const Pair& other) const
        {
            return key < other.key;
        }
    };
    
    typedef Vector<Pair, 16> PairVector;
    
    PairVector findExact(PairVector& asVector, char key)
    {
        PairVector result;
        
        for (size_t index = 0; index < asVector.size(); ++index) {
            if (asVector.at(index).key == key)
                result.append(asVector.at(index));
        }
        
        std::sort(result.begin(), result.end());
        
        return result;
    }
    
    void remove(PairVector& asVector, size_t index)
    {
        asVector.at(index) = asVector.last();
        asVector.removeLast();
    }
    
    PairVector findLeastGreaterThanOrEqual(PairVector& asVector, char key)
    {
        char bestKey = 0; // assignment to make gcc happy
        bool foundKey = false;
        
        for (size_t index = 0; index < asVector.size(); ++index) {
            if (asVector.at(index).key >= key) {
                if (asVector.at(index).key < bestKey || !foundKey) {
                    foundKey = true;
                    bestKey = asVector.at(index).key;
                }
            }
        }
        
        PairVector result;
        
        if (!foundKey)
            return result;
        
        return findExact(asVector, bestKey);
    }
    
    void assertFoundAndRemove(PairVector& asVector, char key, unsigned value)
    {
            bool found = false;
            size_t foundIndex = 0; // make compilers happy
            
            for (size_t index = 0; index < asVector.size(); ++index) {
                if (asVector.at(index).key == key
                    && asVector.at(index).value == value) {
                    EXPECT_TRUE(!found);
                    
                    found = true;
                    foundIndex = index;
                }
            }
            
            EXPECT_TRUE(found);
            
            remove(asVector, foundIndex);
    }
    
    // This deliberately passes a copy of the vector.
    void assertEqual(RedBlackTree<TestNode, char>& asTree, PairVector asVector)
    {
        for (TestNode* current = asTree.first(); current; current = current->successor())
            assertFoundAndRemove(asVector, current->m_key, current->m_value);
    }
    
    void assertSameValuesForKey(RedBlackTree<TestNode, char>& asTree, TestNode* node, PairVector foundValues, char key)
    {
        if (node) {
            EXPECT_EQ(node->m_key, key);
            
            TestNode* prevNode = node;
            do {
                node = prevNode;
                prevNode = prevNode->predecessor();
            } while (prevNode && prevNode->m_key == key);
            
            EXPECT_EQ(node->m_key, key);
            EXPECT_TRUE(!prevNode || prevNode->m_key < key);
            
            do {
                assertFoundAndRemove(foundValues, node->m_key, node->m_value);
                
                node = node->successor();
                EXPECT_TRUE(!node || node->m_key >= key);
            } while (node && node->m_key == key);
        }
        
        EXPECT_TRUE(foundValues.isEmpty());
    }
    
    // The control string is a null-terminated list of commands. Each
    // command is two characters, with the first identifying the operation
    // and the second giving a key. The commands are:
    //  +x  Add x
    //  *x  Find all elements equal to x
    //  @x  Find all elements that have the smallest key that is greater than or equal to x
    //  !x  Remove all elements equal to x
    void testDriver(const char* controlString)
    {
        PairVector asVector;
        RedBlackTree<TestNode, char> asTree;
        
        for (const char* current = controlString; *current; current += 2) {
            char command = current[0];
            char key = current[1];
            unsigned value = ++m_counter;
            
            ASSERT(command);
            ASSERT(key);
            
            switch (command) {
            case '+': {
                TestNode* node = new TestNode(key, value);
                asTree.insert(node);
                asVector.append(Pair(key, value));
                break;
            }
                
            case '*': {
                TestNode* node = asTree.findExact(key);
                if (node)
                    EXPECT_EQ(node->m_key, key);
                assertSameValuesForKey(asTree, node, findExact(asVector, key), key);
                break;
            }

            case '@': {
                TestNode* node = asTree.findLeastGreaterThanOrEqual(key);
                if (node) {
                    EXPECT_TRUE(node->m_key >= key);
                    assertSameValuesForKey(asTree, node, findLeastGreaterThanOrEqual(asVector, key), node->m_key);
                } else
                    EXPECT_TRUE(findLeastGreaterThanOrEqual(asVector, key).isEmpty());
                break;
            }
                
            case '!': {
                while (true) {
                    TestNode* node = asTree.remove(key);
                    if (node) {
                        EXPECT_EQ(node->m_key, key);
                        assertFoundAndRemove(asVector, node->m_key, node->m_value);
                    } else {
                        EXPECT_TRUE(findExact(asVector, key).isEmpty());
                        break;
                    }
                }
                break;
            }
                
            default:
                ASSERT_NOT_REACHED();
                break;
            }
            
            EXPECT_EQ(asTree.size(), asVector.size());
            assertEqual(asTree, asVector);
        }
    }
};

TEST_F(RedBlackTreeTest, Empty)
{
    testDriver("");
}

TEST_F(RedBlackTreeTest, EmptyGetFindRemove)
{
    testDriver("*x@y!z");
}

TEST_F(RedBlackTreeTest, SingleAdd)
{
    testDriver("+a");
}

TEST_F(RedBlackTreeTest, SingleAddGetFindRemoveNotFound)
{
    testDriver("+a*x@y!z");
}

TEST_F(RedBlackTreeTest, SingleAddGetFindRemove)
{
    testDriver("+a*a@a!a");
}

TEST_F(RedBlackTreeTest, TwoAdds)
{
    testDriver("+a+b");
}

TEST_F(RedBlackTreeTest, ThreeAdds)
{
    testDriver("+a+b+c");
}

TEST_F(RedBlackTreeTest, FourAdds)
{
    testDriver("+a+b+c+d");
}

TEST_F(RedBlackTreeTest, LotsOfRepeatAdds)
{
    testDriver("+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d");
}

TEST_F(RedBlackTreeTest, LotsOfRepeatAndUniqueAdds)
{
    testDriver("+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p+q+r+s+t+u+v+x+y+z");
}

TEST_F(RedBlackTreeTest, LotsOfRepeatAndUniqueAddsAndGetsAndRemoves)
{
    testDriver("+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p+q+r+s+t+u+v+x+y+z*a*b*c*d*e*f*g*h*i*j*k*l*m*n*o*p*q*r*s*t*u*v*w*x*y*z!a!b!c!d!e!f!g!h!i!j!k!l!m!n!o!p!q!r!s!t!u!v!w!x!y!z");
}

TEST_F(RedBlackTreeTest, SimpleBestFitSearch)
{
    testDriver("+d+d+m+w@d@m@w@a@g@q");
}

TEST_F(RedBlackTreeTest, BiggerBestFitSearch)
{
    testDriver("+d+d+d+d+d+d+d+d+d+d+f+f+f+f+f+f+f+h+h+i+j+k+l+m+o+p+q+r+z@a@b@c@d@e@f@g@h@i@j@k@l@m@n@o@p@q@r@s@t@u@v@w@x@y@z");
}

} // namespace TestWebKitAPI
