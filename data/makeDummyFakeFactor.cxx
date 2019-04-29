void makeDummyFakeFactor() {
    cout << "\n========================================\n";
    cout << "Making dummy fake factors\n";
    // Configurations
    string ofile_name = "fakeFactorDummy.root";
    float nom_val = 0.5;
    float stat_unc = 0.1;
    float sys_unc = 0.2;

    int nbins = 1;
    float xmin = 0;
    float xmax = 100;

    // Create file and histograms
    TFile *f = new TFile(ofile_name.c_str(), "RECREATE");
    TH1D h_mu("FakeFactor_mu_pt","", nbins, xmin, xmax);
    TH1D h_mu_syst("FakeFactor_mu_pt__Syst","", nbins, xmin, xmax);
    TH1D h_el("FakeFactor_el_pt","", nbins, xmin, xmax);
    TH1D h_el_syst("FakeFactor_el_pt__Syst","", nbins, xmin, xmax);

    // Set histograms
    h_mu.SetBinContent(1, nom_val);
    h_mu.SetBinError(1, stat_unc);
    h_mu_syst.SetBinContent(1, sys_unc);
    h_mu_syst.SetBinError(1, 0);

    h_el.SetBinContent(1, nom_val);
    h_el.SetBinError(1, stat_unc);
    h_el_syst.SetBinContent(1, sys_unc);
    h_el_syst.SetBinError(1, 0);

    // Save histograms to file
    f->Write();
    f->Close();
    cout << "\n========================================\n\n";
}
