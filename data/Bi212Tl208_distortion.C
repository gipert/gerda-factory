//usr/bin/env root -l -b -x -q ${0}\(\""$*"\"\); exit $?

void Bi212Tl208_distortion(std::string /*arguments*/) {

    std::vector<TH1*> histos;

    for (auto& s : std::vector<std::string>{"M1_enrBEGe", "M1_enrCoax"}) {
        TFile forig1("gerda-pdfs/gerda-pdfs-best/cables/cables_all/Bi212/pdf-cables-cables_all-Bi212-lar.root");
        TFile forig2("gerda-pdfs/gerda-pdfs-best/cables/cables_all/Tl208/pdf-cables-cables_all-Tl208-lar.root");
        auto _horig = dynamic_cast<TH1*>(forig2.Get(s.c_str()));
        _horig->SetName((s + "_Tl208_orig").c_str());
        auto horig = dynamic_cast<TH1*>(forig1.Get(s.c_str()));
        horig->SetName((s + "_orig").c_str());
        horig->SetDirectory(nullptr);

        horig->Add(_horig, 0.3539);

        TFile fdist1("gerda-pdfs/gerda-pdfs-best/larveto/fibers/Bi212/pdf-larveto-fibers-Bi212-lar.root");
        TFile fdist2("gerda-pdfs/gerda-pdfs-best/larveto/fibers/Tl208/pdf-larveto-fibers-Tl208-lar.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        _hdist->SetName((s + "_Tl208_dist").c_str());
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetName((s + "_dist").c_str());

        hdist->Add(_hdist, 0.3539);

        std::cout << "INFO: computing Bi212-Tl208 distortion (" << s << "s" << std::endl;
        horig->Divide(hdist);
        horig->SetName(s.c_str());
        histos.push_back(horig);
    }

    auto outname = "distortions/pdf-Bi212_Tl208-cables_vs_fibers-lar.root";
    std::cout << "INFO: saving to " << outname << std::endl;
    TFile fout(outname, "recreate");
    for (auto& h : histos) h->Write();
}
