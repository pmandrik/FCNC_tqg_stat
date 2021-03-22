
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"

#include "getQuantiles.cpp"
#include "TxtDatabase.cpp"

void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}


template <typename T>
std::string get_string(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

template <typename T>
std::string get_perc(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value * 100;
    return out.str() + "\\%";
}

using namespace std;

class MyParameter {
  public:
    // collect here all information associated with parameter
    MyParameter(TTree *tree, string n, double bfrac, string postfix="") : name(n){
      tree->SetBranchAddress(n.c_str(), &val);
      if( name == "KU" or name == "KC" or name == "cta_norm" or name == "uta_norm")
        hist = new TH1D((n + " bf= "+ to_string(bfrac) + postfix).c_str(), (n + postfix).c_str(), 100000,    0, 1.); // FIXME
      else 
        hist = new TH1D((n + " bf= "+ to_string(bfrac) + postfix).c_str(), (n + postfix).c_str(), 100000, -10, 10.); // FIXME
      mean = -1999, rms = -1999;

      burn_frac   = bfrac;
      burn_events = burn_frac * tree->GetEntriesFast();
    };

    void ReadEntrie(double weight){
      if(burn_events >= 0) {burn_events -= 1; return;}
      hist->Fill( val, weight );
      weights.push_back(weight);
      vals.push_back(val);
    }

    double GetMean(){
      if(mean > -999) return mean;
      mean = 0;
      for(int i = 0; i < vals.size(); i++) mean  += vals.at(i) * weights.at(i);
      mean /= std::accumulate(weights.begin(), weights.end(), 0.);
      return mean;
    }

    double GetRMS(){
      if(rms > -999) return rms;
      rms = 0;
      mean = GetMean();
      for(int i = 0; i < vals.size(); i++) 
        rms  += pow(vals.at(i) - mean, 2) * weights.at(i);
      rms /= std::accumulate(weights.begin(), weights.end(), 0.);
      rms = sqrt(rms);
      return rms;
    }

    double GetCovariance(MyParameter * other){
      double mean_x = GetMean();
      double mean_y = other->GetMean();
      double cov = 0;
      for(int i = 0; i < vals.size(); i++){
        cov += (vals.at(i) - mean_x) * (other->vals.at(i) - mean_y) * weights.at(i);
      }
      cov /= std::accumulate(weights.begin(), weights.end(), 0.);
      return cov;
    }

    double GetCorrelation(MyParameter * other){
      double cor = GetCovariance(other) / (GetRMS() * other->GetRMS()); 
      return cor;
    }
    
    int burn_events;
    double val, mean, rms, burn_frac;
    string name;
    TH1D * hist;
    vector<double> vals;
    vector<double> weights;
};

double getTable(string filename, string postfix, double def_bfrac){
  gErrorIgnoreLevel = kWarning;
  TxtDatabase dbase("../analyse.db");
  TFile file( filename.c_str() );

  TTree *tree = dynamic_cast<TTree*>(file.Get("chain_1"));

  vector <MyParameter*> parameters;
  TObjArray * mycopy = tree->GetListOfBranches();
  TIter iObj(mycopy);
  while (TObject* obj = iObj()) {
    if(obj->GetName() == string("weight")) continue;
    if(obj->GetName() == string("nll_MarkovChain_local_")) continue;
    MyParameter * parameter = new MyParameter(tree, obj->GetName(), def_bfrac);
    parameters.push_back( parameter );
  }

  Int_t weight;
  tree->SetBranchAddress("weight",  &weight);

  for(int l = 0; l < (int)tree->GetEntries(); ++l){
    tree->GetEntry(l);
    for(auto param : parameters) param->ReadEntrie( weight );
  }

  // QUANTILES TABLE ===================================================================
  string out_string = "\\documentclass{article} \n \\usepackage{graphicx}";
         //out_string += "\\usepackage[paperheight=16.75in,paperwidth=27.25in]{geometry}";
         out_string += "\n \\begin{document} \n";
         out_string += "Burn-In fraction = " + to_string(def_bfrac) + "\n";
         out_string += "\\begin{center} \n \\begin{tabular}{ | c | c | c | c |} \n";
         out_string += "\\hline parameter & $-\\sigma$ & central & $+\\sigma$ \\\\ \n \\hline \n";

  for(auto param : parameters){
    TH1D * hist = param->hist;
    string hname = param->name;
    if( hname == "KU" or hname == "KC" or hname == "cta_norm" or hname == "uta_norm") continue;

    double alpha = 0.3173;
    double l = get_qv(hist, alpha*0.5 );
    double c = get_qv(hist, 0.5) ;
    double u = get_qv(hist,  1. - alpha*0.5 );

    cout << hist->GetTitle() << " " << l << " " << c << " " << u << endl;
    out_string += string(hist->GetTitle()) + " & " + get_string(l, 3) + " & " + get_string(c, 3) + " & " + get_string(u, 3) + " \\\\ \n ";
    dbase.SetItem( string(hist->GetTitle()) + "_ll_" + postfix, get_string(l, 6),  "getTable("+filename+"," + postfix +")");
    dbase.SetItem( string(hist->GetTitle()) + "_cl_" + postfix, get_string(c, 6),  "getTable("+filename+"," + postfix +")");
    dbase.SetItem( string(hist->GetTitle()) + "_ul_" + postfix, get_string(u, 6),  "getTable("+filename+"," + postfix +")");
  }
  out_string += " \\hline \\end{tabular} \n \\end{center} \n";
  
  // UPPER LIMITS TABLE ===================================================================
  out_string += "\\begin{center} \n \\begin{tabular}{ | c | c | c |} \n";
  out_string += " \\hline parameter & 95 \\% UL & 98 \\% UL \\\\ \n \\hline \n";
  for(auto param : parameters){
    TH1D * hist = param->hist;
    string hname = param->name;
    if( hname != "KU" and hname != "KC" and hname != "cta_norm" and hname != "uta_norm") continue;

    double up1 = get_qv(hist, 0.95 );
    double up2 = get_qv(hist, 0.98 );

    cout << hist->GetTitle() << " " << up1 << " " << up2 << endl;
    out_string += string(hist->GetTitle()) + " process normalisation & " + get_string(up1, 3) + " & " + get_string(up2, 3) + " \\\\ \n ";

    // https://arxiv.org/pdf/0810.3889.pdf
    double Cq = 1.1964;
    double Kappa_q = 0.03;
    double Br_1 = Cq * up1 * Kappa_q * Kappa_q;
    double Br_2 = Cq * up2 * Kappa_q * Kappa_q;
    out_string += string(hist->GetTitle()) + " branching" + " & " + get_string(Br_1, 3) + " & " + get_string(Br_2, 3) + " \\\\ \n ";
  }
  out_string += " 7+8 TeV branching KU obs (exp) & 2.0 (2.8) \\times 10^{-5} & \\\\ \n";
  out_string += " 7+8 TeV branching KC obs (exp) & 4.1 (2.8) \\times 10^{-4} & \\\\ \n";
  out_string += " \\hline \\end{tabular} \n \\end{center} \n";

  // INCLUDE BURN IN STUDY IMAGE ===================================================================
  out_string += "\n \\newpage \n";
  out_string += "\n \\textbf{BURN IN STUDY} \\\\ \n";
  out_string += " \\includegraphics[width=0.9\\linewidth]{BurnInStudy" + postfix + "Theta.png} ";
  cout << "BurnInStudy" + postfix + "Theta.png " << endl;

  // COVARIANCE TABLE =================================================================== 
  out_string += "\n \\newpage \n"; 
  mRoot::msg("add COVARIANCE TABLE ...");
  out_string += "\n \\textbf{COVARIANCE TABLE} \\\\ \n";
  /*
  out_string += "\\begin{center} \n \\begin{tabular}{|c|";
  for(auto par : parameters) out_string += " c |";
  out_string += "} \\hline \n - ";
  for(auto par : parameters) out_string += "  & " + par->name;
  out_string += " \\\\ \n \\hline \n ";
  for(auto par_y : parameters){
    out_string += par_y->name;
    for(auto par_x : parameters){
      double variance = par_y->GetCovariance( par_x );
      out_string += " & " + get_string(variance, 3);
    }
    out_string += " \\\\ \n ";
  }
  out_string += " \\hline \\end{tabular} \n \\end{center} \n";
  */

  TH2D * hist_cov = new TH2D("Covariance", "Covariance", parameters.size(), 0, parameters.size(), parameters.size(), 0, parameters.size());
  for(int i=1; i <= hist_cov->GetNbinsX(); i++) hist_cov->GetXaxis()->SetBinLabel(i, parameters.at(i-1)->name.c_str() );
  for(int i=1; i <= hist_cov->GetNbinsX(); i++) hist_cov->GetYaxis()->SetBinLabel(i, parameters.at(i-1)->name.c_str() );
  for(int x = 0; x < parameters.size(); x++){
    for(int y = 0; y < parameters.size(); y++){
      double variance = parameters.at(y)->GetCovariance( parameters.at(x) );
      if(x == y) mRoot::msg( x, y, variance );
      hist_cov->Fill(x, y, variance);
    }
  }

  auto canv = mRoot::plotCorrelationHist( hist_cov, false, 2 );
  string cov_hist_out_name = "Cov" + postfix + ".png";
  canv->Print( cov_hist_out_name.c_str() );
  out_string += " \\includegraphics[width=0.7\\linewidth]{" + cov_hist_out_name + "} ";

  mRoot::msg("add CORRELATION TABLE ...");
  out_string += "\n \\\\ \\textbf{CORRELATION TABLE} \\\\ \n";
  TH2D * hist_cor = new TH2D("Correlation", "Correlation", parameters.size(), 0, parameters.size(), parameters.size(), 0, parameters.size());
  for(int i=1; i <= hist_cor->GetNbinsX(); i++) hist_cor->GetXaxis()->SetBinLabel(i, parameters.at(i-1)->name.c_str() );
  for(int i=1; i <= hist_cor->GetNbinsX(); i++) hist_cor->GetYaxis()->SetBinLabel(i, parameters.at(i-1)->name.c_str() );
  for(int x = 0; x < parameters.size(); x++){
    for(int y = 0; y < parameters.size(); y++){
      double variance = parameters.at(y)->GetCorrelation( parameters.at(x) );
      //hist_cor->Fill(x, y, int(abs(variance)*100));
      hist_cor->Fill(x, y, abs(variance)*100 );
      //cout << variance << " " << parameters.at(y)->name << " " << parameters.at(x)->name << endl;
      //cout << parameters.at(y)->GetRMS() << " " << parameters.at(x)->GetRMS() << endl;
      //cout << parameters.at(y)->GetMean() << " " << parameters.at(x)->GetMean() << endl;
    }
  }

  canv = mRoot::plotCorrelationHist( hist_cor, 2 );
  string cor_hist_out_name = "Cor" + postfix + ".png";
  canv->Print(cor_hist_out_name.c_str());
  out_string += " \\includegraphics[width=0.7\\linewidth]{" + cor_hist_out_name + "} ";

  // ALL HISTOGRAMMS =================================================================== 
  out_string += "\n \\newpage \n";
  out_string += "\n \\textbf{INPUT HISTOGRAMMS} \\\\ \n";
  string hists_path = "../hists/";
  if(postfix == "KU" or postfix == "KC") hists_path = "../hists_fcnc/";
  TSystemDirectory dir(hists_path.c_str(), hists_path.c_str()); 
  TList *files = dir.GetListOfFiles();
  if (files) { 
    TSystemFile *file; 
    TString fname; 
    TIter next(files); 
    while ((file=(TSystemFile*)next())) { 
      fname = file->GetName(); 
      string name = fname.Data();
      if( name.find(".png") == string::npos) continue;
      string pattern = postfix;
      if( pattern == "KU" ) pattern = "FCNC_tug";
      if( pattern == "KC" ) pattern = "FCNC_tcg";
      if( name.find( pattern ) == string::npos) continue;
      cout << "!!!!!!!!!!!!!!!!!!!!1" << name << endl;
      name = " \\includegraphics[width=0.7\\linewidth]{" + hists_path + name + "} \\\\ \n ";
      ReplaceStringInPlace( name, string("_"), string("@@@@@@@@"));
      out_string += name;
    } 
  }

  //for(int i = 0; i < channels_names.size(); i++){
  //  out_string += " \\includegraphics[width=0.7\\linewidth]{ ../hists/" + channel_prefix + channels_names.at(i) + "} ";
  //}

  // ALL MCMC CHAINS TABLE ===================================================================
  out_string += "\n \\newpage \n";
  out_string += "\n \\textbf{MCMC OUPUT CHAINS} \\\\ \n";
  int new_line = 0;
  chains_path = ("chains_" + postfix);
  ReplaceStringInPlace(chains_path, string("_"), string("X"));
  gSystem->mkdir( chains_path.c_str() );
  for(auto param : parameters){
    TH1D * hist = param->hist;
    TCanvas * canv = mRoot::get_canvas();
    mRoot::tune_hist(hist);
    hist->Draw("HIST");
  
    canv->Update();
    string hname = chains_path + "/" + hist->GetTitle() + "_" + postfix + ".png";
    ReplaceStringInPlace(hname, string("_"), string("X"));
    canv->Print( hname.c_str() );

    out_string += " \\includegraphics[width=0.33\\linewidth]{" + hname + "} ";
    if(new_line == 2) { out_string += " \\ \\ \n "; new_line = 0; }
    else         out_string += "  ";
    new_line++;
  }
  // POSTPROCESS ===================================================================
  out_string += " \n \\end{document} \n";

  ReplaceStringInPlace( out_string, string("_"), string("\\_"));
  ReplaceStringInPlace( out_string, string("@@@@@@@@"), string("_"));

  ofstream out( ("getTable_" + postfix + ".tex").c_str() );
  out << out_string << endl;
  out.close();
  dbase.Write();

  return 0;
}


  /*
  \begin{center}
  \begin{tabular}{ c c c }
   cell1 & cell2 & cell3 \\ 
   cell4 & cell5 & cell6 \\  
   cell7 & cell8 & cell9    
  \end{tabular}
  \end{center}
  */

vector <MyParameter*> * get_par_vector(string filename, double def_bfrac, string postfix){
  TH1::AddDirectory(kFALSE);
  TFile file( filename.c_str() );
  TTree *tree = dynamic_cast<TTree*>(file.Get("chain_1"));

  vector <MyParameter*> * parameters = new vector <MyParameter*>();
  TObjArray * mycopy = tree->GetListOfBranches();
  TIter iObj(mycopy);
  while (TObject* obj = iObj()) {
    if(obj->GetName() == string("weight")) continue;
    if(obj->GetName() == string("nll_MarkovChain_local_")) continue;
    MyParameter * parameter = new MyParameter(tree, obj->GetName(), def_bfrac, postfix);
    parameters->push_back( parameter );
  }

  Int_t weight;
  tree->SetBranchAddress("weight",  &weight);

  for(int l = 0; l < (int)tree->GetEntries(); ++l){
    tree->GetEntry(l);
    for(auto param : *parameters) param->ReadEntrie( weight );
  }

  return parameters;
}

double getTable(string filename_A, string filename_B, double bfrac_A, double bfrac_B, double non){
  vector <MyParameter*> *parameters_A = get_par_vector(filename_A, bfrac_A, "_theta");
  vector <MyParameter*> *parameters_B = get_par_vector(filename_B, bfrac_B, "_cl");
  
  string chains_path = ("chains_comp");
  gSystem->mkdir( chains_path.c_str() );
  for(auto pa : *parameters_A){
    MyParameter* pb = nullptr;
    for(int i =0 ; i < parameters_B->size(); i++)
      if(parameters_B->at(i)->name == pa->name) pb = parameters_B->at(i);
  
    if(pb == nullptr) {
      cout << "Cant find parameter name ... " << pa->name << endl;
      continue;
    }

    TCanvas * canv = mRoot::get_canvas();

    TH1D * hist_a = pa->hist;
    mRoot::tune_hist(hist_a);
    hist_a->Scale(1./hist_a->Integral(), "width");
    hist_a->Draw("HIST");
    hist_a->SetLineColor(2);
    hist_a->SetLineWidth(3);

    TH1D * hist_b = pb->hist;
    mRoot::tune_hist(hist_b);
    hist_b->Rebin( hist_b->GetXaxis()->GetNbins()/hist_a->GetXaxis()->GetNbins());
    hist_b->Scale(1./hist_b->Integral(), "width");
    hist_b->Draw("HIST same");
    hist_b->SetLineWidth(2);

    hist_a->SetMaximum( std::max( hist_b->GetMaximum(), hist_a->GetMaximum()) * 1.01 );

    canv->Update();
    string hname = chains_path + "/" + pa->name + ".png";
    canv->Print( hname.c_str() );
  }

  return 0;
}



