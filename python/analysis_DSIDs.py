################################################################################
# Dictionary of all samples used in Stop2L analysis
#
# Data Good Run Lists twiki: https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/GoodRunListsForAnalysisRun2
#   - data15: PHYS_StandardGRL_All_Good_25ns [http://atlasdqm.web.cern.ch/atlasdqm/grlgen/All_Good/data15_13TeV.periodAllYear_DetStatus-v89-pro21-02_Unknown_PHYS_StandardGRL_All_Good_25ns.xml]
#   - data16: PHYS_StandardGRL_All_Good_25ns [https://atlasdqm.web.cern.ch/atlasdqm/grlgen/All_Good/data16_13TeV.periodAllYear_DetStatus-v89-pro21-01_DQDefects-00-02-04_PHYS_StandardGRL_All_Good_25ns.xml]
#   - data17: PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim [http://atlasdqm.web.cern.ch/atlasdqm/grlgen/All_Good/data17_13TeV.periodAllYear_DetStatus-v99-pro22-01_Unknown_PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim.xml]
#   - data18: PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim [http://atlasdqm.web.cern.ch/atlasdqm/grlgen/All_Good/data18_13TeV.periodAllYear_DetStatus-v102-pro22-04_Unknown_PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim.xml]
#
# Monte Carlo samples listed on CentralMC16ProductionList [https://twiki.cern.ch/twiki/bin/view/AtlasProtected/CentralMC16ProductionList] 
################################################################################

DSID_GROUPS = {
    ############################################################################
    # SIGNAL
    ############################################################################
    'stop2l_350_260' : ['436200'], # MGPy8EG_A14N23LO_TT_bWN_350_260_MadSpin_2L15 (DeltaM =  90); Not available in mc16e
    'stop2l_350_230' : ['436201'], # MGPy8EG_A14N23LO_TT_bWN_350_230_MadSpin_2L15 (DeltaM = 120); Not available in mc16e
    'stop2l_350_200' : ['436202'], # MGPy8EG_A14N23LO_TT_bWN_350_200_MadSpin_2L15 (DeltaM = 150); Not available in mc16e
    'stop2l_350_185' : ['436203'], # MGPy8EG_A14N23LO_TT_bWN_350_185_MadSpin_2L15 (DeltaM = 165)
    'stop2l_375_285' : ['436204'], # MGPy8EG_A14N23LO_TT_bWN_375_285_MadSpin_2L15 (DeltaM =  90); Not available in mc16e
    'stop2l_375_255' : ['436205'], # MGPy8EG_A14N23LO_TT_bWN_375_255_MadSpin_2L15 (DeltaM = 120); Not available in mc16e
    'stop2l_375_225' : ['436206'], # MGPy8EG_A14N23LO_TT_bWN_375_225_MadSpin_2L15 (DeltaM = 150); Not available in mc16e
    'stop2l_375_210' : ['436207'], # MGPy8EG_A14N23LO_TT_bWN_375_210_MadSpin_2L15 (DeltaM = 165)
    'stop2l_400_310' : ['436208'], # MGPy8EG_A14N23LO_TT_bWN_400_310_MadSpin_2L15 (DeltaM =  90); Not available in mc16e
    'stop2l_400_280' : ['436209'], # MGPy8EG_A14N23LO_TT_bWN_400_280_MadSpin_2L15 (DeltaM = 120); Not available in mc16e
    'stop2l_400_250' : ['436210'], # MGPy8EG_A14N23LO_TT_bWN_400_250_MadSpin_2L15 (DeltaM = 150)
    'stop2l_400_235' : ['436211'], # MGPy8EG_A14N23LO_TT_bWN_400_235_MadSpin_2L15 (DeltaM = 165)
    'stop2l_425_335' : ['436212'], # MGPy8EG_A14N23LO_TT_bWN_425_335_MadSpin_2L15 (DeltaM =  90)
    'stop2l_425_305' : ['436213'], # MGPy8EG_A14N23LO_TT_bWN_435_305_MadSpin_2L15 (DeltaM = 120)
    'stop2l_425_275' : ['436214'], # MGPy8EG_A14N23LO_TT_bWN_425_275_MadSpin_2L15 (DeltaM = 150)
    'stop2l_425_260' : ['436215'], # MGPy8EG_A14N23LO_TT_bWN_425_260_MadSpin_2L15 (DeltaM = 165)
    'stop2l_450_360' : ['436216'], # MGPy8EG_A14N23LO_TT_bWN_450_360_MadSpin_2L15 (DeltaM =  90)
    'stop2l_450_330' : ['436217'], # MGPy8EG_A14N23LO_TT_bWN_450_330_MadSpin_2L15 (DeltaM = 120)
    'stop2l_450_300' : ['436218'], # MGPy8EG_A14N23LO_TT_bWN_450_300_MadSpin_2L15 (DeltaM = 150)
    'stop2l_450_285' : ['436219'], # MGPy8EG_A14N23LO_TT_bWN_450_285_MadSpin_2L15 (DeltaM = 165)
    'stop2l_475_385' : ['436220'], # MGPy8EG_A14N23LO_TT_bWN_475_385_MadSpin_2L15 (DeltaM =  90) EXTRA STATS
    'stop2l_475_355' : ['436221'], # MGPy8EG_A14N23LO_TT_bWN_475_355_MadSpin_2L15 (DeltaM = 120) EXTRA STATS
    'stop2l_475_325' : ['436222'], # MGPy8EG_A14N23LO_TT_bWN_475_325_MadSpin_2L15 (DeltaM = 150) EXTRA STATS
    'stop2l_475_310' : ['436223'], # MGPy8EG_A14N23LO_TT_bWN_475_310_MadSpin_2L15 (DeltaM = 165)
    'stop2l_500_410' : ['436224'], # MGPy8EG_A14N23LO_TT_bWN_500_410_MadSpin_2L15 (DeltaM =  90)
    'stop2l_500_380' : ['436225'], # MGPy8EG_A14N23LO_TT_bWN_500_380_MadSpin_2L15 (DeltaM = 120)
    'stop2l_500_350' : ['436226'], # MGPy8EG_A14N23LO_TT_bWN_500_350_MadSpin_2L15 (DeltaM = 150)
    'stop2l_500_335' : ['436227'], # MGPy8EG_A14N23LO_TT_bWN_500_335_MadSpin_2L15 (DeltaM = 165)
    'stop2l_550_460' : ['436228'], # MGPy8EG_A14N23LO_TT_bWN_550_460_MadSpin_2L15 (DeltaM =  90)
    'stop2l_550_430' : ['436229'], # MGPy8EG_A14N23LO_TT_bWN_550_430_MadSpin_2L15 (DeltaM = 120)
    'stop2l_550_400' : ['436230'], # MGPy8EG_A14N23LO_TT_bWN_550_400_MadSpin_2L15 (DeltaM = 150)
    'stop2l_550_385' : ['436231'], # MGPy8EG_A14N23LO_TT_bWN_550_385_MadSpin_2L15 (DeltaM = 165)
    'stop2l_600_510' : ['436232'], # MGPy8EG_A14N23LO_TT_bWN_600_510_MadSpin_2L15 (DeltaM =  90)
    'stop2l_600_480' : ['436233'], # MGPy8EG_A14N23LO_TT_bWN_600_480_MadSpin_2L15 (DeltaM = 120)
    'stop2l_600_450' : ['436234'], # MGPy8EG_A14N23LO_TT_bWN_600_450_MadSpin_2L15 (DeltaM = 150)
    'stop2l_600_435' : ['436235'], # MGPy8EG_A14N23LO_TT_bWN_600_435_MadSpin_2L15 (DeltaM = 165) 

    ############################################################################
    # TOP
    ############################################################################
    # TTbar
    "ttbar_dilep" : [
                "410472", #PhPy8EG_A14_ttbar_hdamp258p75_dilep
    ],
    "ttbar_nonallhad" : [
                "410470", #PhPy8EG_A14_ttbar_hdamp258p75_nonallhad
    ],

    # Single Top
    "st-channel" : [
                "410644", #PowhegPythia8EvtGen_A14_singletop_schan_lept_top
                "410645", #PowhegPythia8EvtGen_A14_singletop_schan_lept_antitop
                "410658", #PhPy8EG_A14_tchan_BW50_lept_top
                "410659", #PhPy8EG_A14_tchan_BW50_lept_antitop
    ],
    "tZ" : [
                "410560", #MadGraphPythia8EvtGen_A14_tZ_4fl_tchan_noAllHad
    ],
    "Wt_dilep" : [
                "410648", #PP8_A14_Wt_DR_dilepton_top
                "410649", #PP8_A14_Wt_DR_dilepton_antitop
    ],
    "Wt_incl" : [ # NOTE: Overlaps with Wt_dilep
                "410646", #PowhegPythia8EvtGen_A14_Wt_DR_inclusive_top
                "410647", #PowhegPythia8EvtGen_A14_Wt_DR_inclusive_antitop
    ],

    ############################################################################
    # TTBAR+X
    ############################################################################
    "ttgamma" : [
                "410389", #MadGraphPythia8EvtGen_A14NNPDF23_ttgamma_nonallhadronic
    ],
    "ttVV" : [
                "410081", #MadGraphPythia8EvtGen_A14NNPDF23_ttbarWW
                ##"410217", #aMcAtNloHerwigppEvtGen_UEEE5_CTEQ6L1_CT10ME_260000_tWZDR
    ],
    "ttV" : [
                "410155", #aMcAtNloPythia8EvtGen_MEN30NLO_A14N23LO_ttW
                "410218", #aMcAtNloPythia8EvtGen_MEN30NLO_A14N23LO_ttee
                "410219", #aMcAtNloPythia8EvtGen_MEN30NLO_A14N23LO_ttmumu
                "410220", #aMcAtNloPythia8EvtGen_MEN30NLO_A14N23LO_tttautau
    ],
    '4topSM' : [
                "410080", #MadGraphPythia8EvtGen_A14NNPDF23_4topSM
    ],

    ############################################################################
    # V+JETS
    ############################################################################
    # Drell-Yan Samples
    "drellyan" : [
                "364198", #Sherpa221_Zmm_Mll10_40_0_70_BV
                "364199", #Sherpa221_Zmm_Mll10_40_0_70_BFilt
                "364200", #Sherpa221_Zmm_Mll10_40_70_280_BV
                "364201", #Sherpa221_Zmm_Mll10_40_70_280_BFilt
                "364202", #Sherpa221_Zmm_Mll10_40_280_E_CMS_BV
                "364203", #Sherpa221_Zmm_Mll10_40_280_E_CMS_BFilt
                "364204", #Sherpa221_Zee_Mll10_40_0_70_BV
                "364205", #Sherpa221_Zee_Mll10_40_0_70_BFilt
                "364206", #Sherpa221_Zee_Mll10_40_70_280_BV
                "364207", #Sherpa221_Zee_Mll10_40_70_280_BFilt
                "364208", #Sherpa221_Zee_Mll10_40_280_E_CMS_BV
                "364209", #Sherpa221_Zee_Mll10_40_280_E_CMS_BFilt
                "364210", #Sherpa221_Ztt_Mll10_40_0_70_BV
                "364211", #Sherpa221_Ztt_Mll10_40_0_70_BFilt
                "364212", #Sherpa221_Ztt_Mll10_40_70_280_BV
                "364213", #Sherpa221_Ztt_Mll10_40_70_280_BFilt
                "364214", #Sherpa221_Ztt_Mll10_40_280_E_CMS_BV
                "364215", #Sherpa221_Ztt_Mll10_40_280_E_CMS_BFil
    ],
    
    # W(->lnu)+jets
    "wjets" : [
                "364156", #Sherpa221_Wmunu_0_70_CVBV
                "364157", #Sherpa221_Wmunu_0_70_CFiltBV
                "364158", #Sherpa221_Wmunu_0_70_BFilt
                "364159", #Sherpa221_Wmunu_70_140_CVBV
                "364160", #Sherpa221_Wmunu_70_140_CFiltBV
                "364161", #Sherpa221_Wmunu_70_140_BFilt
                "364162", #Sherpa221_Wmunu_140_280_CVBV
                "364163", #Sherpa221_Wmunu_140_280_CFiltBV
                "364164", #Sherpa221_Wmunu_140_280_BFilt
                "364165", #Sherpa221_Wmunu_280_500_CVBV
                "364166", #Sherpa221_Wmunu_280_500_CFiltBV
                "364167", #Sherpa221_Wmunu_280_500_BFilt
                "364168", #Sherpa221_Wmunu_500_1000
                "364169", #Sherpa221_Wmunu_1000_E_CMS
                "364170", #Sherpa221_Wenu_0_70_CVBV
                "364171", #Sherpa221_Wenu_0_70_CFiltBV
                "364172", #Sherpa221_Wenu_0_70_BFilt
                "364173", #Sherpa221_Wenu_70_140_CVBV
                "364174", #Sherpa221_Wenu_70_140_CFiltBV
                "364175", #Sherpa221_Wenu_70_140_BFilt
                "364176", #Sherpa221_Wenu_140_280_CVBV
                "364177", #Sherpa221_Wenu_140_280_CFiltBV
                "364179", #Sherpa221_Wenu_280_500_CVBV
                "364180", #Sherpa221_Wenu_280_500_CFiltBV
                "364181", #Sherpa221_Wenu_280_500_BFilt
                "364182", #Sherpa221_Wenu_500_1000
                "364183", #Sherpa221_Wenu_1000_E_CMS
                "364184", #Sherpa221_Wtaunu_0_70_CVBV
                "364185", #Sherpa221_Wtaunu_0_70_CFiltBV
                "364186", #Sherpa221_Wtaunu_0_70_BFilt
                "364187", #Sherpa221_Wtaunu_70_140_CVBV
                "364189", #Sherpa221_Wtaunu_70_140_BFilt
                "364190", #Sherpa221_Wtaunu_140_280_CVBV
                "364191", #Sherpa221_Wtaunu_140_280_CFiltBV
                "364192", #Sherpa221_Wtaunu_140_280_BFilt
                "364193", #Sherpa221_Wtaunu_280_500_CVBV
                "364194", #Sherpa221_Wtaunu_280_500_CFiltBV
                "364195", #Sherpa221_Wtaunu_280_500_BFilt
                "364196", #Sherpa221_Wtaunu_500_1000
                "364197", #Sherpa221_Wtaunu_1000_E_CM
    ],
    
    # Z(->ll)+jets
    "zjets_mumu" : [
                "364100", #Sherpa221_Zmumu_0_70_CVBV
                "364101", #Sherpa221_Zmumu_0_70_CFiltBV
                "364102", #Sherpa221_Zmumu_0_70_BFilt
                "364103", #Sherpa221_Zmumu_70_140_CVBV
                "364104", #Sherpa221_Zmumu_70_140_CFiltBV
                "364105", #Sherpa221_Zmumu_70_140_BFilt
                "364106", #Sherpa221_Zmumu_140_280_CVBV
                "364107", #Sherpa221_Zmumu_140_280_CFiltBV
                "364108", #Sherpa221_Zmumu_140_280_BFilt
                "364109", #Sherpa221_Zmumu_280_500_CVBV
                "364110", #Sherpa221_Zmumu_280_500_CFiltBV
                "364111", #Sherpa221_Zmumu_280_500_BFilt
                "364112", #Sherpa221_Zmumu_500_1000
                "364113", #Sherpa221_Zmumu_1000_E_CMS
    ],
    "zjets_ee" : [
                "364114", #Sherpa221_Zee_0_70_CVBV
                "364115", #Sherpa221_Zee_0_70_CFiltBV
                "364116", #Sherpa221_Zee_0_70_BFilt
                "364117", #Sherpa221_Zee_70_140_CVBV
                "364118", #Sherpa221_Zee_70_140_CFiltBV
                "364119", #Sherpa221_Zee_70_140_BFilt
                "364120", #Sherpa221_Zee_140_280_CVBV
                "364121", #Sherpa221_Zee_140_280_CFiltBV
                "364122", #Sherpa221_Zee_140_280_BFilt
                "364123", #Sherpa221_Zee_280_500_CVBV
                "364124", #Sherpa221_Zee_280_500_CFiltBV
                "364125", #Sherpa221_Zee_280_500_BFilt
                "364126", #Sherpa221_Zee_500_1000
                "364127", #Sherpa221_Zee_1000_E_CMS
    ],
    "zjets_tautau" : [
                "364128", #Sherpa221_Ztautau_0_70_CVBV
                "364129", #Sherpa221_Ztautau_0_70_CFiltBV
                "364130", #Sherpa221_Ztautau_0_70_BFilt
                "364131", #Sherpa221_Ztautau_70_140_CVBV
                "364132", #Sherpa221_Ztautau_70_140_CFiltBV
                "364133", #Sherpa221_Ztautau_70_140_BFilt
                "364134", #Sherpa221_Ztautau_140_280_CVBV
                "364135", #Sherpa221_Ztautau_140_280_CFiltBV
                "364136", #Sherpa221_Ztautau_140_280_BFilt
                "364137", #Sherpa221_Ztautau_280_500_CVBV
                "364138", #Sherpa221_Ztautau_280_500_CFiltBV
                "364139", #Sherpa221_Ztautau_280_500_BFilt
                "364140", #Sherpa221_Ztautau_500_1000
                "364141", #Sherpa221_Ztautau_1000_E_CM
    ],
    "zgamma" : [
            "364500", #Sherpa_222_NNPDF30NNLO_eegamma_pty_7_15
            "364501", #Sherpa_222_NNPDF30NNLO_eegamma_pty_15_35
            "364502", #Sherpa_222_NNPDF30NNLO_eegamma_pty_35_70
            "364503", #Sherpa_222_NNPDF30NNLO_eegamma_pty_70_140
            "364504", #Sherpa_222_NNPDF30NNLO_eegamma_pty_140_E_CMS
            "364505", #Sherpa_222_NNPDF30NNLO_mumugamma_pty_7_15
            "364506", #Sherpa_222_NNPDF30NNLO_mumugamma_pty_15_35
            "364507", #Sherpa_222_NNPDF30NNLO_mumugamma_pty_35_70
            "364508", #Sherpa_222_NNPDF30NNLO_mumugamma_pty_70_140
            "364509", #Sherpa_222_NNPDF30NNLO_mumugamma_pty_140_E_CMS
            "364510", #Sherpa_222_NNPDF30NNLO_tautaugamma_pty_7_15
            "364511", #Sherpa_222_NNPDF30NNLO_tautaugamma_pty_15_35
            "364512", #Sherpa_222_NNPDF30NNLO_tautaugamma_pty_35_70
            "364514", #Sherpa_222_NNPDF30NNLO_tautaugamma_pty_140_E_CMS
    ],
    "wgamma" : [
            "364521", #Sherpa_222_NNPDF30NNLO_enugamma_pty_7_15
            "364522", #Sherpa_222_NNPDF30NNLO_enugamma_pty_15_35
            "364523", #Sherpa_222_NNPDF30NNLO_enugamma_pty_35_70
            "364524", #Sherpa_222_NNPDF30NNLO_enugamma_pty_70_140
            "364525", #Sherpa_222_NNPDF30NNLO_enugamma_pty_140_E_CMS
            "364526", #Sherpa_222_NNPDF30NNLO_munugamma_pty_7_15
            "364527", #Sherpa_222_NNPDF30NNLO_munugamma_pty_15_35
            "364528", #Sherpa_222_NNPDF30NNLO_munugamma_pty_35_70
            "364529", #Sherpa_222_NNPDF30NNLO_munugamma_pty_70_140
            "364530", #Sherpa_222_NNPDF30NNLO_munugamma_pty_140_E_CMS
            "364531", #Sherpa_222_NNPDF30NNLO_taunugamma_pty_7_15
            "364532", #Sherpa_222_NNPDF30NNLO_taunugamma_pty_15_35
            "364533", #Sherpa_222_NNPDF30NNLO_taunugamma_pty_35_70
            "364534", #Sherpa_222_NNPDF30NNLO_taunugamma_pty_70_140
            "364535", #Sherpa_222_NNPDF30NNLO_taunugamma_pty_140_E_CMS
    ],
    ############################################################################
    # MULTIBOSON
    ############################################################################
    # 1L Diboson
    "1l_diboson" : [
                "364255", #Sherpa_222_lvvv
                "364304", #Sherpa_222_NNPDF30NNLO_ggWmlvWpqq
                "364305", #Sherpa_222_NNPDF30NNLO_ggWplvWmqq
    ],

    # 2L Diboson
    "2l_diboson" : [
                "345715", #Sherpa_222_NNPDF30NNLO_ggllvvInt
                "345718", #Sherpa_222_NNPDF30NNLO_ggllvvWW
                "345723", #Sherpa_222_NNPDF30NNLO_ggllvvZZ
                "364254", #Sherpa_222_NNPDF30NNLO_llvv
                "364285", #Sherpa_222_NNPDF30NNLO_llvvjj_EW6
                "364286", #Sherpa_222_NNPDF30NNLO_llvvjj_ss_EW4
                "364287", #Sherpa_222_NNPDF30NNLO_llvvjj_ss_EW6
                "364290", #Sherpa_222_NNPDF30NNLO_llvv_lowMllPtComplement
                "364302", #Sherpa_222_NNPDF30NNLO_ggZllZqq
    ],

    # 3L Diboson
    "3l_diboson" : [
                "364253", #Sherpa_222_lllv
                "364284", #Sherpa_222_lllvjj
                "364289", #Sherpa_222_lllv_lowMllPtComplement
    ],

    # 4L Diboson
    "4l_diboson" : [
                "345705", #Sherpa_222_NNPDF30NNLO_ggllll_0M4l130
                "345706", #Sherpa_222_NNPDF30NNLO_ggllll_130M4l
                "364250", #Sherpa_222_NNPDF30NNLO_llll
                "364283", #Sherpa_222_NNPDF30NNLO_lllljj_EW6
                "364288", #Sherpa_222_llll_lowMllPtComplement
    ],

    # Triboson
    "triboson" : [
                ##"363507", #Sherpa_222_WWZ_3l1v2j_EW6
                ##"363508", #Sherpa_222_WZZ_4l2j_EW6
                ##"363509", #Sherpa_222_WZZ_3l1v2j_EW6
                "364242", #Sherpa_222_NNPDF30NNLO_WWW_3l3v_EW6
                "364243", #Sherpa_222_NNPDF30NNLO_WWZ_4l2v_EW6
                "364244", #Sherpa_222_NNPDF30NNLO_WWZ_2l4v_EW6
                "364245", #Sherpa_222_NNPDF30NNLO_WZZ_5l1v_EW6
                "364246", #Sherpa_222_NNPDF30NNLO_WZZ_3l3v_EW6
                "364247", #Sherpa_222_NNPDF30NNLO_ZZZ_6l0v_EW6
                "364248", #Sherpa_222_NNPDF30NNLO_ZZZ_4l2v_EW6
                "364249", #Sherpa_222_NNPDF30NNLO_ZZZ_2l4v_EW6
                ##"364336", #Sherpa_222_NNPDF30NNLO_WpWpWn_sslvlvjj_EW6
                ##"364337", #Sherpa_222_NNPDF30NNLO_WnWnWp_sslvlvjj_EW6
                ##"364338", #Sherpa_222_NNPDF30NNLO_WpWpWn_oslvlvjj_EW6
                ##"364339", #Sherpa_222_NNPDF30NNLO_WnWnWp_oslvlvjj_EW6
    ],

    ############################################################################
    # HIGGS
    ############################################################################
    # Higgs ggH
    "higgs_ggH" : [
                "345120", #PowhegPy8EG_NNLOPS_nnlo_30_ggH125_tautaul13l7
                "345121", #PowhegPy8EG_NNLOPS_nnlo_30_ggH125_tautaulm15hp20
                "345122", #PowhegPy8EG_NNLOPS_nnlo_30_ggH125_tautaulp15hm20
                "343393", #PowhegPy8EG_CT10_AZNLOCTEQ6L1_ggH125_WWlvlv_EF_15_5_highMjj
    ],

    # Higgs VBF
    "higgs_VBF" : [
                "346190", #PowhegPy8EG_NNPDF30_AZNLOCTEQ6L1_VBFH125_tautaul13l7
                ##"345074", #PowhegPy8EG_NNPDF30_AZNLOCTEQ6L1_VBFH125_tautaulm15hp20
                ##"345075", #PowhegPy8EG_NNPDF30_AZNLOCTEQ6L1_VBFH125_tautaulp15hm20
    ],

    # Higgs ttH   
    "higgs_ttH" : [
                ## No samples available in all campaigns
    ],

    # Higgs VH
    "higgs_VH" : [
                "342284", #Pythia8EvtGen_A14NNPDF23LO_WH125_inc
                "342285", #Pythia8EvtGen_A14NNPDF23LO_ZH125_inc
    ],

    ############################################################################
    # MULTIJET AND MINIMUM BIAS
    ############################################################################

    ############################################################################
    # B PHYSICS
    ############################################################################

    ############################################################################
    # DATA
    ############################################################################
    "data15" : [ 
                "284484",
                "284427",
                "284420",
                "284285",
                "284213",
                "284154",
                "284006",
                "283780",
                "283608",
                "283429",
                "283270",
                "283155",
                "283074",
                "282992",
                "282784",
                "282712",
                "282631",
                "282625",
                "281411",
                "281385",
                "281317",
                "281075",
                "281074",
                "281070",
                "280977",
                "280950",
                "280862",
                "280853",
                "280753",
                "280673",
                "280614",
                "280520",
                "280500",
                "280464",
                "280423",
                "280368",
                "280319",
                "280273",
                "280231",
                "279984",
                "279932",
                "279928",
                "279867",
                "279813",
                "279764",
                "279685",
                "279598",
                "279515",
                "279345",
                "279284",
                "279279",
                "279259",
                "279169",
                "278968",
                "278912",
                "278880",
                "276954",
                "276952",
                "276790",
                "276778",
                "276689",
                "276511",
                "276416",
                "276336",
                "276329",
                "276262"
    ],
    "data16" : [
                "297730",
                "298595",
                "298609",
                "298633",
                "298687",
                "298690",
                "298771",
                "298773",
                "298862",
                "298967",
                "299055",
                "299144",
                "299147",
                "299184",
                "299243",
                "299584",
                "300279",
                "300345",
                "300415",
                "300418",
                "300487",
                "300540",
                "300571",
                "300600",
                "300655",
                "300687",
                "300784",
                "300800",
                "300863",
                "300908",
                "301912",
                "301915",
                "301918",
                "301932",
                "301973",
                "302053",
                "302137",
                "302265",
                "302269",
                "302300",
                "302347",
                "302380",
                "302391",
                "302393",
                "302737",
                "302831",
                "302872",
                "302919",
                "302925",
                "302956",
                "303007",
                "303079",
                "303201",
                "303208",
                "303264",
                "303266",
                "303291",
                "303304",
                "303338",
                "303421",
                "303499",
                "303560",
                "303638",
                "303832",
                "303846",
                "303892",
                "303943",
                "304006",
                "304008",
                "304128",
                "304178",
                "304198",
                "304211",
                "304243",
                "304308",
                "304337",
                "304409",
                "304431",
                "304494",
                "305380",
                "305543",
                "305571",
                "305618",
                "305671",
                "305674",
                "305723",
                "305727",
                "305735",
                "305777",
                "305811",
                "305920",
                "306269",
                "306278",
                "306310",
                "306384",
                "306419",
                "306442",
                "306448",
                "306451",
                "307126",
                "307195",
                "307259",
                "307306",
                "307354",
                "307358",
                "307394",
                "307454",
                "307514",
                "307539",
                "307569",
                "307601",
                "307619",
                "307656",
                "307710",
                "307716",
                "307732",
                "307861",
                "307935",
                "308047",
                "308084",
                "309375",
                "309390",
                "309440",
                "309516",
                "309640",
                "309674",
                "309759",
                "310015",
                "310247",
                "310249",
                "310341",
                "310370",
                "310405",
                "310468",
                "310473",
                "310634",
                "310691",
                "310738",
                "310809",
                "310863",
                "310872",
                "310969",
                "311071",
                "311170",
                "311244",
                "311287",
                "311321",
                "311365",
                "311402",
                "311473",
                "311481",
    ],
    "data17" : [
                "340453",
                "340368",
                "340072",
                "340030",
                "339957",
                "339849",
                "339758",
                "339590",
                "339562",
                "339535",
                "339500",
                "339435",
                "339396",
                "339387",
                "339346",
                "339205",
                "339070",
                "339037",
                "338987",
                "338967",
                "338933",
                "338897",
                "338846",
                "338834",
                "338767",
                "338712",
                "338675",
                "338608",
                "338498",
                "338480",
                "338377",
                "338349",
                "338263",
                "338259",
                "338220",
                "338183",
                "337833",
                "337705",
                "337662",
                "337542",
                "337491",
                "337451",
                "337404",
                "337371",
                "337335",
                "337263",
                "337215",
                "337176",
                "337156",
                "337107",
                "337052",
                "337005",
                "336998",
                "336944",
                "336927",
                "336915",
                "336852",
                "336832",
                "336782",
                "336719",
                "336678",
                "336630",
                "336567",
                "336548",
                "336506",
                "335290",
                "335282",
                "335222",
                "335177",
                "335170",
                "335131",
                "335083",
                "335082",
                "335056",
                "335022",
                "335016",
                "334993",
                "334960",
                "334907",
                "334890",
                "334878",
                "334849",
                "334842",
                "334779",
                "334737",
                "334678",
                "334637",
                "334588",
                "334580",
                "334564",
                "334487",
                "334455",
                "334443",
                "334413",
                "334384",
                "334350",
                "334317",
                "334264",
                "333994",
                "333979",
                "333904",
                "333853",
                "333828",
                "333778",
                "333707",
                "333650",
                "333519",
                "333487",
                "333469",
                "333426",
                "333380",
                "333367",
                "333192",
                "333181",
                "332955",
                "332953",
                "332915",
                "332896",
                "332720",
                "332304",
                "332303",
                "331975",
                "331951",
                "331905",
                "331875",
                "331860",
                "331825",
                "331804",
                "331772",
                "331742",
                "331710",
                "331697",
                "331239",
                "331215",
                "331129",
                "331085",
                "331082",
                "331033",
                "330470",
                "330294",
                "330203",
                "330166",
                "330160",
                "330101",
                "330079",
                "330074",
                "330025",
                "329964",
                "329869",
                "329835",
                "329829",
                "329780",
                "329778",
                "329716",
                "328393",
                "328374",
                "328333",
                "328263",
                "328221",
                "328099",
                "328042",
                "327862",
                "327860",
                "327764",
                "327761",
                "327745",
                "327662",
                "327636",
                "327582",
                "327490",
                "327342",
                "327265",
                "327103",
                "327057",
                "326945",
                "326923",
                "326870",
                "326834",
                "326695",
                "326657",
                "326551",
                "326468",
                "326446",
                "326439",
                "325790",
                "325713",
    ],
    "data18" : [
                "364292",
                "364214",
                "364160",
                "364098",
                "364076",
                "364030",
                "363979",
                "363947",
                "363910",
                "363830",
                "363738",
                "363710",
                "363664",
                "363400",
                "363262",
                "363198",
                "363129",
                "363096",
                "363033",
                "362776",
                "362661",
                "362619",
                "362552",
                "362445",
                "362388",
                "362354",
                "362345",
                "362297",
                "362204",
                "361862",
                "361795",
                "361738",
                "360402",
                "360373",
                "360348",
                "360309",
                "360293",
                "360244",
                "360209",
                "360161",
                "360129",
                "360063",
                "360026",
                "359918",
                "359872",
                "359823",
                "359766",
                "359735",
                "359717",
                "359678",
                "359677",
                "359623",
                "359593",
                "359586",
                "359541",
                "359472",
                "359441",
                "359398",
                "359355",
                "359310",
                "359286",
                "359279",
                "359191",
                "359171",
                "359170",
                "359124",
                "359058",
                "359010",
                "358985",
                "358656",
                "358615",
                "358577",
                "358541",
                "358516",
                "358395",
                "358333",
                "358325",
                "358300",
                "358233",
                "358215",
                "358175",
                "358115",
                "358096",
                "358031",
                "357962",
                "357887",
                "357821",
                "357772",
                "357750",
                "357713",
                "357679",
                "357620",
                "357539",
                "357500",
                "357451",
                "357409",
                "357355",
                "357293",
                "357283",
                "357193",
                "356259",
                "356250",
                "356205",
                "356177",
                "356124",
                "356095",
                "356077",
                "355995",
                "355877",
                "355861",
                "355848",
                "355754",
                "355651",
                "355650",
                "355599",
                "355563",
                "355544",
                "355529",
                "355273",
                "355261",
                "352514",
                "352494",
                "352448",
                "352436",
                "352394",
                "352340",
                "352274",
                "352107",
                "352056",
                "351969",
                "351894",
                "351832",
                "351698",
                "351671",
                "351636",
                "351628",
                "351550",
                "351455",
                "351364",
                "351359",
                "351325",
                "351296",
                "351223",
                "351160",
                "351062",
                "350923",
                "350880",
                "350848",
                "350842",
                "350803",
                "350751",
                "350749",
                "350682",
                "350531",
                "350479",
                "350440",
                "350361",
                "350310",
                "350220",
                "350184",
                "350160",
                "350144",
                "350121",
                "350067",
                "350013",
                "349977",
                "349944",
                "349842",
                "349841",
                "349693",
                "349646",
                "349637",
                "349592",
                "349582",
                "349534",
                "349533",
                "349526",
                "349498",
                "349481",
                "349451",
                "349335",
                "349327",
                "349309",
                "349268",
                "349169",
                "349114",
                "349111",
                "349051",
                "349033",
                "349014",
                "349011",
                "348895",
                "348894",
                "348885",
    ],
}
def get_mc_groups():
    return {k: DSID_GROUPS[k] for k in DSID_GROUPS if "data" not in k}

def get_data_groups(year=0):
    pattern = "data"
    if year: pattern += str(year)
    return {k: DSID_GROUPS[k] for k in DSID_GROUPS if pattern in k}

def get_all_groups():
    return DSID_GROUPS
