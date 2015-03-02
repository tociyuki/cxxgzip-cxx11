/* Length-limited Huffman coding for Deflate format
 *
 * References:
 *
 *   http://en.wikipedia.org/wiki/Package-merge_algorithm
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
#include "deflate.hpp"

namespace deflate {

class coin_base {
public:
    coin_base (int afreq) : mfreq (afreq) {}
    virtual ~coin_base () {}
    virtual void accumulate (std::vector<int>& hfsize) {}
    virtual int code () const { return 0; }
    int freq () const { return mfreq; }
private:
    int mfreq;
};

class coin : public coin_base {
public:
    coin (int afreq, int acode) : coin_base (afreq), mcode (acode) {}
    virtual int code () const { return mcode; }
    virtual void accumulate (std::vector<int>& hfsize) { ++hfsize[mcode]; }
private:
    int mcode;
};

typedef std::shared_ptr<coin_base> coin_ptr;

class pair_coin : public coin_base {
public:
    pair_coin (coin_ptr const& c0, coin_ptr const& c1)
        : coin_base (c0->freq () + c1->freq ()), first (c0), second (c1) {}

    virtual void accumulate (std::vector<int>& hfsize)
    {
        first->accumulate (hfsize);
        second->accumulate (hfsize);
    }

private:
    coin_ptr const first;
    coin_ptr const second;
};

void make_huffman_limitedsize (std::vector<int> const& counts,
    int const nhfsize, int const limit, std::vector<int>& hfsize)
{
    struct {
        bool operator() (coin_ptr a, coin_ptr b)
        {
            return a->freq () < b->freq ();
        }
    } coinptr_less;

    hfsize.clear ();
    hfsize.resize (nhfsize, 0);
    std::vector<coin_ptr> freq;
    for (std::size_t i = 0; i < counts.size (); ++i)
        if (counts[i] > 0)
            freq.push_back (std::make_shared<coin> (counts[i], i));
    if (freq.size () == 1)
        hfsize[freq[0]->code ()] = 1;
    else if (freq.size () > 1) {
        std::sort (freq.begin (), freq.end (), coinptr_less);
        std::vector<coin_ptr> coins (freq.begin (), freq.end ());
        for (int i = limit - 1; i >= 0; --i) {
            std::vector<coin_ptr> pairs;
            for (int j = 0; j < coins.size (); j += 2)
                if (j + 1 < coins.size ())
                    pairs.push_back (
                        std::make_shared<pair_coin> (coins[j], coins[j + 1]));
            if (i == 0)
                coins.assign (pairs.begin (), pairs.end ());
            else {
                coins.resize (freq.size () + pairs.size (), nullptr);
                std::merge (freq.begin (), freq.end (),
                            pairs.begin (), pairs.end (),
                            coins.begin (), coinptr_less);
            }
        }
        for (int i = 0; i < freq.size () - 1; ++i)
            coins[i]->accumulate (hfsize);
    }
}

}// namespace deflate

