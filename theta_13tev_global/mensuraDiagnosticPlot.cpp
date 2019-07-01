
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRootNiceColors.cpp"

using namespace mRoot;

class HistsDatabase{
  public:
    vector<vector<string>> super_keys;
    vector<TH1D*> entries;

    TH1D * GetHist(vector<string> key){
      int index = 0;
      for(index = 0; index < super_keys.size(); index++)
        if(super_keys[index] == key) break;
      if(index < entries.size()) return entries.at(index);
      return nullptr;
    }

    vector<string> GetUnicKeys(int level){
      vector<string> answer;
      for(auto keys : super_keys){
        if(level >= keys.size() ) continue;
        string key = keys.at(level);
        if( not count(answer.begin(), answer.end(), key) ) answer.push_back( key );
      }
      return answer;
    }

    void AddHist(TH1D * hist){
      entries.push_back(hist);
      super_keys.push_back( split_string(hist->GetName(), "@") );
    }

    void AddHistsFromFile(TFile * file){
      auto keys = file->GetListOfKeys();
      for(int i = 0; i < keys->GetSize(); i++){
        auto obj =  ((TKey*) keys->At(i))->ReadObj();
        if( string(obj->ClassName()) == string("TH1D")) AddHist( (TH1D*)obj );
      }
    }

    vector<TH1D*> GetHists(string pattern){
      vector<TH1D*> answer;
      regex re( pattern );
      for( auto hist : entries ) 
        if( regex_match(hist->GetName(), re) )
          answer.push_back(hist);
      return answer;
    }
};

void fill_hist(TH1D * source, TH1D * target){
  for(int i = 1; i <= source->GetNbinsX(); i++){
    target->Fill( source->GetBinCenter(i), source->GetBinContent(i) );
  }
}

void mensuraDiagnosticPlot(){
  TFile * file = TFile::Open("mensuraDiagnostic.root");
  HistsDatabase * base = new HistsDatabase();
  base->AddHistsFromFile( file );

  auto releases = base->GetUnicKeys(0);
  auto chanals  = base->GetUnicKeys(1);
  auto vars     = base->GetUnicKeys(2);

  for(auto chanal : chanals) cout << chanal << endl;
  for(auto r : releases)     cout << r << endl;

  vector <int> colors = getNiceColors(10);

  gStyle->SetOptStat(0);
  gStyle->SetTitleAlign(33);

  // for every chanal compere 
  // (BNN_SM_new in 17-04-27_new_syst) vs (BNN_SM_pw in 17-04-27_PUPPI) vs (BNN_SM_mix in 17-04-27_CHS)
  for(auto chanal : chanals){
    string ch_name = split_string(chanal, ".").at(0);
    vector<TH1D*> hists;
    vector<string> hist_names;
    for(auto release : releases){
      string bnn = "BNN_SM_new";
      if(release == "17-10-31_PUPPI") bnn = "BNN_SM_mix";
      if(release == "17-10-21_CHS")   bnn = "BNN_SM_mix";
      if(release == "17-12-01_DeepCSV")   bnn = "BNN_SM_mix2";
  
      auto g = base->GetHists(release+"@"+chanal+"@"+bnn);
      if(not g.size()) continue;
      hists.push_back( g.at(0) );
      hist_names.push_back( release + " " + bnn );
    }
    if(not hists.size()) continue;
    auto canv = new TCanvas(chanal.c_str(), chanal.c_str(), 640, 480);
    double max = 0;

    TLegend * legend = new TLegend(0.50,0.85,0.9,0.99);
    for(int i = 0; i < hists.size(); i++){
      cout << i << " " << hists.at(i) << endl;
      TH1D*hist = hists.at(i);
      legend->AddEntry(hist, hist_names.at(i).c_str(), "l");

      if(i) hist->Draw("hist same");
      else  hist->Draw("hist");

      hist->SetLineWidth(3);
      hist->SetLineColor( colors[i] );

      hist->SetStats(false);
      hist->SetTitle( ("BNN " + ch_name).c_str());
      max = TMath::Max( hist->GetMaximum(), max );
    }

    hists.at(0)->GetYaxis()->SetRangeUser(1, 1.1*max);
    legend->Draw();
    canv->SetTopMargin(0.15);
    canv->Update();
    canv->Print( (ch_name + ".png").c_str() );
  }

  // for every chanal compere 
  // (BNN_SM_new_w in 17-04-27_new_syst) vs (BNN_SM_pw_w in 17-04-27_PUPPI) vs (BNN_SM_mix_w in 17-04-27_CHS)
  for(auto chanal : chanals){
    if(chanal == "Data.root" or chanal == "QCD_data.root") continue;
    string ch_name = split_string(chanal, ".").at(0);
    vector<TH1D*> hists;
    vector<string> hist_names;
    for(auto release : releases){
      string bnn = "BNN_SM_new_w";
      if(release == "17-10-31_PUPPI") bnn = "BNN_SM_mix_w";
      if(release == "17-10-21_CHS")   bnn = "BNN_SM_mix_w";
      if(release == "17-12-01_DeepCSV")   bnn = "BNN_SM_mix2_w";
  
      auto g = base->GetHists(release+"@"+chanal+"@"+bnn);
      if(not g.size()) continue;
      hists.push_back( g.at(0) );
      hist_names.push_back( release + " " + bnn );
    }
    if(not hists.size()) continue;
    auto canv = new TCanvas(chanal.c_str(), chanal.c_str(), 640, 480);
    double max = 0;

    TLegend * legend = new TLegend(0.50,0.85,0.9,0.99);
    for(int i = 0; i < hists.size(); i++){
      cout << i << " " << hists.at(i) << endl;
      TH1D*hist = hists.at(i);
      legend->AddEntry(hist, hist_names.at(i).c_str(), "l");

      if(i) hist->Draw("hist same");
      else  hist->Draw("hist");

      hist->SetLineWidth(3);
      hist->SetLineColor( colors[i] );

      hist->SetStats(false);
      hist->SetTitle( ("BNN " + ch_name).c_str());
      max = TMath::Max( hist->GetMaximum(), max );
    }

    hists.at(0)->GetYaxis()->SetRangeUser(1, 1.1*max);
    legend->Draw();
    canv->SetTopMargin(0.15);
    canv->Update();
    canv->Print( (ch_name + "_w.png").c_str() );
  }

  // plot chanal comp throu releases
  for(auto var : vars){
    for(auto chanal : chanals){
      string ch_name = split_string(chanal, ".").at(0);
      if(chanal == "Data.root" or chanal == "QCD_data.root") continue;
      if(var[0] != 'w' or var[1] != 'e' or var[2] != 'i' /*ght*/) continue;
      string name = chanal + "@" + var;

      vector<TH1D*> hists = base->GetHists(".+@"+chanal+"@"+var);
      if(not hists.size()) continue;

      auto canv = new TCanvas(name.c_str(), name.c_str(), 640, 480);
      double max = 0, min_x = 999999, max_x = -999999;
      for(int i = 0; i < hists.size(); i++){
        TH1D*hist = hists.at(i);
        max = TMath::Max( hist->GetMaximum(), max );
        min_x = TMath::Min( hist->GetBinCenter(hist->FindFirstBinAbove(0)), min_x );
        max_x = TMath::Max( hist->GetBinCenter(hist->FindLastBinAbove(0)), max_x );
      }

      TH1D* plot_hist_0 = nullptr;
      TLegend * legend = new TLegend(0.6,0.85,0.9,0.99);

      for(int i = 0; i < hists.size(); i++){
        TH1D*hist = hists.at(i);
        string release = split_string(hist->GetName(), "@").at(0);
        TH1D*plot_hist = new TH1D( (name + to_string(i)).c_str(), (var + " " + ch_name).c_str(), 50, min_x-0.1,max_x+0.1  );
        fill_hist(hist, plot_hist);
        legend->AddEntry(plot_hist,  release.c_str(), "l");

        if(i) plot_hist->Draw("hist same");
        else  plot_hist->Draw("hist");

        plot_hist->SetLineWidth(3);
        plot_hist->SetLineColor( colors[i] );

        plot_hist->SetStats(false);
        max = TMath::Max( plot_hist->GetMaximum(), max );
        if(not plot_hist_0) plot_hist_0 = plot_hist;
      }

      plot_hist_0->GetYaxis()->SetRangeUser(0.1, 2*max);
      //cout << name << " " << min_x << " " << max_x << endl;
      canv->SetLogy();
      legend->Draw();
      canv->SetTopMargin(0.15);
      canv->Update();
      canv->Print( (name + ".png").c_str() );
    }
  }

  // plot t-channel and t-channel_ch comparison
   vector<TH1D*> tch_hists = base->GetHists(".+@t-channel.+@BNN_SM_mix.*_w");

    auto canv = new TCanvas("tCH_vs_tPW", "tCH_vs_tPW", 640, 480);
    double max = 0;

    TLegend * lega = new TLegend(0.50,0.85,0.9,0.99);
    for(int i = 0; i < tch_hists.size(); i++){
      cout << i << " " << tch_hists.at(i) << endl;
      TH1D*hist = tch_hists.at(i);
      lega->AddEntry(hist, hist->GetName(), "l");

      if(i) hist->Draw("hist same");
      else  hist->Draw("hist");

      hist->SetLineWidth(3);
      hist->SetLineColor( colors[i] );

      hist->SetStats(false);
      max = TMath::Max( hist->GetMaximum(), max );
    }

    tch_hists.at(0)->GetYaxis()->SetRangeUser(1, 1.1*max);
    lega->Draw();
    canv->SetTopMargin(0.15);
    canv->Update();
    canv->Print( "tCH_vs_tPW.png" );
}










