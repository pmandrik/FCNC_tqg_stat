

#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_tree_to_hist.hh"
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

  cout << "treeToHists.C, Will use MODE \"" << MODE << "\", RELEASE \"" << RELEASE << "\"" << ", QCD normalization = " << QCD_norm << endl;

  if(QCD_norm < 0 and MODE != "QCD"){
    cerr << "Bad  QCD normalization, please provide correct value " << QCD_norm << ", exit ..." << endl;
    return 1;
  }

  //---------- 0. PATH -------------------------------------------------------------------------------------------------------------------------
  string CENTRAL_FOLDER        = "Central";
  vector<string> VARIATION_SYS_T1 = {"JEC", "JER", "UnclMET", }; // SYSTEMATIC WITCH PRESENT IN THE DIFFERENT FILES
  vector<string> VARIATION_SYS_T2 = {"PileUp", /*"btag_jes",*/ "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" }; // SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
  vector<string> VARIATION_SYS_T2_base = {"PileUp", "TagRate", "MistagRate", "Ren", "Fac", "RenFac", "LepId", "LepIso", "LepTrig"}; // SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
  vector<string> VARIATION_SYS_T2_extra1 = {"Isr", "Fsr"}; // _red _con
  vector<string> VARIATION_SYS_T2_extra2 = {"_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"}; // isr up
  vector<string> VARIATION_SYS_T2_extra3 = {"fsr_G2GG_muR_", "fsr_G2QQ_muR_", "fsr_Q2QG_muR_", "fsr_X2XG_muR_", "fsr_G2GG_cNS_", "fsr_G2QQ_cNS_", "fsr_Q2QG_cNS_", "fsr_X2XG_cNS_"}; // isr up

  // VARIATION_SYS_T2 = vector_sum( VARIATION_SYS_T2_base, VARIATION_SYS_T2_extra1, VARIATION_SYS_T2_extra2, VARIATION_SYS_T2_extra3);
  VARIATION_SYS_T2 = vector_sum( VARIATION_SYS_T2_base, VARIATION_SYS_T2_extra1 );

  string PREFIX_NTUPLES;
  string PATH_PREFIX;
  string PATH_EXCLUDE;
  string PATH_SUSTEMATIC;
  if(RELEASE=="SYS_DEC_N"){ 
/*
    PATH_PREFIX     = "/scratch/gvorotni/13TeV/samples/19-12-04_psweights/tuples_merged/";
    PATH_EXCLUDE    = "/scratch/gvorotni/13TeV/samples/19-12-04_psweights/";
    PATH_SUSTEMATIC = "/scratch/gvorotni/13TeV/samples/19-12-04_psweights/tuples_merged/Syst/";
*/
    string ppath = "/scratch/common/samples/2020-03-17_bugfix/";
    ppath = "/scratch/common/samples/2020-05-01_pujetid/";
    PATH_PREFIX     = ppath + "tuples_merged/";
    PATH_EXCLUDE    = ppath;
    PATH_SUSTEMATIC = ppath + "tuples_merged/Syst/";
  }
  else{
    cerr << "Unknow RELEASE, please provide correct value, exit ..." << endl;
    return 1;
  }

  string SELECTION_WQQ    = "*((Wjets_type ==2)+(Wjets_type ==1))";
  string SELECTION_Wc     = "*(Wjets_type ==4)";
  string SELECTION_Wb     = "*(Wjets_type ==3)";
  string SELECTION_Wother = "*((Wjets_type ==5)+(Wjets_type ==6))";
  // string SELECTION_Wother = "*((Wjets_type ==6))";
  // string SELECTION_Wlight = "*((Wjets_type ==5))";


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
  vector <string> FILES_QCD_DATA    = {"QCD_new.root"}; // QCD_data.root

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

  vector <string> FILES_OTHER;
  FILES_OTHER = vector_sum(FILES_DY, FILES_TT, FILES_DB, FILES_WJ, FILES_SC, FILES_TC,       FILES_TW);

  //---------- 4. ANOMAL -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_FCNC_TUG    = {"FCNC_tug.root"};
  vector <string> FILES_FCNC_TCG    = {"FCNC_tcg.root"};

  //---------- FILL HISTS -------------------------------------------------------------------------------------------------------------------------
  string tree_name = "Vars";
  // double rmin = 30, rmax = 180; 
  double rmin = 0., rmax = 1.; 
  string vrule, wrule;
  TFile * out_file;
  EventsExcluder * excl = nullptr;

  if(MODE == "QCD"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    vrule    = "BNN_qcd_fcnc_new"; // "BNN_qcd"; 

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER +"/";
    EventsExcluder * excl = nullptr;
    //new EventsExcluder( PATH_EXCLUDE + "qcd_11v/qcd_JID_2_trainEvents.txt" );
    //excl->Print();

    //fill_hist(hist_name, nbins, rmax, rmin, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule, event_excluder)
    fill_hist("data",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, "1",           nullptr);
    fill_hist("QCD",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, "weight",      excl);
    fill_hist("other",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_OTHER,    tree_name, vrule, "weight",      excl);

    out_file->Close();
  }
  else if(MODE == "SM" or MODE == "FCNC_tug" or MODE == "FCNC_tcg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd_fcnc_new > " + std::to_string( QCD_qut ) + ") * (N_BJ==1)"; // "(BNN_qcd > 0.7)";

    string data_selection   = qcd_qut;
    string mc_selection     = qcd_qut + " * weight";
    string mc_selection_TW  = mc_selection;
    string mc_selection_sch = mc_selection;
    EventsExcluder * excl   = nullptr;

    // SM < 
    if(MODE == "SM"){
      vrule = "BNN_sm_new"; // "BNN_sm"
      // vrule = "Pt_J2";
      excl = new EventsExcluder( PATH_EXCLUDE + "sm/bnn_sm_trainEvents.txt" );
    }
    //vrule = "Eta_LJ";
    //rmin = -5, rmax = 5; 
    // >

    // FCNC <
    if(MODE == "FCNC_tcg"){
      vrule = "BNN_tcg";
      vrule = "BNN_tcg_NBJ_2";
      //excl = new EventsExcluder( PATH_EXCLUDE + "fcnc_tcg/bnn_tcg_trainEvents.txt" );
    }
    if(MODE == "FCNC_tug"){
      vrule = "BNN_tug";
      vrule = "BNN_tug_NBJ_2";
      //excl = new EventsExcluder( PATH_EXCLUDE + "fcnc_tug/bnn_tug_trainEvents.txt" );
    }
    // >
    excl = nullptr;
    if(excl != nullptr) excl->Print();

    string mc_selection_WQQ    = mc_selection+SELECTION_WQQ;
    string mc_selection_Wc     = mc_selection+SELECTION_Wc;
    string mc_selection_Wb     = mc_selection+SELECTION_Wb;
    string mc_selection_Wother = mc_selection+SELECTION_Wother;
    // string mc_selection_Wlight = mc_selection+SELECTION_Wlight;

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER+"/";
    fill_hist("data",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, data_selection, nullptr);
    fill_hist("QCD",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, data_selection + "*weight*" + to_string(QCD_norm), excl);
    fill_hist("DY",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection,        excl);
    fill_hist("Diboson", NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection,        excl);
    fill_hist("s_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch,    excl);
    fill_hist("tW_ch",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW,     excl);
    fill_hist("WQQ",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ,    excl);
    fill_hist("Wc",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc,     excl);
    fill_hist("Wb",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb,     excl);
    fill_hist("Wother",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother, excl);
    // fill_hist("Wlight",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wlight, excl);
    // fill_hist("Wjets",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection,        excl);

    if(MODE == "FCNC_tug") fill_hist("fcnc_tug",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection, excl);
    if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection, excl);

    // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
    for(auto systematic : VARIATION_SYS_T2){
      string mc_selection_up   = qcd_qut + "* weight_" + systematic + "Up";
      string mc_selection_down = qcd_qut + "* weight_" + systematic + "Down";
      string systematic_ = systematic;

      // {"Isr", "Fsr"}
      if( std::find(VARIATION_SYS_T2_extra1.begin(), VARIATION_SYS_T2_extra1.end(), systematic) != VARIATION_SYS_T2_extra1.end() ){
        mc_selection_up   = qcd_qut + "* weight_" + systematic + "Up / weight_gen * weight";
        mc_selection_down = qcd_qut + "* weight_" + systematic + "Down / weight_gen * weight";
        systematic_ = systematic + "_red";
      }
      if( std::find(VARIATION_SYS_T2_extra2.begin(), VARIATION_SYS_T2_extra2.end(), systematic) != VARIATION_SYS_T2_extra2.end() ){
        mc_selection_up   = qcd_qut + "* isr" + systematic + "dn / weight_gen * weight";
        mc_selection_down = qcd_qut + "* isr" + systematic + "up / weight_gen * weight";
        systematic_ = "isr" + systematic;
      }
      if( std::find(VARIATION_SYS_T2_extra3.begin(), VARIATION_SYS_T2_extra3.end(), systematic) != VARIATION_SYS_T2_extra3.end() ){
        mc_selection_up   = qcd_qut + "* " + systematic + "up / weight_gen * weight";
        mc_selection_down = qcd_qut + "* " + systematic + "dn / weight_gen * weight";
      }
      
      std::string dy_fact_up, tt_fact_up, db_fact_up, sc_fact_up, tw_fact_up, wqq_fact_up;
      std::string wc_fact_up, wb_fact_up, wother_fact_up, wj_fact_up, tc_fact_up, tug_fact_up, tcg_fact_up;
      std::string dy_fact_dn, tt_fact_dn, db_fact_dn, sc_fact_dn, tw_fact_dn, wqq_fact_dn;
      std::string wc_fact_dn, wb_fact_dn, wother_fact_dn, wj_fact_dn, tc_fact_dn, tug_fact_dn, tcg_fact_dn;
      if( systematic == "Ren" or systematic == "Fac" or systematic == "RenFac" ){
        mc_selection_up   = qcd_qut + "* weight_" + systematic + "Up / weight_gen  * weight";
        mc_selection_down = qcd_qut + "* weight_" + systematic + "Down / weight_gen  * weight";
        string sel_centr  = qcd_qut + " * weight";
        
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
      // fill_hist_sys("Wjets_"+systematic_,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, wj_fact_up + mc_selection_up, wj_fact_dn + mc_selection_down, excl);
      fill_hist_sys("t_ch_"+systematic_,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, tc_fact_up + mc_selection_up, tc_fact_dn + mc_selection_down, excl);
      if(MODE == "FCNC_tug") fill_hist_sys("fcnc_tug_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, tug_fact_up + mc_selection_up, tug_fact_dn + mc_selection_down,        excl);
      if(MODE == "FCNC_tcg") fill_hist_sys("fcnc_tcg_"+systematic_,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, tcg_fact_dn + mc_selection_up, tcg_fact_dn + mc_selection_down,        excl);
    }

    // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT F0LDERS
    for(auto fprefix : VARIATION_SYS_T1){
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
        // fill_hist("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("t_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection + extra_select,        excl);

        if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
      }
    }
    out_file->Close();
  }
  else if(MODE == "SM_unmarg" or MODE == "FCNC_tug_unmarg" or MODE == "FCNC_tcg_unmarg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd_bugfix > 0.7)";

    string data_selection = qcd_qut;
    string mc_selection   = qcd_qut + " * weight_norm";
    EventsExcluder * excl = nullptr;

    // SM < 
    if(MODE == "SM_unmarg"){
      vrule = "BNN_SM";
    }
    // >

    // FCNC <
    if(MODE == "FCNC_tcg_unmarg"){
      vrule = "BNN_tcg2";
    }
    if(MODE == "FCNC_tug_unmarg"){
      vrule = "BNN_tug2";
    }
    // >

    PREFIX_NTUPLES = PATH_SUSTEMATIC+"/";

/*
    fill_hist("t_ch_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel_scale_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_scale_down",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel_scale_down.root"},  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_up",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_up.root"},        tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_down",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_down.root"},      tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_scale_up",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel_scale_up.root"},   tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_scale_down",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel_scale_down.root"}, tree_name, vrule, mc_selection,        excl);
*/

    fill_hist("ttbar_matching_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_matching_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_matching_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_matching_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_up.root"},    tree_name, vrule, mc_selection,        excl);

    fill_hist("tW_ch_top_scale_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel-top_scale_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_tbar_scale_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel-tbar_scale_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_tbar_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel-tbar_scale_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_top_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel-top_scale_up.root"},    tree_name, vrule, mc_selection,        excl);

    fill_hist("t_ch_top_matching_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-top_matching_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_tbar_matching_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-tbar_matching_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_top_scale_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-top_scale_down.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_tbar_scale_down",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-tbar_scale_down.root"},    tree_name, vrule, mc_selection,        excl);

    fill_hist("t_ch_top_matching_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-top_matching_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_tbar_matching_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-tbar_matching_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_top_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-top_scale_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_tbar_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel-tbar_scale_up.root"},    tree_name, vrule, mc_selection,        excl);
  }
  else cout << __FILE__ " : unknow mode " << MODE << endl;

  cout << "hists13Charlie.cpp, done ..." << endl;

  return 0;
}














