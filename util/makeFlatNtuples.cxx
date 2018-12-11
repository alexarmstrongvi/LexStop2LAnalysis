////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) <2018> by Alex Armstrong
///
/// @file makeFlatNtuple.cxx
/// @author Alex Armstrong <alarmstr@cern.ch>
/// @date <December 2018>
/// @brief Make flat ntuples from SusyNts
///
////////////////////////////////////////////////////////////////////////////////
// TODO:
// - Fix dilep trigger matching
// - Improve all the variable nameing schemes

#include "LexStop2LAnalysis/makeFlatNtuples.h"
#include "SusyNtuple/KinematicTools.h"

using std::string;
using std::cout;
using std::cerr;
using std::vector;
using std::find;
using kin::getMetRel;

// Various superflow classes
using namespace sflow;

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////
// Useful macros
#define ADD_1LEP_TRIGGER_VAR(trig_name, leptons) { \
    *superflow << NewVar(#trig_name" trigger bit"); { \
        *superflow << HFTname(#trig_name); \
        *superflow << [=](Superlink* sl, var_bool*) -> bool { \
            return is_1lep_trig_matched(sl, #trig_name, leptons); \
        }; \
        *superflow << SaveVar(); \
    } \
}

// Trig Matching for dilep triggers is buggy
// so currently not trigger matching
#define ADD_2LEP_TRIGGER_VAR(trig_name, lep0, lep1) { \
  *superflow << NewVar(#trig_name" trigger bit"); { \
      *superflow << HFTname(#trig_name); \
      *superflow << [=](Superlink* sl, var_bool*) -> bool { \
          if(!lep0 || ! lep1) return false;\
          bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, #trig_name); \
          return trig_fired;\
          if (!trig_fired) return false; \
          bool trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep0, lep1, #trig_name);\
          return trig_matched; }; \
      *superflow << SaveVar(); \
  } \
}
#define ADD_MULTIPLICITY_VAR(container) { \
  *superflow << NewVar("number of "#container); { \
    *superflow << HFTname("n_"#container); \
    *superflow << [=](Superlink* sl, var_int*) -> int { \
      return sl->container->size(); \
    }; \
    *superflow << SaveVar(); \
  } \
}

/// @brief Command line argument parser
struct Args {
    ////////////////////////////////////////////////////////////////////////////
    // Initialize arguments
    ////////////////////////////////////////////////////////////////////////////
    string PROG_NAME;  // always required

    // Required arguments
    string input_name;  ///< Input file name

    // Optional arguments with defaults
    unsigned int n_events  = -1; ///< number of events processed
    unsigned int n_skipped = 0; ///< number of initial entries to skip
    string name_suffix = ""; ///< suffix appended to output name
    bool baseline_sel = false; ///< Apply baseline selection
    bool zll_cr = false; ///< Apply Zll CR selection

    ////////////////////////////////////////////////////////////////////////////
    // Print information
    ////////////////////////////////////////////////////////////////////////////
    void print_usage() const {
        printf("===========================================================\n");
        printf(" %s\n", PROG_NAME.c_str());
        printf(" Makeing analysis flat ntuples from SusyNts\n");
        printf("===========================================================\n");
        printf("Required Parameters:\n");
        printf("\t-i, --input     Input file name\n");
        printf("\nOptional Parameters:\n");
        printf("\t-n, --nevents   number of events to process\n");
        printf("\t-k, --nskipped  number of initial entries to skip\n");
        printf("\t-s, --suffix    suffix appended to output name\n");
        printf("\t--baseline_sel  use baseline event selection \n");
        printf("\t--zll_cr       use Zll event selection \n");
        printf("\t-h, --help      print this help message\n");
        printf("===========================================================\n");
    }

    void print() const {
        printf("===========================================================\n");
        printf(" %s Configuration\n", PROG_NAME.c_str());
        printf("===========================================================\n");
        printf("\tInput file name : %s\n", input_name.c_str());
        printf("\tnevents         : %i\n", n_events);
        printf("\tnskipped        : %i\n", n_skipped);
        printf("\tsuffix          : %s\n", name_suffix.c_str());
        printf("\tbaseline_sel    : %i\n", baseline_sel);
        printf("\tzll_cr          : %i\n", zll_cr);
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

            // Check for required arguments
            if (arg == "-i" || arg == "--input") {
                input_name = arg_value;
            } else if (arg == "-n" || arg == "--nevents") {
                n_events = atoi(arg_value.c_str());
            } else if (arg == "-k" || arg == "--nskipped") {
                n_skipped = atoi(arg_value.c_str());
            } else if (arg == "-s" || arg == "--suffix") {
                name_suffix = arg_value;
            } else if (arg == "--baseline_sel") {baseline_sel = true;
            } else if (arg == "--zll_cr") {zll_cr = true;
            } else if (arg == "-h" || arg == "--help") {
                print_usage();
                return false;
            } else {
                cerr << "ERROR :: Unrecognized input argument: "
                     << arg << " -> " << arg_value << '\n';
                print_usage();
            }
        }

        // Check arguments
        if (input_name.size() == 0) {
            cerr << "ERROR :: No input source given\n";
            return false;
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
        return 1;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Main implementation
    ////////////////////////////////////////////////////////////////////////////
    // Build list of cutflows to run
    setup_chain(m_chain, args.input_name);
    printf("makeFlatNtuple :: Total events available : %lli\n",m_chain->GetEntries());

    // Run the chosen cutflows
    if (args.baseline_sel) {
        cout << "\n\n Running baseline cutflow \n\n";
        run_superflow(BASELINE_SEL);
    }
    if (args.zll_cr) {
        cout << "\n\n Running ZLL CR cutflow \n\n";
        run_superflow(ZLL_CR);
    }

    delete m_chain;
    cout << "\n====== SUCCESSFULLY RAN " << argv[0] << " ====== \n";
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
void setup_chain(TChain* chain, string iname) {
    chain->SetDirectory(0);  // Remove ROOT ownership

    bool inputIsFile = Susy::utils::endswith(iname, ".root");
    bool inputIsList = Susy::utils::endswith(iname, ".txt");
    bool inputIsDir  = Susy::utils::endswith(iname, "/");

    if (inputIsFile) {
        ChainHelper::addFile(chain, iname);
    } else if (inputIsList) {
      // If a list of ROOT files
        ChainHelper::addFileList(chain, iname);
    } else if (inputIsDir) {
        ChainHelper::addFileDir(chain, iname);
    } else {
        printf("ERROR (initialize_chain) :: Unrecognized input %s", iname.c_str());
    }
}

void run_superflow(Sel sel_type) {
    Superflow* sf = get_cutflow(m_chain, sel_type);
    m_chain->Process(sf, args.input_name.c_str(), args.n_events, args.n_skipped);
    delete sf; sf = 0;
}

Superflow* get_cutflow(TChain* chain, Sel sel_type) {
    string name_suffix = determine_suffix(args.name_suffix, sel_type);

    Superflow* superflow = initialize_superflow(chain, name_suffix);

    // How to add cutflow entry:
    // Create lambda function that returns bool value of cut.
    // Pass that with "<<" into the function CutName("Cut name").
    // Pass that with "<<" into the dereferenced cutflow object.

    // *IMPORTANT* The order that cuts are added is very important as that is
    // order in which cuts are applied and global superflow variables filled

    ////////////////////////////////////////////////////////////////////////////
    // Define helpful variables for use inside superflow variable definitions
    add_shortcut_variables(superflow, sel_type);

    ////////////////////////////////////////////////////////////////////////////
    // Add cuts
    add_cleaning_cuts(superflow);
    if (sel_type == BASELINE_SEL) {
        add_baseline_lepton_cuts(superflow);
    } else if (sel_type == ZLL_CR) {
        add_zll_cr_lepton_cuts(superflow);
    }
    add_final_cuts(superflow);

    ////////////////////////////////////////////////////////////////////////////
    // Add superflow variables
    add_event_variables(superflow);
    add_trigger_variables(superflow);
    add_met_variables(superflow);
    add_prelepton_variables(superflow);
    add_baselepton_variables(superflow);
    add_signallepton_variables(superflow);
    add_other_lepton_variables(superflow);
    add_jet_variables(superflow);

    return superflow;
}

// TODO: Switch to using references instead of pointers
string determine_suffix(string user_suffix, Sel sel_type) {
    string full_suffix = user_suffix;

    if (user_suffix != "") full_suffix += "_";

    if (sel_type == BASELINE_SEL) full_suffix += "baseline";
    else if (sel_type == ZLL_CR) full_suffix += "zll_cr";

    return full_suffix;
}
Superflow* initialize_superflow(TChain *chain, string name_suffix) {
    // Move run_mode to globals

    Superflow* superflow = new Superflow();        // initialize the cutflow
    superflow->setAnaName("SuperflowAna");         // arbitrary
    superflow->setAnaType(AnalysisType::Ana_Stop2L); // analysis type, passed to SusyNt
    superflow->setLumi(1.0);                       // set the MC normalized to X pb-1
    superflow->setSampleName(args.input_name);     // sample name, check to make sure it's set OK
    superflow->setRunMode(m_run_mode);             // make configurable via run_mode
    superflow->setCountWeights(true);              // print the weighted cutflows
    cout << "Added suffix = " << name_suffix << '\n';
    if(name_suffix != "") superflow->setFileSuffix(name_suffix);
    superflow->setChain(chain);
    superflow->nttools().initTriggerTool(ChainHelper::firstFile(args.input_name, 0.));

    return superflow;
}


////////////////////////////////////////////////////////////////////////////////
// Add lambda expression to superflow
void add_shortcut_variables(Superflow* superflow, Sel /*sel_type*/) {
    *superflow << CutName("read in") << [=](Superlink* sl) -> bool {
        // Reset all shortcut variables
        m_cutflags = 0;
        m_MET = {};
        m_selectLeptons.clear(); m_triggerLeptons.clear();
        m_dileptonP4 = m_lepton0 = m_lepton1 = {};
        m_el0 = m_el1 = 0;
        m_mu0 = m_mu1 = 0;
        m_lightJets.clear(); m_BJets.clear(); m_forwardJets.clear();
        m_Dijet_TLV = m_Jet1_TLV = m_Jet0_TLV = {};
        
        ///////////////////////////////////////////////////////////////////////
        // Fill shortcut variables
        m_cutflags = sl->nt->evt()->cutFlags[NtSys::NOM];

        // Seperate superlink lepton container from the leptons used for variables
        // so that the order can be changed if needed
        m_selectLeptons = *sl->leptons;
        if (m_selectLeptons.size() > 0 && m_selectLeptons.at(0)) {
            m_triggerLeptons.push_back(m_selectLeptons.at(0));
        }
        if (m_selectLeptons.size() > 1 && m_selectLeptons.at(1)) {
            m_triggerLeptons.push_back(m_selectLeptons.at(1));
        }

        int n_selectleps = 0;
        for (Susy::Lepton* lep : m_selectLeptons) {
            if (!lep) {
                continue;
                
            }
            n_selectleps++;


            // For dilepton trigger matching
            if (lep->isEle()) {
                if (!m_el0) m_el0 = dynamic_cast<Susy::Electron*>(lep);
                else if (!m_el1) m_el1 = dynamic_cast<Susy::Electron*>(lep);
            }
            else if (lep->isMu()) {
                if (!m_mu0) m_mu0 = dynamic_cast<Susy::Muon*>(lep);
                else if (!m_mu1) m_mu1 = dynamic_cast<Susy::Muon*>(lep);
            }
        }

        if (m_selectLeptons.size() > 0 && m_selectLeptons.at(0)) {
            m_lepton0 = *m_selectLeptons.at(0);
        }
        if (m_selectLeptons.size() > 1 && m_selectLeptons.at(1)) {
            m_lepton1 = *m_selectLeptons.at(1);
        }
        m_dileptonP4 = m_lepton0 + m_lepton1;

        ////////////////////////////////////////////////////////////////////////
        // Jet shortcuts
        if (sl->baseJets->size() > 0) {
            m_Jet0_TLV = *sl->baseJets->at(0);
            if (sl->baseJets->size() > 1) {
               m_Jet1_TLV = *sl->baseJets->at(1);
               m_Dijet_TLV = m_Jet0_TLV + m_Jet1_TLV;
            }
        }
        // TODO: replace auto with explicit class
        for (Susy::Jet* jet : *sl->baseJets) {
              if (sl->tools->jetSelector().isB(jet)) {
                  m_BJets.push_back(jet);
              } else if (sl->tools->jetSelector().isForward(jet))  {
                  m_forwardJets.push_back(jet);
              } else {
                   m_lightJets.push_back(jet);
              }
        }
        std::sort(m_lightJets.begin()  , m_lightJets.end()  , comparePt);
        std::sort(m_BJets.begin()      , m_BJets.end()      , comparePt);
        std::sort(m_forwardJets.begin(), m_forwardJets.end(), comparePt);
        return true; // All events pass this cut
    };
}
// TODO: Replace [=] with only the needed variables to see if it improves performance
void add_cleaning_cuts(Superflow* superflow) {
  *superflow << CutName("Pass GRL") << [=](Superlink* sl) -> bool {
      return (sl->tools->passGRL(m_cutflags));
  };
  *superflow << CutName("LAr error") << [=](Superlink* sl) -> bool {
      return (sl->tools->passLarErr(m_cutflags));
  };
  *superflow << CutName("Tile error") << [=](Superlink* sl) -> bool {
      return (sl->tools->passTileErr(m_cutflags));
  };
  *superflow << CutName("TTC veto") << [=](Superlink* sl) -> bool {
      return (sl->tools->passTTC(m_cutflags));
  };
  *superflow << CutName("SCT err") << [=](Superlink* sl) -> bool {
      return (sl->tools->passSCTErr(m_cutflags));
  };
}
void add_baseline_lepton_cuts(Superflow* superflow) {
    *superflow << CutName("2 Signal Leptons") << [=](Superlink* sl) -> bool {
        return (sl->leptons->size() == 2);
    };
    add_DFOS_lepton_cut(superflow);
}
void add_zll_cr_lepton_cuts(Superflow* superflow) {
    *superflow << CutName("2-ID Leptons") << [=](Superlink* sl) -> bool {
        return (sl->leptons->size() >= 2);
    };
    add_SFOS_lepton_cut(superflow);
}
void add_final_cuts(Superflow* superflow) {
    *superflow << CutName("pass good vertex") << [=](Superlink* sl) -> bool {
        return (sl->tools->passGoodVtx(m_cutflags));
    };
    *superflow << CutName("jet cleaning") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };
}

void add_event_variables(Superflow* superflow) {
  *superflow << NewVar("Event run number"); {
    *superflow << HFTname("RunNumber");
    *superflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->run; };
    *superflow << SaveVar();
  }
  *superflow << NewVar("Event number"); {
    *superflow << HFTname("event_number");
    *superflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->eventNumber; };
    *superflow << SaveVar();
  }
  *superflow << NewVar("is Monte Carlo"); {
    *superflow << HFTname("isMC");
    *superflow << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
    *superflow << SaveVar();
  }
  *superflow << NewVar("event weight"); {
    *superflow << HFTname("eventweight");
    *superflow << [=](Superlink* sl, var_double*) -> double {
        return sl->weights->product() * sl->nt->evt()->wPileup;
    };
    *superflow << SaveVar();
  }
  *superflow << NewVar("sample DSID"); {
    *superflow << HFTname("dsid");
    *superflow << [](Superlink* sl, var_int*) -> int {return sl->nt->evt()->mcChannel;};
    *superflow << SaveVar();
  }
  *superflow << NewVar("treatAsYear"); {
      // 15/16 Year ID
      *superflow << HFTname("treatAsYear");
      *superflow << [](Superlink* sl, var_double*) -> int { return sl->nt->evt()->treatAsYear; };
      *superflow << SaveVar();
  }
}
void add_trigger_variables(Superflow* superflow) {
    ////////////////////////////////////////////////////////////////////////////
    // Trigger Variables
    // ADD_*_TRIGGER_VAR preprocessor defined

    ////////////////////////////////////////////////////////////////////////////
    // 2015
    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_mu14, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e24_lhmedium_L1EM20VHI_mu8noL1, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_mu24, m_mu0, m_el0)
    ADD_2LEP_TRIGGER_VAR(HLT_2e12_lhloose_L12EM10VH, m_el0, m_el1)
    // TODO: Add to SusyNts HLT_2mu10)

    // Single Electron Triggers
    ADD_1LEP_TRIGGER_VAR(HLT_e24_lhmedium_L1EM20VH, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e120_lhloose, m_triggerLeptons)

    // Single Muon Triggers
    ADD_1LEP_TRIGGER_VAR(HLT_mu20_iloose_L1MU15, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu40, m_triggerLeptons)


    ////////////////////////////////////////////////////////////////////////////
    // 2016

    // Dilepton Triggers
    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_nod0_mu14, m_el0, m_mu0)
    // TODO: Add to SusyNts HLT_e24_lhmedium_nod0_L1EM20VHI_mu8noL1, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_nod0_mu24, m_mu0, m_el0)
    ADD_2LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0, m_el0, m_el1)
    // TODO: Add to SusyNts HLT_2mu14)

    // Single Electron Triggers
    ADD_1LEP_TRIGGER_VAR(HLT_e26_lhtight_nod0_ivarloose, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium_nod0, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e140_lhloose_nod0, m_triggerLeptons)

    // Single Muon Triggers
    ADD_1LEP_TRIGGER_VAR(HLT_mu26_ivarmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu50, m_triggerLeptons)
    //TODO : Add combined trigger flags 
}
void add_met_variables(Superflow* superflow) {
    //////////////////////////////////////////////////////////////////////////////
    // MET

    // Fill MET variable inside Et var
    *superflow << NewVar("transverse missing energy (Et)"); {
    *superflow << HFTname("MET");
    *superflow << [=](Superlink* sl, var_double*) -> double {
        m_MET.SetPxPyPzE(sl->met->Et*cos(sl->met->phi),
                       sl->met->Et*sin(sl->met->phi),
                       0.,
                       sl->met->Et);
        return m_MET.Pt();
    };
    *superflow << SaveVar();
    }
    *superflow << NewVar("transverse missing energy (Phi)"); {
    *superflow << HFTname("METPhi");
    *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        return m_MET.Phi();
    };
    *superflow << SaveVar();
    }
}
void add_prelepton_variables(Superflow* superflow) {
    ADD_MULTIPLICITY_VAR(preElectrons)
    ADD_MULTIPLICITY_VAR(preMuons)
    *superflow << NewVar("Pre-electron track pt"); {
      *superflow << HFTname("preel_trackPt");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->preElectrons) {
            if (lep) out.push_back(lep->trackPt);
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Pre-electron track eta"); {
      *superflow << HFTname("preel_trackEta");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->preElectrons) {
            if (lep) out.push_back(lep->trackEta);
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Pre-electron d0sigBSCorr"); {
      *superflow << HFTname("preel_d0sigBSCorr");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->preElectrons) {
            if (lep) out.push_back(lep->d0sigBSCorr);
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Pre-electron z0SinTheta"); {
      *superflow << HFTname("preel_z0SinTheta");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->preElectrons) {
            if (lep) out.push_back(lep->z0SinTheta());
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
}
void add_baselepton_variables(Superflow* superflow) {
    ADD_MULTIPLICITY_VAR(baseLeptons)
    ADD_MULTIPLICITY_VAR(baseElectrons)
    ADD_MULTIPLICITY_VAR(baseMuons)
}
void add_signallepton_variables(Superflow* superflow) {
    ADD_MULTIPLICITY_VAR(leptons)
    ADD_MULTIPLICITY_VAR(electrons)
    ADD_MULTIPLICITY_VAR(muons)
    //////////////////////////////////////////////////////////////////////////////
    // Signal Electrons
    *superflow << NewVar("Electron ID (non-inclusive)"); {
      *superflow << HFTname("El_ID");
      *superflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& el : *sl->electrons) {
          if (el->tightLLH) out.push_back(0);
          else if (el->mediumLLH) out.push_back(1);
          else if (el->looseLLHBLayer) out.push_back(2);
          else if (el->looseLLH) out.push_back(3);
          else if (el->veryLooseLLH) out.push_back(4);
          else out.push_back(5);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("subleading electron track pt"); {
      *superflow << HFTname("el1_track_pt");
      *superflow << [](Superlink* sl, var_double*) -> double {
        if (sl->leptons->size() > 1 && sl->leptons->at(1)->isEle()) {
            const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(sl->leptons->at(1));
            return ele ? ele->trackPt : -DBL_MAX;
        } else {
            return -DBL_MAX;
        }
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("subleading electron clus pt"); {
      *superflow << HFTname("el1_clus_pt");
      *superflow << [](Superlink* sl, var_double*) -> double {
        if (sl->leptons->size() > 1 && sl->leptons->at(1)->isEle()) {
            const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(sl->leptons->at(1));
            if (ele) {
               double p = sqrt(ele->clusE*ele->clusE - ele->M()*ele->M() ); 
               double aEta = std::abs( ele->clusEta );
               if( aEta > 710.0 ) {aEta = 710.0;} // this is what they do in CaloCluster_v1.cxx. Not sure why.
               double sinTh = 1.0 / std::cosh(ele->clusEta);
               return p * sinTh;
            }
            return -DBL_MAX;
        } else {
            return -DBL_MAX;
        }
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Subleading Electron pT track-cluster ratio"); {
      *superflow << HFTname("el1pT_trackclus_ratio");
      *superflow << [](Superlink* sl, var_double*) -> double {
        if (sl->leptons->size() > 1 && sl->leptons->at(1)->isEle()) {
            const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(sl->leptons->at(1));
            return ele ? ele->trackPt / ele->clusE : -DBL_MAX;
        } else {
            return -DBL_MAX;
        }
      };
      *superflow << SaveVar();
    }
    
    //////////////////////////////////////////////////////////////////////////////
    // Signal Muons
    *superflow << NewVar("Muon ID (non-inclusive)"); {
      *superflow << HFTname("Mu_ID");
      *superflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& mu : *sl->muons) {
          if (mu->tight) out.push_back(0);
          else if (mu->medium) out.push_back(1);
          else if (mu->loose) out.push_back(2);
          else if (mu->veryLoose) out.push_back(3);
          else if (!mu->veryLoose) out.push_back(4);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    
    //////////////////////////////////////////////////////////////////////////////
    // Signal Leptons
    *superflow << NewVar("Lepton is IsoGrad"); {
      *superflow << HFTname("l_IsoGrad");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& lep : m_selectLeptons) {
            if (!lep) continue;
            if (lep->isoGradient) out.push_back(0);
            else if (lep->isoGradientLoose) out.push_back(1);
            else out.push_back(2);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton0 Iso"); {
      *superflow << HFTname("l_iso0");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        if (m_selectLeptons.size() <= 0 || !m_selectLeptons.at(0)) return out;
        Susy::Lepton* lep = m_selectLeptons.at(0);
        out.push_back(-1);  // for tracking all entries and normalizing bins
        bool flag = false;
        if (lep->isoGradient)               { flag=true; out.push_back(0);}
        if (lep->isoGradientLoose)          { flag=true; out.push_back(1);}
        if (lep->isoLoose)                  { flag=true; out.push_back(2);}
        if (lep->isoLooseTrackOnly)         { flag=true; out.push_back(3);}
        if (lep->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);}
        if (!flag) out.push_back(5);
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton1 Iso"); {
      *superflow << HFTname("l_iso1");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        if (m_selectLeptons.size() <= 1 || !m_selectLeptons.at(1)) return out;
        Susy::Lepton* lep = m_selectLeptons.at(1);
        out.push_back(-1);  // for tracking all entries and normalizing bins
        bool flag = false;
        if (lep->isoGradient)               { flag=true; out.push_back(0);}
        if (lep->isoGradientLoose)          { flag=true; out.push_back(1);}
        if (lep->isoLoose)                  { flag=true; out.push_back(2);}
        if (lep->isoLooseTrackOnly)         { flag=true; out.push_back(3);}
        if (lep->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);}
        if (!flag) out.push_back(5);
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton2 Iso"); {
      *superflow << HFTname("l_iso2");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        if (m_selectLeptons.size() <= 2 || !m_selectLeptons.at(2)) return out;
        Susy::Lepton* lep = m_selectLeptons.at(2);
        out.push_back(-1);  // for tracking all entries and normalizing bins
        bool flag = false;
        if (lep->isoGradient)               { flag=true; out.push_back(0);}
        if (lep->isoGradientLoose)          { flag=true; out.push_back(1);}
        if (lep->isoLoose)                  { flag=true; out.push_back(2);}
        if (lep->isoLooseTrackOnly)         { flag=true; out.push_back(3);}
        if (lep->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);}
        if (!flag) out.push_back(5);
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton ID (non-inclusive)"); {
      *superflow << HFTname("l_ID");
      *superflow << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& lep : m_selectLeptons) {
            if (!lep) {
                out.push_back(-1);
            } else if (lep->isMu()) {
                const Susy::Muon* mu = dynamic_cast<const Susy::Muon*>(lep);
                if (mu->tight) out.push_back(0);
                else if (mu->medium) out.push_back(1);
                else if (mu->loose) out.push_back(2);
                else if (mu->veryLoose) out.push_back(3);
                else out.push_back(4);
            } else if (lep->isEle()) {
                const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(lep);
                if (ele->tightLLH) out.push_back(0);
                else if (ele->mediumLLH) out.push_back(1);
                else if (ele->looseLLHBLayer) out.push_back(2);
                else if (ele->looseLLH) out.push_back(3);
                else if (ele->veryLooseLLH) out.push_back(4);
                else out.push_back(5);
            }
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton Author"); {
      *superflow << HFTname("l_author");
      *superflow << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& lep : m_selectLeptons) {
            if (!lep) {
                out.push_back(-1);
            } else if (lep->isMu()) {
                out.push_back(-2);
            } else if (lep->isEle()) {
                const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(lep);
                out.push_back(ele->author);
            }
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton pt"); {
      *superflow << HFTname("l_pt");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(1.1*lepton->Pt());
          else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton eta"); {
      *superflow << HFTname("l_eta");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->Eta());
          else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton phi"); {
      *superflow << HFTname("l_phi");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->Phi());
          else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton d0sigBSCorr"); {
      *superflow << HFTname("lep_d0sigBSCorr");
      *superflow << [](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : m_selectLeptons) {
            if (lep) out.push_back(lep->d0sigBSCorr);
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Lepton z0SinTheta"); {
      *superflow << HFTname("lep_z0SinTheta");
      *superflow << [](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : m_selectLeptons) {
            if (lep) out.push_back(lep->z0SinTheta());
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton flavor (0: e, 1: m)"); {
      *superflow << HFTname("l_flav");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->isEle() ? 0 : 1);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton type"); {
      *superflow << HFTname("l_type");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->mcType);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton origin"); {
      *superflow << HFTname("l_origin");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->mcOrigin);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton charge"); {
      *superflow << HFTname("l_q");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->q);
          else out.push_back(-INT_MAX);
        }
        return out;
        };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton BkgMotherPdgId"); {
      *superflow << HFTname("l_BkgMotherPdgId");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->mcBkgMotherPdgId);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton BkgTruthOrigin"); {
      *superflow << HFTname("l_BkgTruthOrigin");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->mcBkgTruthOrigin);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton matched2TruthLepton"); {
      *superflow << HFTname("l_matched2TruthLepton");
      *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
        vector<int> out;
        for(auto& lepton : m_selectLeptons) {
          if (lepton) out.push_back(lepton->matched2TruthLepton);
          else out.push_back(-INT_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton classification"); {
        *superflow << HFTname("l_truthClass");
        *superflow << [=](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for(auto& lepton : m_selectLeptons) {
                if(lepton) out.push_back(get_lepton_truth_class(lepton));
                else out.push_back(-INT_MAX);
            }
            return out;
        };
        *superflow << SaveVar();
    }
}
void add_other_lepton_variables(Superflow* superflow) {
    //////////////////////////////////////////////////////////////////////////////
    // Two-lepton properties
    *superflow << NewVar("Delta eta between leptons"); {
      *superflow << HFTname("DEtaLL");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
        return fabs(m_lepton0.Eta() - m_lepton1.Eta()); };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Delta phi between leptons"); {
      *superflow << HFTname("DphiLL");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
        return fabs(m_lepton0.DeltaPhi(m_lepton1));};
      *superflow << SaveVar();
    }
    *superflow << NewVar("delta R of di-lepton system"); {
      *superflow << HFTname("drll");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
        return m_lepton0.DeltaR(m_lepton1); };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and leading lepton"); {
      *superflow << HFTname("DphiLep0MET");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
          if (m_selectLeptons.size() < 1) return -DBL_MAX;
          return fabs(m_lepton0.DeltaPhi(m_MET));
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and subleading lepton"); {
      *superflow << HFTname("DphiLep1MET");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
          if (m_selectLeptons.size() <= 1) return -DBL_MAX;
          return fabs(m_lepton1.DeltaPhi(m_MET));
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Delta eta between lep2 and other leptons"); {
      *superflow << HFTname("DEtaL2L");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        if (m_selectLeptons.size() <= 3) return out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
            if (!lepton) continue;
            double dEta = fabs(m_selectLeptons.at(2)->Eta() - lepton->Eta());
            out.push_back(dEta);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Delta phi between lep2 and other leptons"); {
      *superflow << HFTname("DphiL2L");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        if (m_selectLeptons.size() < 3) return out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
            if (!lepton) continue;
            double dPhi = fabs(m_selectLeptons.at(2)->DeltaPhi(*lepton));
            out.push_back(dPhi);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("delta R between lep2 and other leptons"); {
      *superflow << HFTname("drl2l");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        if (m_selectLeptons.size() < 3) return out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
            if (!lepton) continue;
            double dR = fabs(m_selectLeptons.at(2)->DeltaR(*lepton));
            out.push_back(dR);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dR between lep and closest base jet"); {
      *superflow << HFTname("dRLepBaseJet");
      *superflow << [=](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
          if (!lepton) continue;
          float dR = FLT_MAX;
          for (Susy::Jet* jet : *sl->baseJets) {
            float tmp_dR = fabs(jet->DeltaR(*lepton));
            if (tmp_dR < dR) dR = tmp_dR; 
          }
          out.push_back(dR);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dR between lep and closest jet"); {
      *superflow << HFTname("dRLepJet");
      *superflow << [=](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
          if (!lepton) continue;
          float dR = FLT_MAX;
          for (Susy::Jet* jet : *sl->jets) {
            float tmp_dR = fabs(jet->DeltaR(*lepton));
            if (tmp_dR < dR) dR = tmp_dR; 
          }
          out.push_back(dR);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dilepton flavor"); {
      *superflow << HFTname("dilep_flav");
      *superflow << [=](Superlink* /*sl*/, var_int*) -> int {
        if(m_selectLeptons.size()<2) return -INT_MAX;
        if(m_selectLeptons.at(0)->isEle() && m_selectLeptons.at(1)->isMu()){return 0;}       // e mu  case
        else if(m_selectLeptons.at(0)->isMu() && m_selectLeptons.at(1)->isEle()){return 1;}  // mu e  case
        else if(m_selectLeptons.at(0)->isEle() && m_selectLeptons.at(1)->isEle()){return 2;} // e e   case
        else if(m_selectLeptons.at(0)->isMu() && m_selectLeptons.at(1)->isMu()){return 3;}   // mu mu case
        return 4;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("collinear mass, M_coll"); {
      *superflow << HFTname("MCollASym");
      *superflow << [=](Superlink* sl, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
          double deta = fabs(m_lepton0.Eta()-m_lepton1.Eta());
          double dphi = m_lepton0.DeltaPhi(m_lepton1);
          return sqrt(2*m_lepton0.Pt()*(m_lepton1.Pt()+sl->met->Et)*(cosh(deta)-cos(dphi)));
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("mass of di-lepton system, M_ll"); {
      *superflow << HFTname("MLL");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
        return m_dileptonP4.M(); };
      *superflow << SaveVar();
    }
    *superflow << NewVar("mass of tri-lepton system, M_lll"); {
      *superflow << HFTname("MLLL");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 3) return -DBL_MAX;
        TLorentzVector lep2_TLV = *m_selectLeptons.at(2);
        return (m_dileptonP4 + lep2_TLV).M(); };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Pt of di-lepton system, Pt_ll"); {
      *superflow << HFTname("ptll");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
        if (m_selectLeptons.size() < 2) return -DBL_MAX;
        return m_dileptonP4.Pt(); };
      *superflow << SaveVar();
    }
    *superflow << NewVar("diff_pt between leading and sub-leading lepton"); {
      *superflow << HFTname("dpt_ll");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
          if (m_selectLeptons.size() < 2) return -DBL_MAX;
          return m_lepton0.Pt() - m_lepton1.Pt();
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("lepton mT"); {
      *superflow << HFTname("l_mT");
      *superflow << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(Susy::Lepton* lepton : m_selectLeptons) {
          if (!lepton) continue;
          double dphi = lepton->DeltaPhi(m_MET);
          double pT2 = lepton->Pt()*m_MET.Pt();
          double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) ));
          out.push_back(lep_mT);
        }
        return out;
      };
      *superflow << SaveVar();
    }
}
void add_relative_met_variables(Superflow* superflow) {
    *superflow << NewVar("dphi between MET and closest baseline lepton"); {
      *superflow << HFTname("DphiBaseLepMET");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = m_selectLeptons.size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Lepton* lepton : *sl->baseLeptons) {
            float tmp_dphi = fabs(lepton->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and closest lepton"); {
      *superflow << HFTname("DphiLepMET");
      *superflow << [=](Superlink* /*sl*/, var_double*) -> double {
          double dPhi = m_selectLeptons.size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Lepton* lepton : m_selectLeptons) {
            if (!lepton) continue;
            float tmp_dphi = fabs(lepton->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and closest jet"); {
      *superflow << HFTname("DphiJetMET");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Jet* jet : *sl->jets) {
            float tmp_dphi = fabs(jet->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and closest base jet"); {
      *superflow << HFTname("DphiBaseJetMET");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Jet* jet : *sl->baseJets) {
            float tmp_dphi = fabs(jet->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dphi between MET and closest lep/jet"); {
      *superflow << HFTname("DphiLepJetMET");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = (sl->jets->size() || m_selectLeptons.size()) ? DBL_MAX : -DBL_MAX;
          for (Susy::Jet* jet : *sl->jets) {
            float tmp_dphi = fabs(jet->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          for (Susy::Lepton* lepton : m_selectLeptons) {
            if (!lepton) continue;
            float tmp_dphi = fabs(lepton->DeltaPhi(m_MET));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Relative MET with baseline jets/leps"); {
      *superflow << HFTname("RelMETbase");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          return getMetRel(*sl->met, *sl->baseLeptons, *sl->baseJets);
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Relative MET"); {
      *superflow << HFTname("RelMET");
      *superflow << [=](Superlink* sl, var_double*) -> double {
          LeptonVector no_null_leps;
          for (Susy::Lepton* lepton : m_selectLeptons) {
            if (lepton) no_null_leps.push_back(lepton);
          }
          return getMetRel(*sl->met, no_null_leps, *sl->jets);
      };
      *superflow << SaveVar();
    }
}
void add_jet_variables(Superflow* superflow) {
    ////////////////////////////////////////////////////////////////////////////
    // Multiplicity variables
    ADD_MULTIPLICITY_VAR(preJets)
    ADD_MULTIPLICITY_VAR(baseJets)
    ADD_MULTIPLICITY_VAR(jets)
    *superflow << NewVar("number of b jets"); {
      *superflow << HFTname("nBJets");
      *superflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfBJets(*sl->jets)/*(**sl->baseJets)*/; };
      *superflow << SaveVar();
    }
    *superflow << NewVar("number of forward jets"); {
      *superflow << HFTname("nForwardJets");
      *superflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfFJets(*sl->jets)/*(**sl->baseJets)*/; };
      *superflow << SaveVar();
    }
    *superflow << NewVar("preJet JVT if eta<=2.4 and pT<60"); {
      *superflow << HFTname("preJet_JVT");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& jet : *sl->preJets) {
          if (jet->Pt() < 60 && fabs(jet->Eta()) <= 2.4) out.push_back(jet->jvt);
          else
            out.push_back(-DBL_MAX);
        }
        return out;
      };
      *superflow << SaveVar();
    }

    //////////////////////////////////////////////////////////////////////////////
    // Baseline Jets
    *superflow << NewVar("Baseline Jet eta"); {
      *superflow << HFTname("baseJet_eta");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& jet : *sl->baseJets) {out.push_back(jet->Eta()); }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("Baseline Jet mv2c10"); {
      *superflow << HFTname("baseJet_mv2c10");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& jet : *sl->baseJets) {out.push_back(jet->mv2c10); }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("jet eta"); {
      *superflow << HFTname("j_eta");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& jet : *sl->baseJets) {
          out.push_back(jet->Eta());
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("jet JVT"); {
      *superflow << HFTname("j_jvt");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& jet : *sl->baseJets) {
          out.push_back(jet->jvt);
        }
        return out;
      };
      *superflow << SaveVar();
    }
    //*superflow << NewVar("jet JVF"); {
    //  *superflow << HFTname("j_jvf");
    //  *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
    //    vector<double> out;
    //    for(auto& jet : *sl->baseJets) {
    //      out.push_back(jet->jvf);
    //    }
    //    return out;
    //  };
    //  *superflow << SaveVar();
    //}
    *superflow << NewVar("jet phi"); {
      *superflow << HFTname("j_phi");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& jet : *sl->baseJets) {
          out.push_back(jet->Phi());
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("jet flavor (0: NA, 1: B, 2: F, 3: L)"); {
      *superflow << HFTname("j_flav");
      *superflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out; int flav = 0;
        for(auto& jet : *sl->baseJets) {
          if(sl->tools->jetSelector().isB(jet)) { flav = 1; }
          else if(sl->tools->jetSelector().isForward(jet))  { flav = 2; }
          else { flav = 3; }
          out.push_back(flav);
          flav=0;
        }
        return out;
      };
      *superflow << SaveVar();
    }

    *superflow << NewVar("jet pt"); {
      *superflow << HFTname("j_pt");
      *superflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& jet : *sl->baseJets) {
          out.push_back(jet->Pt());
        }
        return out;
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("dijet mass"); {
      *superflow << HFTname("Mjj");
      *superflow << [](Superlink* sl, var_double*) -> double {
          if (sl->baseJets->size() < 2) return -DBL_MAX;
          return m_Dijet_TLV.M();
      };
      *superflow << SaveVar();
    }
    *superflow << NewVar("DeltaEta between two leading jets"); {
      *superflow << HFTname("DEtaJJ");
      *superflow << [](Superlink* sl, var_double*) -> double {
          if (sl->baseJets->size() < 2) return -DBL_MAX;
          return fabs(m_Jet0_TLV.Eta() - m_Jet1_TLV.Eta());
      };
      *superflow << SaveVar();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Useful functions
bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons) {
    for (Susy::Lepton* lep : leptons) {
        if(!lep) continue;
        bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
        if (!trig_fired) continue;
        bool trig_matched = sl->tools->triggerTool().lepton_trigger_match(lep, trig_name);
        if (trig_matched) return true;
    }
    return false;
}

void add_SFOS_lepton_cut(Superflow* superflow) {
    *superflow << CutName("SFOS leptons") << [=](Superlink* /*sl*/) -> bool {
        bool SF = m_selectLeptons.at(0)->isEle() == m_selectLeptons.at(1)->isEle();
        bool OS = m_selectLeptons.at(0)->q != m_selectLeptons.at(1)->q;
        return SF && OS;
    };
}

void add_DFOS_lepton_cut(Superflow* superflow) {
    *superflow << CutName("DFOS leptons") << [=](Superlink* /*sl*/) -> bool {
        bool DF = m_selectLeptons.at(0)->isEle() != m_selectLeptons.at(1)->isEle();
        bool OS = m_selectLeptons.at(0)->q != m_selectLeptons.at(1)->q;
        return DF && OS;
    };
}

int get_lepton_truth_class(Susy::Lepton* lepton) {
    if (lepton==nullptr) return -INT_MAX;

    // Get Truth information
    int T = lepton->mcType;
    int O = lepton->mcOrigin;
    int MO = lepton->mcBkgTruthOrigin;
    int MT = 0; // Not stored in SusyNt::Lepton
    int M_ID = lepton->mcBkgMotherPdgId;

    using namespace MCTruthPartClassifier;

    bool mother_is_el = fabs(M_ID) == 11;
    //bool mother_is_piZero = fabs(M_ID) == 111;
    bool bkgEl_from_phoConv = T==BkgElectron && O==PhotonConv;
    bool bkgEl_from_EMproc = T==BkgElectron && O==ElMagProc;
    bool fromSMBoson = O==WBoson || O==ZBoson || O==Higgs || O==DiBoson;
    bool MfromSMBoson = MO==WBoson || MO==ZBoson || MO==Higgs || MO==DiBoson;
    //bool noChargeFlip = M_ID*lepton->q < 0;
    //bool chargeFlip = M_ID*lepton->q > 0;

    // Defs from https://indico.cern.ch/event/725960/contributions/2987219/attachments/1641430/2621432/TruthDef_April242018.pdf
    bool promptEl1 = T==IsoElectron; //&& noChargeFlip;
    bool promptEl2 = (bkgEl_from_phoConv && mother_is_el); //&& noChargeFlip;
    bool promptEl3 = bkgEl_from_EMproc && MT==IsoElectron && (MO==top || MfromSMBoson);
    bool promptEl = promptEl1 || promptEl2 || promptEl3;

    bool promptEl_from_FSR1 = bkgEl_from_phoConv && MO==FSRPhot;
    bool promptEl_from_FSR2 = T==NonIsoPhoton && O==FSRPhot;
    bool promptEl_from_FSR = promptEl_from_FSR1 || promptEl_from_FSR2;

    //bool promptChargeFlipEl1 = T==IsoElectron && chargeFlip;
    //bool promptChargeFlipEl2 = (bkgEl_from_phoConv && mother_is_el) && chargeFlip;
    //bool promptChargeFlipEl = promptChargeFlipEl1 || promptChargeFlipEl2;

    bool promptMuon = T==IsoMuon && (O==top || fromSMBoson || O==HiggsMSSM || O==MCTruthPartClassifier::SUSY);

    bool promptPho1 = T==IsoPhoton && O==PromptPhot;
    bool promptPho2 = bkgEl_from_phoConv && MT==IsoPhoton && MO==PromptPhot;
    bool promptPho3 = bkgEl_from_EMproc  && MT==IsoPhoton && MO==PromptPhot;
    bool promptPho4 = bkgEl_from_phoConv && MT==BkgPhoton && MO==UndrPhot;
    bool promptPho5 = T==BkgPhoton && O==UndrPhot;
    bool promptPho = promptPho1 || promptPho2 || promptPho3 || promptPho4 || promptPho5;

    bool hadDecay1 = T==BkgElectron && (O==DalitzDec || O==ElMagProc || O==LightMeson || O==StrangeMeson);
    bool hadDecay2 = bkgEl_from_phoConv && MT==BkgPhoton && (MO==PiZero || MO==LightMeson || MO==StrangeMeson);
    bool hadDecay3 = bkgEl_from_EMproc && ((MT==BkgElectron && MO==StrangeMeson) || (MT==BkgPhoton && MO==PiZero));
    bool hadDecay4 = T==BkgPhoton && (O==LightMeson || O==PiZero);
    bool hadDecay5 = T==BkgMuon && (O==LightMeson || O==StrangeMeson || O==PionDecay || O==KaonDecay);
    bool hadDecay6 = T==Hadron;
    bool hadDecay = hadDecay1 || hadDecay2 || hadDecay3 || hadDecay4 || hadDecay5 || hadDecay6;

    bool Mu_as_e1 = (T==NonIsoElectron || T==NonIsoPhoton) && O==Mu;
    bool Mu_as_e2 = bkgEl_from_EMproc && MT==NonIsoElectron && MO==Mu;
    bool Mu_as_e3 = bkgEl_from_phoConv && MT==NonIsoPhoton && MO==Mu;
    bool Mu_as_e = Mu_as_e1 || Mu_as_e2 || Mu_as_e3;

    bool HF_tau1 =  (T==NonIsoElectron || T==NonIsoPhoton) && O==TauLep;
    bool HF_tau2 =  bkgEl_from_phoConv && MT==NonIsoPhoton && MO==TauLep;
    bool HF_tau3 =  T==NonIsoMuon && O==TauLep;
    bool HF_tau =  HF_tau1 || HF_tau2 || HF_tau3;

    bool HF_B1 = T==NonIsoElectron && (O==BottomMeson || O==BBbarMeson || O==BottomBaryon);
    bool HF_B2 = T==BkgPhoton && O==BottomMeson;
    bool HF_B3 = bkgEl_from_phoConv && MT==BkgPhoton && MO==BottomMeson;
    bool HF_B4 = (T==IsoMuon || T==NonIsoMuon) && (O==BottomMeson || O==BBbarMeson || O==BottomBaryon);
    bool HF_B = HF_B1 || HF_B2 || HF_B3 || HF_B4;

    bool HF_C1 = T==NonIsoElectron && (O==CharmedMeson || O==CharmedBaryon || O==CCbarMeson);
    bool HF_C2 = T==BkgElectron && O==CCbarMeson;
    bool HF_C3 = T==BkgPhoton && (O==CharmedMeson || O==CCbarMeson);
    bool HF_C4 = bkgEl_from_phoConv && MT==BkgPhoton && (MO==CharmedMeson || MO==CCbarMeson);
    bool HF_C5 = T==NonIsoMuon && (O==CharmedMeson || O==CharmedBaryon || O==CCbarMeson);
    bool HF_C6 = (T==IsoMuon || T==BkgMuon) && (O==CCbarMeson || MO==CCbarMeson);
    bool HF_C =  HF_C1 || HF_C2 || HF_C3 || HF_C4 || HF_C5 || HF_C6;

    if      (promptEl)           return 1;
    else if (promptMuon)         return 2;
    else if (promptPho)          return 3;
    else if (promptEl_from_FSR)  return 4;
    else if (hadDecay)           return 5;
    else if (Mu_as_e)            return 6;
    else if (HF_tau)             return 7;
    else if (HF_B)               return 8;
    else if (HF_C)               return 9;
    else if (bkgEl_from_phoConv) return 10;
    else if (!(T || O || MT || MO || M_ID)) return -1; // No Truth Info
    else if (T && O && !(MT || MO || M_ID)) return -2; // No Mother Info
    else if (T && !O) return -3; // No Origin Info
    // else if (promptChargeFlipEl) return 2;
    else if (T && O && M_ID) {
        cout << "Unexpected Truth Class: "
             << "T = " << T << ", "
             << "O = " << O << ", "
             << "MT = " << MT << ", "
             << "MO = " << MO << ", "
             << "M_ID = " << M_ID << endl;
    }
    return 0;
}


void print_weight_info(Superlink* sl) {
    cout << "Event weight printout\n";
    cout << "\t Run: " << sl->nt->evt()->run << " - Evt: " << sl->nt->evt()->eventNumber << "\n";
    //cout << "\t Pileup = " << sl->nt->evt()->wPileup << "\n";
    cout << "\t Overal event weight = " << sl->weights->susynt << "\n";
    //cout << "\t\t Generator event weight = " << sl->nt->evt()->w << "\n";
    //cout << "\t Superweight Pileup = " << sl->weights->pileup << "\n";
    //cout << "\t Lepton SF = " << sl->weights->lepSf << "\n";
    //cout << "\t Trigger SF = " << sl->weights->trigSf << "\n";
    //cout << "\t B-tagging SF = " << sl->weights->btagSf << "\n";
    //cout << "\t JVT SF = " << sl->weights->jvtSf << "\n";
}
