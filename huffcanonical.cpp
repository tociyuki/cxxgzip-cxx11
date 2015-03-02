/* Huffman coding for Deflate format
 *
 * References:
 *
 *  P. Deutsch, ``RFC 1951 DEFLATE Compressed Data Format Specification
 *     version 1.3'', 1996, 3.2.2. Use of Huffman coding in the "deflate" format
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

void make_huffman_canonical (std::vector<int> const& hfsize,
    int const limit, std::vector<int>& hfcode)
{
    std::vector<int> blcount (limit + 1, 0);
    std::vector<int> nextcode (limit + 1, 0);
    for (int n : hfsize)
        if (n > 0)
            ++blcount[n];
    int code = 0;
    blcount[0] = 0;
    for (int bits = 1; bits <= limit; ++bits)
        nextcode[bits] = code = (code + blcount[bits - 1]) << 1;
    hfcode.clear ();
    for (int n : hfsize)
        hfcode.push_back (n == 0 ? 0 : nextcode[n]++);
}

}// namespace deflate

