//usr/bin/env root -l -b -x -q ${0}\(\""$*"\"\); exit $?

void take_ratio(std::string arguments) {
    std::istringstream ss(arguments);
    std::string item;

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
        hdist->SetName((item + "_dist").c_str());
        auto horig = dynamic_cast<TH1*>(forig.Get(item.c_str()));
        horig->SetDirectory(nullptr);

        horig->Divide(hdist);
        histos.push_back(horig);
        ss >> item;
    }

    std::cout << "INFO: saving to " << item << std::endl;
    TFile fout(item.c_str(), "recreate");
    for (auto& h : histos) h->Write();
}
