

void CL_workspace_to_tree(const char * filename_in, const char * filename_out){
  cout << "CL_workspace_to_tree() start ... " << endl;
  RooWorkspace * workspace = new RooWorkspace();

  cout << " input file name = " << filename_in << endl;
  cout << " output file name = "  << filename_out << endl;
  TFile * f_in  = new TFile(filename_in  ,"READ");
  TFile * f_out = new TFile(filename_out,"RECREATE") ; 
  f_in->cd("toys");

  gDirectory->Print();

  TIter next(gDirectory->GetListOfKeys());
  int counter = 1;
  for(TKey* key = (TKey *)next(); key; key = (TKey *)next()){
    f_in->cd("toys");
    const char * name = key->GetName();
    TObject * obj = gDirectory->Get( name );
    if (obj->IsA() != RooStats::MarkovChain::Class()) continue;

    cout << "   import chain " << name << endl;
    RooStats::MarkovChain * chain = (RooStats::MarkovChain *)obj;
    workspace->import( *chain );
    cout << "    chain size = " << chain->Size() << endl << "    chain content = ";
    const RooArgSet * argset = chain->Get ( 0 );
    argset->Print();

    f_out->cd();
    string tree_name = "chain_" + to_string(counter);
    TTree * chain_tree = new TTree(tree_name.c_str(), tree_name.c_str()); // same as theta name
    counter++;

    vector<double*> vals;
    int chain_size = chain->Size();

    // add brunch for every variable in chain
    auto iter = argset->createIterator();
    auto var = iter->Next();
    while( var ){
      cout << "add branch " << var->GetName() << endl;
      vals.push_back(new double);
      chain_tree->Branch( var->GetName(), vals.at(vals.size() - 1) );
      var = iter->Next();
    }

    // extra one for weights
    Int_t weight;
    chain_tree->Branch("weight", &weight);

    // fill tree
    for(int i = 0; i < chain_size; i++){
      const RooArgSet * argset = chain->Get( i );
      auto niter = argset->createIterator();
      auto nvar = niter->Next();
      int index = 0;
      while( nvar ){
        *vals.at(index) = argset->getRealValue( nvar->GetName() );
        index++;
        nvar = niter->Next();
      }
      weight = chain->Weight(i);
      chain_tree->Fill();
    }

    // write tree
    chain_tree->Write();
  }
}

double transform_from_cl_to_theta(double val, double unc){
  if(unc < -3) return val; // dont need to recalcullate this parameter
  if(unc <  0) return 1. + val; // it is unifrom
  return pow(1. + unc, val); // its log-normal
}

void CL_workspace_to_tree(const char * filename_in, const char * filename_out, const char * par_vs_uncert){
  cout << "CL_workspace_to_tree() start  with uncertanties ... " << endl;
  RooWorkspace * workspace = new RooWorkspace();

  // parce the input uncertanties
  string par_vs_uncert_str = string(par_vs_uncert);
  istringstream iss(par_vs_uncert_str);
  vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}};
  map<string, float> uncertanties_map;
  map<string, float> unif_map;
  cout << " will use following uncertanties:";
  for(auto item : tokens){
    std::size_t found = item.find(":");
    if (found!=std::string::npos){
      if( item.substr(found+1) == "unif" ) unif_map[ item.substr(0, found) ] = 1.0;
      else uncertanties_map[ item.substr(0, found) ] = atof( item.substr(found+1).c_str() );
      cout << "   " << item.substr(0, found) << ":" << item.substr(found+1) << endl;
    }
  }

  // main part
  cout << " input file name = " << filename_in << endl;
  cout << " output file name = "  << filename_out << endl;
  TFile * f_in  = new TFile(filename_in , "READ");
  TFile * f_out = new TFile(filename_out, "RECREATE"); 
  f_in->cd("toys");

  gDirectory->Print();

  TIter next(gDirectory->GetListOfKeys());
  int counter = 1;
  for(TKey* key = (TKey *)next(); key; key = (TKey *)next()){
    f_in->cd("toys");
    const char * name = key->GetName();
    TObject * obj = gDirectory->Get( name );
    if (obj->IsA() != RooStats::MarkovChain::Class()) continue;

    cout << "   import chain " << name << endl;
    RooStats::MarkovChain * chain = (RooStats::MarkovChain *)obj;
    workspace->import( *chain );
    cout << "    chain size = " << chain->Size() << endl << "    chain content = ";
    const RooArgSet * argset = chain->Get ( 0 );
    argset->Print();

    f_out->cd();
    string tree_name = "chain_" + to_string(counter);
    TTree * chain_tree = new TTree(tree_name.c_str(), tree_name.c_str()); // same as theta name
    counter++;

    vector<double*> vals;
    vector<double> uncs;
    int chain_size = chain->Size();

    // add brunch for every variable in chain
    auto iter = argset->createIterator();
    auto var = iter->Next();
    while( var ){
      cout << "add branch " << var->GetName() << endl;
      vals.push_back(new double);
      chain_tree->Branch( var->GetName(), vals.at(vals.size() - 1) );

      auto find = uncertanties_map.find( string(var->GetName()) );
      auto find2 = unif_map.find( string(var->GetName()) );
      if(find != uncertanties_map.end()) uncs.push_back(find->second);
      else if(find2 != unif_map.end()) uncs.push_back(-2.);
      else uncs.push_back(-5.);
      var = iter->Next();
    }

    // extra one for weights
    Int_t weight;
    chain_tree->Branch("weight", &weight);

    // fill tree
    for(int i = 0; i < chain_size; i++){
      const RooArgSet * argset = chain->Get( i );
      auto niter = argset->createIterator();
      auto nvar = niter->Next();
      int index = 0;
      while( nvar ){
        *vals.at(index) = transform_from_cl_to_theta( argset->getRealValue( nvar->GetName() ), uncs.at(index) ); // transformation here
        index++;
        nvar = niter->Next();
      }
      weight = chain->Weight(i);
      chain_tree->Fill();
    }

    // write tree
    chain_tree->Write();
  }
}








