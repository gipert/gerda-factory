//usr/bin/env root -l -b -x -q ${0}\(\""$*"\"\); exit $?

void Bi212Tl208_distortion(std::string /*arguments*/) {

    std::string dirname;
    std::vector<TH1*> histos;

    for (auto& s : std::vector<std::string>{"lar/M1_enrBEGe", "lar/M1_enrCoax"}) {
        dirname = s.substr(0, s.find_last_of('/'));
        TFile forig1("gerda-pdfs/gerda-pdfs-2nufit-best/cables/cables_all/Bi212/pdf-cables-cables_all-Bi212.root");
        TFile forig2("gerda-pdfs/gerda-pdfs-2nufit-best/cables/cables_all/Tl208/pdf-cables-cables_all-Tl208.root");
        auto _horig = dynamic_cast<TH1*>(forig2.Get(s.c_str()));
        auto horig = dynamic_cast<TH1*>(forig1.Get(s.c_str()));

        horig->Add(_horig, 0.3539);

        TFile fdist1("gerda-pdfs/gerda-pdfs-2nufit-best/larveto/outer_fibers/Bi212/pdf-larveto-outer_fibers-Bi212.root");
        TFile fdist2("gerda-pdfs/gerda-pdfs-2nufit-best/larveto/outer_fibers/Tl208/pdf-larveto-outer_fibers-Tl208.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetDirectory(nullptr);

        hdist->Add(_hdist, 0.3539);

        std::cout << "INFO: computing Bi212-Tl208 distortion (" << s << "s" << std::endl;
        hdist->Divide(horig);
        hdist->SetName(s.substr(s.find_last_of('/')+1).c_str());
        histos.push_back(hdist);
    }

    auto outname = "distortions/pdf-Bi212_Tl208-cables_vs_fibers.root";
    std::cout << "INFO: saving to " << outname << std::endl;
    TFile fout(outname, "recreate");
    for (auto& h : histos) {
        if (!fout.GetDirectory(dirname.c_str())) {
            fout.mkdir(dirname.c_str());
        }
        fout.cd(dirname.c_str());
        h->Write();
    }
}
