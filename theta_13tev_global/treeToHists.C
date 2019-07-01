
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
using namespace mRoot;

//=================================== SRC PART ================================================================

template <typename T, typename... Args>
vector <string> vsum(vector<T> v1, vector<T> v2){
  v1.insert(v1.begin(), v2.begin(), v2.end());
  return v1;
}

template <typename T, typename... Args>
vector <string> vsum(vector<T> v1, vector<T> v2, Args... args){
  v1.insert(v1.begin(), v2.begin(), v2.end());
  return vsum(v1, args...);
}

class EventsExcluder {
  public:
  EventsExcluder(string fname){
    original_fname = fname;
    exclude_events = parce_train_events( fname );
  }

  void Print(){
    cout << "EventsExcluder( " << original_fname << " )"<< endl;
    for(auto iter : *exclude_events){
      cout << " " << iter.first << " " << iter.second->size() << endl;
    }
  }

  void SetExcludedEventsFile(string fname){
    for(auto iter : *exclude_events)
      if(iter.first == fname){ 
        active_events = iter.second;
        active_event_number = active_events->at(0);
        active_event_counter = 0;
        return;
      }
    active_events = nullptr;
  }

  bool IsExcluded(int event_number){
    if(active_events == nullptr) return false;
    if(event_number != active_event_number) return false;
      
    active_event_counter++;
    if(active_event_counter < active_events->size())
      active_event_number  = active_events->at( active_event_counter );
    else active_events = nullptr;
    return true;
  }

  #include "parceTrainEvents.C"

  string original_fname;
  vector<int>* active_events;
  int active_event_number;
  int active_event_counter;
  vector<pair<string, vector<int>*>> * exclude_events;
};


// multiple files with same ttree and value_rule -> single hist -> save in file
// fill_hist(hist_name, nbins, rmax, rmin, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule, event_excluder){
void fill_hist(string hist_name, // input TH1D name
  int nbins,                 // number of bins
  double rmin,               // x-axis range min
  double rmax,               // x-axis range max
  TFile * output_file,       // output file
  string prefix,             // path to input files
  vector<string> input_file_names, // vector of names of input files
  string tree_name,                // name of tree
  string value_rule,               // formula of value to evaluate
  string weight_rule,              // formula of weight to evaluate
  EventsExcluder * event_excluder = nullptr  // contain list of events per file
){
  cout << " fill_hist(): process " << hist_name << " with value rule = " << "\"" << value_rule << "\"" << ", weight rule = " << "\"" << weight_rule << "\"" << endl;
  // open file
  output_file->cd();
  TH1D * hist = new TH1D(hist_name.c_str(), hist_name.c_str(), nbins, rmin, rmax);
  hist->Sumw2();
  int prev_integral = 0;

  double totl_weight = 0;
  double excl_weight = 0;
  double weight = 0;
  int event_index = 0;
  int print_n_entries = 20;

  for(auto name : input_file_names){
    TFile * file = TFile::Open( (prefix + name).c_str() );
    if(!file or file->IsZombie()){
      cerr << __FUNCTION__ << ": can't open file name \"" << name << "\", skip ... " << endl;
      continue;
    }

    TTreeReader * reader = new TTreeReader(tree_name.c_str(), file);
    if(!reader->GetTree()){
      cerr << __FUNCTION__ << ": can't get ttree \"" << tree_name << "\" in file \"" << file->GetName() << "\", skip ... " << endl;
      continue;
    }

    // reader->GetTree()->Print();

    TTreeFormula value_f(value_rule.c_str(), value_rule.c_str(), reader->GetTree());
    TTreeFormula weight_f(weight_rule.c_str(), weight_rule.c_str(), reader->GetTree());

    if(event_excluder != nullptr) event_excluder->SetExcludedEventsFile( name );
    event_index = -1;

    while(reader->Next()){
      event_index++;
      weight = weight_f.EvalInstance();
      totl_weight += weight;
      if(event_excluder != nullptr and event_excluder->IsExcluded( event_index )){
        excl_weight += weight;
        continue;
      }
      // if(weight < 0.) cout << weight << endl;
      hist->Fill(value_f.EvalInstance(), weight);
      if(print_n_entries > 0 and weight > 0){ 
        msg(value_f.EvalInstance(), weight);
        print_n_entries--;
      };
    }

    if((int)hist->Integral() == prev_integral){
      cerr << __FUNCTION__ << ": fill no events from file " << name << " to hist " << hist_name << ", continue ... " << endl;
      continue;
    }
    prev_integral += (int)hist->Integral();
  }

  // now we need to reweight hists because of excluded events
  if( totl_weight <= excl_weight){
    cerr << __FUNCTION__ << ": something wrong with events weights, can't rescale " << hist_name << " - " << totl_weight << ", " << excl_weight << ", continue ... " << endl;
  } else {
    // cout << __FUNCTION__ << ": reweight factor for " << hist_name << " = " << totl_weight / (totl_weight - excl_weight) << endl;
    hist->Scale( totl_weight / (totl_weight - excl_weight) );
  }
  
  // save, exit
  output_file->cd();
  hist->Write();
}

void fill_hist_sys(string hist_name, // input TH1D name
  int nbins,                 // number of bins
  double rmin,               // x-axis range min
  double rmax,               // x-axis range max
  TFile * output_file,       // output file
  string prefix,             // path to input files
  vector<string> input_file_names, // vector of names of input files
  string tree_name,                // name of tree
  string value_rule,               // formula of value to evaluate
  string weight_rule_up,                // formula of weight to evaluate
  string weight_rule_down,              // formula of weight to evaluate
  EventsExcluder * event_excluder = nullptr  // contain list of events per file
){
  // cout << weight_rule_up << " " << weight_rule_down << endl;
  fill_hist(hist_name+"Up",   nbins, rmin, rmax, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule_up, event_excluder);
  fill_hist(hist_name+"Down", nbins, rmin, rmax, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule_down, event_excluder);
}

//=================================== CFG PART ================================================================

int treeToHists(string MODE, string RELEASE, string OUTPUT_FILE_NAME, int NBINS, double QCD_norm = -1.0){
  cout << "treeToHists.C, welcome to the converter from mensura ntuples to hists ..." << endl;
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
  vector<string> VARIATION_SYS_T1 = {"JEC", "JER", "UnclMET"}; // SYSTEMATIC WITCH PRESENT IN THE DIFFERENT FILES
  vector<string> VARIATION_SYS_T2 = {"PileUp", /*"btag_jes",*/ "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" }; // SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES

  string PREFIX_NTUPLES;
  string PATH_PREFIX;
  string PATH_EXCLUDE;
  string PATH_SUSTEMATIC;
  if(RELEASE=="april_Sys" or RELEASE=="jul_Sys"){ 
    PATH_PREFIX     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/tuples_merged/";
    PATH_EXCLUDE    = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/";
    PATH_SUSTEMATIC = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/tuples_merged/Syst";
  }
  else if(RELEASE=="nov_puppi_ch" or RELEASE=="nov_puppi_pw"){ 
    PATH_PREFIX     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-31_PUPPI/tuples_merged/";
    PATH_EXCLUDE    = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-31_PUPPI/";
    PATH_SUSTEMATIC = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-31_PUPPI/tuples_merged/Syst";
  }
  else if(RELEASE=="nov_chs"){ 
    PATH_PREFIX     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-21_CHS/tuples_merged/";
    PATH_EXCLUDE    = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-21_CHS/";
    PATH_SUSTEMATIC = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-21_CHS/tuples_merged/Syst";
  }
  else{
    cerr << "Unknow RELEASE, please provide correct value, exit ..." << endl;
    return 1;
  }

  string SELECTION_WQQ    = "*((Wjets_type ==2)+(Wjets_type ==1))";
  string SELECTION_Wc     =  "*(Wjets_type ==4)";
  string SELECTION_Wb     =  "*(Wjets_type ==3)";
  string SELECTION_Wother = "*((Wjets_type ==5)+(Wjets_type ==6))";

  //---------- 1. DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DATA        = {"Data.root"};

  //---------- 2. QCD DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_QCD_DATA    = {"QCD_data.root"};

  //---------- 3. OTHER -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DY     = {"DY.root"};
  vector <string> FILES_TT     = {"ttbar.root"};
  vector <string> FILES_DB     = {"WW.root", "WZ.root", "ZZ.root"};
  vector <string> FILES_WJ     = {"Wjets.root"};
  vector <string> FILES_SC     = {"s-channel.root"};
  vector <string> FILES_TC     = {"t-channel.root"};
  vector <string> FILES_TW     = {"tW-channel.root"};
  vector <string> FILES_TC_CH  = {"t-channel_ch.root"};

  vector <string> FILES_OTHER       = vsum(FILES_DY, FILES_TT, FILES_DB, FILES_WJ, FILES_SC, FILES_TC,   FILES_TW);
  if(RELEASE=="nov_puppi_ch")
    vector <string> FILES_OTHER     = vsum(FILES_DY, FILES_TT, FILES_DB, FILES_WJ, FILES_SC, FILES_TC_CH, FILES_TW);

  //---------- 4. ANOMAL -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_FCNC_TUG    = {"FCNC_tug.root"};
  vector <string> FILES_FCNC_TCG    = {"FCNC_tcg.root"};

  //---------- FILL HISTS -------------------------------------------------------------------------------------------------------------------------
  string tree_name = "Vars";
  double rmin = 0, rmax = 1; 
  string vrule, wrule;
  TFile * out_file;
  EventsExcluder * excl = nullptr;

  if(MODE == "QCD"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    vrule       = "BNN_qcd";

    //EventsExcluder * excl = new ();

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER +"/";
    EventsExcluder * excl = new EventsExcluder( PATH_EXCLUDE + "bnn_QCD_PUPPY/qcd_trainEvents.txt" );
    excl->Print();

    //fill_hist(hist_name, nbins, rmax, rmin, output_file, prefix, input_file_names, tree_name, value_rule, weight_rule, event_excluder)
    fill_hist("data",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DATA,     tree_name, vrule, "1",           nullptr);
    fill_hist("QCD",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_QCD_DATA, tree_name, vrule, "weight",      excl);
    fill_hist("other",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_OTHER,    tree_name, vrule, "weight_norm", excl);

    out_file->Close();
  }
  else if(MODE == "SM" or MODE == "FCNC_tug" or MODE == "FCNC_tcg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd > 0.7)";
    string data_selection = qcd_qut;
    string mc_selection   = qcd_qut + " * weight_norm";
    EventsExcluder * excl = nullptr;

    // SM < 
    if(MODE == "SM"){
      vrule = "BNN_SM_new";
      excl = new EventsExcluder( PATH_EXCLUDE + "bnn_SM_new/bnn_sm7_trainEvents.txt" );
      /*
      if(RELEASE=="nov_puppi_ch")  { vrule =  "BNN_SM_ch";  excl = nullptr; }
      if(RELEASE=="nov_puppi_pw")  { vrule =  "BNN_SM_pw";  excl = nullptr; }
      if(RELEASE=="nov_chs")       { vrule =  "BNN_SM_mix"; excl = nullptr; }
      */
      if(RELEASE=="nov_puppi_ch")  { vrule =  "BNN_SM_mix";  excl = nullptr; }
      if(RELEASE=="nov_puppi_pw")  { vrule =  "BNN_SM_mix";  excl = nullptr; }
      if(RELEASE=="nov_chs")       { vrule =  "BNN_SM_mix";  excl = nullptr; }
    }
    // >

    // FCNC <
    if(MODE == "FCNC_tcg"){
      vrule = "BNN_tcg";
      excl = new EventsExcluder( PATH_EXCLUDE + "bnn_FCNC_tcq/bnn_tcg_trainEvents.txt" );
    }
    if(MODE == "FCNC_tug"){
      vrule = "BNN_tug";
      excl = new EventsExcluder( PATH_EXCLUDE + "bnn_FCNC_tug/bnn_tug_trainEvents.txt" );
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
    fill_hist("s_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection,        excl);
    fill_hist("WQQ",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ,    excl);
    fill_hist("Wc",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc,     excl);
    fill_hist("Wb",      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb,     excl);
    fill_hist("Wother",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother, excl);
    fill_hist("Wjets",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection,        excl);

    if(RELEASE=="nov_puppi_ch"){
      fill_hist("t_ch_alt",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection,        excl);
      fill_hist("t_ch",       NBINS, rmin, rmax, out_file, PREFIX_NTUPLES,  FILES_TC_CH,  tree_name, vrule, mc_selection,        excl);
    }
    else if(RELEASE=="nov_puppi_pw"){
      fill_hist("t_ch_alt",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection,        excl);
      fill_hist("t_ch",       NBINS, rmin, rmax, out_file, PREFIX_NTUPLES,  FILES_TC,     tree_name, vrule, mc_selection,        excl);
    }
    else 
      fill_hist("t_ch",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection,        excl);

    if(MODE == "FCNC_tug") fill_hist("fcnc_tug",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection, excl);
    if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection, excl);

    // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
    for(auto systematic : VARIATION_SYS_T2){
      string mc_selection_up   = qcd_qut + "* weight_" + systematic + "Up_norm";
      string mc_selection_down = qcd_qut + "* weight_" + systematic + "Down_norm";

      fill_hist_sys("DY_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("ttbar_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("Diboson_"+systematic, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("s_ch_"+systematic,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("tW_ch_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      fill_hist_sys("WQQ_"+systematic,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_WQQ, mc_selection_down+SELECTION_WQQ, excl);
      fill_hist_sys("Wc_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wc, mc_selection_down+SELECTION_Wc, excl);
      fill_hist_sys("Wb_"+systematic,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wb, mc_selection_down+SELECTION_Wb, excl);
      fill_hist_sys("Wother_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up+SELECTION_Wother, mc_selection_down+SELECTION_Wother, excl);
      fill_hist_sys("Wjets_"+systematic,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);

      if(RELEASE=="nov_puppi_ch"){
        fill_hist_sys("t_ch_alt_"+systematic,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection_up, mc_selection_down, excl);
        fill_hist_sys("t_ch_"+systematic,        NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      }
      else if(RELEASE=="nov_puppi_pw"){
        fill_hist_sys("t_ch_alt_"+systematic,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);
        fill_hist_sys("t_ch_"+systematic,        NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection_up, mc_selection_down, excl);
      }
      else fill_hist_sys("t_ch_"+systematic,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection_up, mc_selection_down, excl);

      if(MODE == "FCNC_tug") fill_hist_sys("fcnc_tug_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection_up, mc_selection_down,        excl);
      if(MODE == "FCNC_tcg") fill_hist_sys("fcnc_tcg_"+systematic,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection_up, mc_selection_down,        excl);
    }

    // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT F0LDERS
    for(auto fprefix : VARIATION_SYS_T1){
      vector<string> pstfixs = {"Down", "Up"};
      for(auto pstfix : pstfixs){
        string extra_select = "";
        if(fprefix == "JEC") extra_select = "* weight_btag_jes" + pstfix + "_norm / weight_norm";

        string sname = fprefix + pstfix;
        PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        fill_hist("DY_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DY,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("ttbar_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TT,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("Diboson_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_DB,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("s_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_SC,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("tW_ch_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TW,  tree_name, vrule, mc_selection + extra_select,        excl);
        fill_hist("WQQ_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_WQQ + extra_select,    excl);
        fill_hist("Wc_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wc + extra_select,     excl);
        fill_hist("Wb_"+sname,      NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wb + extra_select,     excl);
        fill_hist("Wother_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection_Wother + extra_select, excl);
        fill_hist("Wjets_"+sname,   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_WJ,  tree_name, vrule, mc_selection + extra_select,        excl);

        if(RELEASE=="nov_puppi_ch"){
          fill_hist("t_ch_alt_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection + extra_select,        excl);
          fill_hist("t_ch_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection + extra_select,        excl);
        }
        else if(RELEASE=="nov_puppi_pw"){
          fill_hist("t_ch_alt_"+sname, NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC_CH,  tree_name, vrule, mc_selection + extra_select,        excl);
          fill_hist("t_ch_"+sname,     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,     tree_name, vrule, mc_selection + extra_select,        excl);
        }
        else fill_hist("t_ch_"+sname,    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_TC,  tree_name, vrule, mc_selection + extra_select,        excl);

        if(MODE == "FCNC_tug") fill_hist("fcnc_tug_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TUG,  tree_name, vrule, mc_selection + extra_select, excl);
        if(MODE == "FCNC_tcg") fill_hist("fcnc_tcg_"+sname,  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, FILES_FCNC_TCG,  tree_name, vrule, mc_selection + extra_select, excl);
      }
    }

    out_file->Close();
  }
  else if(MODE == "SM_unmarg" or MODE == "FCNC_tug_unmarg" or MODE == "FCNC_tcg_unmarg"){
    out_file = new TFile(OUTPUT_FILE_NAME.c_str(), "RECREATE");
    string qcd_qut = "(BNN_qcd > 0.7)";
    string data_selection = qcd_qut;
    string mc_selection   = qcd_qut + " * weight_norm";
    EventsExcluder * excl = nullptr;

    // SM < 
    if(MODE == "SM_unmarg"){
      vrule = "BNN_SM_new";
      if(RELEASE=="nov_puppi_ch")  { vrule =  "BNN_SM_mix";  excl = nullptr; }
      if(RELEASE=="nov_puppi_pw")  { vrule =  "BNN_SM_mix";  excl = nullptr; }
      if(RELEASE=="nov_chs")       { vrule =  "BNN_SM_mix"; excl = nullptr; }
    }
    // >

    // FCNC <
    if(MODE == "FCNC_tcg_unmarg"){
      vrule = "BNN_tcg";
    }
    if(MODE == "FCNC_tug_unmarg"){
      vrule = "BNN_tug";
    }
    // >

    PREFIX_NTUPLES = PATH_SUSTEMATIC+"/";

    fill_hist("t_ch_scale_up",     NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel_scale_up.root"},    tree_name, vrule, mc_selection,        excl);
    fill_hist("t_ch_scale_down",   NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"t-channel_scale_down.root"},  tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_up",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_up.root"},        tree_name, vrule, mc_selection,        excl);
    fill_hist("ttbar_scale_down",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"ttbar_scale_down.root"},      tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_scale_up",    NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel_scale_up.root"},   tree_name, vrule, mc_selection,        excl);
    fill_hist("tW_ch_scale_down",  NBINS, rmin, rmax, out_file, PREFIX_NTUPLES, {"tW-channel_scale_down.root"}, tree_name, vrule, mc_selection,        excl);

  }
  else cout << __FILE__ " : unknow mode " << MODE << endl;

  cout << "hists13Charlie.cpp, done ..." << endl;

  return 0;
}






















