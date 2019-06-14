////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) <2019> by Alex Armstrong
///
/// @file addFakeFactor.cpp
/// @author Alex Armstrong <alarmstr@cern.ch>
/// @date 2019-05-23
/// @brief Add friend tree with fake weights to input file
///
////////////////////////////////////////////////////////////////////////////////
#include "LexStop2LAnalysis/addFakeFactor.h"
#include "LexStop2LAnalysis/TreeHelper.h"

// ROOT
#include "TSystem.h"
#include "TFile.h"

// xAOD
#include "xAODBase/IParticle.h"
#include "xAODEgamma/Electron.h"
#include "xAODMuon/Muon.h"

// FakeBkdTools
#include "FakeBkgTools/FakeBkgHelpers.h"

////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
const float GeVtoMeV = 1000.0;

////////////////////////////////////////////////////////////////////////////////
/// @brief Command line argument parser
struct Args {
    ////////////////////////////////////////////////////////////////////////////
    // Initialize arguments
    ////////////////////////////////////////////////////////////////////////////
    string PROG_NAME;  // always required

    // Required arguments
    string ifile_name;  ///< Input file name
    string ttree_name;  ///< TTree name
    string fake_name;  ///< Fake factor file for FakeBkgTools
    string selection; ///< selection option for FakeBkgTools

    // Optional arguments with defaults
    string fake_tree_name = "fakeFactorInfo"; ///< Name of added friend tree with fake info
    bool test = false;  ///< Run test without modifying file
    bool debug = false;  ///< Run in debug mode

    ////////////////////////////////////////////////////////////////////////////
    // Print information
    ////////////////////////////////////////////////////////////////////////////
    void print_usage() const {
        printf("===========================================================\n");
        printf(" %s\n", PROG_NAME.c_str());
        printf(" Add friend tree with fake weights to input file\n");
        printf("===========================================================\n");
        printf("Required Parameters:\n");
        printf("\t-i, --input       Input file name\n");
        printf("\t-t, --tree        TTree name\n");
        printf("\t-f, --fake-file   Fake factor file for FakeBkgTools\n");
        printf("\t-s, --selection   selection option for FakeBkgTools\n");
        printf("\nOptional Parameters:\n");
        printf("\t--ftree           Name of added friend tree with fake info [%s]\n", fake_tree_name.c_str());
        printf("\t--test            Run test without modifying file [%d]\n", test);
        printf("\t--debug           Run in debug mode [%d]\n", debug);
        printf("\t-h, --help        Print this help message\n");
        printf("===========================================================\n");
    }

    void print() const {
        printf("===========================================================\n");
        printf(" %s Configuration\n", PROG_NAME.c_str());
        printf("===========================================================\n");
        printf("\tInput file name: %s\n", ifile_name.c_str());
        printf("\tInput tree name: %s\n", ttree_name.c_str());
        printf("\tFake factor file name: %s\n", fake_name.c_str());
        printf("\tLepton selection: %s\n", selection.c_str());
        printf("\tOutput tree name: %s\n", fake_tree_name.c_str());
        printf("\ttest mode: %d\n", test);
        printf("\tdebug mode: %d\n", debug);
        printf("===========================================================\n");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Parser
    ////////////////////////////////////////////////////////////////////////////
    bool parse(int argc, char* argv[]) {
        PROG_NAME = argv[0];

        // Parse arguments
        for (int i = 0; i< argc; ++i) {
            // Grab arguments
            string arg = argv[i];
            string arg_value = argc > i+1 ? argv[i+1] : "";
            // Skip if arg set to arg value and not arg name
            if (arg.at(0) != '-') continue;

            // Set arguments
            if (arg == "-i" || arg == "--input") {
                ifile_name = arg_value;
            } else if (arg == "-t" || arg == "--tree") {
                ttree_name = arg_value;
            } else if (arg == "-f" || arg == "--fake-file") {
                fake_name = arg_value;
            } else if (arg == "-s" || arg == "--selection") {
                selection = arg_value;
            } else if (arg == "--ftree") {
                fake_tree_name = arg_value;
            } else if (arg == "--test") {
                test = true;
            } else if (arg == "--debug") {
                debug = true;
            } else if (arg == "-h" || arg == "--help") {
                print_usage();
                return false;
            } else {
                cerr << "ERROR :: Unrecognized input argument: "
                     << arg << " -> " << arg_value << '\n';
                print_usage();
                return false;
            }
        }

        // Check arguments
        if (ifile_name.size() == 0) {
            cerr << "ERROR :: No input file given\n";
            return false;
        } else if (gSystem->AccessPathName(ifile_name.c_str())) {
            cerr << "ERROR :: Input file not found: " << ifile_name << "\n";
            return false;
        } else if (fake_name.size() == 0) {
            cerr << "ERROR :: No fake file given\n";
            return false;
        } else if (gSystem->AccessPathName(fake_name.c_str())) {
            cerr << "ERROR :: Fake file not found: " << fake_name << "\n";
            return false;
        } else if (selection.size() == 0) {
            cerr << "ERROR :: No lepton selection provided [-s] \n";
        }
        return true;
    }
} args;

////////////////////////////////////////////////////////////////////////////////
/// @brief Main function
///
/// Run with help option (-h, --help) to see available parameters
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    cout << "\n====== RUNNING " << argv[0] << " ====== \n";
    if (args.parse(argc, argv)) {
        // Finished parsing arguments
        args.print();
    } else {
        // Failed to parse arguments or help requested
        return 3;
    }
    
    // Process = fake processes one aims to model with data-driven estimates 
    // Questionable if fake factor method works for estimating anything other than
    // all events with >=1 fake lepton(s) passing ID requirements (i.e. ">=F[T]")
    string process = ">=1F[T]";
    // Selection = final reco state for which one wants fake estimates
    string selection = args.selection;

    ////////////////////////////////////////////////////////////////////////////
    // Main implementation
    ////////////////////////////////////////////////////////////////////////////
    // Initialize
    string file_mode = args.test ? "READ" : "UPDATE";
    TFile ifile(args.ifile_name.c_str(), file_mode.c_str());
    if (ifile.IsZombie()) {
        cerr << "ERROR :: Input file was not opened successfully: " << args.ifile_name << "\n";
        return 3;
    } else {
        ifile.cd();
    }
    TTree* itree = (TTree*)ifile.Get(args.ttree_name.c_str());
    TreeHelper tree_helper(itree);
    tree_helper.initialize();

    ApplyFakeFactor fake_tool;
    if (!initialize_fakefactor_tool(fake_tool, args.fake_name)) {
        cerr << "ERROR :: Unable to initialize fake factor tool\n";
        return 1;
    } else {
        cout << "INFO :: Fake background tool initialized\n";
    }
    FakeBkgTools::Weight fake_wgt;

    TTree fake_tree(args.fake_tree_name.c_str(), "");
    double fakeweight = 0; 
    fake_tree.Branch(m_fakeweight_branch_name.c_str(), &fakeweight);
    double syst_FAKEFACTOR_Stat = 0; 
    fake_tree.Branch(m_fakeweight_stat_err_branch_name.c_str(), &syst_FAKEFACTOR_Stat);
    double syst_FAKEFACTOR_Syst = 0; 
    fake_tree.Branch(m_fakeweight_syst_err_branch_name.c_str(), &syst_FAKEFACTOR_Syst);
    
    ////////////////////////////////////////////////////////////////////////////    
    // Loop over input tree
    for (Int_t i=0; i < tree_helper.tr().GetEntries(); i++) {
        tree_helper.clear();
        tree_helper.tr().GetEntry(i);
        if (args.debug) {
            cout << "\n=============================================================\n";
            cout << "DEBUG :: Processing event " << tree_helper.event_number() << '\n';
        }

        // Build vector of xAOD leptons
        const vector<leptonProperties>& leptons = tree_helper.leptons();
        vector<const xAOD::IParticle*> aod_leptons;
        for (const leptonProperties& lep : leptons) { 
            // Only pass ID and Anti-ID leptons to fake tool
            if (lep.recoStatus != RecoStatus::ID && lep.recoStatus != RecoStatus::ANTIID) {
                continue;
            }
            aod_leptons.push_back(to_iparticle(lep)); 
        }

        // Get fake weights and error
        fake_tool.addEvent(aod_leptons);
        if (fake_tool.getEventWeight(fake_wgt, selection, process) != StatusCode::SUCCESS) { 
            cerr << "ERROR: ApplyFakeFactor::getEventWeight() failed\n";
            return 3;
        } else {
            for (const xAOD::IParticle* p : aod_leptons) { delete p; }
        }
        fakeweight = get_fake_weight(fake_tool, fake_wgt);
        syst_FAKEFACTOR_Stat = get_stat_error(fake_tool, fake_wgt, fakeweight);
        syst_FAKEFACTOR_Syst = get_syst_error(fake_tool, fake_wgt, fakeweight);

        if (tree_helper.isMC()) {
            if (all_prompt(leptons)) {
                fakeweight *= -1;
            } else {
                fakeweight = 0;
                syst_FAKEFACTOR_Stat = 0;
                syst_FAKEFACTOR_Syst = 0;
            }
        }

        if (args.debug) {
            cout << "DEBUG :: nLeptons = " << leptons.size() << "; isMC = " << tree_helper.isMC() << '\n';
            cout << "DEBUG :: Fake weight = " << fakeweight << " "
                 << "+/- " << syst_FAKEFACTOR_Stat << " (stat) "
                 << "+/- " << syst_FAKEFACTOR_Syst << " (syst);\n";
            uint ii = 0;
            for (const leptonProperties& lep : leptons) { 
                cout << "DEBUG :: Lepton " << ++ii << ") " << lep.info_str() << '\n';
            }
        }

        fake_tree.Fill();
    }

    if (!args.test) { 
        ifile.cd();
        fake_tree.Write("", TObject::kOverwrite);
        tree_helper.tr().AddFriend(args.fake_tree_name.c_str());
        tree_helper.tr().Write("",TObject::kOverwrite);
    }
    ifile.Close();

    cout << "\n====== SUCCESSFULLY RAN " << argv[0] << " ====== \n";
    return 0;
}

bool initialize_fakefactor_tool(ApplyFakeFactor& tool, string input_file) {
    tool.setProperty("InputFiles", input_file);
    tool.setProperty("EnergyUnit", "GeV");
    //tool.setProperty("OutputLevel", );
    //tool.setProperty("SkipUncertainties", true);
    //tool.setProperty("ConvertWhenMissing", true);
    return tool.initialize().isSuccess();
}

const xAOD::IParticle* to_iparticle(const leptonProperties& lp) { 
    if (lp.isEle) {
        xAOD::Electron* e = new xAOD::Electron();
        e->makePrivateStore();
        e->setP4(lp.Pt * GeVtoMeV, lp.Eta, lp.Phi, lp.M);
        e->setCharge(lp.q);
        e->auxdata<char>("Tight") = lp.recoStatus == RecoStatus::ID;
        return static_cast<xAOD::IParticle*>(e);
    } else {
        xAOD::Muon* m = new xAOD::Muon();
        m->makePrivateStore();
        m->setP4(lp.Pt * GeVtoMeV, lp.Eta, lp.Phi);
        m->setCharge(lp.q);
        m->auxdata<char>("Tight") = lp.recoStatus == RecoStatus::ID;
        return static_cast<xAOD::IParticle*>(m);
    }
}
double get_fake_weight(const ApplyFakeFactor& /*tool*/, FakeBkgTools::Weight& wgt) {
    return wgt.value;
}

double get_stat_error(const ApplyFakeFactor& tool, FakeBkgTools::Weight& wgt, double nom) {
    float rel_unc_sq = 0;
    for(auto& kv : wgt.uncertainties) {
        if (tool.isStatisticalUncertainty(kv.first)) {
            double sym_unc = 0.5 * (fabs(kv.second.up) + fabs(kv.second.down));
            double rel_unc = sym_unc / nom;
            rel_unc_sq += rel_unc * rel_unc;
        }
    }
    return sqrt(rel_unc_sq) * fabs(nom);
}

double get_syst_error(const ApplyFakeFactor& tool, FakeBkgTools::Weight& wgt, double nom) {
    double rel_unc_sq = 0;
    for(auto& kv : wgt.uncertainties) {
        if (tool.isSystematicUncertainty(kv.first)) {
            double sym_unc = 0.5 * (fabs(kv.second.up) + fabs(kv.second.down));
            double rel_unc = sym_unc / nom;
            rel_unc_sq += rel_unc * rel_unc;
        }
    }
    return sqrt(rel_unc_sq) * fabs(nom);
}

bool all_prompt(const vector<leptonProperties>& lps) {
    for (const auto& lp : lps) {
        if (lp.truthStatus != TruthStatus::PROMPT) return false;
    }
    return true;
}
