/* bitoutput for Deflate format
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
#include "deflate.hpp"

namespace deflate {

void bitoutput::puthuffman (int const n, std::uint32_t const huff)
{
    for (int i = 0; i < n; ++i)
        putbit ((huff >> (n - i - 1)) & 0x01);  /* to LSB */
}

void bitoutput::putdata (int const n, std::uint32_t const data)
{
    for (int i = 0; i < n; ++i)
        putbit ((data >> i) & 0x01);            /* from LSB */
}

void bitoutput::put4byte (std::uint32_t const data)
{
    putbyte (data & 0xffL);             /* little endian */
    putbyte ((data >> 8) & 0xffL);
    putbyte ((data >> 16) & 0xffL);
    putbyte ((data >> 24) & 0xffL);
}

void bitoutput::put2byte (std::uint32_t const data)
{
    putbyte (data & 0xffL);             /* little endian */
    putbyte ((data >> 8) & 0xffL);
}

void bitoutput::putbyte (std::uint32_t const data)
{
    if (bitpos != 0) {
        cout.put (bitbuf);
        bitpos = 0;
        bitbuf = 0;
    }
    cout.put (data);
}

void bitoutput::putbit (std::uint32_t const data)
{
    bitbuf |= (data & 0x01L) << bitpos; /* from LSB to MSB */
    ++bitpos;
    if (bitpos > 7) {
        cout.put (bitbuf);
        bitpos = 0;
        bitbuf = 0;
    }
}

}// namespace deflate

