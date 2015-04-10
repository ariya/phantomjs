/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

function shouldBe(a, b)
{
    var evalA;
    try {
        evalA = eval(a);
    } catch(e) {
        evalA = e;
    }
    
    if (evalA == b || isNaN(evalA) && typeof evalA == 'number' && isNaN(b) && typeof b == 'number')
        print("PASS: " + a + " should be " + b + " and is.", "green");
    else
        print("__FAIL__: " + a + " should be " + b + " but instead is " + evalA + ".", "red");
}

function test()
{
    print("Node is " + Node);
    for (var p in Node)
        print(p + ": " + Node[p]);
    
    node = new Node();
    print("node is " + node);
    for (var p in node)
        print(p + ": " + node[p]);

    child1 = new Node();
    child2 = new Node();
    child3 = new Node();
    
    node.appendChild(child1);
    node.appendChild(child2);

    var childNodes = node.childNodes;
    
    for (var i = 0; i < childNodes.length + 1; i++) {
        print("item " + i + ": " + childNodes.item(i));
    }
    
    for (var i = 0; i < childNodes.length + 1; i++) {
        print(i + ": " + childNodes[i]);
    }

    node.removeChild(child1);
    node.replaceChild(child3, child2);
    
    for (var i = 0; i < childNodes.length + 1; i++) {
        print("item " + i + ": " + childNodes.item(i));
    }

    for (var i = 0; i < childNodes.length + 1; i++) {
        print(i + ": " + childNodes[i]);
    }

    try {
        node.appendChild(null);
    } catch(e) {
        print("caught: " + e);
    }
    
    try {
        var o = new Object();
        o.appendChild = node.appendChild;
        o.appendChild(node);
    } catch(e) {
        print("caught: " + e);
    }
    
    try {
        node.appendChild();
    } catch(e) {
        print("caught: " + e);
    }
    
    oldNodeType = node.nodeType;
    node.nodeType = 1;
    shouldBe("node.nodeType", oldNodeType);
    
    shouldBe("node instanceof Node", true);
    shouldBe("new Object() instanceof Node", false);
    
    print(Node);
}

test();
