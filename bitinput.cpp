/* bitinput for Deflate format
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1951 DEFLATE Compressed Data Format Specification
 *     version 1.3'', 1996, 3.1.1. Packing into bytes
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

void bitinput::gethuffman (int const n,
    std::uint32_t& c, int& bits, std::uint32_t& huff)
{
    huff = 0;
    for (int i = 0; i < n; ++i)
        huff = (huff << 1) | getbit (); /* to LSB */
    bits = n;
    c = huff;
}

void bitinput::gethuffman (std::shared_ptr<huffman_tree> tree,
    std::uint32_t& c, int& bits, std::uint32_t& huff)
{
    bits = 0;
    huff = 0;
    for (;;) {
        int b = getbit ();
        ++bits;
        huff = (huff << 1) | b;         /* to LSB */
        tree = b == 0 ? tree->zero : tree->one;
        if (tree == nullptr)
            throw std::runtime_error ("huffman_decoder: invalid huffman coding.");
        if (tree->zero == nullptr && tree->one == nullptr)
            break;
    }
    c = tree->code;
}

void bitinput::getdata (int const n, std::uint32_t& c)
{
    c = 0;
    for (int i = 0; i < n; ++i)
        c |= getbit () << i;            /* from LSB */
}

void bitinput::getasciiz (std::string& s)
{
    s.clear ();
    for (;;) {
        std::uint32_t c = getbyte ();
        if (c == 0)
            return;
        s.push_back (c);
    }
}

std::uint32_t bitinput::get4byte ()
{
    return getbyte () | (getbyte () << 8) | (getbyte () << 16) | (getbyte () << 24);
}

std::uint32_t bitinput::get2byte ()
{
    return getbyte () | (getbyte () << 8);
}

std::uint32_t bitinput::getbyte ()
{
    bitpos = 8;
    int c = cin.get ();
    if (c == EOF)
        throw std::runtime_error ("huffman_decoder: unexpected end-of-file.");
    bitbuf = static_cast<std::uint32_t> (c);
    return bitbuf;
}

std::uint32_t bitinput::getbit ()
{
    if (bitpos > 7) {
        getbyte ();
        bitpos = 0;
    }
    return (bitbuf >> (bitpos++)) & 0x01;   /* from LSB to MSB */
}

}// namespace deflate

