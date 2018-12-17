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
void add_lepton_variables(Superflow* cutflow);
void add_jet_variables(Superflow* cutflow);
void add_met_variables(Superflow* cutflow);
void add_dilepton_variables(Superflow* cutflow);
void add_super_razor_variables(Superflow* cutflow);
void add_miscellaneous_variables(Superflow* cutflow);

void add_weight_systematics(Superflow* cutflow);
void add_shape_systematics(Superflow* cutflow);

// globals for use in superflow cuts and variables
static int m_cutflags = 0;
static JetVector m_light_jets;
static TLorentzVector m_dileptonP4;
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
    add_lepton_variables(cutflow);
    add_jet_variables(cutflow);
    add_met_variables(cutflow);
    add_dilepton_variables(cutflow);
    add_super_razor_variables(cutflow);
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
    *cutflow << CutName("read in") << [](Superlink* sl) -> bool {
        ////////////////////////////////////////////////////////////////////////
        // Reset all globals used in cuts/variables
        m_cutflags = 0;
        m_light_jets.clear();
        m_dileptonP4 = {};

        ////////////////////////////////////////////////////////////////////////
        // Set globals
        // Note: No cuts have been applied so add appropriate checks before
        //       dereferencing pointers or accessing vector indices
        m_cutflags = sl->nt->evt()->cutFlags[NtSys::NOM];

        // Light jets are jets that are neither forward nor b-tagged
        for (int i = 0; i < (int)sl->jets->size(); i++) {
            if ( !sl->tools->jetSelector().isBJet(sl->jets->at(i))
              && !sl->tools->jetSelector().isForward(sl->jets->at(i))) {
                m_light_jets.push_back(sl->jets->at(i));
            }
        }

        if (sl->leptons->size() >= 2) {
            m_dileptonP4 = (*sl->leptons->at(0) + *sl->leptons->at(1));
        }
        
        ////////////////////////////////////////////////////////////////////////
        return true; // All events pass this cut
    };
}
void add_cleaning_cuts(Superflow* cutflow) {
    *cutflow << CutName("Pass GRL") << [](Superlink* sl) -> bool {
        return (sl->tools->passGRL(m_cutflags));
    };
    *cutflow << CutName("LAr error") << [](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(m_cutflags));
    };
    *cutflow << CutName("Tile Error") << [](Superlink* sl) -> bool {
        return (sl->tools->passTileErr(m_cutflags));
    };
    *cutflow << CutName("SCT error") << [](Superlink* sl) -> bool {
        return (sl->tools->passSCTErr(m_cutflags));
    };
    *cutflow << CutName("TTC veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passTTC(m_cutflags));
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

    *cutflow << CutName("opposite sign") << [](Superlink* sl) -> bool {
        return (sl->leptons->at(0)->q * sl->leptons->at(1)->q < 0);
    };
}
void add_event_variables(Superflow* cutflow) {
    *cutflow << NewVar("event weight"); {
        *cutflow << HFTname("eventweight");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->product(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mcChannel (dsid)"); {
        *cutflow << HFTname("dsid");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->isMC ? sl->nt->evt()->mcChannel : 0.0;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Monte-Carlo generator event weight"); {
        *cutflow << HFTname("w");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->w;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pile-up weight"); {
        *cutflow << HFTname("pupw");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup;};
        *cutflow << SaveVar();
    }

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

    *cutflow << NewVar("is Monte Carlo"); {
        *cutflow << HFTname("isMC");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
        *cutflow << SaveVar();
    }
}
void add_lepton_variables(Superflow* cutflow) {
    *cutflow << NewVar("lepton-1 Pt"); {
        *cutflow << HFTname("lept1Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(0)->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Eta"); {
        *cutflow << HFTname("lept1Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(0)->Eta(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Phi"); {
        *cutflow << HFTname("lept1Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(0)->Phi(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 Energy"); {
        *cutflow << HFTname("lept1E");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(0)->E(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 charge"); {
        *cutflow << HFTname("lept1q");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->leptons->at(0)->q; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-1 flavor"); { // 0=el, 1=mu, see HistFitterTree.h
        *cutflow << HFTname("lept1Flav");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->leptons->at(0)->isEle() ? 0 : 1; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Pt"); {
        *cutflow << HFTname("lept2Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(1)->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Eta"); {
        *cutflow << HFTname("lept2Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(1)->Eta(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Phi"); {
        *cutflow << HFTname("lept2Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(1)->Phi(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 Energy"); {
        *cutflow << HFTname("lept2E");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->leptons->at(1)->E(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 charge"); {
        *cutflow << HFTname("lept2q");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->leptons->at(1)->q; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lepton-2 flavor"); {
        *cutflow << HFTname("lept2Flav");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->leptons->at(1)->isEle() ? 0 : 1; };
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

    *cutflow << NewVar("Etmiss Rel"); {
        *cutflow << HFTname("metrel");
        *cutflow << [](Superlink* sl, var_float*) -> double { return kin::getMetRel(sl->met, *sl->leptons, *sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of leading lepton and met"); {
        *cutflow << HFTname("deltaPhi_met_l1");
        *cutflow << [](Superlink* sl, var_float*) -> double { return abs(sl->leptons->at(0)->Phi() - sl->met->phi); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of subleading lepton and met"); {
        *cutflow << HFTname("deltaPhi_met_l2");
        *cutflow << [](Superlink* sl, var_float*) -> double { return abs(sl->leptons->at(1)->Phi() - sl->met->phi); };
        *cutflow << SaveVar();
    }
}
void add_dilepton_variables(Superflow* cutflow) {
    *cutflow << NewVar("is e + e"); {
        *cutflow << HFTname("isElEl");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is e + mu"); {
        *cutflow << HFTname("isElMu");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->isEle() ^ sl->leptons->at(1)->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is mu + mu"); {
        *cutflow << HFTname("isMuMu");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->isMu() && sl->leptons->at(1)->isMu(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is opposite-sign"); {
        *cutflow << HFTname("isOS");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->q * sl->leptons->at(1)->q < 0; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is Mu (lead) + E (sub)"); {
        *cutflow << HFTname("isME");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->isMu() && sl->leptons->at(1)->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is E (lead) + Mu (sub)"); {
        *cutflow << HFTname("isEM");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->leptons->at(0)->isEle() && sl->leptons->at(1)->isMu(); };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("mass of di-lepton system, M_ll"); {
        *cutflow << HFTname("mll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double {return m_dileptonP4.M();};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pt of di-lepton system, Pt_ll"); {
        *cutflow << HFTname("pTll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_dileptonP4.Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Eta of di-lepton system"); {
        *cutflow << HFTname("deta_ll");
        *cutflow << [](Superlink* sl, var_float*) -> double { return abs(sl->leptons->at(0)->Eta() - sl->leptons->at(1)->Eta()); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of di-lepton system"); {
        *cutflow << HFTname("dphi_ll");
        *cutflow << [](Superlink* sl, var_float*) -> double { return abs(sl->leptons->at(0)->Phi() - sl->leptons->at(1)->Phi()); };
        *cutflow << SaveVar();
    }
}
void add_super_razor_variables(Superflow* cutflow) {
    *cutflow << NewVar("stransverse mass"); {
        *cutflow << HFTname("MT2");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double mt2_ = kin::getMT2(*sl->leptons, *sl->met);
            return mt2_;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Super-Razor Var: shatr"); {
        *cutflow << HFTname("shatr");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double   mDeltaR              = 0.0;
            double   SHATR                = 0.0;
            double   cosThetaRp1          = 0.0;
            double   dphi_LL_vBETA_T      = 0.0;
            double   dphi_L1_L2           = 0.0;
            double   gamma_R              = 0.0;
            double   dphi_vBETA_R_vBETA_T = 0.0;
            TVector3 vBETA_z;
            TVector3 pT_CM;
            TVector3 vBETA_T_CMtoR;
            TVector3 vBETA_R;
            kin::superRazor(*sl->leptons, sl->met, vBETA_z, pT_CM,
                                    vBETA_T_CMtoR, vBETA_R, SHATR, dphi_LL_vBETA_T,
                                    dphi_L1_L2, gamma_R, dphi_vBETA_R_vBETA_T,
                                    mDeltaR, cosThetaRp1);
            return SHATR;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Super-Razor Var: mDeltaR"); {
        *cutflow << HFTname("mDeltaR");
        *cutflow << [](Superlink* sl, var_double*) -> double {
            double   mDeltaR              = 0.0;
            double   SHATR                = 0.0;
            double   cosThetaRp1          = 0.0;
            double   dphi_LL_vBETA_T      = 0.0;
            double   dphi_L1_L2           = 0.0;
            double   gamma_R              = 0.0;
            double   dphi_vBETA_R_vBETA_T = 0.0;
            TVector3 vBETA_z;
            TVector3 pT_CM;
            TVector3 vBETA_T_CMtoR;
            TVector3 vBETA_R;
            kin::superRazor(*sl->leptons, sl->met, vBETA_z, pT_CM,
                                    vBETA_T_CMtoR, vBETA_R, SHATR, dphi_LL_vBETA_T,
                                    dphi_L1_L2, gamma_R, dphi_vBETA_R_vBETA_T,
                                    mDeltaR, cosThetaRp1);
            return mDeltaR;
        };
        *cutflow << SaveVar();
    }
}
void add_miscellaneous_variables(Superflow* cutflow) {
    *cutflow << NewVar("Ht (m_Eff: lep + met + jet)"); {
        *cutflow << HFTname("ht");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;

            ht += sl->leptons->at(0)->Pt() + sl->leptons->at(1)->Pt();
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




