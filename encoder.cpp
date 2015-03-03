/* Huffman block decoder for Deflate format
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1951 DEFLATE Compressed Data Format Specification
 *     version 1.3'', 1996, 3.2.3. Details of block format
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
#include "deflate.hpp"

namespace deflate {

void huffman_encoder::start_block ()
{
    hclist.clear ();
    codelist.clear ();
    std::fill (hccounts.begin (), hccounts.end (), 0);
    std::fill (litcounts.begin (), litcounts.end (), 0);
    std::fill (distcounts.begin (), distcounts.end (), 0);
    stat_extra = stat_lendist = stat_fixed = 0;
}

void huffman_encoder::put_literal (int code)
{
    int bits;
    std::uint32_t huff;
    /* code in 0 .. 255 */
    codelist.push_back (code);
    /* update statistics */
    /* for Huffman coding */
    ++litcounts[code];
    /* for block type selection */
    fixed_huffman_code (code, bits, huff);
    stat_fixed += bits;
}

void huffman_encoder::put_length_distance (int len, int dist)
{
    int lencode, lenbits, lexbits, distcode, dexbits;
    std::uint32_t lenhuff, lextra, dextra;
    /* 257 is temporal alphabet for <length, distance> */
    codelist.push_back (257);
    codelist.push_back (len);
    codelist.push_back (dist);
    /* update statistics */
    encode_length (len, lencode, lexbits, lextra);
    encode_distance (dist, distcode, dexbits, dextra);
    /* for Huffman coding */
    ++litcounts[lencode];       
    ++distcounts[distcode];
    /* for block type selection */
    fixed_huffman_code (lencode, lenbits, lenhuff);
    stat_fixed += lenbits + lexbits + 5 + dexbits;  
    stat_extra += lexbits + dexbits;
    ++stat_lendist;
}

void huffman_encoder::end_block ()
{
    /* the value 256 indicates end-of-block */
    codelist.push_back (256);
    ++litcounts[256];
    stat_fixed += 8;
    encode_block ();
}

void huffman_encoder::encode_block ()
{
    std::vector<int> hcsize;
    std::vector<int> litsize;
    std::vector<int> distsize;
    if (codelist.size () == 1) {
        encode_fixed_block ();      /* empty block */
        return;
    }
    make_huffman_limitedsize (litcounts, litcounts.size (), LIMIT, litsize);
    make_huffman_limitedsize (distcounts, distcounts.size (), LIMIT, distsize);
    compress_custom_table (litsize, distsize);
    make_huffman_limitedsize (hccounts, hccounts.size (), 7, hcsize);
    /* estimate bit length for each three type of blocks */
    int stat_custom = estimate_stat_custom (hcsize, litsize, distsize);
    int stat_non = std::max (stat_custom, stat_fixed) + 8;
    if (stat_lendist == 0)
        stat_non = estimate_stat_non ();
    /* select shortest blocks type */
    int stat_min = std::min (std::min (stat_custom, stat_fixed), stat_non);
    if (stat_custom == stat_min)
        encode_custom_block (hcsize, litsize, distsize);
    else if (stat_fixed == stat_min)
        encode_fixed_block ();
    else
        encode_plain_block ();
}

/* 3.2.4. Non-compressed blocks (BTYPE=00) */
void huffman_encoder::encode_plain_block ()
{
    codelist.pop_back ();
    int len = codelist.size ();
    auto p = codelist.begin ();
    /* 2. Compressed representation overview
     * non-compressible blocks are limited to 65,535 bytes
     */
    while (len > 65535) {
        int n = 65535;
        putbit (0);
        putdata (2, 0);
        put2byte (n);
        put2byte (n ^ 0x0000ffffL);
        for (int i = 0; i < n; ++i)
            putbyte (*p++);
        len -= n;
    }
    putbit (1);
    putdata (2, 0);
    put2byte (len);
    put2byte (len ^ 0x0000ffffL);
    for (int i = 0; i < len; ++i)
        putbyte (*p++);
}

/*  3.2.6. Compression with fixed Huffman codes (BTYPE=01) */
void huffman_encoder::encode_fixed_block ()
{
    putbit (1);
    putdata (2, 1);
    for (std::size_t i = 0; i < codelist.size (); ++i) {
        int c = codelist[i];
        if (c <= 256) {
            int bits;
            std::uint32_t huff;
            fixed_huffman_code (c, bits, huff);
            puthuffman (bits, huff);
        }
        else {
            int lencode, lenbits, lexbits, distcode, dexbits;
            std::uint32_t lenhuff, lextra, dextra;
            int len = codelist[++i];
            int dist = codelist[++i];
            encode_length (len, lencode, lexbits, lextra);
            encode_distance (dist, distcode, dexbits, dextra);
            fixed_huffman_code (lencode, lenbits, lenhuff);
            puthuffman (lenbits, lenhuff);
            if (lexbits > 0)
                putdata (lexbits, lextra);
            puthuffman (5, distcode);
            if (dexbits > 0)
                putdata (dexbits, dextra);
        }
    }
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_encoder::encode_custom_block (
    std::vector<int> const& hcsize,
    std::vector<int> const& litsize,
    std::vector<int> const& distsize)
{
    std::vector<int> hcindex{
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::vector<int> hchuff;
    std::vector<int> lithuff;
    std::vector<int> disthuff;
    make_huffman_canonical (hcsize, LIMIT, hchuff);
    make_huffman_canonical (litsize, LIMIT, lithuff);
    make_huffman_canonical (distsize, LIMIT, disthuff);
    putbit (1);
    putdata (2, 2);
    putdata (5, litcounts.size () - 257);
    putdata (5, distcounts.size () - 1);
    putdata (4, 19 - 4);
    /* (HCLEN + 4) x 3 bits: code lengths for the code length alphabet*/
    for (int i : hcindex)
        putdata (3, hcsize[i]);
    /* HLIT + 257 code lengths for the literal/length alphabet,
     * encoded using the code length Huffman code
     * HDIST + 1 code lengths for the distance alphabet,
     * encoded using the code length Huffman code
     */
    for (std::size_t i = 0; i < hclist.size (); ++i) {
        int c = hclist[i];
        puthuffman (hcsize[c], hchuff[c]);
        if (c == 16)
            putdata (2, hclist[++i]);
        else if (c == 17)
            putdata (3, hclist[++i]);
        else if (c == 18)
            putdata (7, hclist[++i]);
    }
    /* The actual compressed data of the block,
     * The literal/length symbol 256 (end of data)
     */
    for (std::size_t i = 0; i < codelist.size (); ++i) {
        int c = codelist[i];
        if (c <= 256)
            puthuffman (litsize[c], lithuff[c]);
        else {
            int lencode, lexbits, distcode, dexbits;
            std::uint32_t lextra, dextra;
            int len = codelist[++i];
            int dist = codelist[++i];
            encode_length (len, lencode, lexbits, lextra);
            encode_distance (dist, distcode, dexbits, dextra);
            puthuffman (litsize[lencode], lithuff[lencode]);
            if (lexbits > 0)
                putdata (lexbits, lextra);
            puthuffman (distsize[distcode], disthuff[distcode]);
            if (dexbits > 0)
                putdata (dexbits, dextra);
        }
    }
}

int huffman_encoder::estimate_stat_custom (
    std::vector<int> const& hcsize,
    std::vector<int> const& litsize,
    std::vector<int> const& distsize)
{
    int n = 5 + 5 + 4 + stat_extra;
    n += hccounts.size () * 3;
    for (std::size_t i = 0; i < hccounts.size (); ++i) {
        n += hccounts[i] * hcsize[i];
    }
    for (std::size_t i = 0; i < litcounts.size (); ++i) {
        n += litcounts[i] * litsize[i];
    }
    for (std::size_t i = 0; i < distcounts.size (); ++i) {
        n += distcounts[i] * distsize[i];
    }
    return n;
}

int huffman_encoder::estimate_stat_non ()
{
    int m = (codelist.size () - 1) / 65535;
    int n = (codelist.size () - 1) % 65535;
    return m * (65535 * 8 + 32) + n * 8 + 32;
}

/*  3.2.6. Compression with fixed Huffman codes (BTYPE=01) */
void huffman_encoder::fixed_huffman_code (
    int code, int& bits, std::uint32_t& huff)
{
    if (code <= 143) {
        bits = 8;
        huff = code + 0x0030L;
    }
    else if (code <= 255) {
        bits = 9;
        huff = code - 144 + 0x0190L;
    }
    else if (code <= 279) {
        bits = 7;
        huff = code - 256;
    }
    else {
        bits = 8;
        huff = code - 280 + 0x00c0L;
    }
}

/* 3.2.5. Compressed blocks (length and distance codes) */
void huffman_encoder::encode_length (int const n,
    int& code, int& bits, std::uint32_t& data)
{
    bits = data = 0;
    if (n <= 10)
        code = n + 254;
    else if (n == 258)
        code = 285;
    else {
        int i = n - 3;
        bits = i <= 15 ? 1 : i <= 31 ? 2 : i <= 63 ? 3 : i <= 127 ? 4 : 5;
        code = bits * 4 + (i >> bits) + 257;
        data = i & ((1 << bits) - 1);
    }
}

/* 3.2.5. Compressed blocks (length and distance codes) */
void huffman_encoder::encode_distance (
    int const n, int& code, int& bits, std::uint32_t& data)
{
    bits = data = 0;
    if (n <= 4)
        code = n - 1;
    else {
        int i = n - 1;
        bits = i <= 7 ? 1 : i <= 15 ? 2 : i <= 31 ? 3 : i <= 63 ? 4
             : i <= 127 ? 5 : i <= 255 ? 6 : i <= 511 ? 7
             : i <= 1023 ? 8 : i <= 2047 ? 9 : i <= 4095 ? 10
             : i <= 8191 ? 11 : i <= 16383 ? 12 : 13;
        code = bits * 2 + (i >> bits);
        data = i & ((1 << bits) - 1);
    }
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_encoder::compress_custom_table (
    std::vector<int> const& litsize,
    std::vector<int> const& distsize)
{
    std::vector<int> code;
    std::vector<int> runlength;
    /* code lengths for the literal/length alphabet */
    for (int c : litsize)
        if (! code.empty () && code.back () == c)
            ++runlength[runlength.size () - 1];
        else {
            code.push_back (c);
            runlength.push_back (1);
        }
    /* code lengths for the distance alphabet */
    for (int c : distsize)
        if (! code.empty () && code.back () == c)
            ++runlength[runlength.size () - 1];
        else {
            code.push_back (c);
            runlength.push_back (1);
        }
    /* encoded using the code length Huffman code */
    for (std::size_t i = 0; i < code.size (); ++i)
        if (code[i] == 0)
            runlength_zeros (runlength[i]);
        else
            runlength_nonzeros (code[i], runlength[i]);
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_encoder::runlength_nonzeros (int c, int n)
{
    hclist.push_back (c);
    ++hccounts[c];
    --n;
    for (; n > 6; n -= 6) {
        hclist.push_back (16);
        hclist.push_back (3);
        ++hccounts[16];
        stat_extra += 2;
    }
    if (n >= 3) {
        hclist.push_back (16);
        hclist.push_back (n - 3);
        ++hccounts[16];
        stat_extra += 2;
    }
    else if (n > 0) {
        for (int i = 0; i < n; ++i)
            hclist.push_back (c);
        hccounts[c] += n;
    }
}

/* 3.2.7. Compression with dynamic Huffman codes (BTYPE=10) */
void huffman_encoder::runlength_zeros (int n)
{
    for (; n > 138; n -= 138) {
        hclist.push_back (18);
        hclist.push_back (127);
        ++hccounts[18];
        stat_extra += 7;
    }
    if (n >= 11) {
        hclist.push_back (18);
        hclist.push_back (n - 11);
        ++hccounts[18];
        stat_extra += 7;
    }
    else if (n >= 3) {
        hclist.push_back (17);
        hclist.push_back (n - 3);
        ++hccounts[17];
        stat_extra += 3;
    }
    else if (n > 0) {
        for (int i = 0; i < n; ++i)
            hclist.push_back (0);
        hccounts[0] += n;
    }
}

}// namespace deflate

