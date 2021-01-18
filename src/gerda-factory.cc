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
#include "TObjArray.h"
#include "utils.hpp"
#include "progressbar.hpp"
namespace logs = utils::logging;

#include "GerdaFactory.h"
#include "GerdaFastFactory.h"

int main(int argc, char** argv) {

    TH1::AddDirectory(false);

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
     * create experiment factory
     */

    GerdaFastFactory factory;

    // set range for counts
    if (config["range-for-counts"].is_array()) {
        factory.SetCountsRange(
            config["range-for-counts"][0].get<float>(),
            config["range-for-counts"][1].get<float>()
        );
    }

    // parse and build reference model
    logs::out(logs::detail) << "getting base component list from JSON config" << std::endl;
    auto comp_list = utils::get_components_json(config);
    // save it (deep copy), we'll need it after resetting the factory before the next iterations
    const auto comp_list_save = utils::deep_copy(comp_list);

    if (!config["pdf-distortions"].is_object()) {
        throw std::runtime_error("could not find 'pdf-distortions' field in the config file");
    }
    if (!config["pdf-distortions"]["global"].is_object() and !config["pdf-distortions"]["specific"].is_object()) {
        throw std::runtime_error("please specify a 'global' and/or 'specific' field under 'pdf-distortions' in the config file");
    }

    auto dist_prefix = config["pdf-distortions"].value("prefix", ".") + "/";
    TRandom3 rndgen(0);

    auto outname = utils::get_file_obj(config["output"]["file"].get<std::string>());

    // container for output histograms
    std::vector<std::unique_ptr<TH1>> experiments;

    auto niter = config.value("number-of-experiments", 100);
    progressbar bar(niter);
    bar.set_todo_char(" ");
    bar.set_done_char("█");
    bar.set_opening_bracket_char("[");
    bar.set_closing_bracket_char("]");
    logs::out(logs::info) << "generating " << niter << " experiments ";
    logs::out(logs::detail) << std::endl;

    for (int i = 0; i < niter; ++i) {
        if (logs::min_level > logs::detail) bar.update();
        // reset model from last iteration
        factory.Reset();
        comp_list.clear();
        // we restart from base model
        comp_list = utils::deep_copy(comp_list_save);

        bool done_something = false;
        // for distortions given with gerda-pdfs structure
        if (config["pdf-distortions"]["global"].is_object()) {
            logs::out(logs::detail) << "applying 'global' distortions" << std::endl;
            for (auto& it : config["pdf-distortions"]["global"].items()) {

                bool interpolate = false;
                if (it.value().contains("interpolate")) {
                    if (it.value()["interpolate"].get<bool>() == true) interpolate = true;
                }

                // Interpolation with unitary distortion
                //
                // Must be used with care, as it modifies the prior on the
                // distortions from a certain group.  After a discrete (user
                // input) distortion is selected, a random number w in [0,1] is
                // drawn.  This number w defines the admixture of the
                // distortion D with the unitary distortion U according to the
                // following simple formula:
                //
                //     pdf' = pdf * [ w * D + (1-w) * U ]
                if (interpolate) {
                    logs::out(logs::debug) << "randomly choosing a distortion for '" << it.key()
                                           << " and interpolating with the unitary distortion" << std::endl;
                    // first choose a discrete distortion randomly
                    auto choice = rndgen.Integer(it.value()["pdfs"].size());
                    logs::out(logs::detail) << "chosen random distortion: '"
                                            << it.value()["pdfs"][choice].get<std::string>()
                                            << "'" << (interpolate ? " -> interpolate" : "") << std::endl;

                    // get histograms
                    auto dist_list = utils::get_components_json(config, dist_prefix + it.value()["pdfs"][choice].get<std::string>(), true);
                    for (auto itt = dist_list.begin(); itt != dist_list.end(); itt++) {
                        // see if we have a corresponding fit component
                        auto result = std::find_if(
                            comp_list.begin(), comp_list.end(),
                            [&itt](utils::bkg_comp& a) { return a.name == itt->name; }
                        );

                        // then choose a distortion weight
                        if (result != comp_list.end()) {
                            auto weight = rndgen.Uniform(1);
                            logs::out(logs::debug) << "successfully found corresponding fit component '" << itt->name
                                                   << "', distorting with weight = " << weight << std::endl;

                            std::unique_ptr<TH1> result_tmp(dynamic_cast<TH1*>(result->hist->Clone()));
                            result_tmp->Multiply(itt->hist.get());
                            result_tmp->Scale(weight/result_tmp->Integral());

                            result->hist->Scale((1-weight)/result->hist->Integral());
                            result->hist->Add(result_tmp.get());

                            done_something = true;
                        }
                        else {
                            logs::out(logs::warning) << "could not find component '" << it.key()
                                << "' to distort" << std::endl;
                        }
                    }
                }
                else {
                    logs::out(logs::debug) << "randomly choosing a discrete distortion for '" << it.key() << std::endl;
                    // choose a distortion randomly
                    // the +1 corresponds to no distortion applied
                    auto choice = rndgen.Integer(it.value()["pdfs"].size()+1);
                    if (choice != it.value()["pdfs"].size()) {
                        logs::out(logs::detail) << "chosen random distortion: '"
                                                << it.value()["pdfs"][choice].get<std::string>()
                                                << "'" << std::endl;

                        // get histograms
                        auto dist_list = utils::get_components_json(config, dist_prefix + it.value()["pdfs"][choice].get<std::string>(), true);
                        for (auto itt = dist_list.begin(); itt != dist_list.end(); itt++) {
                            // see if we have a corresponding fit component
                            auto result = std::find_if(
                                    comp_list.begin(), comp_list.end(),
                                    [&itt](utils::bkg_comp& a) { return a.name == itt->name; }
                                    );

                            // distort
                            if (result != comp_list.end()) {
                                logs::out(logs::debug) << "successfully found corresponding fit component '" << itt->name
                                    << "', distorting" << std::endl;
                                result->hist->Multiply(itt->hist.get());
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
        }
        // for distortions given for single components
        if (config["pdf-distortions"]["specific"].is_object()) {
            logs::out(logs::detail) << "applying 'specific' distortions" << std::endl;
            for (auto& it : config["pdf-distortions"]["specific"].items()) {

                // see if we have a corresponding fit component
                auto result = std::find_if(
                    comp_list.begin(), comp_list.end(),
                    [&it](utils::bkg_comp& a) { return a.name == it.key(); }
                );

                if (result == comp_list.end()) {
                    logs::out(logs::warning) << "could not find component '" << it.key()
                                             << "' to distort" << std::endl;
                    continue;
                }
                else {
                    logs::out(logs::debug) << "successfully found corresponding fit component '" << it.key()
                                           << "'" << std::endl;
                }

                bool interpolate = false;
                if (it.value().contains("interpolate")) {
                    if (it.value()["interpolate"].get<bool>() == true) interpolate = true;
                }

                // Interpolate with unitary distortion
                if (interpolate) {
                    logs::out(logs::debug) << "randomly choosing a discrete distortion for '"
                                           << it.key() << "and interpolating with the unitary distortion" << std::endl;
                    // choose a distortion randomly
                    auto choice = rndgen.Integer(it.value()["pdfs"].size());

                    if (it.value()["pdfs"][choice].is_string()) {
                        logs::out(logs::detail) << "chosen random distortion: '"
                                                << it.value()["pdfs"][choice].get<std::string>()
                                                << "'" << (interpolate ? " -> interpolate" : "") << std::endl;

                        auto hdist = utils::get_component(
                            dist_prefix + it.value()["pdfs"][choice].get<std::string>(),
                            config["hist-name"].get<std::string>(),
                            8000, 0, 8000
                        );

                        auto weight = rndgen.Uniform(1);
                        logs::out(logs::debug) << "distorting with weight = " << weight << std::endl;

                        std::unique_ptr<TH1> result_tmp(dynamic_cast<TH1*>(result->hist->Clone()));
                        result_tmp->Multiply(hdist.get());
                        result_tmp->Scale(weight/result_tmp->Integral());

                        result->hist->Scale((1-weight)/result->hist->Integral());
                        result->hist->Add(result_tmp.get());

                        done_something = true;
                    }
                    else {
                        throw std::runtime_error("elements in arrays of distortions must be strings");
                    }
                }
                else {

                    logs::out(logs::debug) << "randomly choosing a discrete distortion for '"
                                           << it.key() << std::endl;
                    // choose a distortion randomly
                    // the +1 corresponds to no distortion applied
                    auto choice = rndgen.Integer(it.value()["pdfs"].size()+1);

                    if (choice != it.value()["pdfs"].size()) {
                        if (it.value()["pdfs"][choice].is_string()) {
                            logs::out(logs::detail) << "chosen random distortion: '"
                                                    << it.value()["pdfs"][choice].get<std::string>()
                                                    << "'" << std::endl;
                            auto hdist = utils::get_component(
                                dist_prefix + it.value()["pdfs"][choice].get<std::string>(),
                                config["hist-name"].get<std::string>(),
                                8000, 0, 8000
                            );
                            logs::out(logs::debug) << "distorting" << std::endl;
                            result->hist->Multiply(hdist.get());
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
            }
        }
        if (!done_something) logs::out(logs::warning) << "did not distort anything!" << std::endl;

        // add components to the factory
        for (auto& e : comp_list) factory.AddComponent(e.hist.get(), e.counts);

        // now generate the experiment
        logs::out(logs::detail) << "filling output histogram" << std::endl;

        experiments.push_back(factory.GetPseudoExp());
        auto hexp = experiments.back().get();

        int n_orig_bins = factory.GetModel()->GetNbinsX();

        if (n_orig_bins % config["output"]["number-of-bins"].get<int>() != 0) {
            throw std::runtime_error("\"number-of-bins\" is incompatible with reference model number of bins (" +
                    std::to_string(n_orig_bins) + ")");
        }
        else hexp->Rebin(n_orig_bins / config["output"]["number-of-bins"].get<int>());

        hexp->SetName(((outname.second != "" ? outname.second : "h") + "_" + std::to_string(i)).c_str());
        hexp->SetTitle("Pseudo experiment");

        logs::out(logs::debug) << "object " << hexp->GetName()
                               << " added to collection " << std::endl;
    }

    // output file
    logs::out(logs::debug) << "opening output file" << std::endl;
    system(("mkdir -p " + outname.first.substr(0, outname.first.find_last_of('/'))).c_str());
    TFile fout(outname.first.c_str(), "recreate");
    for (const auto& e : experiments) dynamic_cast<TH1D*>(e.get())->Write();
    fout.Close();

    logs::out(logs::debug) << "exiting" << std::endl;

    return 0;
}
