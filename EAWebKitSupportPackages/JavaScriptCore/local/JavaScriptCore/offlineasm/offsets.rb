# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require "config"
require "ast"

def to32Bit(value)
    if value > 0x7fffffff
        value -= 1 << 32
    end
    value
end

OFFSET_HEADER_MAGIC_NUMBERS = [ to32Bit(0x9e43fd66), to32Bit(0x4379bfba) ]
OFFSET_MAGIC_NUMBERS = [ to32Bit(0xec577ac7), to32Bit(0x0ff5e755) ]

#
# MissingMagicValuesException
#
# Thrown when magic values are missing from the binary.
#

class MissingMagicValuesException < Exception
end

#
# offsetsList(ast)
# sizesList(ast)
#
# Returns a list of offsets and sizes used by the AST.
#

def offsetsList(ast)
    ast.filter(StructOffset).uniq.sort
end

def sizesList(ast)
    ast.filter(Sizeof).uniq.sort
end

#
# offsetsAndConfigurationIndex(ast, file) ->
#     [[offsets, index], ...]
#
# Parses the offsets from a file and returns a list of offsets and the
# index of the configuration that is valid in this build target.
#

def offsetsAndConfigurationIndex(file)

    fileBytes = File.read(file, mode: 'rb');
    
    def improvedFind(byteArray, pattern, startAt)
			positions = [];
			i = startAt;
	    while i = byteArray.index(pattern, i)
	  		positions << i;
	  		i += pattern.length;
			end
			return positions
    end
    
		headerMagicBytesString = [0x66,0xfd,0x43,0x9e,0xba,0xbf,0x79,0x43].pack('C*')
    headerLocations = improvedFind(fileBytes, headerMagicBytesString, 0);
    
    # we can really only have one table in here or else it makes no sense and violates our expectations (are the tables the same? shouldn't we verify?)
    raise MissingMagicValuesException unless headerLocations.length == 1
    
    at = headerLocations[0] + 8; # skip header magic bytes
    
    # look for magic bytes indicating offsets until we no longer find it; that's the end of the data structure
    # assuming little endian for now...
    offsets = []
    while true;
    	a = fileBytes[at+0,4].unpack("L<")[0];
    	b = fileBytes[at+4,4].unpack("L<")[0];
    	if (a != 0xec577ac7) || (b != 0x0ff5e755)
    		break;
    	end
    	# magic bytes found. skip over magic bytes... and..
    	at += 8;
    	# read the offset
    	o = fileBytes[at,4].unpack("L<")[0];
    	#p o;
    	offsets << o;
    	# skip offset to move to next magic bytes (hopefully)
    	at += 4;
    end
    
    # exception if we didn't find the index and at least one other offset
    raise MissingMagicValuesException unless offsets.length > 1;
    
    # we want to return: [[offsets1, index1], [offsets2, index2]].
    ret = [ [offsets[1..-1],offsets[0]] ];
    # p ret
    return ret;
end

#
# buildOffsetsMap(ast, offsetsList) -> [offsets, sizes]
#
# Builds a mapping between StructOffset nodes and their values.
#

def buildOffsetsMap(ast, offsetsList)
    offsetsMap = {}
    sizesMap = {}
    astOffsetsList = offsetsList(ast)
    astSizesList = sizesList(ast)
    raise unless astOffsetsList.size + astSizesList.size == offsetsList.size
    offsetsList(ast).each_with_index {
        | structOffset, index |
        offsetsMap[structOffset] = offsetsList.shift
    }
    sizesList(ast).each_with_index {
        | sizeof, index |
        sizesMap[sizeof] = offsetsList.shift
    }
    [offsetsMap, sizesMap]
end

