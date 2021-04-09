

#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_tree_to_hist.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_other.hh"
using namespace mRoot;

void get_renorm_factor(std::string prefix, vector<string> files, std::string tree_name, string central_weight_rule, string alt_weight_rule_1, string alt_weight_rule_2, string & answer_1, string & answer_2){
  double central_integral = pm::get_ttree_integral(prefix, files, tree_name, central_weight_rule);
  double alt_integral_1   = pm::get_ttree_integral(prefix, files, tree_name, alt_weight_rule_1);
  double alt_integral_2   = pm::get_ttree_integral(prefix, files, tree_name, alt_weight_rule_2);
  answer_1 = to_string(central_integral / alt_integral_1) + " * ";
  answer_2 = to_string(central_integral / alt_integral_2) + " * ";
}

void tree_to_hists(string MODE, string RELEASE, string OUTPUT_FILE_NAME, int NBINS, double QCD_norm = -1.0, double QCD_qut = -1.0){
  cout << "tree_to_hists.C, welcome to the converter from mensura ntuples to hists ..." << endl;
  if((not MODE.size()) or (not RELEASE.size()) or (not OUTPUT_FILE_NAME.size()) or (NBINS <= 0)) {
      cerr << "treeToHists.C, wrong parameters, exit" << endl;
      cerr << "   MODE - this is what to to bild - qcd, fcnc, sm etc = " << MODE << endl;
      cerr << "   RELEASE - this is to apply changes beetwen mensura releases = " << RELEASE << endl;
      cerr << "   OUTPUT_FILE_NAME - output file name for hists = " << OUTPUT_FILE_NAME << endl;
      cerr << "   NBINS - number of bins in hists = " << NBINS << endl;
      cerr << "   QCD_norm optional qcd normalization factor" << QCD_norm << endl;
      return 1;
  }

  vector<string> OPTIONS;
  pm::split_string(RELEASE, OPTIONS);
  RELEASE = OPTIONS[0];
  bool BACKGROUND_QCD_CUT = (OPTIONS[1] == "BACK");

  cout << "treeToHists.C, Will use MODE \"" << MODE << "\", RELEASE \"" << RELEASE << "\"" << ", QCD normalization = " << QCD_norm << endl;
  cout << "B-channel QCD CUT = " << BACKGROUND_QCD_CUT << endl;

  if(QCD_norm < 0 and MODE != "QCD"){
    cerr << "Bad  QCD normalization, please provide correct value " << QCD_norm << ", exit ..." << endl;
    return 1;
  }

  //---------- 0. PATH -------------------------------------------------------------------------------------------------------------------------
  string CENTRAL_FOLDER        = "Central";

  // SYSTEMATIC IN THE CENTRAL SAME FILES
  vector<string> VARIATION_SYS_T2 = {};
  vector<string> VARIATION_SYS_T2_base = {"pdf", "PUJetIdTag", /*"PUJetIdMistag",*/ "PileUp", /*"TagRate", "MistagRate",*/ "Ren", "Fac", "RenFac", "LepId", "LepIso", "LepTrig"};
  vector<string> VARIATION_SYS_T2_extra1 = {"Isr", "Fsr"}; // _red _con
  vector<string> VARIATION_SYS_T2_extra2 = {"_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"}; // isr up
  vector<string> VARIATION_SYS_T2_extra3 = {"fsr_G2GG_muR_", "fsr_G2QQ_muR_", "fsr_Q2QG_muR_", "fsr_X2XG_muR_", "fsr_G2GG_cNS_", "fsr_G2QQ_cNS_", "fsr_Q2QG_cNS_", "fsr_X2XG_cNS_"}; // isr up
  vector<string> VARIATION_SYS_btag = {"jes", "lf", "hf", "hfstats1", "hfstats2", "lfstats1", "lfstats2", "cferr1", "cferr2" };
  VARIATION_SYS_T2 = vector_sum( VARIATION_SYS_T2_base, VARIATION_SYS_T2_extra1, VARIATION_SYS_btag );

  // SYSTEMATIC IN THE DIFFERENT FILES
  vector<string> VARIATION_SYS_T1 = { "UnclMET", "MER", "JER", "JEC" };
  vector<string> JER_SYS_FOLDERS = { "eta0-193", "eta193-25", "eta25-3_p0-50", "eta25-3_p50-Inf", "eta3-5_p0-50", "eta3-5_p50-Inf" };
  vector<string> JER_SYS_U, JER_SYS_D   ;
  for(auto it : JER_SYS_FOLDERS){
    JER_SYS_U.push_back( "JERUp_" + it );
    JER_SYS_D.push_back( "JERDown_" + it );
  }
  vector<string> JER_SYS_NAMES = {"JER_eta0_193", "JER_eta193_25", "JER_eta25_3_p0_50", "JER_eta25_3_p50_Inf", "JER_eta3_5_p0_50", "JER_eta3_5_p50_Inf"};

  vector<string> JEC_SYS_FOLDERS = {"eta0-25", "eta25-5"};
  vector<string> JEC_SYS_U, JEC_SYS_D;
  for(auto it : JEC_SYS_FOLDERS){
    JEC_SYS_U.push_back( "JECUp_" + it );
    JEC_SYS_D.push_back( "JECDown_" + it );
  }
  vector<string> JEC_SYS_NAMES = {"JEC_eta0_25", "JEC_eta25_5",};

  string PREFIX_NTUPLES;
  string PATH_PREFIX;
  string PATH_EXCLUDE;
  string PATH_SUSTEMATIC;

  // FILES_TC = FILES_TC_CH;
  bool use_rel_iso_cut = false, use_comphep = false;
  if( RELEASE=="2020_novenber" ){ 
    string ppath = "/scratch2/gvorotni/13TeV/samples/2020-10-18_roch_pdf/" ;
    PATH_PREFIX     = ppath + "tuples_merged/" ;
    PATH_EXCLUDE    = ppath ;
    PATH_SUSTEMATIC = ppath + "tuples_merged/Syst/" ;
    
    use_rel_iso_cut = true;
  }
  else if( RELEASE=="2020_novenber_NoIsoCut" or RELEASE == "2021_jan_NoIsoCut" ){ 
    string ppath = "/scratch2/gvorotni/13TeV/samples/2020-10-18_roch_pdf/" ;
    PATH_PREFIX     = ppath + "tuples_merged/" ;
    PATH_EXCLUDE    = ppath ;
    PATH_SUSTEMATIC = ppath + "tuples_merged/Syst/" ;
    use_rel_iso_cut = false;
  }
  else if( RELEASE=="2020_novenber_CompHEP" ){ 
    string ppath = "/scratch2/gvorotni/13TeV/samples/2020-10-18_roch_pdf/" ;
    PATH_PREFIX     = ppath + "tuples_merged/" ;
    PATH_EXCLUDE    = ppath ;
    PATH_SUSTEMATIC = ppath + "tuples_merged/Syst/" ;
    use_rel_iso_cut = false;
    use_comphep     = true;
  }
  else if( RELEASE=="2021_deep" ){ 
    string ppath = "/scratch2/pvolkov/samples/" ;
    PATH_PREFIX     = ppath + "tuples_merged/"  ;
    PATH_EXCLUDE    = ppath ;
    PATH_SUSTEMATIC = ppath + "tuples_merged/Syst/" ;
    PATH_SUSTEMATIC = "/scratch2/pvolkov/samples/tuples_merged/Syst_new/";
    use_rel_iso_cut = false;
    use_comphep     = false;
  }
  else{
    cerr << "Unknow RELEASE, please provide correct value, exit ..." << endl;
    return 1;
  }

  string SELECTION_WQQ    = "*((Wjets_type ==2)+(Wjets_type ==1))";
  string SELECTION_Wc     = "*(Wjets_type ==4)";
  string SELECTION_Wb     = "*(Wjets_type ==3)";
  // string SELECTION_Wother = "*((Wjets_type ==5)+(Wjets_type ==6))";
  string SELECTION_Wother = "*((Wjets_type == 5))";
  string SELECTION_Wlight = "*((Wjets_type == 6))";


  /*
    Data.root       QCD_data.root        ttbar.root            WW.root
    DY-10-50.root   s-channel.root       tW-channel-tbar.root  WZ.root
    DY-50-Inf.root  t-channel_ch.root    tW-channel-top.root   ZZ.root
    FCNC_tcg.root   t-channel-tbar.root  Wjets_mean.root
    FCNC_tug.root   t-channel-top.root   Wjets.root
  */

  //---------- 1. DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DATA        = {"Data.root"};

  //---------- 2. QCD DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_QCD_DATA    = {"QCD_data.root"}; // QCD_data.root

  //---------- 3. OTHER -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DY     = {"DY-10-50.root", "DY-50-Inf.root"};
  vector <string> FILES_DY_ALT = {"DY_10-50.root", "DY_50-Inf.root"};
  vector <string> FILES_TT     = {"ttbar.root"};
  vector <string> FILES_DB     = {"WW.root", "WZ.root", "ZZ.root"};
  vector <string> FILES_WJ     = {"Wjets.root"};
  vector <string> FILES_SC     = {"s-channel.root"};
  vector <string> FILES_TW     = {"tW-channel-top.root","tW-channel-tbar.root"};
  vector <string> FILES_TC     = {"t-channel-top.root", "t-channel-tbar.root"};
  vector <string> FILES_TC_CH  = {"t-channel_ch.root"}; 

  if(use_comphep) FILES_TC = FILES_TC_CH;

  vector <string> FILES_OTHER;
  FILES_OTHER = vector_sum(FILES_DY, FILES_TT, FILES_DB, FILES_WJ, FILES_SC, FILES_TC,       FILES_TW);

  //---------- 4. ANOMAL -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_FCNC_TUG    = {"FCNC_tug.root"};
  vector <string> FILES_FCNC_TCG    = {"FCNC_tcg.root"};

  //---------- FILL HISTS -------------------------------------------------------------------------------------------------------------------------
  string tree_name = "Vars";
  // double rmin = 30, rmax = 180; BNN_sm_powheg_comphep
  double rmin = 0., rmax = 1.; 
  string vrule, wrule;
  TFile * out_file;
  EventsExcluder * excl = nullptr;

  if(MODE == "QCD"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    // vrule    = "BNN_qcd_tchan_5vars"; // "BNN_qcd"; 
    vrule    = "BNN_qcd_tchan_5vars_2"; // "BNN_qcd"; 

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER +"/";
    EventsExcluder * excl = new EventsExcluder( "/afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/qcd_tchan_5vars/qcd_tchan_5vars_trainEvents.txt" );
    excl->Print();

    string data_weight = "(N_BJ==1) * (RelIso_Lep < 0.04)";
    string qcd_weight  = "weight * (N_BJ==1)";
    string mc_weight   = "weight * (N_BJ==1) * (RelIso_Lep < 0.04)";
    if( not use_rel_iso_cut ){
      data_weight = "(N_BJ==1)";
      qcd_weight  = "weight * (N_BJ==1)";
      mc_weight   = "weight * (N_BJ==1)";
    }

    //fill_hist(hist_name, nbins, rmax, rmin, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule, event_excluder)
    fill_hist("data",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, data_weight,     nullptr);
    fill_hist("QCD",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, qcd_weight,      excl);
    fill_hist("other",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_OTHER,    tree_name, vrule, mc_weight ,      excl);

    out_file->Close();
  }
  else if(MODE == "SM" or MODE == "FCNC_tug" or MODE == "FCNC_tcg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");

    // BNN_sm_pow_comph и BNN_qcd_tchan_2 BNN_qcd_tchan_5vars
    string qcd_qut   = "(BNN_qcd_tchan_5vars_2 > " + std::to_string( QCD_qut ) + ")";
    if( BACKGROUND_QCD_CUT ) qcd_qut = "(BNN_qcd_tchan_5vars_2 > 0.2) * (BNN_qcd_tchan_5vars_2 < " + std::to_string( QCD_qut ) + ")";

    string data_selection   = qcd_qut + " * (N_BJ==1) * (RelIso_Lep < 0.04)";
    string qcd_selection    = qcd_qut + " * weight * " + to_string( QCD_norm );
    string mc_selection     = qcd_qut + " * weight * (N_BJ==1) * (RelIso_Lep < 0.04)";
    if( not use_rel_iso_cut ){
      data_selection = qcd_qut + " * (N_BJ==1)";
      qcd_selection  = qcd_qut + " * weight * (N_BJ==1)";
      mc_selection   = qcd_qut + " * weight * (N_BJ==1)";
    }

    string mc_selection_TW  = mc_selection;
    string mc_selection_sch = mc_selection;
    EventsExcluder * excl   = nullptr;

    // SM < 
    if(MODE == "SM"){
      vrule = "DNN_sm_pow_comph_Wjets"; // BNN_sm_powheg_comphep_wjets
      if( BACKGROUND_QCD_CUT ) vrule = "DNN_sm_pow_comph_Wjets"; // "BNN_qcd_tchan_5vars"; DNN_sm_pow_comph_Wjets, BNN_sm_powheg_comphep_wjets
      // /afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/sm_pow_comh_3/bnn_sm_pow_comph_qcd_5vars_trainEvents.txt
      // /afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/qcd_tchan_5vars/qcd_tchan_5vars_trainEvents.txt
      excl = new EventsExcluder( "/afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/sm_pow_comh_3/bnn_sm_pow_comph_qcd_5vars_trainEvents.txt" );
    }
    //vrule = "Eta_LJ";
    //rmin = -5, rmax = 5; 
    // >

    // FCNC <
    if(MODE == "FCNC_tcg"){
      vrule = "BNN_tcg_2";
      excl = new EventsExcluder( "/afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/fcnc_tcg/bnn_tcg_qcd_5vars_sm_powheg_compheg_2_trainEvents.txt" );
    }
    if(MODE == "FCNC_tug"){
      vrule = "BNN_tug_2";
      excl = new EventsExcluder( "/afs/cern.ch/work/p/pvolkov/public/Networks/13Tev/nov20/fcnc_tug/bnn_tug_qcd_5vars_sm_powheg_compheg_2_trainEvents.txt" );
    }
    // >
    if(excl != nullptr) excl->Print();

    string mc_selection_WQQ    = mc_selection+SELECTION_WQQ;
    string mc_selection_Wc     = mc_selection+SELECTION_Wc;
    string mc_selection_Wb     = mc_selection+SELECTION_Wb;
    string mc_selection_Wother = mc_selection+SELECTION_Wother;
    string mc_selection_Wlight = mc_selection+SELECTION_Wlight;

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER+"/";
    fill_hist("data",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, data_selection, nullptr);
    fill_hist("QCD",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule,  qcd_selection, excl);
    fill_hist("DY",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection,        excl);
    fill_hist("Diboson", NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection,        excl);
    fill_hist("s_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch,    excl);
    fill_hist("tW_ch",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW,     excl);
    fill_hist("WQQ",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ,    excl);
    fill_hist("Wc",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc,     excl);
    fill_hist("Wb",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb,     excl);
    fill_hist("Wother",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother, excl);
    fill_hist("Wlight",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wlight, excl);
    // fill_hist("Wjets",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection,        excl);
    //fill_hist("t_alt",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection,        excl);

    if(MODE == "FCNC_tug") fill_hist("fcnc_tug",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection, excl);
    if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection, excl);

    // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
    for(auto systematic : VARIATION_SYS_T2){
      string mc_selection_up   = mc_selection + "/ weight * weight_" + systematic + "Up";
      string mc_selection_down = mc_selection + "/ weight * weight_" + systematic + "Down";
      string systematic_ = systematic;

      // {"Isr", "Fsr"}
      if( std::find(VARIATION_SYS_T2_extra1.begin(), VARIATION_SYS_T2_extra1.end(), systematic) != VARIATION_SYS_T2_extra1.end() ){
        mc_selection_up   = mc_selection + "* weight_" + systematic + "Up / weight_gen";
        mc_selection_down = mc_selection + "* weight_" + systematic + "Down / weight_gen";
        systematic_ = systematic;
      }
      // {"_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"};
      if( std::find(VARIATION_SYS_T2_extra2.begin(), VARIATION_SYS_T2_extra2.end(), systematic) != VARIATION_SYS_T2_extra2.end() ){
        mc_selection_up   = mc_selection + "/ weight * isr" + systematic + "dn / weight_gen * weight";
        mc_selection_down = mc_selection + "/ weight * isr" + systematic + "up / weight_gen * weight";
        systematic_ = "isr" + systematic;
      }
      // {"fsr_G2GG_muR_", "fsr_G2QQ_muR_", "fsr_Q2QG_muR_", "fsr_X2XG_muR_", "fsr_G2GG_cNS_", "fsr_G2QQ_cNS_", "fsr_Q2QG_cNS_", "fsr_X2XG_cNS_"};
      if( std::find(VARIATION_SYS_T2_extra3.begin(), VARIATION_SYS_T2_extra3.end(), systematic) != VARIATION_SYS_T2_extra3.end() ){
        mc_selection_up   = mc_selection + "/ weight * " + systematic + "up / weight_gen * weight";
        mc_selection_down = mc_selection + "/ weight * " + systematic + "dn / weight_gen * weight";
      }
      if( std::find(VARIATION_SYS_btag.begin(), VARIATION_SYS_btag.end(), systematic) != VARIATION_SYS_btag.end() ){
        mc_selection_up   = mc_selection + "/ weight * weight_btag_up_" + systematic;
        mc_selection_down = mc_selection + "/ weight * weight_btag_down_" + systematic;
      }
      
      std::string dy_fact_up, tt_fact_up, db_fact_up, sc_fact_up, tw_fact_up, wqq_fact_up;
      std::string wc_fact_up, wb_fact_up, wother_fact_up, wj_fact_up, tc_fact_up, tug_fact_up, tcg_fact_up;
      std::string dy_fact_dn, tt_fact_dn, db_fact_dn, sc_fact_dn, tw_fact_dn, wqq_fact_dn;
      std::string wc_fact_dn, wb_fact_dn, wother_fact_dn, wj_fact_dn, tc_fact_dn, tug_fact_dn, tcg_fact_dn;
      if( systematic == "Ren" or systematic == "Fac" or systematic == "RenFac" or systematic == "pdf" ){
        mc_selection_up   = mc_selection + "/ weight * weight_" + systematic + "Up / weight_gen  * weight";
        mc_selection_down = mc_selection + "/ weight * weight_" + systematic + "Down / weight_gen  * weight";
        string sel_centr  = mc_selection;
        if( systematic == "pdf" ){
          mc_selection_up   = mc_selection + "/ weight * weight_" + systematic + "Up";
          mc_selection_down = mc_selection + "/ weight * weight_" + systematic + "Down";
        }
        
        get_renorm_factor(PREFIX_NTUPLES, FILES_DY, tree_name, sel_centr, mc_selection_up, mc_selection_down, dy_fact_up, dy_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_TT, tree_name, sel_centr, mc_selection_up, mc_selection_down, tt_fact_up, tt_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_DB, tree_name, sel_centr, mc_selection_up, mc_selection_down, db_fact_up, db_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_SC, tree_name, sel_centr, mc_selection_up, mc_selection_down, sc_fact_up, sc_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_TW, tree_name, sel_centr, mc_selection_up, mc_selection_down, tw_fact_up, tw_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_WJ, tree_name, sel_centr, mc_selection_up, mc_selection_down, wj_fact_up, wj_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_WJ, tree_name, sel_centr+SELECTION_WQQ, mc_selection_up+SELECTION_WQQ, mc_selection_down+SELECTION_WQQ, wqq_fact_up, wqq_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_WJ, tree_name, sel_centr+SELECTION_Wb, mc_selection_up+SELECTION_Wb, mc_selection_down+SELECTION_Wb, wb_fact_up, wb_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_WJ, tree_name, sel_centr+SELECTION_Wother, mc_selection_up+SELECTION_Wother, mc_selection_down+SELECTION_Wother, wother_fact_up, wother_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_WJ, tree_name, sel_centr+SELECTION_Wc, mc_selection_up+SELECTION_Wc, mc_selection_down+SELECTION_Wc, wc_fact_up, wc_fact_dn);
        get_renorm_factor(PREFIX_NTUPLES, FILES_TC, tree_name, sel_centr, mc_selection_up, mc_selection_down, tc_fact_up, tc_fact_dn);

        // get_renorm_factor(PREFIX_NTUPLES, FILES_TC, tree_name, sel_centr, mc_selection_up, mc_selection_down, tug_fact_up, tug_fact_dn);
        // get_renorm_factor(PREFIX_NTUPLES, FILES_TC, tree_name, sel_centr, mc_selection_up, mc_selection_down, tcg_fact_up, tcg_fact_dn);
        if(MODE == "FCNC_tug") get_renorm_factor(PREFIX_NTUPLES, FILES_FCNC_TUG, tree_name, sel_centr, mc_selection_up, mc_selection_down, tug_fact_up, tug_fact_dn);
        if(MODE == "FCNC_tcg") get_renorm_factor(PREFIX_NTUPLES, FILES_FCNC_TCG, tree_name, sel_centr, mc_selection_up, mc_selection_down, tcg_fact_up, tcg_fact_dn);
      } 

      fill_hist_sys("DY_"+systematic_,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, dy_fact_up + mc_selection_up, dy_fact_dn + mc_selection_down, excl);
      fill_hist_sys("ttbar_"+systematic_,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, tt_fact_up + mc_selection_up, tt_fact_dn + mc_selection_down, excl);
      fill_hist_sys("Diboson_"+systematic_, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, db_fact_up + mc_selection_up, db_fact_dn + mc_selection_down, excl);
      fill_hist_sys("s_ch_"+systematic_,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, sc_fact_up + mc_selection_up, sc_fact_dn + mc_selection_down, excl);
      fill_hist_sys("tW_ch_"+systematic_,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, tw_fact_up + mc_selection_up,  tw_fact_dn + mc_selection_down,  excl);
      fill_hist_sys("WQQ_"+systematic_,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wqq_fact_up + mc_selection_up+SELECTION_WQQ, wqq_fact_dn + mc_selection_down+SELECTION_WQQ, excl);
      fill_hist_sys("Wc_"+systematic_,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wc_fact_up + mc_selection_up+SELECTION_Wc, wc_fact_dn + mc_selection_down+SELECTION_Wc,   excl);
      fill_hist_sys("Wb_"+systematic_,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wb_fact_up + mc_selection_up+SELECTION_Wb, wb_fact_dn + mc_selection_down+SELECTION_Wb,   excl);
      fill_hist_sys("Wother_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wother_fact_up + mc_selection_up+SELECTION_Wother, wother_fact_dn + mc_selection_down+SELECTION_Wother, excl);
      fill_hist_sys("Wlight_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wother_fact_up + mc_selection_up+SELECTION_Wlight, wother_fact_dn + mc_selection_down+SELECTION_Wlight, excl);
      // fill_hist_sys("Wjets_"+systematic_,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wj_fact_up + mc_selection_up, wj_fact_dn + mc_selection_down, excl);
      fill_hist_sys("t_ch_"+systematic_,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC, tree_name, vrule, tc_fact_up+mc_selection_up, tc_fact_dn+mc_selection_down, excl);
      //fill_hist_sys("t_alt_"+systematic_, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC, tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      if(MODE == "FCNC_tug") fill_hist_sys("fcnc_tug_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, tug_fact_up + mc_selection_up, tug_fact_dn + mc_selection_down,        excl);
      if(MODE == "FCNC_tcg") fill_hist_sys("fcnc_tcg_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, tcg_fact_up + mc_selection_up, tcg_fact_dn + mc_selection_down,        excl);
    }


    // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT F0LDERS
    for(auto fprefix : VARIATION_SYS_T1){
      if( fprefix == "JER" ){
        for(int i = 0; i < JER_SYS_NAMES.size(); i++){
          string jer_bin_name = JER_SYS_NAMES[i];
          vector<string> jer_bin_files = { JER_SYS_D[i], JER_SYS_U[i] };
          vector<string> jer_bin_file_pstfixs = {"Down", "Up"};
          for(int j = 0; j < 2; j++){
            string extra_select = "";
            string pstfix = jer_bin_file_pstfixs[j];
            string sname = jer_bin_name + pstfix;
            PREFIX_NTUPLES = PATH_PREFIX + jer_bin_files[j] + "/";
            
            cout << "process folder ... " << PREFIX_NTUPLES << endl;
            fill_hist("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY_ALT,  tree_name, vrule, mc_selection + extra_select,    excl);
            fill_hist("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection + extra_select,        excl);
            fill_hist("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection + extra_select,        excl);
            fill_hist("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch + extra_select,    excl);
            fill_hist("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW + extra_select,     excl);
            fill_hist("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
            fill_hist("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
            fill_hist("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
            fill_hist("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
            fill_hist("Wlight_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wlight + extra_select, excl);
            fill_hist("t_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection + extra_select,        excl);
            //fill_hist("t_alt_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection + extra_select,        excl);

            if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
            if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
          }
        }
      }
      if( fprefix == "JEC" ){
        for(int i = 0; i < JEC_SYS_NAMES.size(); i++){
          string jec_bin_name = JEC_SYS_NAMES[i];
          vector<string> jec_bin_files = { JEC_SYS_D[i], JEC_SYS_U[i] };
          vector<string> jec_bin_file_pstfixs = {"Down", "Up"};
          for(int j = 0; j < 2; j++){
            string extra_select = "";
            string pstfix = jec_bin_file_pstfixs[j];
            string sname = jec_bin_name + pstfix;
            PREFIX_NTUPLES = PATH_PREFIX + jec_bin_files[j] + "/";

            cout << "process folder ... " << PREFIX_NTUPLES << endl;
            fill_hist("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY_ALT,  tree_name, vrule, mc_selection + extra_select,        excl);
            fill_hist("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection + extra_select,        excl);
            fill_hist("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection + extra_select,        excl);
            fill_hist("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch + extra_select,        excl);
            fill_hist("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW + extra_select,        excl);
            fill_hist("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
            fill_hist("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
            fill_hist("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
            fill_hist("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
            fill_hist("Wlight_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wlight + extra_select, excl);
            fill_hist("t_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection + extra_select,        excl);
            //fill_hist("t_alt_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection + extra_select,        excl);

            if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
            if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
          }
        }
      }

      vector<string> pstfixs = {"Down", "Up"};
      for(auto pstfix : pstfixs){
        string extra_select = "";
        // if(fprefix == "JEC") extra_select = "* weight_btag_jes" + pstfix + "_norm / weight_norm";

        string sname = fprefix + pstfix;
        PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        fill_hist("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY_ALT,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch + extra_select,        excl);
        fill_hist("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW + extra_select,        excl);
        fill_hist("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
        fill_hist("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
        fill_hist("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
        fill_hist("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
        fill_hist("Wlight_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wlight + extra_select, excl);
        // fill_hist("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("t_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection + extra_select,        excl);
        //fill_hist("t_alt_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection + extra_select,        excl);

        if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
      }
    }

    vrule = "DNN_sm_pow_comph_wjets";
    // tW-chan: hdamp и isr/fsr (в tW нет PS weights, но не знаю, используешь ли ты эти семплы сейчас)
    // ttbar: hdamp, tune, colourFlip, erdOn, qcd_based и отдельные isr/fsr (как альтернатива PSweights).
    fill_hist("tW_ch_hdampUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_hdamp_up.root", "tW-channel-top_hdamp_up.root"},      tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_hdampDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_hdamp_down.root", "tW-channel-top_hdamp_down.root"},  tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_UETuneUp", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-top_tune_up.root", "tW-channel-tbar_tune_up.root"},  tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_UETuneDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-top_tune_down.root", "tW-channel-tbar_tune_down.root"},  tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_isrUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_isr_up.root", "tW-channel-top_isr_up.root"},      tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_isrDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_isr_down.root", "tW-channel-top_isr_down.root"},  tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_fsrUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_fsr_up.root", "tW-channel-top_fsr_up.root"},      tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("tW_ch_fsrDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"tW-channel-tbar_fsr_down.root", "tW-channel-top_fsr_down.root"},  tree_name, vrule, mc_selection_TW,        excl);

    fill_hist("ttbar_hdampUp",    NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_hdamp_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_hdampDown",  NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_hdamp_down.root"},  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_UETuneUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_tune_up.root"},     tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_UETuneDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_tune_down.root"},   tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_isrUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_isr_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_isrDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_isr_down.root"},  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_fsrUp",   NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_fsr_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_fsrDown", NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_fsr_down.root"},  tree_name, vrule, mc_selection,        excl);

    fill_hist("t_ch_UETuneDown",  NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"t-channel-tbar_tune_down.root", "t-channel-top_tune_down.root"}, tree_name, vrule, mc_selection, excl);
    fill_hist("t_ch_UETuneUp",    NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"t-channel-tbar_tune_up.root",   "t-channel-top_tune_up.root"}, tree_name, vrule, mc_selection, excl);

    // unmariginalysed ... 
    fill_hist("ttbar_colourFlipUp",  NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_colourFlip.root"},   tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_erdOnUp",       NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_erdOn.root"},        tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_QCDbasedUp",    NBINS, rmin, rmax, out_file, PATH_SUSTEMATIC, {"ttbar_qcd_based.root"},    tree_name, vrule, mc_selection,        excl);

    out_file->Close();
  }
  else cout << __FILE__ " : unknow mode " << MODE << endl;

  cout << "hists13Charlie.cpp, done ..." << endl;

  return 0;
}














