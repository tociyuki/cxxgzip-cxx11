cxxgzip - Deflate + gzip (de)compression
========================================

This is a practical implementation of (de)compresssion byte sequences
from stdin to stdout. It only supports Deflate compressed data format
and gzip data format: custom huffman blocks, fixed huffman blocks,
and non-compression blocks with LZSS data compression.

Version
------

0.0.1

Build and Clean
-------------

    $ make
    $ ./cxxgzip < decoder.cpp | ./cxxgzip -d > a.txt
    $ diff decoder.cpp a.txt
    $ make clean

References
--------

 1. P. Deutsch, [RFC 1951 DEFLATE Compressed Data Format Specification version 1.3](https://www.ietf.org/rfc/rfc1951.txt), 1996
 2. P. Deutsch, [RFC 1952 GZIP file format specification version 4.3](https://www.ietf.org/rfc/rfc1952.txt), 1996
 3. [Package-merge algorithm](http://en.wikipedia.org/wiki/Package-merge_algorithm)

License
------

The BSD 3-Clause

Copyright (c) 2015, MIZUTANI Tociyuki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
