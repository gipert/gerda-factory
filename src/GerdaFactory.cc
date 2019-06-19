/* GerdaFactory.cc
 *
 * Author: Luigi Pertoldi - pertoldi@pd.infn.it
 * Created: Tue 18 Jun 2019
 *
 */
#include "GerdaFactory.h"

GerdaFactory::GerdaFactory() :
    _rndgen(0),
    _range(0, 0) {
}

void GerdaFactory::SetCountsRange(float xmin, float xmax) {
    if (xmin == xmax != 0 || xmax < xmin) throw std::runtime_error("GerdaFactory::SetCountsRange] invalid range.");
    _range.first = xmin;
    _range.second = xmax;
}

void GerdaFactory::AddComponent(const TH1* hist, const float counts) {
    if (!hist) throw std::runtime_error("GerdaFactory::AddComponent] invalid pointer detected.");
    if (counts <= 0) throw std::runtime_error("GerdaFactory::AddComponent] counts argument is <= 0.");

    // clone
    htmp = dynamic_cast<TH1*>(
        hist->Clone(("comp_" + _std::to_string(_comp_list.size())).c_str())
    );

    // precaution, detach from any TDirectory
    htmp->SetDirectory(nullptr);

    // insert
    _comp_list.insert(htmp, counts);
}

void GerdaFactory::FillPseudoExp(TH1* out) {

    if (_comp_list.empty()) throw std::runtime_error("GerdaFactory::GetPseudoExp] must call GerdaFactory::AddComponent first.");

    // loop over components in list
    for (comp : _comp_list) {
        // determine experiment actualization
        int real_cts;
        if (_range->first == _range.second == 0) real_cts = _rndgen.Poisson(comp->second);
        else real_cts = _rndgen.Poisson(comp->second)*comp->first->Integral()/comp->first->Integral(_range.first, _range.second);

        for (int i = 0; i < real_cts; ++i) {
            out->Fill(comp->first->GetRandom());
        }
    }

    return out;
}
