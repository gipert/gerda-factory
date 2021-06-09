//usr/bin/env root -l -b -x -q ${0}\(\""$*"\"\); exit $?

void Pb214Bi214_distortion(std::string /*arguments*/) {

    std::string dirname;
    std::vector<TH1*> histos;

    for (auto& s : std::vector<std::string>{"raw/M1_ch0"}) {
        dirname = s.substr(0, s.find_last_of('/'));
        TFile forig2("gerda-pdfs/ar39-study/tl-reference/cables/cables_all/Pb214/pdf-cables-cables_all-Pb214.root");
        TFile forig1("gerda-pdfs/ar39-study/tl-reference/cables/cables_all/Bi214/pdf-cables-cables_all-Bi214.root");
        auto _horig = dynamic_cast<TH1*>(forig2.Get(s.c_str()));
        auto horig = dynamic_cast<TH1*>(forig1.Get(s.c_str()));

        horig->Add(_horig, 0.3539);
/*
	TFile fdist2("gerda-pdfs/ar39-study/tl-reference/ge_holders/ge_holders_all/Pb214/pdf-ge_holders-ge_holders_all-Pb214.root");
        TFile fdist1("gerda-pdfs/ar39-study/tl-reference/ge_holders/ge_holders_all/Bi214/pdf-ge_holders-ge_holders_all-Bi214.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetDirectory(nullptr);

        hdist->Add(_hdist, 0.3539);
	
	TFile fdist2("gerda-pdfs/ar39-study/tl-reference/minishroud/ms_all/Pb214/pdf-minishroud-ms_all-Pb214.root");
        TFile fdist1("gerda-pdfs/ar39-study/tl-reference/minishroud/ms_all/Bi214/pdf-minishroud-ms_all-Bi214.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetDirectory(nullptr);

        hdist->Add(_hdist, 0.3539);
	
	TFile fdist2("gerda-pdfs/ar39-study/tl-reference/larveto/outer_fibers/Pb214/pdf-larveto-outer_fibers-Pb214.root");
        TFile fdist1("gerda-pdfs/ar39-study/tl-reference/larveto/outer_fibers/Bi214/pdf-larveto-outer_fibers-Bi214.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetDirectory(nullptr);

        hdist->Add(_hdist, 0.3539);
*/
        TFile fdist2("gerda-pdfs/ar39-study/tl-reference/larveto/inner_fibers/Pb214/pdf-larveto-inner_fibers-Pb214.root");
        TFile fdist1("gerda-pdfs/ar39-study/tl-reference/larveto/inner_fibers/Bi214/pdf-larveto-inner_fibers-Bi214.root");
        auto _hdist = dynamic_cast<TH1*>(fdist2.Get(s.c_str()));
        auto hdist = dynamic_cast<TH1*>(fdist1.Get(s.c_str()));
        hdist->SetDirectory(nullptr);

        hdist->Add(_hdist, 0.3539);

        std::cout << "INFO: computing Pb-214-Bi214 distortion (" << s << "s" << std::endl;
        hdist->Divide(horig);
        hdist->SetName(s.substr(s.find_last_of('/')+1).c_str());
        histos.push_back(hdist);
    }

    auto outname = "distortions/Ar/pdf-Pb214_Bi214-cables_vs_inner_fibers.root";
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
