//usr/bin/env root -l -b -q -x ${0}\(\""${1}"\"\); exit $?

void ROOTroutine(const std::string& filelist) {
    std::cout << filelist << std::endl;
    std::istringstream ss(filelist);
    std::vector<std::string> files;

    std::string word;
    while (ss >> word) {
        ss >> word;
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
        auto outname = "distortions/" + dirname + "/" + basepath;

        system(("mkdir -p distortions/" + dirname + "/" + volume + "/" + part + "/" + isotope).c_str());

        TFile fdist(file.c_str());
        TFile forig(("gerda-pdfs/gerda-pdfs-latest/" + basepath).c_str());
        if (!forig.IsOpen()) continue;
        TFile fout(outname.c_str(), "recreate");

        for (auto& n : std::vector<std::string>{"M1_enrBEGe", "M1_enrCoax"}) {
            auto hdist = dynamic_cast<TH1*>(fdist.Get(n.c_str()));
            hdist->SetName((n + "_dist").c_str());
            auto horig = dynamic_cast<TH1*>(forig.Get(n.c_str()));
            horig->Divide(hdist);
            fout.cd();
            horig->Write();
        }
    }
}

// vim: filetype=cpp
