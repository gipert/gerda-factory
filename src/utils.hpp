/* ROOTUtils.hpp
 *
 * Author: Luigi Pertoldi - pertoldi@pd.infn.it
 * Created: Tue 18 Jun 2019
 *
 */
#include <iostream>
#include <string>
#include <memory>

#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TParameter.h"

#include "json.hpp"
using json = nlohmann::json;

#ifndef UTILS_HH
#define UTILS_HH

namespace utils {

    struct bkg_comp {
        std::string name;
        std::unique_ptr<TH1> hist;
        float counts;

        // constructor
        bkg_comp(const std::string& n, TH1* h, float c) :
            name(n), hist(h), counts(c) {}
        // (deep) copy constructor
        bkg_comp(const bkg_comp& orig) :
            name(orig.name),
            hist(dynamic_cast<TH1*>(orig.hist->Clone())),
            counts(orig.counts) {}
    };

    std::vector<bkg_comp> deep_copy(const std::vector<bkg_comp>& orig) {
        std::vector<utils::bkg_comp> out;
        for (auto& el : orig) out.emplace_back(el);
        return out;
    };

    // The returned TH1 is owned by the user
    std::unique_ptr<TH1> get_component(std::string filename, std::string objectname, int nbinsx = 100, double xmin = 0, double xmax = 100) {
        TFile _tf(filename.c_str());
        if (!_tf.IsOpen()) throw std::runtime_error("invalid ROOT file: " + filename);
        auto obj = _tf.Get(objectname.c_str());
        if (!obj) throw std::runtime_error("could not find object '" + objectname + "' in file " + filename);
        // please do not delete it when the TFile goes out of scope
        if (obj->InheritsFrom(TH1::Class())) {
            std::unique_ptr<TH1> _th(dynamic_cast<TH1*>(obj));
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
            return _th; // expect compiler to copy-elide here
        }
        else if (obj->InheritsFrom(TF1::Class())) {
            std::unique_ptr<TH1> _th(new TH1D(obj->GetName(), obj->GetTitle(), nbinsx, xmin, xmax));
            for (int b = 1; b < _th->GetNbinsX(); ++b) {
                _th->SetBinContent(b, dynamic_cast<TF1*>(obj)->Eval(_th->GetBinCenter(b)));
            }
            _th->SetDirectory(nullptr); // just to be sure
            return _th; // expect compiler to copy-elide here
        }
        else {
            throw std::runtime_error("object '" + objectname + "' in file " + filename + " isn't of type TH1 or TF1");
        }
    }

    std::pair<std::string, std::string> get_file_obj(std::string expr) {
        std::string filename;
        std::string objname = "";
        if (expr.find(':') != std::string::npos) {
            filename = expr.substr(0, expr.find_first_of(':'));
            objname = expr.substr(expr.find_first_of(':')+1, std::string::npos);
        }
        else filename = expr;

        return std::pair<std::string, std::string>(filename, objname);
    }

    namespace logging {

        enum level {debug, detail, info, warning, error};
        level min_level = info;
        std::ofstream devnull("/dev/null");

        NLOHMANN_JSON_SERIALIZE_ENUM(utils::logging::level, {
            {utils::logging::debug,   "debug"},
            {utils::logging::detail,  "detail"},
            {utils::logging::info,    "info"},
            {utils::logging::warning, "warning"},
            {utils::logging::error,   "error"},
        })

        std::ostream& out(const level& lvl) {

            if (lvl == debug and min_level <= debug) {
                std::cout << "\033[36m[ Debug:\033[0m ";
                return std::cout;
            }
            if (lvl == detail and min_level <= detail) {
                std::cout << "\033[34m[ Detail:\033[0m ";
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
            else return devnull;
        }
    }

  std::vector<bkg_comp> get_components_json(json& config, std::string gerda_pdfs = "", bool discard_user_files = false) {
        std::vector<bkg_comp> comp_map;

        // eventually get a global value for the gerda-pdfs path
        if (gerda_pdfs == "") gerda_pdfs = config.value("gerda-pdfs", ".");
        // eventually get a global value for the hist name in the ROOT files
        auto hist_name = config.value("hist-name", "");

        // loop over first level of "components"
        for (auto& it : config["components"]) {
            /* START INTERMEZZO */
            // utility to sum over the requested parts (with weight) given isotope
            auto sum_parts = [&](std::string i) {
                std::string true_iso = i;
                if (i.find('-') != std::string::npos) true_iso = i.substr(0, i.find('-'));

                std::vector<std::unique_ptr<TH1>> collection;
                if (it["part"].is_object()) {
                    hist_name = it.value("hist-name", hist_name);

                    // compute sum of weights
                    double sumw = 0;
                    for (auto& p : it["part"].items()) sumw += p.value().get<double>();

                    for (auto& p : it["part"].items()) {
                        // get volume name
                        auto path_to_part = gerda_pdfs + "/" + p.key();
                        if (path_to_part.back() == '/') path_to_part.pop_back();
                        auto part = path_to_part.substr(path_to_part.find_last_of('/')+1);
                        path_to_part.erase(path_to_part.find_last_of('/'));
                        auto volume = path_to_part.substr(path_to_part.find_last_of('/')+1);
                        auto filename = gerda_pdfs + "/" + p.key() + "/" + true_iso + "/" + "pdf-"
                            + volume + "-" + part + "-" + i + ".root";
                        logging::out(logging::debug) << "opening file " << filename << std::endl;
                        logging::out(logging::debug) << "summing object '" << hist_name << " with weight "
                                                     << p.value().get<double>()/sumw << std::endl;
                        // get histogram (owned by us)
                        collection.emplace_back(utils::get_component(filename, hist_name, 8000, 0, 8000));
                        // apply weight
                        collection.back()->Scale(p.value().get<double>()/sumw);
                    }
                    // now sum them all
                    for (auto it = collection.begin()+1; it != collection.end(); it++) collection[0]->Add(it->get());
                    return std::move(collection[0]);
                }
                else if (it["part"].is_string()) {
                    // get volume name
                    auto path_to_part = gerda_pdfs + "/" + it["part"].get<std::string>();
                    if (path_to_part.back() == '/') path_to_part.pop_back();
                    auto part = path_to_part.substr(path_to_part.find_last_of('/')+1);
                    path_to_part.erase(path_to_part.find_last_of('/'));
                    auto volume = path_to_part.substr(path_to_part.find_last_of('/')+1);
                    auto filename = gerda_pdfs + "/" + it["part"].get<std::string>() + "/" + true_iso + "/" + "pdf-"
                        + volume + "-" + part + "-" + i + ".root";
                    logging::out(logging::debug) << "getting object '" << hist_name << "' in file " << filename << std::endl;
                    // get histogram (owned by us)
                    auto thh = utils::get_component(filename, hist_name, 8000, 0, 8000);
                    return thh;
                }
                else throw std::runtime_error("unexpected 'part' value found in \"components\"");
            };
            /* END INTERMEZZO */

            // loop over the second level of components
            for (auto& iso : it["components"].items()) {
                logging::out(logging::debug) << "building PDF for entry " << iso.key() << std::endl;

                // it's a user defined file
                if (it.contains("root-file")) {
		  if (!discard_user_files) {
		    auto filename = it["root-file"].get<std::string>();
		    auto objname = iso.value()["hist-name"].get<std::string>();
		    auto th = utils::get_component(filename, objname, 8000, 0, 8000);
		    th->SetName((iso.key() + "_" + std::string(th->GetName())).c_str());
		    for (int b = 0; b <= th->GetNbinsX()+1; ++b) {
                      if (th->GetBinContent(b) < 0) {
                        th->SetBinContent(b, 0);
                      }
                    }
		    // comp_map now owns the histogram
		    comp_map.emplace_back(iso.key(), th.release(), iso.value()["amount-cts"].get<float>());
		  }
                }
                else { // look into gerda-pdfs database
                    if (iso.value()["isotope"].is_string()) {
                        comp_map.emplace_back(
                            iso.key(),
                            sum_parts(iso.value()["isotope"]).release(),
                            iso.value()["amount-cts"].get<float>()
                        );
                    }
                    else if (iso.value()["isotope"].is_object()) {
                        std::vector<std::unique_ptr<TH1>> collection;
                        double sumwi = 0;
                        for (auto& i : iso.value()["isotope"].items()) sumwi += i.value().get<double>();

                        for (auto& i : iso.value()["isotope"].items()) {
                            logging::out(logging::debug) << "scaling pdf for " << i.key() << " by a factor "
                                                         << i.value().get<double>()/sumwi << std::endl;

                            collection.emplace_back(sum_parts(i.key()));
                            collection.back()->Scale(i.value().get<double>()/sumwi);
                        }
                        // now sum them all
                        for (auto it = collection.begin()+1; it != collection.end(); it++) collection[0]->Add(it->get());
			for (int b = 0; b <= collection[0]->GetNbinsX()+1; ++b) {
			  if (collection[0]->GetBinContent(b) < 0) {
			    collection[0]->SetBinContent(b, 0);
			  }
			}
                        comp_map.emplace_back(iso.key(), collection[0].release(), iso.value()["amount-cts"].get<float>());
                    }
                    else throw std::runtime_error("unexpected entry " + iso.value()["isotope"].dump()
                            + "found in [\"components\"][\"" + iso.key() + "\"][\"isotope\"]");

                    comp_map.back().hist->SetName(
                        (iso.key() + "_" + std::string(comp_map.back().hist->GetName())).c_str()
                    );

                }
            }
        }
        return comp_map;
    }
}

#endif
