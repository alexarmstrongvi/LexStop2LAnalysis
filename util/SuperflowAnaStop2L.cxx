////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) <2018> by Alex Armstrong 
///
/// @file SuperflowAnaStop2L.cxx
/// @author Alex Armstrong (alarmstr@cern.ch)
/// @date <December 2018>
/// @brief Make flat ntuples from SusyNts
///
///////////////////////////////////////////////////////////////////////////////

// std
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <getopt.h>
#include <map>

// ROOT
#include "TChain.h"
#include "TVectorD.h"

// SusyNtuple
#include "SusyNtuple/ChainHelper.h"
#include "SusyNtuple/string_utils.h"
#include "SusyNtuple/SusyNtSys.h"
#include "SusyNtuple/KinematicTools.h"
#include "SusyNtuple/AnalysisType.h"

// Superflow
#include "Superflow/Superflow.h"
#include "Superflow/Superlink.h"
#include "Superflow/Cut.h"
#include "Superflow/StringTools.h"
#include "Superflow/input_options.h"

//Jigsaw
#include "jigsawcalculator/JigsawCalculator.h"

using namespace std;
using namespace sflow;

////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
string m_ana_name = "SuperflowAnaStop2L";
string m_input_ttree_name = "susyNt";
bool m_verbose = true;
bool m_print_weighted_cutflow = true;
int m_lumi = 1000; // inverse picobarns (ipb)
Susy::AnalysisType m_ana_type = Susy::AnalysisType::Ana_Stop2L;

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string ttree_name, bool verbose);
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain);
void set_global_variables(Superflow* cutflow);
void add_cleaning_cuts(Superflow* cutflow);
void add_analysis_cuts(Superflow* cutflow);
void add_event_variables(Superflow* cutflow);
void add_trigger_variables(Superflow* cutflow);
void add_lepton_variables(Superflow* cutflow);
void add_jet_variables(Superflow* cutflow);
void add_met_variables(Superflow* cutflow);
void add_dilepton_variables(Superflow* cutflow);
void add_multi_object_variables(Superflow* cutflow);
void add_jigsaw_variables(Superflow* cutflow);
void add_miscellaneous_variables(Superflow* cutflow);

void add_weight_systematics(Superflow* cutflow);
void add_shape_systematics(Superflow* cutflow);

// globals for use in superflow cuts and variables
static int m_cutflags = 0;
static JetVector m_light_jets;
static TLorentzVector m_dileptonP4;
static TLorentzVector m_MET;
static Susy::Lepton* m_lept1;
static Susy::Lepton* m_lept2;
static Susy::Electron *m_el0, *m_el1;
static Susy::Muon *m_mu0, *m_mu1;
static LeptonVector m_triggerLeptons;

static jigsaw::JigsawCalculator m_calculator;
static std::map< std::string, float> m_jigsaw_vars;


// Helpful functions for trigger matching
bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons);
#define ADD_1LEP_TRIGGER_VAR(trig_name, leptons) { \
    *cutflow << NewVar(#trig_name" trigger bit"); { \
        *cutflow << HFTname(#trig_name); \
        *cutflow << [=](Superlink* sl, var_bool*) -> bool { \
            return is_1lep_trig_matched(sl, #trig_name, leptons); \
        }; \
        *cutflow << SaveVar(); \
    } \
}
// Trig Matching for dilep triggers is buggy
// so currently not trigger matching
bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2);
#define ADD_2LEP_TRIGGER_VAR(trig_name, lep1, lep2) { \
  *cutflow << NewVar(#trig_name" trigger bit"); { \
      *cutflow << HFTname(#trig_name); \
      *cutflow << [=](Superlink* sl, var_bool*) -> bool { \
          return is_2lep_trig_matched(sl, #trig_name, lep1, lep2); \
      }; \
      *cutflow << SaveVar(); \
  } \
}

// Addings a jigsaw variable
#define ADD_JIGSAW_VAR(var_name) { \
    *cutflow << NewVar(#var_name); { \
        *cutflow << HFTname(#var_name); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_jigsaw_vars.at(#var_name); }; \
        *cutflow << SaveVar(); \
    } \
}

////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    /////////////////////////////////////////////////////////////////////
    // Read in the command-line options (input file, num events, etc...)
    ////////////////////////////////////////////////////////////////////
    SFOptions options(argc, argv);
    options.ana_name = m_ana_name;
    if(!read_options(options)) {
        exit(1);
    }

    // New TChain* added to heap, remember to delete later
    TChain* chain = create_new_chain(options.input, m_input_ttree_name, m_verbose);

    Long64_t tot_num_events = chain->GetEntries();
    options.n_events_to_process = (options.n_events_to_process < 0 ? tot_num_events : options.n_events_to_process);

    ////////////////////////////////////////////////////////////
    // Initialize & configure the analysis
    //  > Superflow inherits from SusyNtAna : TSelector
    ////////////////////////////////////////////////////////////
    Superflow* cutflow = create_new_superflow(options, chain);

    cout << options.ana_name << "    Total Entries: " << chain->GetEntries() << endl;
    //if (options.run_mode == SuperflowRunMode::single_event_syst) cutflow->setSingleEventSyst(nt_sys_);

    // Set variables for use in other cuts/vars. MUST ADD FIRST!
    set_global_variables(cutflow);

    // Event selections
    add_cleaning_cuts(cutflow);
    add_analysis_cuts(cutflow);

    // Output variables
    add_event_variables(cutflow);
    add_trigger_variables(cutflow);
    add_lepton_variables(cutflow);
    add_jet_variables(cutflow);
    add_met_variables(cutflow);
    add_dilepton_variables(cutflow);
    add_multi_object_variables(cutflow);
    add_jigsaw_variables(cutflow);
    add_miscellaneous_variables(cutflow);

    // Systematics
    add_weight_systematics(cutflow);
    add_shape_systematics(cutflow);

    // Run Superflow
    chain->Process(cutflow, options.input.c_str(), options.n_events_to_process);

    // Clean up
    delete cutflow;
    delete chain;

    cout << m_ana_name << "    Done." << endl;
    exit(0);
}

////////////////////////////////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string input_ttree_name, bool verbose) {
    TChain* chain = new TChain(input_ttree_name.c_str());
    chain->SetDirectory(0);
    ChainHelper::addInput(chain, input, verbose);
    return chain;
}
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain) {
    Superflow* cutflow = new Superflow(); // initialize the cutflow
    cutflow->setAnaName(sf_options.ana_name);
    cutflow->setAnaType(m_ana_type);
    cutflow->setLumi(m_lumi);
    cutflow->setSampleName(sf_options.input);
    cutflow->setRunMode(sf_options.run_mode);
    cutflow->setCountWeights(m_print_weighted_cutflow);
    cutflow->setChain(chain);
    cutflow->setDebug(sf_options.dbg);
    cutflow->nttools().initTriggerTool(ChainHelper::firstFile(sf_options.input, 0.0));
    if(sf_options.suffix_name != "") {
        cutflow->setFileSuffix(sf_options.suffix_name);
    }
    if(sf_options.sumw_file_name != "") {
        cout << sf_options.ana_name 
             << "    Reading sumw for sample from file: " 
             << sf_options.sumw_file_name << endl;
        cutflow->setUseSumwFile(sf_options.sumw_file_name);
    }
    return cutflow;
}
void set_global_variables(Superflow* cutflow) {
    
    m_calculator.initialize("TTMET2LW");
    
    *cutflow << CutName("read in") << [](Superlink* sl) -> bool {
        ////////////////////////////////////////////////////////////////////////
        // Reset all globals used in cuts/variables
        m_cutflags = 0;
        m_light_jets.clear();
        m_dileptonP4 = {};
        m_MET = {};
        m_lept1 = m_lept2 = 0;
        m_el0 = m_el1 = 0;
        m_mu0 = m_mu1 = 0;
        m_triggerLeptons.clear();

        ////////////////////////////////////////////////////////////////////////
        // Set globals
        // Note: No cuts have been applied so add appropriate checks before
        //       dereferencing pointers or accessing vector indices
        m_cutflags = sl->nt->evt()->cutFlags[NtSys::NOM];

        // Light jets: jets that are neither forward nor b-tagged
        for (int i = 0; i < (int)sl->jets->size(); i++) {
            if ( !sl->tools->jetSelector().isBJet(sl->jets->at(i))
              && !sl->tools->jetSelector().isForward(sl->jets->at(i))) {
                m_light_jets.push_back(sl->jets->at(i));
            }
        }

        // Missing transverse momentum
        m_MET.SetPxPyPzE(sl->met->Et * cos(sl->met->phi),
                         sl->met->Et * sin(sl->met->phi),
                         0.,
                         sl->met->Et);

        // Commonly used leptons
        if (sl->leptons->size() >= 1) {
            m_lept1 = sl->leptons->at(0);
            if (sl->leptons->size() >= 2) {
                m_lept2 = sl->leptons->at(1);
                m_dileptonP4 = *m_lept1 + *m_lept2;
            }
        }
        for (Susy::Lepton* lep : *sl->leptons) {
            if (!lep) { continue; }
            if (m_triggerLeptons.size() < 2) m_triggerLeptons.push_back(lep);

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

        // Jigsaw variables
        if (sl->leptons->size() >= 2) {
            // build the object map for the calculator
            // the TTMET2LW calculator expects "leptons" and "met"
            std::map<std::string, std::vector<TLorentzVector>> object_map;
            object_map["leptons"] = { *m_lept1, *m_lept2 };
            object_map["met"] = { m_MET };
            m_calculator.load_event(object_map);
            m_jigsaw_vars = m_calculator.variables();
        }
        
        ////////////////////////////////////////////////////////////////////////
        return true; // All events pass this cut
    };
}
void add_cleaning_cuts(Superflow* cutflow) {
    *cutflow << CutName("Pass GRL") << [](Superlink* sl) -> bool {
        return (sl->tools->passGRL(m_cutflags));
    };
    *cutflow << CutName("Error flags") << [](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(m_cutflags)
                && sl->tools->passTileErr(m_cutflags)
                && sl->tools->passSCTErr(m_cutflags)
                && sl->tools->passTTC(m_cutflags));
    };
    *cutflow << CutName("pass Good Vertex") << [](Superlink * sl) -> bool {
        return (sl->tools->passGoodVtx(m_cutflags));
    };
    *cutflow << CutName("pass bad muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passBadMuon(sl->preMuons));
    };
    *cutflow << CutName("pass cosmic muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passCosmicMuon(sl->baseMuons));
    };
    *cutflow << CutName("pass jet cleaning") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };
}
void add_analysis_cuts(Superflow* cutflow) {
    *cutflow << CutName("exactly two base leptons") << [](Superlink* sl) -> bool {
        return sl->baseLeptons->size() == 2;
    };

    *cutflow << CutName("m_ll > 20 GeV") << [](Superlink* sl) -> bool {
        return (*sl->baseLeptons->at(0) + *sl->baseLeptons->at(1)).M() > 20.0;
    };

    *cutflow << CutName("tau veto") << [](Superlink* sl) -> bool {
        return sl->taus->size() == 0;
    };

    *cutflow << CutName("exactly two signal leptons") << [](Superlink* sl) -> bool {
        return (sl->leptons->size() == 2);
    };

    *cutflow << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
        return (m_lept1->q * m_lept2->q < 0);
    };
    
    *cutflow << CutName("dilepton flavor (emu/mue)") << [](Superlink* /*sl*/) -> bool {
        return (m_lept1->isEle() != m_lept2->isEle());
    };
}
void add_event_variables(Superflow* cutflow) {
    // Event weights
    *cutflow << NewVar("event weight"); {
        *cutflow << HFTname("eventweight");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->weights->product() * sl->nt->evt()->wPileup; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("event weight (multi period)"); {
        *cutflow << HFTname("eventweight_multi");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->weights->product_multi() * sl->nt->evt()->wPileup; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("event weight (single period)"); {
        *cutflow << HFTname("eventweight_single");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->product() * sl->nt->evt()->wPileup / sl->nt->evt()->wPileup_period; };
        *cutflow << SaveVar();
    }
   
    *cutflow << NewVar("event weight (no scale factors)"); {
        *cutflow << HFTname("eventweight_noSF");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt;};
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("multiperiod event weight (no scale factors)"); {
        *cutflow << HFTname("eventweight_noSF_multi");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt_multi;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Period weight"); {
        *cutflow << HFTname("period_weight");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup_period;};
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("Monte-Carlo generator event weight"); {
        *cutflow << HFTname("w");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->w;};
        *cutflow << SaveVar();
    }
    
    // Scale Factors
    *cutflow << NewVar("Pile-up weight"); {
        *cutflow << HFTname("pupw");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup;};
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("Lepton scale factor"); {
        *cutflow << HFTname("lepSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->lepSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Trigger scale factor"); {
        *cutflow << HFTname("trigSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->trigSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("B-tag scale factor"); {
        *cutflow << HFTname("btagSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->btagSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("JVT scale factor"); {
        *cutflow << HFTname("jvtSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->jvtSf;};
        *cutflow << SaveVar();
    }

    // Event identifiers
    *cutflow << NewVar("Event run number"); {
        *cutflow << HFTname("runNumber");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->run; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Event number"); {
        *cutflow << HFTname("eventNumber");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->eventNumber; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lumi block"); {
        *cutflow << HFTname("lb");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->lb; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is Monte Carlo"); {
        *cutflow << HFTname("isMC");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mcChannel (dsid)"); {
        *cutflow << HFTname("dsid");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->isMC ? sl->nt->evt()->mcChannel : 0.0;};
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("treatAsYear"); {
        // 15/16 Year ID
        *cutflow << HFTname("treatAsYear");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->treatAsYear; };
        *cutflow << SaveVar();
    }

    // Event properties
    *cutflow << NewVar("Average Mu"); {
        *cutflow << HFTname("avgMu");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMu; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Average Mu (Data DF)"); {
        *cutflow << HFTname("avgMuDataSF");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMuDataSF; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Actual Mu"); {
        *cutflow << HFTname("actualMu");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMu; };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("Actual Mu (Data SF)"); {
        *cutflow << HFTname("actualMuDataSF");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMuDataSF; };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("Number of vertices"); {
        *cutflow << HFTname("nVtx");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->nVtx; };
        *cutflow << SaveVar();
    }
    
}
void add_trigger_variables(Superflow* cutflow) {
    ////////////////////////////////////////////////////////////////////////////
    // Trigger Variables
    // ADD_*_TRIGGER_VAR preprocessor defined

    ////////////////////////////////////////////////////////////////////////////
    // 2015
    ADD_1LEP_TRIGGER_VAR(HLT_e24_lhmedium_L1EM20VH, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e120_lhloose, m_triggerLeptons)
    
    ADD_2LEP_TRIGGER_VAR(HLT_2e12_lhloose_L12EM10VH, m_el0, m_el1)
    
    ADD_1LEP_TRIGGER_VAR(HLT_mu20_iloose_L1MU15, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu40, m_triggerLeptons)
    
    // TODO: HLT_2mu10
    ADD_2LEP_TRIGGER_VAR(HLT_mu18_mu8noL1, m_mu0, m_mu1)
    
    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_mu14, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_mu24, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2016
    ADD_2LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0, m_el0, m_el1)
    ADD_2LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2016-2018

    ADD_1LEP_TRIGGER_VAR(HLT_e26_lhtight_nod0_ivarloose, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium_nod0, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e140_lhloose_nod0, m_triggerLeptons)
    
    ADD_1LEP_TRIGGER_VAR(HLT_mu26_ivarmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu50, m_triggerLeptons)
    
    // TODO: HLT_2mu14
    ADD_2LEP_TRIGGER_VAR(HLT_mu22_mu8noL1, m_mu0, m_mu1)
    
    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_nod0_mu14, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_nod0_mu24, m_el0, m_mu0)
    
    ////////////////////////////////////////////////////////////////////////////
    // 2017-2018
    ADD_2LEP_TRIGGER_VAR(HLT_2e24_lhvloose_nod0, m_el0, m_el1)
    
    ADD_2LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_mu8noL1, m_el0, m_mu0)
    
    ////////////////////////////////////////////////////////////////////////////
    // 2018
    // L1_2EM15VHI was accidentally prescaled in periods B5-B8 of 2017
    // (runs 326834-328393) with an effective reduction of 0.6 fb-1
    ADD_2LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0_L12EM15VHI, m_el0, m_el1)
}
void add_lepton_variables(Superflow* cutflow) {
    *cutflow << NewVar("lepton-1 Pt"); {
        *cutflow << HFTname("lept1Pt");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Eta"); {
        *cutflow << HFTname("lept1Eta");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->Eta(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Phi"); {
        *cutflow << HFTname("lept1Phi");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->Phi(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Energy"); {
        *cutflow << HFTname("lept1E");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->E(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 charge"); {
        *cutflow << HFTname("lept1q");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lept1->q; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 flavor"); { // 0=el, 1=mu, see HistFitterTree.h
        *cutflow << HFTname("lept1Flav");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lept1->isEle() ? 0 : 1; };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("lepton-2 Pt"); {
        *cutflow << HFTname("lept2Pt");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept2->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Eta"); {
        *cutflow << HFTname("lept2Eta");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept2->Eta(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Phi"); {
        *cutflow << HFTname("lept2Phi");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept2->Phi(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Energy"); {
        *cutflow << HFTname("lept2E");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept2->E(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 charge"); {
        *cutflow << HFTname("lept2q");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lept2->q; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 flavor"); {
        *cutflow << HFTname("lept2Flav");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lept2->isEle() ? 0 : 1; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Lepton0 Iso"); {
      *cutflow << HFTname("l_iso1");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        if (sl->leptons->size() <= 0) return out;
        out.push_back(-1);  // for tracking all entries and normalizing bins
        bool flag = false;
        if (m_lept1->isoGradient)               { flag=true; out.push_back(0);}
        if (m_lept1->isoGradientLoose)          { flag=true; out.push_back(1);}
        if (m_lept1->isoLoose)                  { flag=true; out.push_back(2);}
        if (m_lept1->isoLooseTrackOnly)         { flag=true; out.push_back(3);}
        if (m_lept1->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);}
        if (!flag) out.push_back(5);
        return out;
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("Lepton1 Iso"); {
      *cutflow << HFTname("l_iso2");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        if (sl->leptons->size() <= 1) return out;
        out.push_back(-1);  // for tracking all entries and normalizing bins
        bool flag = false;
        if (m_lept2->isoGradient)               { flag=true; out.push_back(0);}
        if (m_lept2->isoGradientLoose)          { flag=true; out.push_back(1);}
        if (m_lept2->isoLoose)                  { flag=true; out.push_back(2);}
        if (m_lept2->isoLooseTrackOnly)         { flag=true; out.push_back(3);}
        if (m_lept2->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);}
        if (!flag) out.push_back(5);
        return out;
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("Lepton d0sigBSCorr"); {
      *cutflow << HFTname("lep_d0sigBSCorr");
      *cutflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->leptons) {
            if (lep) out.push_back(lep->d0sigBSCorr);
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("Lepton z0SinTheta"); {
      *cutflow << HFTname("lep_z0SinTheta");
      *cutflow << [](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for (auto& lep : *sl->leptons) {
            if (lep) out.push_back(lep->z0SinTheta());
            else out.push_back(-DBL_MAX);
        }
        return out;
      };
      *cutflow << SaveVar();
    }
    //////////////////////////////////////////////////////////////////////////////
    // Electrons
    *cutflow << NewVar("Electron ID (non-inclusive)"); {
      *cutflow << HFTname("El_ID");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
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
      *cutflow << SaveVar();
    }
    //////////////////////////////////////////////////////////////////////////////
    // Muons
    *cutflow << NewVar("Muon ID (non-inclusive)"); {
      *cutflow << HFTname("Mu_ID");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
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
      *cutflow << SaveVar();
    }
}
void add_jet_variables(Superflow* cutflow) {
    *cutflow << NewVar("number of light jets"); {
        *cutflow << HFTname("nLightJets");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int {return m_light_jets.size(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of b-tagged jets"); {
        *cutflow << HFTname("nBJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfBJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of forward jets"); {
        *cutflow << HFTname("nForwardJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfFJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of non-b-tagged jets"); {
        *cutflow << HFTname("nNonBJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->jets->size() - sl->tools->numberOfBJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Pt"); {
        *cutflow << HFTname("jet1Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Pt() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Eta"); {
        *cutflow << HFTname("jet1Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Phi"); {
        *cutflow << HFTname("jet1Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Pt"); {
        *cutflow << HFTname("jet2Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Pt() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Eta"); {
        *cutflow << HFTname("jet2Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Phi"); {
        *cutflow << HFTname("jet2Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Phi() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }
}
void add_met_variables(Superflow* cutflow) {
    *cutflow << NewVar("transverse missing energy (Et)"); {
        *cutflow << HFTname("met");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->met->Et; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("transverse missing energy (Phi)"); {
        *cutflow << HFTname("metPhi");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->met->phi; };
        *cutflow << SaveVar();
    }
}
void add_dilepton_variables(Superflow* cutflow) {
    *cutflow << NewVar("is e + e"); {
        *cutflow << HFTname("isElEl");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->isEle() && m_lept2->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is e + mu"); {
        *cutflow << HFTname("isElMu");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->isEle() ^ m_lept2->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is mu + mu"); {
        *cutflow << HFTname("isMuMu");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->isMu() && m_lept2->isMu(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is opposite-sign"); {
        *cutflow << HFTname("isOS");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->q * m_lept2->q < 0; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is Mu (lead) + E (sub)"); {
        *cutflow << HFTname("isME");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->isMu() && m_lept2->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is E (lead) + Mu (sub)"); {
        *cutflow << HFTname("isEM");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lept1->isEle() && m_lept2->isMu(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mass of di-lepton system"); {
        *cutflow << HFTname("mll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double {return m_dileptonP4.M();};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pt of di-lepton system"); {
        *cutflow << HFTname("pTll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_dileptonP4.Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pt difference of di-lepton system"); {
        *cutflow << HFTname("dpTll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->Pt() - m_lept2->Pt(); };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("delta Eta of di-lepton system"); {
        *cutflow << HFTname("deta_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lept1->Eta() - m_lept2->Eta()); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of di-lepton system"); {
        *cutflow << HFTname("dphi_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lept1->DeltaPhi(*m_lept2)); };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("delta R of di-lepton system"); {
        *cutflow << HFTname("dR_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lept1->DeltaR(*m_lept2); };
        *cutflow << SaveVar();
    }
}
void add_multi_object_variables(Superflow* cutflow) {
    // Leptons and MET
    *cutflow << NewVar("lepton-1 transverse mass"); {
        *cutflow << HFTname("lept1mT");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int {
            double dphi = m_lept1->DeltaPhi(m_MET);
            double pT2 = m_lept1->Pt()*m_MET.Pt();
            double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) ));
            return lep_mT;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 transverse mass"); {
        *cutflow << HFTname("lept2mT");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int {
            double dphi = m_lept2->DeltaPhi(m_MET);
            double pT2 = m_lept2->Pt()*m_MET.Pt();
            double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) ));
            return lep_mT;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of leading lepton and met"); {
        *cutflow << HFTname("deltaPhi_met_l1");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lept1->DeltaPhi(m_MET)); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of subleading lepton and met"); {
        *cutflow << HFTname("deltaPhi_met_l2");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lept2->DeltaPhi(m_MET)); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("stransverse mass"); {
        *cutflow << HFTname("MT2");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double mt2_ = kin::getMT2(*sl->leptons, *sl->met);
            return mt2_;
        };
        *cutflow << SaveVar();
    }

    // Leptons and Jets
    *cutflow << NewVar("dR between lepton-1 and closest jet"); {
      *cutflow << HFTname("dR_lept1_jet");
      *cutflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Jet* jet : *sl->jets) {
            float tmp_dphi = fabs(jet->DeltaPhi(*m_lept1));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *cutflow << SaveVar();
    }

    *cutflow << NewVar("dR between lepton-2 and closest jet"); {
      *cutflow << HFTname("dR_lept2_jet");
      *cutflow << [=](Superlink* sl, var_double*) -> double {
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX;
          for (Susy::Jet* jet : *sl->jets) {
            float tmp_dphi = fabs(jet->DeltaPhi(*m_lept2));
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; 
          }
          return dPhi;
      };
      *cutflow << SaveVar();
    }
    
    // Leptons, Jets, and MET
    *cutflow << NewVar("Etmiss Rel"); {
        *cutflow << HFTname("metrel");
        *cutflow << [](Superlink* sl, var_float*) -> double { return kin::getMetRel(sl->met, *sl->leptons, *sl->jets); };
        *cutflow << SaveVar();
    }
}

void add_jigsaw_variables(Superflow* cutflow) {
    ADD_JIGSAW_VAR(H_11_SS)
    ADD_JIGSAW_VAR(H_21_SS)
    ADD_JIGSAW_VAR(H_12_SS)
    ADD_JIGSAW_VAR(H_22_SS)
    ADD_JIGSAW_VAR(H_11_S1)
    ADD_JIGSAW_VAR(H_11_SS_T)
    ADD_JIGSAW_VAR(H_21_SS_T)
    ADD_JIGSAW_VAR(H_22_SS_T)
    ADD_JIGSAW_VAR(H_11_S1_T)
    ADD_JIGSAW_VAR(shat)
    ADD_JIGSAW_VAR(pTT_T)
    ADD_JIGSAW_VAR(pTT_Z)
    ADD_JIGSAW_VAR(RPT)
    ADD_JIGSAW_VAR(RPZ)
    ADD_JIGSAW_VAR(RPT_H_11_SS)
    ADD_JIGSAW_VAR(RPT_H_21_SS)
    ADD_JIGSAW_VAR(RPT_H_22_SS)
    ADD_JIGSAW_VAR(RPZ_H_11_SS)
    ADD_JIGSAW_VAR(RPZ_H_21_SS)
    ADD_JIGSAW_VAR(RPZ_H_22_SS)
    ADD_JIGSAW_VAR(RPT_H_11_SS_T)
    ADD_JIGSAW_VAR(RPT_H_21_SS_T)
    ADD_JIGSAW_VAR(RPT_H_22_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_11_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_21_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_22_SS_T)
    ADD_JIGSAW_VAR(gamInvRp1)
    ADD_JIGSAW_VAR(MDR)
    ADD_JIGSAW_VAR(DPB_vSS)
    ADD_JIGSAW_VAR(costheta_SS)
    ADD_JIGSAW_VAR(dphi_v_SS)
    ADD_JIGSAW_VAR(dphi_v1_i1_ss)
    ADD_JIGSAW_VAR(dphi_s1_s2_ss)
    ADD_JIGSAW_VAR(dphi_S_I_ss)
    ADD_JIGSAW_VAR(dphi_S_I_s1)
}
void add_miscellaneous_variables(Superflow* cutflow) {
    *cutflow << NewVar("Ht (m_Eff: lep + met + jet)"); {
        *cutflow << HFTname("ht");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;

            ht += m_lept1->Pt() + m_lept2->Pt();
            ht += sl->met->Et;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                if (sl->jets->at(i)->Pt() > 20.0) {
                    ht += sl->jets->at(i)->Pt();
                }
            }
            return ht;
        };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("|cos(theta_b)|"); {
        *cutflow << HFTname("abs_costheta_b");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double {
            TLorentzVector lp, lm, ll;
            lp = m_lept1->q > 0 ? *m_lept1 : *m_lept2;
            lm = m_lept1->q < 0 ? *m_lept1 : *m_lept2;
            ll = lp + lm;
            
            TVector3 boost = ll.BoostVector();
            lp.Boost(-boost);
            lm.Boost(-boost);
            
            double deta = lp.Eta() - lm.Eta();
            double costheta_b = tanh(0.5 * deta);
            
            return fabs(costheta_b);
        };
        *cutflow << SaveVar();
    }
}
void add_weight_systematics(Superflow* cutflow) {
    *cutflow << NewSystematic("shift in electron ID efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_ID");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron ISO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_Iso");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron RECO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_Reco");
        *cutflow << SaveSystematic();
    }
}
void add_shape_systematics(Superflow* cutflow) {
    *cutflow << NewSystematic("shift in e-gamma resolution (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_UP);
        *cutflow << TreeName("EG_RESOLUTION_ALL_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma resolution (DOWN)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_DN);
        *cutflow << TreeName("EG_RESOLUTION_ALL_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma scale (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_SCALE_ALL_UP);
        *cutflow << TreeName("EG_SCALE_ALL_UP");
        *cutflow << SaveSystematic();
    }
}

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

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2) {
    if(!lep1 || ! lep2) return false;
    bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
    if (!trig_fired) return false;
    bool trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep1, lep2, trig_name);
    return trig_matched;
}

