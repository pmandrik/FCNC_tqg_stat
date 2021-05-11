
#include "getQuantiles.cpp"
#include "getTable.cpp"

#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_stat_analyses.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_other.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_root_hist_drawer.hh"

pm::Limits get_result_from_file(string file_name, string par_name, float burn_factor = 0.10){
  TFile file( file_name.c_str() );

  pm::Limits limit;
  int chain_index = 0;

  while(true){
    chain_index++;
    string cname = "chain_" + to_string( chain_index );
    TTree * tree = (TTree*)(file.Get(cname.c_str()));
    if(not tree) break;
  
    Int_t weight;
    tree->SetBranchAddress("weight",  &weight);
  
    MyParameter * parameter = new MyParameter(tree, par_name, burn_factor);
    for(int l = 0; l < (int)tree->GetEntries(); ++l){
      tree->GetEntry(l);
      parameter->ReadEntrie( weight );
    }

    limit += pm::get_th1_limits( parameter->hist );
    // pm::get_th1_limits( parameter->hist ).Print();
  }

  limit.c_1s /= max(1, chain_index-1);
  limit.u_1s /= max(1, chain_index-1);
  limit.u_2s /= max(1, chain_index-1);
  limit.l_1s /= max(1, chain_index-1);
  limit.l_2s /= max(1, chain_index-1);
  
  limit.Print();
  return limit;
}

void plotResultsForDifferentConditions(std::string input_folder_name, std::string pattern, std::string parameter_name, std::string graph_label){
  
  vector<string> fnames;
  pm::get_all_files_in_folder(input_folder_name, pattern, fnames);
  
  int N_points = fnames.size();
  vector<float> x_vals, y_vals;
  vector<float> y_vals_1s_u, y_vals_2s_u, y_vals_1s_d, y_vals_2s_d;
  std::string x_axis_name;
  for(string fname : fnames){
    vector<string> fname_parts;
    pm::split_string(fname, fname_parts, "_");
    if(fname_parts.size() < 2){
      cout << "plotResultsForDifferentConditions(): Can't parce file name " << fname << endl;
      return;
    }
    x_vals.push_back( atof( fname_parts.at(2).c_str() ) );
    cout << fname << " " << atof( fname_parts.at(2).c_str() ) << endl;
    
    pm::Limits limit = get_result_from_file( input_folder_name + "/" + fname, parameter_name );
    if(limit.c_1s <= 0) continue;
    // l_1s, c_1s, u_1s, l_2s, c_2s, u_2s
    y_vals.push_back( limit.c_1s );
    y_vals_1s_u.push_back( limit.u_1s );
    y_vals_2s_u.push_back( limit.u_2s );
    y_vals_1s_d.push_back( limit.l_1s );
    y_vals_2s_d.push_back( limit.l_2s );
    
    x_axis_name = fname_parts.at(0);
  }

  TCanvas canv(graph_label.c_str(), graph_label.c_str(), 640, 480);
  TGraph * sigma_2 = pm::draw_brasil(x_vals, y_vals_2s_u, y_vals_1s_u, y_vals, y_vals_1s_d, y_vals_2s_d);
  
  sigma_2->GetXaxis()->SetTitle( x_axis_name.c_str() );
  sigma_2->GetYaxis()->SetTitle( parameter_name.c_str() );
  sigma_2->SetTitle( graph_label.c_str() );
  
  canv.SetTitle( graph_label.c_str() );
  
  canv.Print( (graph_label + ".png").c_str() );
}






