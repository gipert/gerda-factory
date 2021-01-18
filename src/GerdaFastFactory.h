/* GerdaFastFactory.h
 *
 * Author: Luigi Pertoldi
 * Created: Tue 18 Jun 2019
 *
 */
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
    std::unique_ptr<TH1> FillPseudoExp();
    void ResetComponents();

    private:

    TRandom3 _rndgen;
    std::unique_ptr<TH1> _model;
    std::pair<float, float> _range;
};

#endif
