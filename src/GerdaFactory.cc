/* GerdaFactory.cc
 *
 * Author: Luigi Pertoldi - pertoldi@pd.infn.it
 * Created: Tue 18 Jun 2019
 *
 */
#include "GerdaFactory.h"
#include <iostream>

GerdaFactory::GerdaFactory() :
    _rndgen(0),
    _range(0, 0) {

    TH1::AddDirectory(false);
}

void GerdaFactory::SetCountsRange(float xmin, float xmax) {
    if (!(xmin == 0 and xmax == 0) and xmax < xmin) throw std::runtime_error("GerdaFactory::SetCountsRange] invalid range.");
    _range.first = xmin;
    _range.second = xmax;
}

// creates an internal copy of the histogram pointer
void GerdaFactory::AddComponent(const TH1* hist, const float counts) {
    if (!hist) throw std::runtime_error("GerdaFactory::AddComponent] invalid pointer detected.");
    if (counts < 0) throw std::runtime_error("GerdaFactory::AddComponent] counts argument is < 0.");

    // insert clone
    std::unique_ptr<TH1> _tmp(dynamic_cast<TH1*>(hist->Clone(("comp_" + std::to_string(_comp_list.size())).c_str())));
    _comp_list.emplace(std::move(_tmp), counts);
}

// creates an internal copy of the histogram pointer
void GerdaFactory::AddComponent(const std::unique_ptr<TH1>& hist, const float counts) {
    this->AddComponent(hist.get(), counts);
}

void GerdaFactory::FillPseudoExp(TH1& out) {

    if (_comp_list.empty()) throw std::runtime_error("GerdaFactory::GetPseudoExp] must call GerdaFactory::AddComponent first.");

    // loop over components in list
    for (auto& comp : _comp_list) {
        // determine experiment actualization
        int real_cts;
        if (_range.first == 0 and _range.second == 0) real_cts = _rndgen.Poisson(comp.second);
        else real_cts = _rndgen.Poisson(comp.second)*comp.first->Integral()/comp.first->Integral(_range.first, _range.second);

        // and fill the provided TH1
        for (int i = 0; i < real_cts; ++i) {
            out.Fill(comp.first->GetRandom());
        }
    }
}

void GerdaFactory::FillPseudoExp(TH1* out) {

    if (_comp_list.empty()) throw std::runtime_error("GerdaFactory::GetPseudoExp] must call GerdaFactory::AddComponent first.");

    // loop over components in list
    for (auto& comp : _comp_list) {
        // determine experiment actualization
        int real_cts;
        if (_range.first == 0 and _range.second == 0) real_cts = _rndgen.Poisson(comp.second);
        else real_cts = _rndgen.Poisson(comp.second)*comp.first->Integral()/comp.first->Integral(_range.first, _range.second);

        // and fill the provided TH1
        for (int i = 0; i < real_cts; ++i) {
            out->Fill(comp.first->GetRandom());
        }
    }
}

void GerdaFactory::ResetComponents() {
    _comp_list.clear();
}
