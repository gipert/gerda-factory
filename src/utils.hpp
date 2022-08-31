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

#define logging_out(loglevel) \
    logging::out(loglevel, "[" + std::string(__PRETTY_FUNCTION__) + \
                 " // " + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]")

namespace utils {

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

        std::ostream& out(const level& lvl, std::string prefix = "") {

            if (lvl == debug and min_level <= debug) {
                std::cout << "\033[36m[ Debug: " << prefix << "\033[0m ";
                return std::cout;
            }
            if (lvl == detail and min_level <= detail) {
                std::cout << "\033[34m[ Detail: " << prefix << "\033[0m ";
                return std::cout;
            }
            if (lvl == info and min_level <= info) {
                std::cout << "\033[97;1m[ Info: " << prefix << "\033[0m ";
                return std::cout;
            }
            if (lvl == warning and min_level <= warning) {
                std::cerr << "\033[33m[ Warning: " << prefix << "\033[0m ";
                return std::cerr;
            }
            if (lvl == error and min_level <= error) {
                std::cerr << "\033[91m[ Error: " << prefix << "\033[0m ";
                return std::cerr;
            }
            else return devnull;
        }
    }

    struct bkg_comp {
        std::string name;
        std::unique_ptr<TH1> hist;
        std::string orig_name;
        float counts;

        // constructor
        bkg_comp(const std::string& n, TH1* h, std::string& on, float c) :
            name(n), hist(h), orig_name(on), counts(c) {}
        // (deep) copy constructor
        bkg_comp(const bkg_comp& orig) :
            name(orig.name),
            hist(dynamic_cast<TH1*>(orig.hist->Clone())),
            orig_name(orig.orig_name),
            counts(orig.counts) {}
    };

    std::vector<bkg_comp> deep_copy(const std::vector<bkg_comp>& orig) {
        std::vector<utils::bkg_comp> out;
        for (auto& el : orig) out.emplace_back(el);
        return out;
    };

    // The returned raw TH1 pointer is owned by the user
    std::unique_ptr<TH1> get_component(std::string filename, std::string objectname, int nbinsx = 100, double xmin = 0, double xmax = 100) {

        logging_out(logging::debug) << "getting histogram '" << objectname << "' from file "
                                     << filename << std::endl;
        TH1::AddDirectory(false);

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
            if (nprim != 1) {
                logging_out(logging::debug) << "...and scaling it by 1/" << nprim << std::endl;
            }
            _th->Scale(1./nprim);

            return _th; // expect compiler to copy-elide here
        }
        else if (obj->InheritsFrom(TF1::Class())) {
            std::unique_ptr<TH1> _th(new TH1D(obj->GetName(), obj->GetTitle(), nbinsx, xmin, xmax));
            for (int b = 1; b <= _th->GetNbinsX(); ++b) {
                _th->SetBinContent(b, dynamic_cast<TF1*>(obj)->Eval(_th->GetBinCenter(b)));
            }
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

    /* Given a JSON configuration, and optionally a path to GERDA pdfs release,
     * return a list of pdfs for each configured "component". set
     * discard_user_files to true to forcibly ignore components defined by
     * "user" (i.e. not part of the GERDa pdfs release) files.
     */
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
            auto sum_parts = [&it, &hist_name, &gerda_pdfs](std::string i, std::string hist_name_override = "") {
                std::string true_iso = i;
                if (i.find('-') != std::string::npos) true_iso = i.substr(0, i.find('-'));

                std::vector<std::unique_ptr<TH1>> collection;

                hist_name = it.value("hist-name", hist_name);
                // the user can override the internal hist_name (if for
                // example set in the second-level "components"
                if (!hist_name_override.empty()) {
                    hist_name = hist_name_override;
                    logging_out(logging::debug) << "received request for overriding hist_name to: "
                                                << hist_name_override << std::endl;
                }

                if (it["part"].is_object()) {
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
                        logging_out(logging::debug) << "opening file " << filename << std::endl;
                        logging_out(logging::debug) << "summing object '" << hist_name << " with weight "
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
                    // get histogram (owned by us)
                    auto thh = utils::get_component(filename, hist_name, 8000, 0, 8000);
                    return thh;
                }
                else throw std::runtime_error("unexpected 'part' value found in \"components\"");
            };
            /* END INTERMEZZO */

            // loop over the second level of components
            for (auto& iso : it["components"].items()) {
                logging_out(logging::debug) << "building PDF for entry " << iso.key() << std::endl;

                // it's a user defined file
                if (it.contains("root-file")) {
                    if (!discard_user_files) {
                        auto filename = it["root-file"].get<std::string>();
                        auto objname = iso.value()["hist-name"].get<std::string>();
                        auto th = utils::get_component(filename, objname, 8000, 0, 8000);
                        th->SetName((iso.key() + "_" + std::string(th->GetName())).c_str());

                        for (int b = 0; b <= th->GetNbinsX()+1; ++b) {
                            if (th->GetBinContent(b) < 0) {
                                logging_out(logging::warning) << "Negative bin content detected in pdf built for "
                                                              << iso.key() << "in bin " << b
                                                              << ", setting it to zero" << std::endl;
                                th->SetBinContent(b, 0);
                            }
                        }

                        // comp_map now owns the histogram
                        comp_map.emplace_back(iso.key(), th.release(), objname, iso.value()["amount-cts"].get<float>());
                    }
                    else {
                        logging_out(logging::debug) << "discard_user_files is set to true, discarding user-defined entry" << std::endl;
                    }
                }
                else { // look into gerda-pdfs database
                    auto hist_name_override = iso.value().value("hist-name", "");
                    if (!hist_name_override.empty()) {
                        logging_out(logging::debug) << "custom hist-name detected in '" << iso.key()
                                                    << "' entry: '" << hist_name_override
                                                    << "'" << std::endl;
                    }

                    if (iso.value()["isotope"].is_string()) {
                        comp_map.emplace_back(
                            iso.key(),
                            sum_parts(iso.value()["isotope"], hist_name_override).release(),
                            hist_name_override,
                            iso.value()["amount-cts"].get<float>()
                        );
                    }
                    else if (iso.value()["isotope"].is_object()) {
                        std::vector<std::unique_ptr<TH1>> collection;
                        double sumwi = 0;
                        for (auto& i : iso.value()["isotope"].items()) sumwi += i.value().get<double>();

                        for (auto& i : iso.value()["isotope"].items()) {
                            logging_out(logging::debug) << "scaling pdf for " << i.key() << " by a factor "
                                                         << i.value().get<double>()/sumwi << std::endl;

                            collection.emplace_back(sum_parts(i.key(), hist_name_override));
                            collection.back()->Scale(i.value().get<double>()/sumwi);
                        }
                        // now sum them all
                        for (auto it = collection.begin()+1; it != collection.end(); it++) collection[0]->Add(it->get());

                        // check for negative bin contents
                        for (int b = 0; b <= collection[0]->GetNbinsX()+1; ++b) {
                            if (collection[0]->GetBinContent(b) < 0) {
                                logging_out(logging::warning) << "Negative bin content detected in pdf built for "
                                    << iso.key() << "in bin " << b << ", setting it to zero" << std::endl;
                                collection[0]->SetBinContent(b, 0);
                            }
                        }

                        comp_map.emplace_back(iso.key(), collection[0].release(), hist_name_override, iso.value()["amount-cts"].get<float>());
                    }
                    else throw std::runtime_error("unexpected entry " + iso.value()["isotope"].dump()
                            + "found in [\"components\"][\"" + iso.key() + "\"][\"isotope\"]");

                    comp_map.back().hist->SetName(
                        (iso.key() + "_" + std::string(comp_map.back().hist->GetName())).c_str()
                    );
                }
                logging_out(logging::debug) << "inserted '" << comp_map.back().name
                                             << "' with histogram '" << comp_map.back().hist->GetName()
                                             << "' and number of counts = " << comp_map.back().counts
                                             << std::endl;
            }
        }
        return comp_map;
    }
}

#endif
