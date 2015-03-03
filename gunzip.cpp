/* decompression from the gzip format
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1952 GZIP file format specification version 4.3'', 1996
 *
 * License: The BSD 3-Clause
 *
 * Copyright (c) 2015, MIZUTANI Tociyuki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdexcept>
#include "deflate.hpp"

namespace deflate {

void gunzip ()
{
    std::string s;
    auto crc32 = std::make_shared<digest_crc32> ();
    lzss_compression lzss (crc32);
    huffman_decoder decoder (std::cin, lzss);
    std::uint32_t id1 = decoder.getbyte ();
    std::uint32_t id2 = decoder.getbyte ();
    if (id1 != 0x1f || id2 != 0x8b)
        throw std::runtime_error ("cppgzip: illegal id1 and id2.");
    std::uint32_t cm = decoder.getbyte ();
    if (cm != 8)
        throw std::runtime_error ("cppgzip: illegal cm.");
    std::uint32_t flg = decoder.getbyte ();
    /* std::uint32_t mtime = */ decoder.get4byte ();
    /* std::uint32_t xfl = */   decoder.getbyte ();
    /* std::uint32_t os = */    decoder.getbyte ();
    if (flg & 4) {
        std::uint32_t xlen = decoder.get2byte ();
        for (std::uint32_t i = 0; i < xlen; ++i)
            decoder.getbyte ();
    }
    if (flg & 8)
        decoder.getasciiz (s);
    if (flg & 16)
        decoder.getasciiz (s);
    if (flg & 2)
        decoder.get2byte ();
    std::size_t got_isize = decoder.decode (std::cout);
    std::uint32_t expected_crc32 = decoder.get4byte ();
    std::uint32_t expected_isize = decoder.get4byte ();
    if (crc32->digest () != expected_crc32)
        throw std::runtime_error ("cppgzip: mismatch CRC32.");
    if (got_isize != expected_isize)
        throw std::runtime_error ("cppgzip: mismatch ISIZE.");
}

}// namespace deflate
