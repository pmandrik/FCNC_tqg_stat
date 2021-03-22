
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"

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
  cout << tot << " " << sum << " " << upe << " " << histe->GetBinLowEdge(upe) << " " << qfrac << endl;
  return histe->GetBinCenter(upe);
}

double getQuantiles(string filename, string par_name, double alpha=0.95, double bfrac=0.1){
  TFile *fe = TFile::Open( filename.c_str() );
  
  if(not fe) return -100;
  fe->Print();
  TTree *te = dynamic_cast<TTree*>(fe->Get("chain_1"));
  cout << te << " " << par_name << endl;
  //te->Print();

  Double_t   kce;
  Int_t      weighte;
  te->SetBranchAddress(par_name.c_str(), &kce);
  te->SetBranchAddress("weight",         &weighte);

  Int_t Nbins = 100000;
  double kc_min = -2, kc_max = 10;
  TH1D *histe = new TH1D("kce", "kce", Nbins, kc_min, kc_max);

  for(int l = bfrac * te->GetEntries(); l < (int)te->GetEntries();++l){
    // cout << l << endl;
    te->GetEntry(l);
    // cout << l << " " << kce << " " << weighte << endl;
    histe->Fill(kce, weighte);

    // if(l > 5000) break;
  }
  return get_qv(histe, alpha );
}

struct sort_item{
  sort_item(string a, double b, double c){
    name = a;
    mm = b;
    mp = c;
  }
  string name;
  double mm, mp;
};

void save_plot_unc(vector<string> labels, vector<double> values, string mode, double qv_nominal, string name = "sys_check_"){
  map<string, string> dic;
        dic["sigma_ttbar"] = "t#bar{t} cross section ";
        dic["lumi"] = "Luminosity";
        dic["iso"] = "Isolation";
        dic["JES"] = "JES";
        dic["btag"] = "b tagging";
        dic["t_ch_scale_RF"] = "t-channel cross section ";
        dic["tW_ch_scale_RF"] = "tW cross section ";
        dic["sigma_QCD"] = "Multijet cross section ";
        dic["Wjets_scale_RF"] = "W+jets cross section ";
        dic["Diboson_scale_RF"] = "Diboson cross section ";

  TGraphAsymmErrors * gr_err_positive = new TGraphAsymmErrors( labels.size() / 2 );
  TGraphAsymmErrors * gr_err_negative = new TGraphAsymmErrors( labels.size() / 2 );

  TH2D* hist = new TH2D(name.c_str(), name.c_str(), 3, -0.10, 0.10, labels.size() / 2, 0, labels.size() / 2);

  vector< sort_item > items;

  for(int i = labels.size()-2; i >=0; i-=2){
    // auto item = labels.at(i);
    auto item = labels.at(i);
    item = labels.at(i).substr(0, item.size() - 6);
    // if( dic.find( item ) != dic.end() ) item = dic.find( item )->second;

    auto qvs_minus = values.at(i);
    auto qvs_plus  = values.at(i+1);

    double mean_minus = (qvs_minus - qv_nominal)/qv_nominal;
    double mean_plus  = (qvs_plus  - qv_nominal)/qv_nominal;
    cout << item  << " " << mean_minus << " " << mean_plus << endl;

    items.push_back( sort_item(item, mean_minus, mean_plus) );
  }

  sort( items.begin(), items.end(), [](const sort_item & a, const sort_item & b) -> bool { return max(abs(a.mm), abs(a.mp)) > max(abs(b.mm), abs(b.mp)); });

  for(int i = items.size()-1; i >=0; i-=1){
    auto item = items.at(i).name;

    double mean_minus = items.at(i).mm;
    double mean_plus  = items.at(i).mp;

    double pos_minus_error = 0, pos_plus_error = 0;
    double neg_minus_error = 0, neg_plus_error = 0;

    double pos_y = items.size() - i - 0.5;

    gr_err_positive->SetPoint(i, 0, pos_y);
    gr_err_negative->SetPoint(i, 0, pos_y);

    if(mean_minus < 0) pos_minus_error = TMath::Abs(mean_minus);
    else               neg_minus_error = TMath::Abs(mean_minus);

    if(mean_plus > 0) pos_plus_error = TMath::Abs(mean_plus);
    else              neg_plus_error = TMath::Abs(mean_plus);

    gr_err_negative->SetPointError(i, neg_minus_error, neg_plus_error, 0.5, 0.5);
    gr_err_positive->SetPointError(i, pos_minus_error, pos_plus_error, 0.5, 0.5);

    hist->GetYaxis()->SetBinLabel(items.size() - i, item.c_str());

    //hist->SetBinContent(i+1, mean);
    //hist->SetBinError(i+1, rms);
    //hist->GetXaxis()->SetBinLabel(i+1, item.c_str());
  }

  TCanvas * canv = new TCanvas( name.c_str(), name.c_str(), 1000, 1800);
  hist->GetYaxis()->SetLabelSize(0.07);
  hist->Draw();
  // hist->GetYaxis()->SetTitle( ("Uncertainty") );
  hist->GetXaxis()->LabelsOption("v");
  hist->GetXaxis()->SetTitle("(#pi_{#theta #pm #sigma_{#theta}} - #pi_{0})/#pi_{0}");
  hist->GetYaxis()->SetTitleOffset(1.4);
  hist->GetYaxis()->SetLabelSize(0.05);
  hist->GetXaxis()->SetLabelSize(0.035);

  hist->GetYaxis()->SetTitleSize(0.06);
  hist->GetXaxis()->SetTitleSize(0.06);

  gr_err_negative->Draw("CP5");
  gr_err_negative->SetFillColor(2);
  gr_err_negative->SetFillStyle(3001);

  gr_err_positive->Draw("CP5");
  gr_err_positive->SetFillColor(3);
  gr_err_positive->SetFillStyle(3001);

  //TLine * l0 = draw_limit_line(qv_nominal, 0, labels.size(),  kRed, canv);
  TLegend * leg  = new TLegend(0.70, 1-0.75, 1.00, 1-0.90);
  leg->SetHeader("");
  leg->AddEntry("", "#pi = #sigma(t-ch)_{exp}", "");
  leg->AddEntry(gr_err_positive, "positive correlation", "f");
  leg->AddEntry(gr_err_negative, "negative correlation", "f");
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->Draw();

  //hist->SetMaximum( qv_nominal*1.1 );
  //hist->SetLineWidth(2);
  canv->SetTopMargin(0.10);
  canv->SetLeftMargin(0.35);
  canv->SetRightMargin(0.02);
  canv->RedrawAxis();

  TLatex Tl;
  Tl.SetTextFont(42);
  Tl.SetTextAlign(13);
  Tl.SetTextSize(0.050);
  Tl.DrawLatex(-0.15, items.size() + 2,"Uncertainty #theta");
  // Tl.DrawLatex(0.10, 10.85,"xxx fb^{-1} (13 TeV)");

  canv->Print( (name + ".png").c_str() );
  canv->Print( (name + ".pdf").c_str() );

  TFile * file = new TFile("SysCheck.root", "RECREATE");
  canv->Write();
  file->Close();
}

void plotSysImpact(){

    int font = 132;
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadBorderMode(0);

    gStyle->SetFrameFillColor(0);
    gStyle->SetFrameFillStyle(0);

    gStyle->SetPadColor(0);
    gStyle->SetCanvasColor(0);
    gStyle->SetTitleColor(1);
    gStyle->SetStatColor(0);

    gStyle->SetLegendBorderSize(0);
    gStyle->SetLegendFillColor(0);
    gStyle->SetLegendFont(font);
    
    gStyle->SetOptStat(0000000);
    gStyle->SetTextFont(font);
    gStyle->SetTextSize(0.05);
    gStyle->SetLabelFont(font,"XYZ");
    gStyle->SetTitleFont(font,"XYZ");
    gStyle->SetLabelSize(0.05,"XYZ"); //0.035
    gStyle->SetTitleSize(0.05,"XYZ");
    
    gStyle->SetTitleOffset(0.75,"X");
    gStyle->SetTitleOffset(2.05,"Y");
    gStyle->SetLabelOffset(0.02,"XY");
    
    // use bold lines and markers
    gStyle->SetMarkerStyle(8);
    gStyle->SetHistLineWidth(3);
    gStyle->SetLineWidth(1);

    gStyle->SetNdivisions(401,"xy");

    // do not display any of the standard histogram decorations
    gStyle->SetOptTitle(0);
    gStyle->SetOptStat(0); //("m")
    gStyle->SetOptFit(0);
    
    //gStyle->SetPalette(1,0)
    gStyle->cd();
    gROOT->ForceStyle();
  
  gStyle->SetLabelSize(0.055, "Y");

  auto dirs_all = mRoot::get_directories( "2021_jan_NoIsoCut/sys_check/sm/." ); // sys_check/cta/btag/
  sort(dirs_all.begin(), dirs_all.end());
  
  vector<double> qvs;
  vector<string> dirs;
  for(auto dir : dirs_all){
    cout << dir << endl;
    if(dir == "*") continue;
    if(dir == "sigma_t_ch_plus") continue;
    if(dir == "sigma_t_ch_minus") continue;

    // continue;

    string path = "2021_jan_NoIsoCut/sys_check/sm/" + dir + "/expected_sm_jul_"+dir+"_theta.root";
    auto qv = getQuantiles(path, "sigma_t_ch", 0.5);
    cout << dir << " " << qv << endl;
    
    if(qv < -50) continue;
    
    dirs.push_back(dir);
    qvs.push_back(qv);
  }
  
  double qv_nominal = getQuantiles("2021_jan_NoIsoCut/sys_check/sm/sigma_t_ch/expected_sm_jul_sigma_t_ch_theta.root", "sigma_t_ch", 0.5);
  
  save_plot_unc(dirs, qvs, "", qv_nominal, "sys_check");
}


