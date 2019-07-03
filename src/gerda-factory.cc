/* gerda-fake-gen.cc
 *
 * Author: Luigi Pertoldi
 * Created Tue 18 Jun 2019
 *
 */
#include <iostream>
#include <algorithm>
#include <getopt.h>

#include "TRandom3.h"
#include "utils.hpp"
namespace logs = utils::logging;

#include "GerdaFactory.h"

#include "json.hpp"
using json = nlohmann::json;

namespace utils { namespace logging {
    NLOHMANN_JSON_SERIALIZE_ENUM(utils::logging::level, {
        {utils::logging::debug,   "debug"},
        {utils::logging::detail,  "detail"},
        {utils::logging::info,    "info"},
        {utils::logging::warning, "warning"},
        {utils::logging::error,   "error"},
    })
}}

struct bkg_comp {
    std::string name;
    TH1* hist;
    float counts;

    ~bkg_comp() { delete hist; }
    bkg_comp(std::string n, TH1* h, float c) :
        name(n), hist(h), counts(c) {}
    bkg_comp(bkg_comp const& orig) :
        name(orig.name),
        hist(dynamic_cast<TH1*>(orig.hist->Clone())),
        counts(orig.counts) {}
};

std::vector<bkg_comp> get_components_json(json config, std::string gerda_pdfs = "");

int main(int argc, char** argv) {

    /*
     * get command line args
     */

    std::string progname(argv[0]);

    auto usage = [&]() {
        std::cerr << "USAGE: " << progname << " [-h|--help] json-config\n";
    };

    const char* const short_opts = ":h";
    const option long_opts[] = {
        { "help",  no_argument, nullptr, 'h' },
        { nullptr, no_argument, nullptr, 0   }
    };

    int opt = 0;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'h': // -h or --help
            case '?': // Unrecognized option
            default:
                usage();
                return 1;
        }
    }

    // extra arguments
    std::vector<std::string> args;
    for(; optind < argc; optind++){
        args.emplace_back(argv[optind]);
    }

    if (args.empty() or args.size() > 1) {usage(); return 1;}

    std::ifstream fconfig(args[0]);
    if (!fconfig.is_open()) {
        logs::out(logs::error) << "config file " << args[0] << " does not exist" << std::endl;
        return 1;
    }
    json config;
    fconfig >> config;

    logs::min_level = config.value("logging", logs::info);

    /*
     * create model
     */

    GerdaFactory factory;

    // set range for counts
    if (config["range-for-counts"].is_array()) {
        factory.SetCountsRange(
            config["range-for-counts"][0].get<float>(),
            config["range-for-counts"][1].get<float>()
        );
    }

    logs::out(logs::detail) << "getting base component list from json config" << std::endl;
    auto comp_list = get_components_json(config);
    // save it, we'll need it to reset the factory before the next iterations
    const auto comp_list_save = comp_list;

    if (!config["pdf-distortions"].is_object()) {
        throw std::runtime_error("could not find 'pdf-distortions' field in the config file");
    }
    if (!config["pdf-distortions"]["global"].is_object() and !config["pdf-distortions"]["specific"].is_object()) {
        throw std::runtime_error("please specify a 'global' and/or 'specific' field under 'pdf-distortions' in the config file");
    }

    auto dist_prefix = config["pdf-distortions"].value("prefix", ".") + "/";
    TRandom3 rndgen(0);

    // output file
    logs::out(logs::debug) << "opening output file" << std::endl;
    auto outname = utils::get_file_obj(config["output-file"].get<std::string>());
    system(("mkdir -p " + outname.first.substr(0, outname.first.find_last_of('/'))).c_str());
    TFile fout(outname.first.c_str(), "recreate");

    auto niter = config.value("numer-of-experiments", 100);
    logs::out(logs::info) << "generating " << niter << " experiments..." << std::endl;
    for (int i = 0; i < niter; ++i) {
        // reset components
        comp_list = comp_list_save;
        factory.ResetComponents();

        bool done_something = false;
        // for distortions given with gerda-pdfs structure
        if (config["pdf-distortions"]["global"].is_object()) {
            logs::out(logs::detail) << "applying 'global' distortions" << std::endl;
            for (auto& it : config["pdf-distortions"]["global"].items()) {
                logs::out(logs::debug) << "randomly choosing a distortion for '" << it.key() << std::endl;
                // choose a distortion randomly
                // the +1 corresponds to no distortion applied
                auto choice = rndgen.Integer(it.value().size()+1);
                if (choice != it.value().size()) {
                    logs::out(logs::detail) << "chosen random distortion: '"
                                            << it.value()[choice].get<std::string>() << "'" << std::endl;
                    // get histograms
                    auto dist_list = get_components_json(config, dist_prefix + it.value()[choice].get<std::string>());
                    for (auto& d : dist_list) {
                        // see if we have a corresponding fit component
                        auto result = std::find_if(
                            comp_list.begin(), comp_list.end(),
                            [&d](bkg_comp& a) { return a.name == d.name; }
                        );

                        // distort
                        if (result != comp_list.end()) {
                            logs::out(logs::debug) << "successfully found corresponding fit component '" << d.name
                                                   << "', distorting" << std::endl;
                            result->hist->Multiply(d.hist);
                            done_something = true;
                        }
                        else {
                            logs::out(logs::warning) << "could not find component '" << it.key()
                                                     << "' to distort" << std::endl;
                        }
                    }
                }
                // no distortion applied
                else {
                    logs::out(logs::detail) << "chosen random distortion: "
                                            << "stay with current PDF" << std::endl;
                    done_something = true;
                }
            }
        }
        // for distortions given for single components
        if (config["pdf-distortions"]["specific"].is_object()) {
            logs::out(logs::detail) << "applying 'specific' distortions" << std::endl;
            for (auto& it : config["pdf-distortions"]["specific"].items()) {
                // see if we have a corresponding fit component
                auto result = std::find_if(
                    comp_list.begin(), comp_list.end(),
                    [&it](bkg_comp& a) { return a.name == it.key(); }
                );

                if (result != comp_list.end()) {
                    logs::out(logs::debug) << "successfully found corresponding fit component '" << it.key()
                                           << "'" << std::endl;
                    logs::out(logs::debug) << "randomly choosing a distortion for '" << it.key() << std::endl;
                    // choose a distortion randomly
                    // the +1 corresponds to no distortion applied
                    auto choice = rndgen.Integer(it.value().size()+1);

                    if (choice != it.value().size()) {
                        if (it.value()[choice].is_string()) {
                            logs::out(logs::detail) << "chosen random distortion: '"
                                                    << it.value()[choice].get<std::string>() << "'" << std::endl;
                            auto hdist = utils::get_component(
                                dist_prefix + it.value()[choice].get<std::string>(),
                                config["hist-name"].get<std::string>(),
                                8000, 0, 8000
                            );
                            logs::out(logs::debug) << "distorting" << std::endl;
                            result->hist->Multiply(hdist);
                            done_something = true;
                        }
                        else {
                            throw std::runtime_error("elements in arrays of distortions must be strings");
                        }
                    }
                    // no distortion applied
                    else {
                        logs::out(logs::detail) << "chosen random distortion: "
                                                << "stay with current PDF" << std::endl;
                        done_something = true;
                    }
                }
                else {
                    logs::out(logs::warning) << "could not find component '" << it.key()
                                             << "' to distort" << std::endl;
                }
            }
        }
        if (!done_something) logs::out(logs::warning) << "did not distort anything!" << std::endl;

        // add components to the factory
        for (auto& e : comp_list) factory.AddComponent(e.hist, e.counts);

        // now generate the experiment
        logs::out(logs::detail) << "filling output histogram" << std::endl;
        TH1D hexp(outname.second.c_str(), "Pseudo experiment", 8000, 0, 8000);
        factory.FillPseudoExp(hexp);

        fout.cd();
        hexp.Write();

        logs::out(logs::debug) << "object " << outname.second
                              << " written on file " << outname.first << std::endl;
    }
    return 0;
}

std::vector<bkg_comp> get_components_json(json config, std::string gerda_pdfs) {
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

            TH1* sum = nullptr;
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
                    logs::out(logs::debug) << "opening file " << filename << std::endl;
                    logs::out(logs::debug) << "summing object '" << hist_name << " with weight "
                                           << p.value().get<double>()/sumw << std::endl;
                    // get histogram
                    auto thh = utils::get_component(filename, hist_name, 8000, 0, 8000);
                    // add it with weight
                    if (!sum) {
                        sum = thh;
                        sum->SetDirectory(nullptr); // please do not delete it when the TFile goes out of scope
                        sum->Scale(p.value().get<double>()/sumw);
                    }
                    else sum->Add(thh, p.value().get<double>()/sumw);
                }
                return sum;
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
                logs::out(logs::debug) << "getting object '" << hist_name << "' in file " << filename << std::endl;
                // get histogram
                auto thh = utils::get_component(filename, hist_name, 8000, 0, 8000);
                return thh;
            }
            else throw std::runtime_error("unexpected 'part' value found in \"components\"");
        };
        /* END INTERMEZZO */

        // loop over requested isotopes on the relative part
        for (auto& iso : it["components"].items()) {
            logs::out(logs::debug) << "building pdf for entry " << iso.key() << std::endl;

            // it's a user defined file
            if (it.contains("root-file")) {
                auto filename = it["root-file"].get<std::string>();
                for (auto& itt : it["components"]) {
                    auto objname = itt["hist-name"].get<std::string>();
                    auto th = utils::get_component(filename, objname, 8000, 0, 8000);

                    comp_map.emplace_back(iso.key(), th, itt["amount-cts"].get<float>());
                }
            }
            else { // look into gerda-pdfs database
                TH1* comp = nullptr;
                if (iso.value()["isotope"].is_string()) {
                    comp = sum_parts(iso.value()["isotope"]);
                }
                else if (iso.value()["isotope"].is_object()) {
                    double sumwi = 0;
                    for (auto& i : iso.value()["isotope"].items()) sumwi += i.value().get<double>();

                    for (auto& i : iso.value()["isotope"].items()) {
                        logs::out(logs::debug) << "scaling pdf for " << i.key() << " by a factor "
                                               << i.value().get<double>()/sumwi << std::endl;
                        if (!comp) {
                            comp = sum_parts(i.key());
                            comp->Scale(i.value().get<double>()/sumwi);
                        }
                        else comp->Add(sum_parts(i.key()), i.value().get<double>()/sumwi);

                    }
                }
                else throw std::runtime_error("unexpected entry " + iso.value()["isotope"].dump()
                        + "found in [\"components\"][\"" + iso.key() + "\"][\"isotope\"]");

                comp_map.emplace_back(iso.key(), comp, iso.value()["amount-cts"].get<float>());
            }
        }
    }

    return comp_map;
}
