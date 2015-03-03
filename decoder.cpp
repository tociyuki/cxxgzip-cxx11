/* Huffman block decoder for Deflate format
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1951 DEFLATE Compressed Data Format Specification
 *              version 1.3'', 1996, 3.2.3. Details of block format
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
#include <algorithm>
#include <stdexcept>
#include "deflate.hpp"

namespace deflate {

/* 3.2.3. Details of block format */
std::size_t huffman_decoder::decode (std::ostream& cout)
{
    for (;;) {
        std::uint32_t fin, typ;
        getdata (1, fin);
        getdata (2, typ);
        if (typ == 0)
            decode_plain_block (cout);
        else if (typ == 1)
            decode_fixed_block (cout);
        else if (typ == 2)
            decode_custom_block (cout);
        else
            throw std::runtime_error ("huffman_decoder: invalid block TYP.");
        if (fin == 1)
            break;
    }
    return lzss.size ();
}

/* 3.2.4. Non-compressed blocks (BTYPE=00) */
void huffman_decoder::decode_plain_block (std::ostream& cout)
{
    std::uint32_t len = get2byte ();
    std::uint32_t nlen = get2byte ();
    if (len != (nlen ^ 0xffff))
        throw std::runtime_error ("huffman_decoder: invalid non-compress block.");
    for (std::uint32_t i = 0; i < len; ++i) {
        int c = getbyte ();
        lzss.decompress_literal (cout, c);
    }
}

/* 3.2.6. Compression with fixed Huffman codes (BTYPE=01) */
void huffman_decoder::decode_fixed_block (std::ostream& cout)
{
    for (;;) {
        std::uint32_t c, huff;
        int bits;
        decode_fixed_huffman (c, bits, huff);
        if (c < 256)
            lzss.decompress_literal (cout, c);
        else if (c > 256) {
            int n, lebits, d, c1bits, debits;
            std::uint32_t c1, c1huff, lext, dext;
            decode_length (c, n, lebits, lext);
            /* Distance codes are represented by (fixed-length) 5-bit codes */
            gethuffman (5, c1, c1bits, c1huff);
            decode_distance (c1, d, debits, dext);
            lzss.decompress_length_distance (cout, n, d);
        }
        else if (c == 256)
            break;
    }
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_decoder::decode_custom_block (std::ostream& cout)
{
    std::shared_ptr<huffman_tree> hctree;
    std::shared_ptr<huffman_tree> littree;
    std::shared_ptr<huffman_tree> disttree;
    std::uint32_t hlit, hdist, hclen;
    getdata (5, hlit);
    getdata (5, hdist);
    getdata (4, hclen);
    decode_custom_block_hctable (hclen, hctree);
    decode_custom_block_table (hlit, hdist, hctree, littree, disttree);
    /* The actual compressed data of the block, */
    for (;;) {
        std::uint32_t c, huff;
        int bits;
        gethuffman (littree, c, bits, huff);
        if (c < 256)
            lzss.decompress_literal (cout, c);
        else if (c > 256) {
            int n, lebits, d, c1bits, debits;
            std::uint32_t c1, c1huff, lext, dext;
            decode_length (c, n, lebits, lext);
            gethuffman (disttree, c1, c1bits, c1huff);
            decode_distance (c1, d, debits, dext);
            lzss.decompress_length_distance (cout, n, d);
        }
        else if (c == 256)
            break;
    }
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_decoder::decode_custom_block_hctable (
    std::uint32_t hclen, std::shared_ptr<huffman_tree>& hctree)
{
    std::vector<int> hcindex{
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::vector<int> hcsize (19, 0);
    std::vector<int> hccode;
    /* (HCLEN + 4) x 3 bits: code lengths for the code length */
    for (std::uint32_t i = 0; i < hclen + 4; ++i) {
        std::uint32_t c;
        getdata (3, c);
        hcsize[hcindex[i]] = c;
    }
    make_huffman_canonical (hcsize, 7, hccode);
    make_huffman_tree (hcsize, hccode, hctree);
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_decoder::decode_custom_block_table (
    std::uint32_t hlit, std::uint32_t hdist,
    std::shared_ptr<huffman_tree> hctree,
    std::shared_ptr<huffman_tree>& littree,
    std::shared_ptr<huffman_tree>& disttree)
{
    /* HLIT + 257 code lengths for the literal/length alphabet,
     *            encoded using the code length Huffman code
     * HDIST + 1  code lengths for the distance alphabet,
     *            encoded using the code length Huffman code
     */
    std::uint32_t n = hlit + 257 + hdist + 1;
    std::vector<int> a;
    for (;;) {
        std::uint32_t c, huff, m;
        int bits, d;
        gethuffman (hctree, c, bits, huff);
        if (c < 16)
            a.push_back (c);
        else if (c == 16) {
            getdata (2, m);
            d = a.back ();
            for (std::uint32_t i = 0; i < m + 3; ++i)
                a.push_back (d);
        }
        else if (c == 17) {
            getdata (3, m);
            for (std::uint32_t i = 0; i < m + 3; ++i)
                a.push_back (0);
        }
        else if (c == 18) {
            getdata (7, m);
            for (std::uint32_t i = 0; i < m + 11; ++i)
                a.push_back (0);
        }
        if (a.size () >= n)
            break;
    }
    std::vector<int> litsize (a.begin (), a.begin () + hlit + 257);
    std::vector<int> litcode;
    std::vector<int> distsize (a.begin () + hlit + 257, a.end ());
    std::vector<int> distcode;
    auto litit = std::max_element (litsize.begin (), litsize.end ());
    make_huffman_canonical (litsize, *litit, litcode);
    make_huffman_tree (litsize, litcode, littree);
    auto distit = std::max_element (distsize.begin (), distsize.end ());
    make_huffman_canonical (distsize, *distit, distcode);
    make_huffman_tree (distsize, distcode, disttree);
}

/* 3.2.6. Compression with fixed Huffman codes (BTYPE=01) */
void huffman_decoder::decode_fixed_huffman (
    std::uint32_t& c, int& bits, std::uint32_t& huff)
{
    gethuffman (7, c, bits, huff);
    if (huff <= 0x0b)
        c = huff + 256;
    else if (huff <= 0x5f) {
        bits = 8;
        huff = (huff << 1) | getbit ();
        c = huff - 48;
    }
    else if (huff <= 0x63) {
        bits = 8;
        huff = (huff << 1) | getbit ();
        c = huff + 88;
    }
    else {
        bits = 9;
        huff = (huff << 1) | getbit ();
        huff = (huff << 1) | getbit ();
        c = huff - 256;
    }
}

/* 3.2.5. Compressed blocks (length and distance codes) */
void huffman_decoder::decode_length (
    std::uint32_t c, int& len, int& bits, std::uint32_t& data)
{
    bits = data = 0;
    if (c <= 264)
        len = c + 3 - 257;
    else if (c <= 284) {
        bits = (c - 265) / 4 + 1;
        getdata (bits, data);
        len = (1 << (bits + 2)) + (((c - 265) & 3) << bits) + data + 3;
    }
    else if (c == 285)
        len = 258;
    else
        throw std::runtime_error ("huffman_decoder: invalid length coding.");
}

/* 3.2.5. Compressed blocks (length and distance codes) */
void huffman_decoder::decode_distance (
    std::uint32_t c, int& dist, int& bits, std::uint32_t& data)
{
    bits = data = 0;
    if (c <= 3)
        dist = c + 1;
    else if (c <= 29) {
        bits = c / 2 - 1;
        getdata (bits, data);
        dist = (1 << (bits + 1)) + ((c & 1) << bits) + data + 1;
    }
    else
        throw std::runtime_error ("huffman_decoder: invalid distance coding.");
}

}// namespace deflate

