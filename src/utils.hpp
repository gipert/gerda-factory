/* ROOTUtils.hpp
 *
 * Author: Luigi Pertoldi - pertoldi@pd.infn.it
 * Created: Tue 18 Jun 2019
 *
 */
#include <iostream>
#include <string>

#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TParameter.h"

#ifndef UTILS_HH
#define UTILS_HH

namespace utils {

    TH1* GetFitComponent(std::string filename, std::string objectname, TH1* data) {
        TFile _tf(filename.c_str());
        if (!_tf.IsOpen()) throw std::runtime_error("invalid ROOT file: " + filename);
        auto obj = _tf.Get(objectname.c_str());
        if (!obj) throw std::runtime_error("could not find object '" + objectname + "' in file " + filename);
        // please do not delete it when the TFile goes out of scope
        if (obj->InheritsFrom(TH1::Class())) {
            auto _th = dynamic_cast<TH1*>(obj);
            if (_th->GetDimension() > 1) throw std::runtime_error("TH2/TH3 are not supported yet");

            TParameter<Long64_t>* _nprim = nullptr;
            if (objectname.substr(0, 3) == "M1_") {
                _nprim = dynamic_cast<TParameter<Long64_t>*>(_tf.Get("NumberOfPrimariesEdep"));
            }
            else if (objectname.substr(0, 3) == "M2_") {
                _nprim = dynamic_cast<TParameter<Long64_t>*>(_tf.Get("NumberOfPrimariesCoin"));
            }
            long long int nprim = (_nprim) ? _nprim->GetVal() : 1;
            _th->Scale(1./nprim);

            _th->SetDirectory(nullptr);
            return _th;
        }
        else if (obj->InheritsFrom(TF1::Class())) {
            auto _th = new TH1D(obj->GetName(), obj->GetTitle(), data->GetNbinsX(),
                data->GetXaxis()->GetXmin(), data->GetXaxis()->GetXmax());
            for (int b = 1; b < _th->GetNbinsX(); ++b) {
                _th->SetBinContent(b, dynamic_cast<TF1*>(obj)->Eval(_th->GetBinCenter(b)));
            }
            _th->SetDirectory(nullptr);
            return _th;
        }
        else {
            throw std::runtime_error("object '" + objectname + "' in file " + filename + " isn't of type TH1 or TF1");
        }
    }

    class logging {

        enum level {debug, info, warning, error};

        static std::ostream& msg(level lvl) {

            static logging instance(info);

            if (lvl == debug and min_level <= debug) {
                std::cout << "\033[36m[ Debug:\033[0m ";
                return std::cout;
            }
            if (lvl == info and min_level <= info) {
                std::cout << "\033[97;1m[ Info:\033[0m ";
                return std::cout;
            }
            if (lvl == warning and min_level <= warning) {
                std::cerr << "\033[33m[ Warning:\033[0m ";
                return std::cerr;
            }
            if (lvl == error and min_level <= error) {
                std::cerr << "\033[91m[ Error:\033[0m ";
                return std::cerr;
            }
            else {
                return devnull;
            }
        };

        static void set_min_level(level lvl) { min_level = lvl; };

        private:
        logging(level lvl) : min_level(lvl) {}

        level min_level;
        const std::ofstream devnull("/dev/null");

        public:
        logging()                      = delete;
        logging(logging const&)        = delete;
        void operator=(logging const&) = delete;
    };
}

#endif
