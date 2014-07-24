# udis86 - scripts/ud_optable.py (optable.xml parser)
# 
# Copyright (c) 2009 Vivek Thampi
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, 
#       this list of conditions and the following disclaimer in the documentation 
#       and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys
from xml.dom import minidom

class UdOptableXmlParser:

    def parseDef( self, node ):
        ven = '' 
        pfx = [] 
        opc = [] 
        opr = []
        for def_node in node.childNodes:
            if not def_node.localName:
                continue
            if def_node.localName == 'pfx':
                pfx = def_node.firstChild.data.split();
            elif def_node.localName == 'opc':
                opc = def_node.firstChild.data.split();
            elif def_node.localName == 'opr':
                opr = def_node.firstChild.data.split();
            elif def_node.localName == 'mode':
                pfx.extend( def_node.firstChild.data.split() );
            elif def_node.localName == 'syn':
                pfx.extend( def_node.firstChild.data.split() );
            elif def_node.localName == 'vendor':
                ven = ( def_node.firstChild.data );
            else:
                print("warning: invalid node - %s" % def_node.localName)
                continue
        return ( pfx, opc, opr, ven )

    def parse( self, xml, fn ):
        xmlDoc = minidom.parse( xml )
        self.TlNode = xmlDoc.firstChild

        while self.TlNode and self.TlNode.localName != "x86optable": 
            self.TlNode = self.TlNode.nextSibling

        for insnNode in self.TlNode.childNodes:
            if not insnNode.localName:
                continue
            if insnNode.localName != "instruction":
                print("warning: invalid insn node - %s" % insnNode.localName)
                continue

            mnemonic = insnNode.getElementsByTagName( 'mnemonic' )[ 0 ].firstChild.data
            vendor   = ''

            for node in insnNode.childNodes:
                if node.localName == 'vendor':
                    vendor = node.firstChild.data
                elif node.localName == 'def':
                    ( prefixes, opcodes, operands, local_vendor ) = \
                        self.parseDef( node )
                    if ( len( local_vendor ) ):
                        vendor = local_vendor
                    # callback
                    fn( prefixes, mnemonic, opcodes, operands, vendor )


def printFn( pfx, mnm, opc, opr, ven ):
    print('def: '),
    if len( pfx ):
        print(' '.join( pfx )),
    print("%s %s %s %s" % \
            ( mnm, ' '.join( opc ), ' '.join( opr ), ven ))


def parse( xml, callback ):
    parser = UdOptableXmlParser()  
    parser.parse( xml, callback )

def main():
    parser = UdOptableXmlParser()  
    parser.parse( sys.argv[ 1 ], printFn )

if __name__ == "__main__":
    main() 
