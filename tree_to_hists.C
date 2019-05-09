

#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
using namespace mRoot;

#include "hist_combo.C"

void tree_to_hists(string MODE, string RELEASE, string OUTPUT_FILE_NAME, int NBINS, double QCD_norm = -1.0){
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
  vector<string> VARIATION_SYS_T1 = {/*"JEC"*/ "JER", "UnclMET"}; // SYSTEMATIC WITCH PRESENT IN THE DIFFERENT FILES
  vector<string> VARIATION_SYS_T2 = {"PileUp", /*"btag_jes", "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2"*/ }; // SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
                 VARIATION_SYS_T2 = {"PileUp", "TagRate", "MistagRate" }; // SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES


  string PREFIX_NTUPLES;
  string PATH_PREFIX;
  string PATH_EXCLUDE;
  string PATH_SUSTEMATIC;
  if(RELEASE=="PUPPI_JID" or RELEASE=="PUPPI_JID_JECF" or RELEASE=="PUPPI_JID_JECB" or RELEASE == "PUPPI_JID_BTAG"){ 
    PATH_PREFIX     = "/scratch/gvorotni/13TeV/samples/17-12-01_DeepCSV/tuples_merged/";
    PATH_EXCLUDE    = "/scratch/gvorotni/13TeV/samples/17-12-01_DeepCSV/";
    PATH_SUSTEMATIC = "/scratch/gvorotni/13TeV/samples/17-12-01_DeepCSV/tuples_merged/Syst";

    PATH_PREFIX     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/18-09-21_sp2/tuples_merged/";
    PATH_EXCLUDE    = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/18-09-21_sp2/";
    PATH_SUSTEMATIC = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/18-09-21_sp2/tuples_merged/Syst/";

    PATH_PREFIX     = "/scratch/gvorotni/13TeV/samples/18-12-11_test_JID_new/tuples_merged/";
    PATH_EXCLUDE    = "/scratch/gvorotni/13TeV/samples/18-12-11_test_JID_new/";
    PATH_SUSTEMATIC = "/scratch/gvorotni/13TeV/samples/18-12-11_test_JID_new/tuples_merged/Syst";
  } else if(RELEASE=="CHS_JID"){ 
    PATH_PREFIX     = "/scratch/gvorotni/13TeV/samples/19-01-21_CHS_JID/tuples_merged/";
    PATH_EXCLUDE    = "/scratch/gvorotni/13TeV/samples/19-01-21_CHS_JID/";
    PATH_SUSTEMATIC = "/scratch/gvorotni/13TeV/samples/19-01-21_CHS_JID/tuples_merged/Syst";
  }
  else{
    cerr << "Unknow RELEASE, please provide correct value, exit ..." << endl;
    return 1;
  }

  string SELECTION_WQQ    = "*((Wjets_type ==2)+(Wjets_type ==1))";
  string SELECTION_Wc     = "*(Wjets_type ==4)";
  string SELECTION_Wb     = "*(Wjets_type ==3)";
  string SELECTION_Wother = "*((Wjets_type ==5)+(Wjets_type ==6))";

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
  vector <string> FILES_QCD_DATA    = {"QCD_data.root"};

  //---------- 3. OTHER -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DY     = {"DY-10-50.root", "DY-50-Inf.root"};
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
    vrule       = "BNN_qcd11"; // "BNN_qcd";
    if(RELEASE=="CHS_JID") vrule       = "BNN_qcd";
    if(RELEASE=="PUPPI_JID_JECB") "(TMath::Abs(Eta_LJ) < 1.4) *" + vrule;
    if(RELEASE=="PUPPI_JID_JECF") "(TMath::Abs(Eta_LJ) > 1.4) *" + vrule;

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER +"/";
    EventsExcluder * excl = nullptr;
    //new EventsExcluder( PATH_EXCLUDE + "qcd_11v/qcd_JID_2_trainEvents.txt" );
    //excl->Print();

    //fill_hist(hist_name, nbins, rmax, rmin, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule, event_excluder)
    fill_hist("data",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, "1",           nullptr);
    fill_hist("QCD",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, "weight",      excl);

    cout << "!!!--" << endl;
    fill_hist("other",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_OTHER,    tree_name, vrule, "weight_norm", excl);
    cout << "!!!--" << endl;

    out_file->Close();
  }
  else if(MODE == "SM" or MODE == "FCNC_tug" or MODE == "FCNC_tcg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd11 > 0.7)"; // "(BNN_qcd > 0.7)";
    if(RELEASE=="PUPPI_JID_BTAG") qcd_qut += "*(N_BJ == 1)";
    if(RELEASE=="CHS_JID") qcd_qut = "(BNN_qcd > 0.7)";
    if(RELEASE=="PUPPI_JID_JECB") qcd_qut = "(TMath::Abs(Eta_LJ) < 1.4) *" + qcd_qut;
    if(RELEASE=="PUPPI_JID_JECF") qcd_qut = "(TMath::Abs(Eta_LJ) > 1.4) *" + qcd_qut;

    string data_selection = qcd_qut;
    string mc_selection    = qcd_qut + " * weight_norm";
    string mc_selection_TW  = qcd_qut + " * weight_norm * 0.5";
    string mc_selection_sch = qcd_qut + " * weight_norm * 0.33";
    EventsExcluder * excl = nullptr;

    // SM < 
    if(MODE == "SM"){
      vrule = "BNN_SM"; // BNN_SM_mix
      // vrule = "Pt_J2";
      excl = new EventsExcluder( PATH_EXCLUDE + "sm/bnn_sm_trainEvents.txt" );
    }
      //vrule = "Eta_LJ";
      //rmin = -5, rmax = 5; 
    // >

    // FCNC <
    if(MODE == "FCNC_tcg"){
      vrule = "BNN_tcg";
      vrule = "BNN_tcg2";
      excl = new EventsExcluder( PATH_EXCLUDE + "fcnc_tcg/bnn_tcg_trainEvents.txt" );
    }
    if(MODE == "FCNC_tug"){
      vrule = "BNN_tug";
      vrule = "BNN_tug2";
      excl = new EventsExcluder( PATH_EXCLUDE + "fcnc_tug/bnn_tug_trainEvents.txt" );
    }
    // >

    if(excl != nullptr) excl->Print();

    string mc_selection_WQQ    = mc_selection+SELECTION_WQQ;
    string mc_selection_Wc     = mc_selection+SELECTION_Wc;
    string mc_selection_Wb     = mc_selection+SELECTION_Wb;
    string mc_selection_Wother = mc_selection+SELECTION_Wother;

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER+"/";
    fill_hist("data",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, data_selection, nullptr);
    fill_hist("QCD",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, data_selection + "*weight*" + to_string(QCD_norm), excl);
    fill_hist("DY",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection,        excl);
    fill_hist("Diboson", NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection,        excl);
    fill_hist("s_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch,        excl);
    fill_hist("tW_ch",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW,        excl);
    fill_hist("WQQ",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ,    excl);
    fill_hist("Wc",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc,     excl);
    fill_hist("Wb",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb,     excl);
    fill_hist("Wother",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother, excl);
    fill_hist("Wjets",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection,        excl);

    fill_hist("t_ch",       NBINS, rmin, rmax, out_file, PREFIX_NTUPLES,  FILES_TC,     tree_name, vrule, mc_selection,        excl);

    if(MODE == "FCNC_tug") fill_hist("fcnc_tug",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection, excl);
    if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection, excl);

    // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
    for(auto systematic : VARIATION_SYS_T2){
      string mc_selection_up   = qcd_qut + "* weight_" + systematic + "Up_norm";
      string mc_selection_down = qcd_qut + "* weight_" + systematic + "Down_norm";
      string mc_selection_up_TW   = qcd_qut + "* weight_" + systematic + "Up_norm * 0.5";
      string mc_selection_down_TW = qcd_qut + "* weight_" + systematic + "Down_norm * 0.5";
      string mc_selection_up_sch   = qcd_qut + "* weight_" + systematic + "Up_norm * 0.33";
      string mc_selection_down_sch = qcd_qut + "* weight_" + systematic + "Down_norm * 0.33";

      fill_hist_sys("DY_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("ttbar_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("Diboson_"+systematic, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("s_ch_"+systematic,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_up_sch, mc_selection_down_sch, excl);
      fill_hist_sys("tW_ch_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_up_TW, mc_selection_down_TW, excl);
      fill_hist_sys("WQQ_"+systematic,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_WQQ, mc_selection_down+SELECTION_WQQ, excl);
      fill_hist_sys("Wc_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wc, mc_selection_down+SELECTION_Wc, excl);
      fill_hist_sys("Wb_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wb, mc_selection_down+SELECTION_Wb, excl);
      fill_hist_sys("Wother_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wother, mc_selection_down+SELECTION_Wother, excl);
      fill_hist_sys("Wjets_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);

        fill_hist_sys("t_ch_"+systematic,        NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection_up, mc_selection_down, excl);

      if(MODE == "FCNC_tug") fill_hist_sys("fcnc_tug_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection_up, mc_selection_down,        excl);
      if(MODE == "FCNC_tcg") fill_hist_sys("fcnc_tcg_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection_up, mc_selection_down,        excl);
    }

    // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT F0LDERS
    for(auto fprefix : VARIATION_SYS_T1){
      vector<string> pstfixs = {"Down", "Up"};
      for(auto pstfix : pstfixs){
        string extra_select = "";
        // if(fprefix == "JEC") extra_select = "* weight_btag_jes" + pstfix + "_norm / weight_norm";

        string sname = fprefix + pstfix;
        PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        fill_hist("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_sch + extra_select,        excl);
        fill_hist("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_TW + extra_select,        excl);
        fill_hist("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
        fill_hist("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
        fill_hist("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
        fill_hist("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
        fill_hist("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection + extra_select,        excl);

          fill_hist("t_ch_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection + extra_select,        excl);

        if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
      }
    }

      // JEC ===================== ===================== ===================== ===================== ===================== 
      vector<string> pstfixs = {"Down", "Up"};
      vector<string> JEC_cuts;
                     JEC_cuts = {"TMath::Abs(Eta_LJ) < 1.4", "TMath::Abs(Eta_LJ) > 1.4"};
      string fprefix = "JEC";
      for(auto pstfix : pstfixs){
        string extra_select = "";
        string sname = fprefix + "F" + pstfix;
        string PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        string PREFIX_NTUPLES_def = PATH_PREFIX + CENTRAL_FOLDER+"/";

        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>> " << sname << endl;
        fill_hist_JEC("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_DY, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TT, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_DB, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_SC, JEC_cuts,  tree_name, vrule, mc_selection_sch + extra_select,        excl);
        fill_hist_JEC("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TW, JEC_cuts,  tree_name, vrule, mc_selection_TW + extra_select,        excl);
        fill_hist_JEC("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
        fill_hist_JEC("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
        fill_hist_JEC("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
        fill_hist_JEC("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
        fill_hist_JEC("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);

          fill_hist_JEC("t_ch_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TC, JEC_cuts,     tree_name, vrule, mc_selection + extra_select,        excl);

        if(MODE == "FCNC_tug") fill_hist_JEC("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_FCNC_TUG, JEC_cuts,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist_JEC("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_FCNC_TCG, JEC_cuts,  tree_name, vrule, mc_selection + extra_select, excl);
      }
                    JEC_cuts = {"TMath::Abs(Eta_LJ) > 1.4", "TMath::Abs(Eta_LJ) < 1.4"};
      for(auto pstfix : pstfixs){
        string extra_select = "";
        string sname = fprefix + "B" + pstfix;
        string PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        string PREFIX_NTUPLES_def = PATH_PREFIX + CENTRAL_FOLDER+"/";
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>> " << sname << endl;
        fill_hist_JEC("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_DY, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TT, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_DB, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist_JEC("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_SC, JEC_cuts,  tree_name, vrule, mc_selection_sch + extra_select,        excl);
        fill_hist_JEC("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TW, JEC_cuts,  tree_name, vrule, mc_selection_TW + extra_select,        excl);
        fill_hist_JEC("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
        fill_hist_JEC("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
        fill_hist_JEC("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
        fill_hist_JEC("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
        fill_hist_JEC("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_WJ, JEC_cuts,  tree_name, vrule, mc_selection + extra_select,        excl);


          fill_hist_JEC("t_ch_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_TC, JEC_cuts,     tree_name, vrule, mc_selection + extra_select,        excl);
        

        if(MODE == "FCNC_tug") fill_hist_JEC("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_FCNC_TUG, JEC_cuts,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist_JEC("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES_def, PREFIX_NTUPLES, FILES_FCNC_TCG, JEC_cuts,  tree_name, vrule, mc_selection + extra_select, excl);
      }

    out_file->Close();
  }
  else if(MODE == "SM_unmarg" or MODE == "FCNC_tug_unmarg" or MODE == "FCNC_tcg_unmarg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd11 > 0.7)";
    if(RELEASE=="CHS_JID") qcd_qut       = "(BNN_qcd > 0.7)";

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














