/* gerda-fake-gen.cc
 *
 * Author: Luigi Pertoldi
 * Created Tue 18 Jun 2019
 *
 */
#include <iostream>
#include <getopt.h>

#include "utils.hpp"
namespace logs = utils::logging;

#include "GerdaFactory.h"

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

    // eventually get a global value for the gerda-pdfs path
    auto gerda_pdfs = config.value("gerda-pdfs", ".");
    // eventually get a global value for the hist name in the ROOT files
    auto hist_name = config.value("hist-name", "");
    // set range for counts
    if (config["range-for-counts"].is_array()) {
        factory.SetCountsRange(
            config["range-for-counts"][0].get<float>(),
            config["range-for-counts"][1].get<float>()
        );
    }

    auto comp_list = utils::get_components_json(config);

    // add components to the factory
    for (auto& e : comp_list) factory.AddComponent(e.hist, e.counts);

    logs::out(logs::debug) << "opening output file" << std::endl;
    auto outname = utils::get_file_obj(config["output"]["file"].get<std::string>());
    TFile fout(outname.first.c_str(), "recreate");

    // now generate the experiment
    TH1D hexp(
        (outname.second != "" ? outname.second : "h").c_str(),
        "Pseudo experiment",
        config["output"].value("number-of-bins", 8000),
        config["output"].value("xaxis-range", std::vector<int>{0, 8000})[0],
        config["output"].value("xaxis-range", std::vector<int>{0, 8000})[1]
    );
    factory.FillPseudoExp(hexp);

    // now generate the experiment
    logs::out(logs::detail) << "filling output histogram" << std::endl;
    factory.FillPseudoExp(hexp);

    hexp.Write();

    logs::out(logs::info) << "object " << outname.second
                          << " written on file " << outname.first << std::endl;

    return 0;
}
