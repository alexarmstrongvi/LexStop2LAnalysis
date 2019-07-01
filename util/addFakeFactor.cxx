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

// std
#include <algorithm>
using std::max;

// ROOT
#include "TSystem.h"
#include "TFile.h"

// xAOD
#include "xAODEventInfo/EventInfo.h"
#include "xAODEventInfo/EventAuxInfo.h"
#include "xAODBase/IParticleContainer.h"
#include "xAODEgamma/Electron.h"
#include "xAODMuon/Muon.h"

using namespace asg::msgUserCode;

////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
const float GeVtoMeV = 1000.0;
asg::AnaToolHandle<CP::ILinearFakeBkgTool> m_fake_tool;
CP::SystematicSet m_sysvars;
// Process = fake processes one aims to model with data-driven estimates 
// Questionable if fake factor method works for estimating anything other than
// all events with >=1 fake lepton(s) passing ID requirements (i.e. ">=F[T]")
string m_process = ">=1F[T]";

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

    std::unique_ptr<xAOD::EventInfo> eventInfo = std::make_unique<xAOD::EventInfo>();
    std::unique_ptr<xAOD::EventAuxInfo> eventAuxInfo = std::make_unique<xAOD::EventAuxInfo>();
    eventInfo->setStore(eventAuxInfo.get());

    if (!initialize_fakefactor_tool(args.fake_name, args.selection)) {
        cerr << "ERROR :: Unable to initialize fake factor tool\n";
        return 1;
    } else {
        cout << "INFO :: Fake background tool initialized\n";
        m_sysvars = m_fake_tool->affectingSystematics();
    }
    if (args.debug) {
        cout << "DEBUG :: Considering the following systematics:\n";
        uint count = 0;
        for(const CP::SystematicVariation& sysvar : m_sysvars) {
            cout << "DEBUG :: \t(" << ++count << ") " << sysvar.name() << '\n';
            m_fake_tool->getSystDescriptor().printUncertaintyDescription(sysvar);
        }
    }

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
        xAOD::IParticleContainer aod_leptons(SG::VIEW_ELEMENTS);
        for (const leptonProperties& lep : leptons) { 
            // Only pass ID and Anti-ID leptons to fake tool
            if (lep.recoStatus != RecoStatus::ID && lep.recoStatus != RecoStatus::ANTIID) {
                continue;
            }
            aod_leptons.push_back(to_iparticle(lep)); 
        }

        // Get fake weights and error
        ANA_CHECK( m_fake_tool->addEvent(aod_leptons) );
        if (set_fake_weight(fakeweight, args.selection) != StatusCode::SUCCESS) {
            cerr << "ERROR :: Failed to get fake factor event weight\n";
            return 3;
        }
        if (set_stat_error(syst_FAKEFACTOR_Stat, fakeweight, args.selection) != StatusCode::SUCCESS) {
            cerr << "ERROR :: Failed to get fake factor event weight statistical error\n";
            return 3;
        }
        if (set_syst_error(syst_FAKEFACTOR_Syst, fakeweight, args.selection) != StatusCode::SUCCESS) {
            cerr << "ERROR :: Failed to get fake factor event weight systematic error\n";
            return 3;
        }
        //for (const xAOD::IParticle* p : aod_leptons) { delete p; }

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

StatusCode initialize_fakefactor_tool(string input_file, string /*selection*/) {
    m_fake_tool = asg::AnaToolHandle<CP::ILinearFakeBkgTool>("CP::ApplyFakeFactor/FakeTool");
    vector<string> vec = {input_file};
    ANA_CHECK( m_fake_tool.setProperty("InputFiles", vec) );
    ANA_CHECK( m_fake_tool.setProperty("EnergyUnit", "GeV") );
    //ANA_CHECK( m_fake_tool.setProperty("Selection", selection) ); // Seems to have no effect
    //ANA_CHECK( m_fake_tool.setProperty("Process", ">=1F[T]") ); // Seems to have no effect
    ANA_CHECK( m_fake_tool.initialize() );
    return StatusCode::SUCCESS;
}

xAOD::IParticle* to_iparticle(const leptonProperties& lp) { 
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

StatusCode set_fake_weight(double& wgt, string selection) {
    float tmp_wgt = 0;
    ANA_CHECK( m_fake_tool->applySystematicVariation({}) );
    ANA_CHECK( m_fake_tool->getEventWeight(tmp_wgt, selection, m_process) );
    wgt = tmp_wgt;
    return StatusCode::SUCCESS;
}

inline double add_in_quad(double x, double y) { return sqrt(x*x + y*y); }

StatusCode set_stat_error(double& stat_err, double nom_weight, string selection) {
    double err_down = 0;
    double err_up = 0;
    for(const CP::SystematicVariation& sysvar : m_sysvars) {
        ANA_CHECK( m_fake_tool->applySystematicVariation({sysvar}) );
        if (!m_fake_tool->getSystDescriptor().isStatisticalUncertainty(sysvar)) {
            continue;
        }

        float sys_weight;
        ANA_CHECK( m_fake_tool->getEventWeight(sys_weight, selection, m_process) );

        double diff = fabs(sys_weight - nom_weight);
        if (sys_weight > nom_weight) {
            err_down = add_in_quad(err_down, diff);
        } else if (sys_weight < nom_weight) {
            err_up = add_in_quad(err_up, diff);
        } 
        //cout << "VERBOSE :: "
        //     << "nom_weight = " << nom_weight << ", "
        //     << "sys_weight = " << sys_weight << ", "
        //     << "diff = " << diff << ", "
        //     << "updated stat error = +" << err_up << " -" << err_down << '\n';
    }

    // Determine total statistical error
    //stat_err = max(err_down, err_up); // Option 1
    stat_err = 0.5 * (err_down + err_down); // Option 2: 
    return StatusCode::SUCCESS;
}

StatusCode set_syst_error(double& syst_err, double nom_weight, string selection) {
    double err_down = 0;
    double err_up = 0;
    for(const CP::SystematicVariation& sysvar : m_sysvars) {
        ANA_CHECK( m_fake_tool->applySystematicVariation({sysvar}) );
        if (!m_fake_tool->getSystDescriptor().isSystematicUncertainty(sysvar)) {
            continue;
        }

        float sys_weight;
        ANA_CHECK( m_fake_tool->getEventWeight(sys_weight, selection, m_process) );

        double diff = fabs(sys_weight - nom_weight);
        if (sys_weight > nom_weight) {
            err_down = add_in_quad(err_down, diff);
        } else if (sys_weight < nom_weight) {
            err_up = add_in_quad(err_up, diff);
        } 
        //cout << "VERBOSE :: "
        //     << "nom_weight = " << nom_weight << ", "
        //     << "sys_weight = " << sys_weight << ", "
        //     << "diff = " << diff << ", "
        //     << "updated syst error = +" << err_up << " -" << err_down << '\n';
    }
    
    // Determine total statistical error
    //syst_err = max(err_down, err_up); // Option 1
    syst_err = 0.5 * (err_down + err_down); // Option 2: 
    return StatusCode::SUCCESS;
}

bool all_prompt(const vector<leptonProperties>& lps) {
    for (const auto& lp : lps) {
        if (lp.truthStatus != TruthStatus::PROMPT) return false;
    }
    return true;
}
