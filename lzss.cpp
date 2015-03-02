/* LZSS compression/decompression for Deflate format
 *
 *  1. a chained hash table to find duplicated strings.
 *  2. Knuth's multiplicative hashing that operates on 3-Ngram.
 *  3. a ring buffer to slide a window of strings.
 *  4. a ring buffer to slide a window of chains of hash table.
 *  5. one byte after lazy matching.
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1951 DEFLATE Compressed Data Format Specification
 *     version 1.3'', 1996, 4. Compression algorithm details
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

void lzss_compression::decompress_literal (std::ostream& cout, int const c)
{
    cout.put (c);
    put (c);            /* put byte into the ring buffer */
}

void lzss_compression::decompress_length_distance (
    std::ostream& cout, int const n, int const d)
{
    int i = msize - d;
    for (int j = 0; j < n; ++j) {
        int c = buf[i % BUFSIZE];
        ++i;
        cout.put (c);
        put (c);        /* put it into the ring buffer */
    }
}

int lzss_compression::compress (
    std::istream& cin, huffman_encoder& huffman)
{
    int c, len, dist;

    /* longest_match ignores WINSIZEs bytes far strings. */
    std::fill (top.begin (), top.end (), -WINSIZE);
    std::fill (idx.begin (), idx.end (), -WINSIZE);
    /* fill the ring buffer for first longest_match and lazy it. */
    msize = 0;
    for (int i = 0; i < DATASIZE + 1; ++i) {
        c = cin.get ();
        if (c == EOF)
            break;
        put (c);
    }
    huffman.start_block ();
    for (int cur = 0; cur < msize;) {
        int lenlazy, distlazy;
        bool m = longest_match (cur, len, dist);
        bool mlazy = false;
        if (m)
            mlazy = longest_match (cur + 1, lenlazy, distlazy);
        int match_offset = 2;
        if (! m || (mlazy && len < lenlazy)) {
            /* output one byte and push new one. */
            c = buf[cur % BUFSIZE];
            ++cur;
            huffman.put_literal (c);
            c = cin.get ();
            if (c != EOF)
                put (c);
            if (! m)
                continue;
            /* use lazy matching strings */
            m = mlazy;
            len = lenlazy;
            dist = distlazy;
            match_offset = 1;
        }
        huffman.put_length_distance (len, dist);
        for (int i = match_offset; i < len; ++i)
            index_3gram (cur + i);
        for (int i = 0; i < len; ++i) {
            c = cin.get ();
            if (c == EOF)
                break;
            put (c);
        }
        cur += len;
    }
    huffman.end_block ();
    return msize;
}

void lzss_compression::put (int const c)
{
    buf[msize % BUFSIZE] = c;
    ++msize;
    digest->put (c);    /* CRC32 or Adler-32 */
}

int lzss_compression::index_3gram (int const cur)
{
    /* D. Knuth, `The Art of Computer Programming Vol.3' 1998, page 516-519
     *    6.4 multiplicative hashing
     *
     * HASHFRAC is modified for 3 bytes integer from the value of
     *
     *      floor (((sqrt(5.0) - 1.0) / 2.0) * (1 << 24))
     *
     * under the discussions at page 518-519.
     *
     *      4/7 < static_cast<double>(0x9e416d) / (1 << 24) < 2/3,
     *      1/4 <   static_cast<double>(0x416d) / (1 << 16) < 3/10,
     *      1/3 <     static_cast<double>(0x6d) / (1 << 8)  < 3/7
     */
    const static std::uint32_t HASHFRAC = 0x009e416dL;
    if (cur + 3 >= msize)
        return -WINSIZE;
    uint32_t const k
        = (static_cast<uint32_t> (buf[ cur      % BUFSIZE]) << 16)
        | (static_cast<uint32_t> (buf[(cur + 1) % BUFSIZE]) << 8)
        |  static_cast<uint32_t> (buf[(cur + 2) % BUFSIZE]);
    uint32_t const h = ((k * HASHFRAC) & 0x00ffffffL) >> (24 - HASHLOG2);
    /* push location into the chain of the hash table */
    int prev = top[h];
    idx[cur % BUFSIZE] = prev;
    top[h] = cur;
    return prev;
}

bool lzss_compression::longest_match (int const cur, int& len, int& dist)
{
    if (cur + THRESHOLD >= msize)
        return false;
    int longest_pos = cur;
    int longest_size = 0;
    /* search sub-strings in the location chains of the hash table */
    int pos = index_3gram (cur);
    while (cur - pos < WINSIZE) {
        int n = 0;
        for (; n < DATASIZE && cur + n < msize; ++n)
            if (buf[(pos + n) % BUFSIZE] != buf[(cur + n) % BUFSIZE])
                break;
        if (n >= THRESHOLD && n > longest_size) {
            longest_pos = pos;
            longest_size = n;
        }
        pos = idx[pos % BUFSIZE];
    }
    len = longest_size;
    dist = cur - longest_pos;
    return len > 0;
}

}// namespace deflate

