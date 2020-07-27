//usr/bin/env root -l -b -x -q ${0}\(\""$*"\"\); exit $?

void take_ratio(std::string arguments) {
    std::istringstream ss(arguments);
    std::string item, dirname;

    ss >> item;
    std::cout << "INFO: dividing " << item;
    TFile forig(item.c_str());
    ss >> item;
    std::cout << " by " << item << std::endl;
    TFile fdist(item.c_str());

    std::vector<TH1*> histos;
    ss >> item;
    while (item.substr(item.size()-5, item.size()) != ".root") {
        std::cout << "INFO: computing " << item << std::endl;
        auto hdist = dynamic_cast<TH1*>(fdist.Get(item.c_str()));
        dirname = item.substr(0, item.find_last_of('/'));
        hdist->SetName((item + "_dist").c_str());
        auto horig = dynamic_cast<TH1*>(forig.Get(item.c_str()));
        horig->SetDirectory(nullptr);

        horig->Divide(hdist);
        horig->SetName((item.substr(item.find_last_of('/')+1)).c_str());
        histos.push_back(horig);
        ss >> item;
    }

    std::cout << "INFO: saving to " << item << std::endl;
    TFile fout(item.c_str(), "recreate");
    for (auto& h : histos) {
        if (!fout.GetDirectory(dirname.c_str())) {
            fout.mkdir(dirname.c_str());
        }
        fout.cd(dirname.c_str());
        h->Write();
    }
}
