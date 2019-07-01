
#ifndef get_quantiles_hh
#define get_quantiles_hh 1

double get_qv(TH1D* histe, double qfrac){
  int Nbins = histe->GetXaxis()->GetNbins();
  double tot = histe->Integral();
  double sum = 0.0;
  int upe=1;
  for (int i=1; i <=Nbins; ++i){
    sum += histe->GetBinContent(i);
    if(sum < qfrac * tot) upe = i;
    else break;
  }
  //cout << upe << " " << histe->GetBinLowEdge(upe) << " " << qfrac << endl;
  return histe->GetBinCenter(upe);
}

double get_qv(std::vector<double> & vals, double qfrac){
  long long int sum = vals.size();
  std::sort(vals.begin(), vals.end(), std::less<double>());
  long long int index = sum * qfrac;
  return vals.at( index );
}

double getQuantiles(string filename, string par_name){
  TFile fe( filename.c_str() );
  TTree *te = dynamic_cast<TTree*>(fe.Get("chain_1"));

  Double_t   kce;
  Int_t      weighte;
  te->SetBranchAddress(par_name.c_str(), &kce);
  te->SetBranchAddress("weight",         &weighte);

  Int_t Nbins = 200000;
  double kc_min = 0., kc_max = 5.;
  TH1D *histe = new TH1D("kce", "kce", Nbins, kc_min, kc_max);

  for(int l = 0; l < (int)te->GetEntries();++l){
    te->GetEntry(l);
    histe->Fill(kce, weighte);
  }

  ofstream out( "getQuantiles_temp.txt" );
  double alpha = 0.3173;
  out << get_qv(histe, alpha*0.5 ) << " ";
  out << get_qv(histe, 0.5) << " ";
  out << get_qv(histe,  1. - alpha*0.5 ) << endl;
  out.close();

  return 0;
}

double getQuantiles(string filename, string par_name, double bfrac, double min=0, double max=0.1){
  TFile fe( filename.c_str() );
  TTree *te = (TTree*)fe.Get("chain_1");
  te->Print();

  Double_t   kce;
  Int_t      weighte;
  te->SetBranchAddress(par_name.c_str(), &kce);
  te->SetBranchAddress("weight",         &weighte);

  Int_t Nbins = 100000;
  double kc_min = min, kc_max = max;
  TH1D *histe = new TH1D("kce", "kce", Nbins, kc_min, kc_max);

  for(int l = bfrac * te->GetEntries(); l < (int)te->GetEntries();++l){
    te->GetEntry(l);
    histe->Fill(kce, weighte);
  }

  ofstream out( "result.txt" );
  double up_95 = get_qv(histe, 0.95 );
  double up_98 = get_qv(histe, 0.98 );
  out << up_95 << "\n" << up_98 << endl;
  cout << filename << ": " << endl;
  cout << par_name  << " " << up_95 << " " << up_98 << endl;
  cout << "Kappa/L" << " tug tcg " << 0.03*TMath::Sqrt(up_95) << " " << 0.03*TMath::Sqrt(up_95) << endl;
  cout << "Branch"  << " tug tcg " << 1.198*pow(0.03*TMath::Sqrt(up_95), 2) << " " << 1.195*pow(0.03*TMath::Sqrt(up_95), 2) << endl;
  out.close();

  return 0;
}

#endif

