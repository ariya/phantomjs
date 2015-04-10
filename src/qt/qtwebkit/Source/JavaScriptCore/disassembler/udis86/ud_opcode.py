# udis86 - scripts/ud_opcode.py
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

class UdOpcodeTables:

    TableInfo = {
        'opctbl'    : { 'name' : 'UD_TAB__OPC_TABLE',   'size' : 256 },
        '/sse'      : { 'name' : 'UD_TAB__OPC_SSE',     'size' : 4 },
        '/reg'      : { 'name' : 'UD_TAB__OPC_REG',     'size' : 8 },
        '/rm'       : { 'name' : 'UD_TAB__OPC_RM',      'size' : 8 },
        '/mod'      : { 'name' : 'UD_TAB__OPC_MOD',     'size' : 2 },
        '/m'        : { 'name' : 'UD_TAB__OPC_MODE',    'size' : 3 },
        '/x87'      : { 'name' : 'UD_TAB__OPC_X87',     'size' : 64 },
        '/a'        : { 'name' : 'UD_TAB__OPC_ASIZE',   'size' : 3 },
        '/o'        : { 'name' : 'UD_TAB__OPC_OSIZE',   'size' : 3 },
        '/3dnow'    : { 'name' : 'UD_TAB__OPC_3DNOW',   'size' : 256 },
        'vendor'    : { 'name' : 'UD_TAB__OPC_VENDOR',  'size' : 3 },
    }

    OpcodeTable0 = {
        'type'      : 'opctbl',
        'entries'   : {},
        'meta'      : 'table0'
    }

    OpcExtIndex = {

        # ssef2, ssef3, sse66
        'sse': {
            'none' : '00', 
            'f2'   : '01', 
            'f3'   : '02', 
            '66'   : '03'
        },

        # /mod=
        'mod': {
            '!11'   : '00', 
            '11'    : '01'
        },

        # /m=, /o=, /a=
        'mode': { 
            '16'    : '00', 
            '32'    : '01', 
            '64'    : '02'
        },

        'vendor' : {
            'amd'   : '00',
            'intel' : '01',
            'any'   : '02'
        }
    }

    InsnTable = []
    MnemonicsTable = []

    ThreeDNowTable = {}

    def sizeOfTable( self, t ): 
        return self.TableInfo[ t ][ 'size' ]

    def nameOfTable( self, t ): 
        return self.TableInfo[ t ][ 'name' ]

    #
    # Updates a table entry: If the entry doesn't exist
    # it will create the entry, otherwise, it will walk
    # while validating the path.
    #
    def updateTable( self, table, index, type, meta ):
        if not index in table[ 'entries' ]:
            table[ 'entries' ][ index ] = { 'type' : type, 'entries' : {}, 'meta' : meta } 
        if table[ 'entries' ][ index ][ 'type' ] != type:
            raise NameError( "error: violation in opcode mapping (overwrite) %s with %s." % 
                                ( table[ 'entries' ][ index ][ 'type' ], type) )
        return table[ 'entries' ][ index ]

    class Insn:
        """An abstract type representing an instruction in the opcode map.
        """

        # A mapping of opcode extensions to their representational
        # values used in the opcode map.
        OpcExtMap = {
            '/rm'    : lambda v: "%02x" % int(v, 16),
            '/x87'   : lambda v: "%02x" % int(v, 16),
            '/3dnow' : lambda v: "%02x" % int(v, 16),
            '/reg'   : lambda v: "%02x" % int(v, 16),
            # modrm.mod
            # (!11, 11)    => (00, 01)
            '/mod'   : lambda v: '00' if v == '!11' else '01',
            # Mode extensions:
            # (16, 32, 64) => (00, 01, 02)
            '/o'     : lambda v: "%02x" % (int(v) / 32),
            '/a'     : lambda v: "%02x" % (int(v) / 32),
            '/m'     : lambda v: "%02x" % (int(v) / 32),
            '/sse'   : lambda v: UdOpcodeTables.OpcExtIndex['sse'][v]
        }

        def __init__(self, prefixes, mnemonic, opcodes, operands, vendor):
            self.opcodes  = opcodes
            self.prefixes = prefixes
            self.mnemonic = mnemonic
            self.operands = operands
            self.vendor   = vendor
            self.opcext   = {}

            ssePrefix = None
            if self.opcodes[0] in ('ssef2', 'ssef3', 'sse66'):
                ssePrefix = self.opcodes[0][3:]
                self.opcodes.pop(0)

            # do some preliminary decoding of the instruction type
            # 1byte, 2byte or 3byte instruction?
            self.nByteInsn = 1
            if self.opcodes[0] == '0f': # 2byte
                # 2+ byte opcodes are always disambiguated by an
                # sse prefix, unless it is a 3d now instruction
                # which is 0f 0f ...
                if self.opcodes[1] != '0f' and ssePrefix is None:
                    ssePrefix = 'none'
                if self.opcodes[1] in ('38', '3a'): # 3byte
                    self.nByteInsn = 3
                else:
                    self.nByteInsn = 2
           
            # The opcode that indexes into the opcode table.
            self.opcode = self.opcodes[self.nByteInsn - 1]
            
            # Record opcode extensions
            for opcode in self.opcodes[self.nByteInsn:]:
                arg, val = opcode.split('=')
                self.opcext[arg] = self.OpcExtMap[arg](val)

            # Record sse extension: the reason sse extension is handled 
            # separately is that historically sse was handled as a first
            # class opcode, not as an extension. Now that sse is handled
            # as an extension, we do the manual conversion here, as opposed
            # to modifying the opcode xml file.
            if ssePrefix is not None:
                self.opcext['/sse'] = self.OpcExtMap['/sse'](ssePrefix)

    def parse(self, table, insn):
        index = insn.opcodes[0];
        if insn.nByteInsn > 1:
            assert index == '0f'
            table = self.updateTable(table, index, 'opctbl', '0f')
            index = insn.opcodes[1]

            if insn.nByteInsn == 3:
                table = self.updateTable(table, index, 'opctbl', index)
                index = insn.opcodes[2]

        # Walk down the tree, create levels as needed, for opcode
        # extensions. The order is important, and determines how
        # well the opcode table is packed. Also note, /sse must be
        # before /o, because /sse may consume operand size prefix
        # affect the outcome of /o.
        for ext in ('/mod', '/x87', '/reg', '/rm', '/sse',
                    '/o',   '/a',   '/m',   '/3dnow'):
            if ext in insn.opcext:
                table = self.updateTable(table, index, ext, ext)
                index = insn.opcext[ext]

        # additional table for disambiguating vendor
        if len(insn.vendor):
            table = self.updateTable(table, index, 'vendor', insn.vendor)
            index = self.OpcExtIndex['vendor'][insn.vendor]

        # make leaf node entries
        leaf = self.updateTable(table, index, 'insn', '')

        leaf['mnemonic'] = insn.mnemonic
        leaf['prefixes'] = insn.prefixes
        leaf['operands'] = insn.operands

        # add instruction to linear table of instruction forms
        self.InsnTable.append({ 'prefixes' : insn.prefixes,  
                                'mnemonic' : insn.mnemonic, 
                                'operands' : insn.operands })

        # add mnemonic to mnemonic table
        if not insn.mnemonic in self.MnemonicsTable:
            self.MnemonicsTable.append(insn.mnemonic)


    # Adds an instruction definition to the opcode tables
    def addInsnDef( self, prefixes, mnemonic, opcodes, operands, vendor ):
        insn = self.Insn(prefixes=prefixes,
                    mnemonic=mnemonic,
                    opcodes=opcodes,
                    operands=operands,
                    vendor=vendor)
        self.parse(self.OpcodeTable0, insn)

    def print_table( self, table, pfxs ):
        print("%s   |" % pfxs)
        keys = table[ 'entries' ].keys()
        if ( len( keys ) ):
            keys.sort()
        for idx in keys:
            e = table[ 'entries' ][ idx ]
            if e[ 'type' ] == 'insn':
                print("%s   |-<%s>" % ( pfxs, idx )),
                print("%s %s" % ( e[ 'mnemonic' ], ' '.join( e[ 'operands'] )))
            else:
                print("%s   |-<%s> %s" % ( pfxs, idx, e['type'] ))
                self.print_table( e, pfxs + '   |' )

    def print_tree( self ): 
        self.print_table( self.OpcodeTable0, '' )
