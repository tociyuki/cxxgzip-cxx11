/* cppgzip - Deflate + gzip (de)compression
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
#ifndef DEFLATE_H
#define DEFLATE_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace deflate {

void gzip ();
void gunzip ();

struct huffman_tree {
    std::shared_ptr<huffman_tree> zero, one;
    std::uint32_t code;
    huffman_tree () : zero (nullptr), one (nullptr), code (0) {}
};

void make_huffman_limitedsize (std::vector<int> const& counts,
    int const nhfsize, int const limit, std::vector<int>& hfsize);
void make_huffman_canonical (std::vector<int> const& hfsize,
    int const limit, std::vector<int>& hfcode);
void make_huffman_tree (std::vector<int> const& hfsize,
    std::vector<int> const& hfcode,
    std::shared_ptr<huffman_tree>& hftree);

class digest_base {
public:
    digest_base () {}
    virtual ~digest_base () {}
    virtual void clear () {}
    virtual std::uint32_t digest () { return 0; }
    virtual void put (int c) {}
};

class digest_crc32 : public digest_base {
public:
    digest_crc32 () : crc (0), buf (), table (), table_ok (false) {}
    void clear ();
    std::uint32_t digest ();
    void put (int c);
private:
    std::uint32_t crc;
    std::vector<int> buf;
    std::vector<std::uint32_t> table;
    bool table_ok;
    void fill_table ();
    void overflow ();
};

class bitoutput {
public:
    bitoutput (std::ostream& acout) : cout (acout), bitbuf (0), bitpos (0) {}
    void puthuffman (int const n, std::uint32_t const huff);
    void putdata (int const n, std::uint32_t const data);
    void put4byte (std::uint32_t const data);
    void put2byte (std::uint32_t const data);
    void putbyte (std::uint32_t const data);
    void putbit (std::uint32_t const data);
private:
    std::ostream& cout;
    std::uint32_t bitbuf;
    int bitpos;
};

class bitinput {
public:
    bitinput (std::istream& acin) : cin (acin), bitbuf (0), bitpos (8) {}
    void gethuffman (int const n,
        std::uint32_t& c, int& bits, std::uint32_t& huff);
    void gethuffman (std::shared_ptr<huffman_tree> tree,
        std::uint32_t& c, int& bits, std::uint32_t& huff);
    void getdata (int const n, std::uint32_t& c);
    void getasciiz (std::string& s);
    std::uint32_t get4byte ();
    std::uint32_t get2byte ();
    std::uint32_t getbyte ();
    std::uint32_t getbit ();
private:
    std::istream& cin;
    std::uint32_t bitbuf;
    int bitpos;
};

class huffman_encoder : public bitoutput {
public:
    huffman_encoder (std::ostream& acout)
        : bitoutput (acout), hclist (), codelist (),
          hccounts (19, 0), litcounts (286, 0), distcounts (30, 0),
          stat_extra (0), stat_lendist (0), stat_fixed (0) {}
    void start_block ();
    void put_literal (int code);
    void put_length_distance (int len, int dist);
    void end_block ();
private:
    enum {LIMIT = 15};
    std::vector<int> hclist;
    std::vector<int> codelist;
    std::vector<int> hccounts;
    std::vector<int> litcounts;
    std::vector<int> distcounts;
    int stat_extra;
    int stat_lendist;
    int stat_fixed;
    void encode_block ();
    void encode_plain_block ();
    void encode_fixed_block ();
    void encode_custom_block (std::vector<int> const& hcsize,
        std::vector<int> const& litsize,
        std::vector<int> const& distsize);
    int estimate_stat_custom (std::vector<int> const& hcsize,
        std::vector<int> const& litsize,
        std::vector<int> const& distsize);
    int estimate_stat_non ();
    void fixed_huffman_code (int code, int& bits, std::uint32_t& huff);
    void encode_length (int const n,
        int& code, int& bits, std::uint32_t& data);
    void encode_distance (int const n,
        int& code, int& bits, std::uint32_t& data);
    void compress_custom_table (std::vector<int> const& litsize,
        std::vector<int> const& distsize);
    void runlength_nonzeros (int c, int n);
    void runlength_zeros (int n);
};

class lzss_compression {
public:
    enum {
        THRESHOLD = 3,
        WINSIZE = 32768,
        DATASIZE = 258,
        BUFSIZE = 65536,
        HASHSIZE = 8192,
        HASHLOG2 = 13
    };
    lzss_compression (std::shared_ptr<digest_base> const& d)
        : buf (BUFSIZE, 0), idx (BUFSIZE, -WINSIZE),
          top (HASHSIZE, -WINSIZE),
          digest (d),
          msize (0) {}
    std::size_t size () const { return msize; }
    void decompress_literal (std::ostream& cout, int const c);
    void decompress_length_distance (std::ostream& cout,
        int const n, int const d);
    int compress (std::istream& cin, huffman_encoder& huffman);
private:
    std::vector<uint8_t> buf;
    std::vector<int> idx;
    std::vector<int> top;
    std::shared_ptr<digest_base> digest;
    int msize;
    void put (int const c);
    int index_3gram (int const cur);
    bool longest_match (int const cur, int& len, int& dist);
};

class huffman_decoder : public bitinput {
public:
    huffman_decoder (std::istream& acin, lzss_compression& alzss)
        : bitinput (acin), lzss (alzss) {}
    std::size_t decode (std::ostream& cout);
    void decode_plain_block (std::ostream& cout);
    void decode_fixed_block (std::ostream& cout);
    void decode_custom_block (std::ostream& cout);
    void decode_custom_block_hctable (std::uint32_t hclen,
        std::shared_ptr<huffman_tree>& hctree);
    void decode_custom_block_table (std::uint32_t hlit, std::uint32_t hdist,
        std::shared_ptr<huffman_tree> hctree,
        std::shared_ptr<huffman_tree>& littree,
        std::shared_ptr<huffman_tree>& disttree);
    void decode_fixed_huffman (std::uint32_t& c,
        int& bits, std::uint32_t& huff);
    void decode_length (std::uint32_t c,
        int& len, int& bits, std::uint32_t& data);
    void decode_distance (std::uint32_t c,
        int& dist, int& bits, std::uint32_t& data);
private:
    lzss_compression& lzss;
};

}// namespace deflate
#endif
