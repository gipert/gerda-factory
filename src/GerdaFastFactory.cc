/* GerdaFastFactory.cc
 *
 * Author: Luigi Pertoldi - pertoldi@pd.infn.it
 * Created: Tue 18 Jun 2019
 *
 */
#include "GerdaFastFactory.h"

GerdaFastFactory::GerdaFastFactory() :
    _rndgen(0),
    _model(nullptr),
    _range(0, 0) {
}

GerdaFastFactory::~GerdaFastFactory() {
    delete _model;
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
    auto htmp = dynamic_cast<TH1*>(
        hist->Clone(("comp_" + std::to_string(_rndgen.Uniform(0,1E6))).c_str())
    );

    // normalize to requested weight
    if (_range.first == 0 and _range.second == 0) htmp->Scale(counts/htmp->Integral());
    else htmp->Scale(counts/htmp->Integral(htmp->GetXaxis()->FindBin(_range.first), htmp->GetXaxis()->FindBin(_range.second)));

    // initialize total model, if needed
    if (!_model) {
        _model = dynamic_cast<TH1*>(hist->Clone("model"));
        _model->Reset();
    }

    // add
    _model->Add(htmp);
}


TH1* GerdaFastFactory::FillPseudoExp() {

  if (!_model) throw std::runtime_error("GerdaFastFactory::FillPseudoExp] must call GerdaFastFactory::AddComponent first.");

  auto out = dynamic_cast<TH1*>(_model->Clone("pseudo_exp"));
  out->Reset();

  for (int b = 1; b <= out->GetNbinsX(); ++b) {
    out->SetBinContent(b, _rndgen.Poisson(_model->GetBinContent(b)));
  }

  return out;

}

