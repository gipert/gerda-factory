//usr/bin/env root -l -b -q -x ${0}\(\""${1}"\"\); exit $?

void ROOTroutine(const std::string& filelist) {
    std::istringstream ss(filelist);
    std::vector<std::string> files;

    std::string word;
    while (ss >> word) {
        files.push_back(word);
    }

    for (const auto& file : files) {
        auto f = file;

        auto fname = f.substr(f.find_last_of('/')+1, f.size());
        f.erase(f.find_last_of('/'), f.size());

        auto isotope = f.substr(f.find_last_of('/')+1, f.size());
        f.erase(f.find_last_of('/'), f.size());

        auto part = f.substr(f.find_last_of('/')+1, f.size());
        f.erase(f.find_last_of('/'), f.size());

        auto volume = f.substr(f.find_last_of('/')+1, f.size());
        f.erase(f.find_last_of('/'), f.size());

        auto dirname = f.substr(f.find_last_of('/')+1, f.size());
        f.erase(f.find_last_of('/'), f.size());

        auto basepath = volume + "/" + part + "/" + isotope + "/" + fname;
        system(("mkdir -p distortions/" + dirname + "/" + volume + "/" + part + "/" + isotope).c_str());

        std::vector<std::string> histnames = {"lar/M1_enrBEGe", "lar/M1_enrCoax"};

        auto outname = "distortions/" + dirname + "/" + basepath;

        TFile fdist(file.c_str());
        TFile forig(("gerda-pdfs/gerda-pdfs-2nufit-best/" + basepath).c_str());
        if (!forig.IsOpen()) {
            std::cerr << "WARNING: could not find " << "gerda-pdfs/gerda-pdfs-2nufit-best/"
                      << basepath << std::endl;
            continue;
        }
        TFile fout(outname.c_str(), "recreate");

        for (auto& n : histnames) {
            auto obj = forig.Get(n.c_str());
            if (!obj) {
                std::cerr << "could not find object " << n << " in " << file << std::endl;
                continue;
            }
            if (obj->InheritsFrom(TH1::Class())) {
                auto horig = dynamic_cast<TH1*>(obj);
                auto hdist = dynamic_cast<TH1*>(fdist.Get(n.c_str()));
                if (!hdist) {
                    std::cerr << "could not find object " << n << " in " << file << std::endl;
                    continue;
                }
                hdist->Divide(horig);
                fout.cd();
                if (n == histnames[0]) {
                    auto dir = fout.mkdir((n.substr(0, n.find_last_of('/'))).c_str());
                }
                fout.cd((n.substr(0, n.find_last_of('/'))).c_str());
                n.erase(0, n.find_last_of('/')+1);
                hdist->Write(n.c_str());
            }
            // for alphas, do not calculate distortion
            else if (obj->InheritsFrom(TF1::Class())) {
                auto horig = dynamic_cast<TF1*>(obj);
                fout.cd();
                horig->Write(n.c_str());
            }
        }
    }
}

// vim: filetype=cpp
