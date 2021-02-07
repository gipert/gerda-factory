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

#include "GerdaFastFactory.h"

#include <stdexcept>

GerdaFastFactory::GerdaFastFactory() :
    _rndgen(0),
    _range(0, 0) {

    TH1::AddDirectory(false);
}

void GerdaFastFactory::SetCountsRange(float xmin, float xmax) {
    if (!(xmin == 0 and xmax == 0) and xmax < xmin) throw std::runtime_error("GerdaFastFactory::SetCountsRange] invalid range.");
    _range.first = xmin;
    _range.second = xmax;
}

void GerdaFastFactory::AddComponent(const TH1* hist, const float counts) {
    if (!hist) throw std::runtime_error("GerdaFastFactory::AddComponent] invalid pointer detected.");
    if (counts < 0) throw std::runtime_error("GerdaFastFactory::AddComponent] weight is < 0.");

    // clone
    std::unique_ptr<TH1> htmp(dynamic_cast<TH1*>(hist->Clone()));

    // normalize to requested weight
    if (_range.first == 0 and _range.second == 0) htmp->Scale(counts/htmp->Integral());
    else htmp->Scale(counts/htmp->Integral(htmp->GetXaxis()->FindBin(_range.first), htmp->GetXaxis()->FindBin(_range.second)));

    // initialize total model, if needed
    if (_model == nullptr) {
        _model = std::unique_ptr<TH1>(dynamic_cast<TH1*>(hist->Clone("model")));
        _model->Reset();
    }

    // add
    _model->Add(htmp.get());
}

std::unique_ptr<TH1> GerdaFastFactory::GetPseudoExp() {

  if (!_model.get()) throw std::runtime_error("GerdaFastFactory::FillPseudoExp] must call GerdaFastFactory::AddComponent first.");

  auto out = std::unique_ptr<TH1>(dynamic_cast<TH1*>(_model->Clone("pseudo_exp")));
  out->Reset();

  for (int b = 1; b <= out->GetNbinsX(); ++b) {
    out->SetBinContent(b, _rndgen.Poisson(_model->GetBinContent(b)));
  }

  return out;
}

void GerdaFastFactory::Reset() {
  _model.release();
}
