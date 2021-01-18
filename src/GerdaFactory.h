/* GerdaFactory.h
 *
 * Author: Luigi Pertoldi
 * Created: Tue 18 Jun 2019
 *
 */
#ifndef _GERDA_FACTORY_H
#define _GERDA_FACTORY_H

#include <vector>
#include <map>
#include <memory>

#include "TH1.h"
#include "TRandom3.h"

class GerdaFactory {

    public:

    // delete dangerous constructors
    GerdaFactory           (GerdaFactory const&) = delete;
    GerdaFactory& operator=(GerdaFactory const&) = delete;

    // custom constructor
    GerdaFactory();

    void SetCountsRange(float xmin, float xmax);
    void AddComponent(const TH1* hist, const float counts);
    void AddComponent(const std::unique_ptr<TH1>& hist, const float counts);
    void FillPseudoExp(TH1* experiment);
    void FillPseudoExp(TH1& experiment);
    void ResetComponents();

    private:

    TRandom3 _rndgen;
    std::map<std::unique_ptr<TH1>, float> _comp_list;
    std::pair<float, float> _range;
};

#endif
