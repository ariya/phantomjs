# udis86 - scripts/itab.py
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

from optparse import OptionParser
import os
import sys

sys.path.append( '../scripts' );

import ud_optable
import ud_opcode

class UdItabGenerator( ud_opcode.UdOpcodeTables ):

    OperandDict = {
        "Ap"       : [    "OP_A"        , "SZ_P"     ],
        "E"        : [    "OP_E"        , "SZ_NA"    ],
        "Eb"       : [    "OP_E"        , "SZ_B"     ],
        "Ew"       : [    "OP_E"        , "SZ_W"     ],
        "Ev"       : [    "OP_E"        , "SZ_V"     ],
        "Ed"       : [    "OP_E"        , "SZ_D"     ],
        "Eq"       : [    "OP_E"        , "SZ_Q"     ],
        "Ez"       : [    "OP_E"        , "SZ_Z"     ],
        "Ex"       : [    "OP_E"        , "SZ_MDQ"   ],
        "Ep"       : [    "OP_E"        , "SZ_P"     ],
        "G"        : [    "OP_G"        , "SZ_NA"    ],
        "Gb"       : [    "OP_G"        , "SZ_B"     ],
        "Gw"       : [    "OP_G"        , "SZ_W"     ],
        "Gv"       : [    "OP_G"        , "SZ_V"     ],
        "Gy"       : [    "OP_G"        , "SZ_MDQ"   ],
        "Gy"       : [    "OP_G"        , "SZ_MDQ"   ],
        "Gd"       : [    "OP_G"        , "SZ_D"     ],
        "Gq"       : [    "OP_G"        , "SZ_Q"     ],
        "Gx"       : [    "OP_G"        , "SZ_MDQ"   ],
        "Gz"       : [    "OP_G"        , "SZ_Z"     ],
        "M"        : [    "OP_M"        , "SZ_NA"    ],
        "Mb"       : [    "OP_M"        , "SZ_B"     ],
        "Mw"       : [    "OP_M"        , "SZ_W"     ],
        "Ms"       : [    "OP_M"        , "SZ_W"     ],
        "Md"       : [    "OP_M"        , "SZ_D"     ],
        "Mq"       : [    "OP_M"        , "SZ_Q"     ],
        "Mt"       : [    "OP_M"        , "SZ_T"     ],
        "Mo"       : [    "OP_M"        , "SZ_O"     ],
        "MwRv"     : [    "OP_MR"       , "SZ_WV"    ],
        "MdRy"     : [    "OP_MR"       , "SZ_DY"    ],
        "MbRv"     : [    "OP_MR"       , "SZ_BV"    ],
        "I1"       : [    "OP_I1"       , "SZ_NA"    ],
        "I3"       : [    "OP_I3"       , "SZ_NA"    ],
        "Ib"       : [    "OP_I"        , "SZ_B"     ],
        "Isb"      : [    "OP_I"        , "SZ_SB"    ],
        "Iw"       : [    "OP_I"        , "SZ_W"     ],
        "Iv"       : [    "OP_I"        , "SZ_V"     ],
        "Iz"       : [    "OP_I"        , "SZ_Z"     ],
        "Jv"       : [    "OP_J"        , "SZ_V"     ],
        "Jz"       : [    "OP_J"        , "SZ_Z"     ],
        "Jb"       : [    "OP_J"        , "SZ_B"     ],
        "R"        : [    "OP_R"        , "SZ_RDQ"   ], 
        "C"        : [    "OP_C"        , "SZ_NA"    ],
        "D"        : [    "OP_D"        , "SZ_NA"    ],
        "S"        : [    "OP_S"        , "SZ_NA"    ],
        "Ob"       : [    "OP_O"        , "SZ_B"     ],
        "Ow"       : [    "OP_O"        , "SZ_W"     ],
        "Ov"       : [    "OP_O"        , "SZ_V"     ],
        "V"        : [    "OP_V"        , "SZ_O"     ],
        "W"        : [    "OP_W"        , "SZ_O"     ],
        "Wsd"      : [    "OP_W"        , "SZ_O"     ],
        "Wss"      : [    "OP_W"        , "SZ_O"     ],
        "P"        : [    "OP_P"        , "SZ_Q"     ],
        "Q"        : [    "OP_Q"        , "SZ_Q"     ],
        "VR"       : [    "OP_VR"       , "SZ_O"     ],
        "PR"       : [    "OP_PR"       , "SZ_Q"     ],
        "AL"       : [    "OP_AL"       , "SZ_NA"    ],
        "CL"       : [    "OP_CL"       , "SZ_NA"    ],
        "DL"       : [    "OP_DL"       , "SZ_NA"    ],
        "BL"       : [    "OP_BL"       , "SZ_NA"    ],
        "AH"       : [    "OP_AH"       , "SZ_NA"    ],
        "CH"       : [    "OP_CH"       , "SZ_NA"    ],
        "DH"       : [    "OP_DH"       , "SZ_NA"    ],
        "BH"       : [    "OP_BH"       , "SZ_NA"    ],
        "AX"       : [    "OP_AX"       , "SZ_NA"    ],
        "CX"       : [    "OP_CX"       , "SZ_NA"    ],
        "DX"       : [    "OP_DX"       , "SZ_NA"    ],
        "BX"       : [    "OP_BX"       , "SZ_NA"    ],
        "SI"       : [    "OP_SI"       , "SZ_NA"    ],
        "DI"       : [    "OP_DI"       , "SZ_NA"    ],
        "SP"       : [    "OP_SP"       , "SZ_NA"    ],
        "BP"       : [    "OP_BP"       , "SZ_NA"    ],
        "eAX"      : [    "OP_eAX"      , "SZ_NA"    ],
        "eCX"      : [    "OP_eCX"      , "SZ_NA"    ],
        "eDX"      : [    "OP_eDX"      , "SZ_NA"    ],
        "eBX"      : [    "OP_eBX"      , "SZ_NA"    ],
        "eSI"      : [    "OP_eSI"      , "SZ_NA"    ],
        "eDI"      : [    "OP_eDI"      , "SZ_NA"    ],
        "eSP"      : [    "OP_eSP"      , "SZ_NA"    ],
        "eBP"      : [    "OP_eBP"      , "SZ_NA"    ],
        "rAX"      : [    "OP_rAX"      , "SZ_NA"    ],
        "rCX"      : [    "OP_rCX"      , "SZ_NA"    ],
        "rBX"      : [    "OP_rBX"      , "SZ_NA"    ],
        "rDX"      : [    "OP_rDX"      , "SZ_NA"    ],
        "rSI"      : [    "OP_rSI"      , "SZ_NA"    ],
        "rDI"      : [    "OP_rDI"      , "SZ_NA"    ],
        "rSP"      : [    "OP_rSP"      , "SZ_NA"    ],
        "rBP"      : [    "OP_rBP"      , "SZ_NA"    ],
        "ES"       : [    "OP_ES"       , "SZ_NA"    ],
        "CS"       : [    "OP_CS"       , "SZ_NA"    ],
        "DS"       : [    "OP_DS"       , "SZ_NA"    ],
        "SS"       : [    "OP_SS"       , "SZ_NA"    ],
        "GS"       : [    "OP_GS"       , "SZ_NA"    ],
        "FS"       : [    "OP_FS"       , "SZ_NA"    ],
        "ST0"      : [    "OP_ST0"      , "SZ_NA"    ],
        "ST1"      : [    "OP_ST1"      , "SZ_NA"    ],
        "ST2"      : [    "OP_ST2"      , "SZ_NA"    ],
        "ST3"      : [    "OP_ST3"      , "SZ_NA"    ],
        "ST4"      : [    "OP_ST4"      , "SZ_NA"    ],
        "ST5"      : [    "OP_ST5"      , "SZ_NA"    ],
        "ST6"      : [    "OP_ST6"      , "SZ_NA"    ],
        "ST7"      : [    "OP_ST7"      , "SZ_NA"    ],
        "NONE"     : [    "OP_NONE"     , "SZ_NA"    ],
        "ALr8b"    : [    "OP_ALr8b"    , "SZ_NA"    ],
        "CLr9b"    : [    "OP_CLr9b"    , "SZ_NA"    ],
        "DLr10b"   : [    "OP_DLr10b"   , "SZ_NA"    ],
        "BLr11b"   : [    "OP_BLr11b"   , "SZ_NA"    ],
        "AHr12b"   : [    "OP_AHr12b"   , "SZ_NA"    ],
        "CHr13b"   : [    "OP_CHr13b"   , "SZ_NA"    ],
        "DHr14b"   : [    "OP_DHr14b"   , "SZ_NA"    ],
        "BHr15b"   : [    "OP_BHr15b"   , "SZ_NA"    ],
        "rAXr8"    : [    "OP_rAXr8"    , "SZ_NA"    ],
        "rCXr9"    : [    "OP_rCXr9"    , "SZ_NA"    ],
        "rDXr10"   : [    "OP_rDXr10"   , "SZ_NA"    ],
        "rBXr11"   : [    "OP_rBXr11"   , "SZ_NA"    ],
        "rSPr12"   : [    "OP_rSPr12"   , "SZ_NA"    ],
        "rBPr13"   : [    "OP_rBPr13"   , "SZ_NA"    ],
        "rSIr14"   : [    "OP_rSIr14"   , "SZ_NA"    ],
        "rDIr15"   : [    "OP_rDIr15"   , "SZ_NA"    ],
        "jWP"      : [    "OP_J"        , "SZ_WP"    ],
        "jDP"      : [    "OP_J"        , "SZ_DP"    ],

    }

    #
    # opcode prefix dictionary
    # 
    PrefixDict = { 
        "aso"      : "P_aso",   
        "oso"      : "P_oso",   
        "rexw"     : "P_rexw", 
        "rexb"     : "P_rexb",  
        "rexx"     : "P_rexx",  
        "rexr"     : "P_rexr",
        "seg"      : "P_seg",
        "inv64"    : "P_inv64", 
        "def64"    : "P_def64", 
        "depM"     : "P_depM",
        "cast1"    : "P_c1",    
        "cast2"    : "P_c2",    
        "cast3"    : "P_c3",
        "cast"     : "P_cast",
        "sext"     : "P_sext"
    }

    InvalidEntryIdx = 0 
    InvalidEntry = { 'type'     : 'invalid', 
                     'mnemonic' : 'invalid', 
                     'operands' : '', 
                     'prefixes' : '',
                     'meta'     : '' }

    Itab     = []   # instruction table
    ItabIdx  = 1    # instruction table index
    GtabIdx  = 0    # group table index
    GtabMeta = []

    ItabLookup = {}

    MnemonicAliases = ( "invalid", "3dnow", "none", "db", "pause" )
    
    def __init__( self, outputDir ):
        # first itab entry (0) is Invalid
        self.Itab.append( self.InvalidEntry )
        self.MnemonicsTable.extend( self.MnemonicAliases )
        self.outputDir = outputDir

    def toGroupId( self, id ):
        return 0x8000 | id

    def genLookupTable( self, table, scope = '' ):
        idxArray = [ ]
        ( tabIdx, self.GtabIdx ) = ( self.GtabIdx, self.GtabIdx + 1 )
        self.GtabMeta.append( { 'type' : table[ 'type' ], 'meta' : table[ 'meta' ] } )

        for _idx in range( self.sizeOfTable( table[ 'type' ] ) ):
            idx = "%02x" % _idx 

            e   = self.InvalidEntry
            i   = self.InvalidEntryIdx

            if idx in table[ 'entries' ].keys():
                e = table[ 'entries' ][ idx ]

            # leaf node (insn)
            if e[ 'type' ] == 'insn':
                ( i, self.ItabIdx ) = ( self.ItabIdx, self.ItabIdx + 1 )
                self.Itab.append( e )
            elif e[ 'type' ] != 'invalid':
                i = self.genLookupTable( e, 'static' )

            idxArray.append( i )

        name = "ud_itab__%s" % tabIdx
        self.ItabLookup[ tabIdx ] = name

        self.ItabC.write( "\n" );
        if len( scope ):
            self.ItabC.write( scope + ' ' )
        self.ItabC.write( "const uint16_t %s[] = {\n" % name )
        for i in range( len( idxArray ) ):
            if i > 0 and i % 4 == 0: 
                self.ItabC.write( "\n" )
            if ( i%4 == 0 ):
                self.ItabC.write( "  /* %2x */" % i)
            if idxArray[ i ] >= 0x8000:
                self.ItabC.write( "%12s," % ("GROUP(%d)" % ( ~0x8000 & idxArray[ i ] )))
            else:
                self.ItabC.write( "%12d," % ( idxArray[ i ] ))
        self.ItabC.write( "\n" )
        self.ItabC.write( "};\n" )

        return self.toGroupId( tabIdx )

    def genLookupTableList( self ):
        self.ItabC.write( "\n\n"  );
        self.ItabC.write( "struct ud_lookup_table_list_entry ud_lookup_table_list[] = {\n" )
        for i in range( len( self.GtabMeta ) ):
            f0 = self.ItabLookup[ i ] + ","
            f1 = ( self.nameOfTable( self.GtabMeta[ i ][ 'type' ] ) ) + ","
            f2 = "\"%s\"" % self.GtabMeta[ i ][ 'meta' ]
            self.ItabC.write( "    /* %03d */ { %s %s %s },\n" % ( i, f0, f1, f2 ) )
        self.ItabC.write( "};" )

    def genInsnTable( self ):
        self.ItabC.write( "struct ud_itab_entry ud_itab[] = {\n" );
        idx = 0
        for e in self.Itab:
            opr_c = [ "O_NONE", "O_NONE", "O_NONE" ]
            pfx_c = []
            opr   = e[ 'operands' ]
            for i in range(len(opr)): 
                if not (opr[i] in self.OperandDict.keys()):
                    print("error: invalid operand declaration: %s\n" % opr[i])
                opr_c[i] = "O_" + opr[i]
            opr = "%s %s %s" % (opr_c[0] + ",", opr_c[1] + ",", opr_c[2])

            for p in e['prefixes']:
                if not ( p in self.PrefixDict.keys() ):
                    print("error: invalid prefix specification: %s \n" % pfx)
                pfx_c.append( self.PrefixDict[p] )
            if len(e['prefixes']) == 0:
                pfx_c.append( "P_none" )
            pfx = "|".join( pfx_c )

            self.ItabC.write( "  /* %04d */ { UD_I%s %s, %s },\n" \
                        % ( idx, e[ 'mnemonic' ] + ',', opr, pfx ) )
            idx += 1
        self.ItabC.write( "};\n" )

        self.ItabC.write( "\n\n"  );
        self.ItabC.write( "const char * ud_mnemonics_str[] = {\n" )
        self.ItabC.write( ",\n    ".join( [ "\"%s\"" % m for m in self.MnemonicsTable ] ) )
        self.ItabC.write( "\n};\n" )
 

    def genItabH( self ):
        self.ItabH = open( os.path.join(self.outputDir, "udis86_itab.h"), "w" )

        # Generate Table Type Enumeration
        self.ItabH.write( "#ifndef UD_ITAB_H\n" )
        self.ItabH.write( "#define UD_ITAB_H\n\n" )

        # table type enumeration
        self.ItabH.write( "/* ud_table_type -- lookup table types (see lookup.c) */\n" )
        self.ItabH.write( "enum ud_table_type {\n    " )
        enum = [ self.TableInfo[ k ][ 'name' ] for k in self.TableInfo.keys() ]
        self.ItabH.write( ",\n    ".join( enum ) )
        self.ItabH.write( "\n};\n\n" );

        # mnemonic enumeration
        self.ItabH.write( "/* ud_mnemonic -- mnemonic constants */\n" )
        enum  = "enum ud_mnemonic_code {\n    "
        enum += ",\n    ".join( [ "UD_I%s" % m for m in self.MnemonicsTable ] )
        enum += "\n} UD_ATTR_PACKED;\n"
        self.ItabH.write( enum )
        self.ItabH.write( "\n" )

        self.ItabH.write("\n/* itab entry operand definitions */\n");
        operands = self.OperandDict.keys()
        operands.sort()
        for o in operands:
            self.ItabH.write("#define O_%-7s { %-12s %-8s }\n" %
                    (o, self.OperandDict[o][0] + ",", self.OperandDict[o][1]));
        self.ItabH.write("\n\n");

        self.ItabH.write( "extern const char * ud_mnemonics_str[];\n" )

        self.ItabH.write( "#define GROUP(n) (0x8000 | (n))" )

        self.ItabH.write( "\n#endif /* UD_ITAB_H */\n" )
    
        self.ItabH.close()


    def genItabC( self ):
        self.ItabC = open( os.path.join(self.outputDir, "udis86_itab.c"), "w" )
        self.ItabC.write( "/* itab.c -- generated by itab.py, do no edit" )
        self.ItabC.write( " */\n" );
        self.ItabC.write( "#include \"udis86_decode.h\"\n\n" );

        self.genLookupTable( self.OpcodeTable0 ) 
        self.genLookupTableList()
        self.genInsnTable()

        self.ItabC.close()

    def genItab( self ):
        self.genItabC()
        self.genItabH()

def main():
    parser = OptionParser()
    parser.add_option("--outputDir", dest="outputDir", default="")
    options, args = parser.parse_args()
    generator = UdItabGenerator(os.path.normpath(options.outputDir))
    optableXmlParser = ud_optable.UdOptableXmlParser()
    optableXmlParser.parse( args[ 0 ], generator.addInsnDef )

    generator.genItab()

if __name__ == '__main__':
    main()
