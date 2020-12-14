
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
#include "getTable.cpp"
using namespace mRoot;

vector<TString> list_files(const char *dirname="", const char *ext=".root"){
  vector<TString> answer;
  TSystemDirectory dir(dirname, dirname);
  TList *files = dir.GetListOfFiles();
  if (files) {
    TSystemFile *file;
    TString fname;
    TIter next(files);
    while ((file=(TSystemFile*)next())) {
       fname = file->GetName();
       if (fname.EndsWith(ext)) {
          answer.push_back( fname );
       }
    }
  }
  return answer;
}

struct file_info{
  int event_number;
  string release, file;
  
  vector<TH1D *> bnns_hists;
  vector<TH1D *> bnns_hists_w;
  vector<TH1D *> weight_hists;
  TH1D * weight_hist;
  double weighted_integral;
};

file_info * check_file(TString file_name, string r, string f){
  msg(file_name, "...");
  TFile * file = TFile::Open( file_name );
  if(!file or file->IsZombie()){
    cerr << __FUNCTION__ << ": can't open file name \"" << file_name << "\", skip ... " << endl;
    return nullptr;
  }
  TTree * tree = (TTree*)file->Get("Vars");
  if(not tree){ 
    msg("cant get ttree Vars");
    return nullptr;
  }

  double weighted_integral = 0;
  float weight_norm;

  string prefix = r + "@" + f + "@";

  vector<string> bnns_names = { "weight" };
  vector<float*> bnns_vars = list_compr<float*>([prefix](string s){ return new float();}, bnns_names);
  vector<TH1D*> bnns_hists = list_compr<TH1D*>([prefix](string s){ return new TH1D( (prefix+s).c_str(), (prefix+s).c_str(), 50,0,1);}, bnns_names);
  vector<TH1D*> bnns_hists_w = list_compr<TH1D*>([prefix](string s){ return new TH1D((prefix+s + "_w").c_str(), (prefix+s + "_w").c_str(), 50,0,1);}, bnns_names);

  vector<string> weight_names = { "weight_xsec", "weight_btag", "weight_pu", "weight_trig", "weight_pttop", "weight_gen" };
  vector<float*> weight_vars = list_compr<float*>([prefix](string s){ return new float();}, weight_names);

  vector<TH1D*> weight_hists = list_compr<TH1D*>([prefix, tree](string s){ return new TH1D( (prefix+s).c_str(), (prefix+s).c_str(), 100, tree->GetMinimum(s.c_str())-0.1, tree->GetMaximum(s.c_str())+0.1);}, weight_names);

  tree->SetBranchAddress("weight", &weight_norm);
  vector<bool> bnns_sets = list_compr<bool>([tree, prefix](string s, float* var){ return (tree->SetBranchAddress( s.c_str(),var) == 0);}, bnns_names, bnns_vars);
  vector<bool> weight_sets = list_compr<bool>([tree, prefix](string s, float* var){ return (tree->SetBranchAddress( s.c_str(),var) == 0);}, weight_names, weight_vars);

  TH1D* weight_hist = new TH1D( (prefix+"weight").c_str(), (prefix+"weight").c_str(), 100, tree->GetMinimum("weight")-0.1, tree->GetMaximum("weight")+0.1);

  long long int nevents = tree->GetEntries();
  for (long long int  i=0; i<nevents; i++) {
    tree->GetEvent(i);

    weighted_integral += weight_norm;
    weight_hist->Fill( weight_norm );

    for(int i =0; i < bnns_names.size(); i++){
      if(not bnns_sets[i]) continue;
      bnns_hists[i]->Fill( *(bnns_vars[i]) );
      bnns_hists_w[i]->Fill( *(bnns_vars[i]), weight_norm );
    }

    for(int i = 0; i < weight_sets.size(); i++){
      if(not weight_sets[i]) continue;
      weight_hists[i]->Fill( *(weight_vars[i]) );
    }
  }

  file_info * fi = new file_info();
  fi->event_number = tree->GetEntries();
  fi->weighted_integral = weighted_integral;
  fi->bnns_hists   = bnns_hists;
  fi->bnns_hists_w = bnns_hists_w;
  fi->weight_hist  = weight_hist;
  fi->weight_hists = weight_hists;
  fi->file = f;
  fi->release = r;
  return fi;
}

struct latex_table{
  /*
  \begin{center}
  \begin{tabular}{ | c | c | c | }
   \\hline
   cell1 & cell2 & cell3 \\ 
   cell4 & cell5 & cell6 \\  
   cell7 & cell8 & cell9    
  \end{tabular}
  \end{center}
  */

  vector< vector<string> > lines;

  string get_text(){
    unsigned long int max_size = 0;
    for(auto line : lines) max_size = TMath::Max(max_size, line.size());

    string text;
    text += "\\begin{center} \\begin{tabular}{ ";
    for(int i = 0; i < max_size-1; i++) text += "c |";
    text += "c } \n \\hline";

    for(int j = 0; j < lines.size(); j++){
      auto line = lines.at(j);
      for(int i = 0; i < line.size(); i++)
        text += line.at(i) + " & ";
      for(int i = line.size(); i < max_size; i++)
        text += " & ";

      if(max_size) text[ text.size() - 2 ] = ' ';
      if(j != lines.size()-1) text += " \\\\";
      if(j == 0             ) text += " \n \\hline \n";
      text += "\n";
    }
    text += "\\end{tabular} \n \\end{center}";
    return text;
  }
};

void mensuraDiagnostic(){
  //TString PREPATH = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/";
  TString PREPATH = "/scratch/common/samples/";
  //vector<TString> RELEASES = {"17-04-27_new_syst", "17-10-31_PUPPI", "17-10-21_CHS"};
  vector<TString> RELEASES = { "2020-05-01_pujetid", "2020-03-17_bugfix" };
  TString POSTPATH = "/tuples_merged/Central";

  vector<file_info*> fis;

  for(auto release : RELEASES){
    TString path = PREPATH + release + POSTPATH;
    auto files = list_files( path );
    for(auto file : files){
      file_info * fi = check_file( path + "/" + file, release.Data(), file.Data());
      if(not fi) continue;
      fis.push_back( fi );
      msg(fi->file, fi->event_number, fi->weighted_integral);
    }
  }

  // create table  _________|____release___|_
  //                file    |    N_events  |

  latex_table table_sizes;
  vector<string> names = {""};
  for(auto release : RELEASES) names.push_back( release.Data() );
  table_sizes.lines.push_back(names);
  
  map<string, int> counted;
  for(auto fi : fis){
    if( counted.find(fi->file) != counted.end() ) continue;
    counted[fi->file] = 1;

    vector<string> line_entrie;
    line_entrie.push_back( fi->file );
    for(auto release : RELEASES){
      string rel = release.Data();
      string fil = fi->file;
      auto entrie = find_if( fis.begin(), fis.end(), 
        [rel, fil](const file_info* s){ return ((s->release == rel) and (s->file == fil)); } );
      if(entrie != fis.end()) line_entrie.push_back( to_string( (*entrie)->event_number ) );
      else line_entrie.push_back( " " );
    }

    table_sizes.lines.push_back( line_entrie );
  }

  // create table  _________|____release____________|_
  //                file    | N_events_weight_norm  |
  latex_table table_weight_norm;
  table_weight_norm.lines.push_back(names);
  counted.clear();
  for(auto fi : fis){
    if( counted.find(fi->file) != counted.end() ) continue;
    counted[fi->file] = 1;

    vector<string> line_entrie;
    line_entrie.push_back( fi->file );
    for(auto release : RELEASES){
      string rel = release.Data();
      string fil = fi->file;
      auto entrie = find_if( fis.begin(), fis.end(), 
        [rel, fil](const file_info* s){ return ((s->release == rel) and (s->file == fil)); } );
      if(entrie != fis.end()) line_entrie.push_back( to_string( (*entrie)->weighted_integral ) );
      else line_entrie.push_back( " " );
    }

    table_weight_norm.lines.push_back( line_entrie );
  }
  
  cout << table_sizes.get_text() << endl;

  cout << table_weight_norm.get_text() << endl;

  string out_string = "\\documentclass{article} \n \\usepackage{graphicx}";
  out_string += "\n \\begin{document} \n";
  out_string += "\n $\\sum Events$ : \n";
  out_string += table_sizes.get_text();
  out_string += "\\newpage \n $\\sum Events * weight_norm$: \n";
  out_string += table_weight_norm.get_text();
  out_string += " \n \\end{document} \n";

  ReplaceStringInPlace( out_string, string("_"), string("\\_"));

  ofstream out( "mensuraDiagnostic.tex" );
  out << out_string << endl;
  out.close();


  TFile * out_file = new TFile("mensuraDiagnostic.root", "RECREATE");
  for(auto fi : fis){
    if(fi->weight_hist->GetEntries()) fi->weight_hist->Write();
    for(auto hist : fi->weight_hists) if(hist->GetEntries()) hist->Write();
    for(auto hist : fi->bnns_hists_w) if(hist->GetEntries()) hist->Write();
    for(auto hist : fi->bnns_hists)   if(hist->GetEntries()) hist->Write();
  }
/*
  if(false){
    PATH_APRIL     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/tuples";
    PATH_OCT_PUPPI = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-31_PUPPI/tuples";
    PATH_OCT_CHS   = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-21_CHS/tuples";
  }
  if(false){
    PATH_APRIL     = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/tuples_merged/Syst";
    PATH_OCT_PUPPI = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-31_PUPPI/tuples_merged/Syst";
    PATH_OCT_CHS   = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-10-21_CHS/tuples_merged/Syst";
  }
*/

}


