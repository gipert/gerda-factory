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
        auto hdist = dynamic_cast<TH1*>(fdist.Get("M1_enrBEGe"));
        hdist->SetName("M1_enrBEGe_dist");
        TFile forig(("gerda-pdfs/gerda-pdfs-latest/" + basepath).c_str());
        if (!forig.IsOpen()) continue;
        auto horig = dynamic_cast<TH1*>(forig.Get("M1_enrBEGe"));

        horig->Divide(hdist);

        TFile fout(outname.c_str(), "recreate");
        horig->Write();
        fout.Close();
    }
}

// vim: filetype=cpp
