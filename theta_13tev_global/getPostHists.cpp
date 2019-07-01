
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRootStatModel.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
#include "getTable.cpp"
using namespace mRoot;

void getPostHists(string hist_file_name, string model_file, string chan_file_name){

  // SETUP MODEL
  msg("getPostHists(",hist_file_name,model_file,chan_file_name,") ... SETUP MODEL");
  mRoot::StatModel model;
  TFile * hist_file = TFile::Open(hist_file_name.c_str());
  if(not hist_file){
    msg_err("cant open file", hist_file_name, "exit");
    return;
  }
  std::string line;
  ifstream ifile ( model_file.c_str() );
  if(ifile.is_open()){
    while( getline(ifile,line) ){
      vector<string> parts = mRoot::split_and_strip_string( line, "|" );
      if(parts.size() < 3) continue;
      // cout << line << endl;
      string ch_name = parts.at(0);
      string interp_list = parts.at(1);
      string norm_list = parts.at(2);

      mRoot::Chanal ch;
      ch.name = ch_name;
      ch.nom_hist = (TH1D*)hist_file->Get( ch_name.c_str() );
      ch.nom_hist->Print();

      // interp params
      for(auto par : mRoot::split_and_strip_string( interp_list, " " )){
        if(par == "") continue;
        ch.interp_pars.push_back(par);
        string up_name   = ch_name + "_" + par + "Up";
        string down_name = ch_name + "_" + par + "Down";
        ch.interp_hists.push_back( make_pair((TH1D*)hist_file->Get( up_name.c_str() ), (TH1D*)hist_file->Get( down_name.c_str() )) );
      }
      cout << ch.interp_hists.size() << endl;

      // norm params
      ch.norm_pars = mRoot::split_and_strip_string( norm_list, " " );
      for(int i = 0 ; i < ch.norm_pars.size(); i++){
        //if(ch.norm_pars[i] == "id" or ch.norm_pars[i] == "iso" or ch.norm_pars[i] == "lumi" or ch.norm_pars[i] == "trig") continue;
        cout << "X" << ch.norm_pars[i] << "X" << endl;;
      }

      model.chanals.push_back( ch );
    }
  }

  // LOAD CHAIN RESULT
  msg("getPostHists() ... LOAD CHAIN RESULT");
  TFile file( chan_file_name.c_str() );
  TTree *tree = dynamic_cast<TTree*>(file.Get("chain_1"));

  vector <MyParameter*> parameters;
  TObjArray * mycopy = tree->GetListOfBranches();
  TIter iObj(mycopy);
  while (TObject* obj = iObj()) {
    if(obj->GetName() == string("weight")) continue;
    if(obj->GetName() == string("nll_MarkovChain_local_")) continue;
    MyParameter * parameter = new MyParameter(tree, obj->GetName(), 0.1);
    parameters.push_back( parameter );
  }

  Int_t weight;
  tree->SetBranchAddress("weight",  &weight);

  for(int l = 0; l < (int)tree->GetEntries(); ++l){
    tree->GetEntry(l);
    for(auto param : parameters) param->ReadEntrie( weight );
  }

  // GET POSTFIT RESULT
  // FIRST OPTION IS JUST SET ALL MODEL PARAMETERS TO THEY CENTRAL QUANTILES VALUES
  msg("getPostHists() ... GET POSTFIT RESULT");
  for(auto param : parameters){
    TH1D * hist = param->hist;
    if( hist->GetTitle() == TString("KU") or hist->GetTitle() == TString("KC") ) continue;

    double alpha = 0.3173;
    double c = get_qv(hist, 0.5) ;

    cout << hist->GetTitle() << " " << c << endl;

    model.parameters[hist->GetTitle()] = c;
  }
  
  gSystem->mkdir( "postfit_hists" );
  TFile * out_file = new TFile("postfit_hists/posthists.root", "RECREATE");
  for(auto chanal : model.chanals){
    auto hist = model.GetHist(chanal.name);
    hist->Write();
  }

  auto hist = (TH1D*)hist_file->Get("data");
  out_file->cd();
  hist->Write();
}















