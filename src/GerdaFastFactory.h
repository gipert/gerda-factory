// MIT License
//
// Copyright (c) 2021 Luigi Pertoldi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef _GERDA_FAST_FACTORY_H
#define _GERDA_FAST_FACTORY_H

#include <vector>
#include <memory>

#include "TH1.h"
#include "TRandom3.h"

class GerdaFastFactory {

    public:

    // delete dangerous constructors
    GerdaFastFactory           (GerdaFastFactory const&) = delete;
    GerdaFastFactory& operator=(GerdaFastFactory const&) = delete;

    // custom constructor
    GerdaFastFactory();
    ~GerdaFastFactory() = default;

    inline TH1* GetModel() const { return _model.get(); }

    void SetCountsRange(float xmin, float xmax);
    void AddComponent(const TH1* hist, const float counts);
    void AddComponent(const std::unique_ptr<TH1>& hist, const float counts);
    std::unique_ptr<TH1> GetPseudoExp();
    void Reset();

    private:

    TRandom3 _rndgen;
    std::unique_ptr<TH1> _model;
    std::pair<float, float> _range;
};

#endif
