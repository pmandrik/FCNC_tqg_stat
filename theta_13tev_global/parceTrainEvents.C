
vector<string> * split_string(string str, const string & sep = " "){
  int sep_size = sep.size();
  vector<string> * answer = new vector<string>();

  for(int i = 0; i + sep_size < str.size(); i++){
    if( sep == str.substr(i, sep_size) ){
      answer->push_back( str.substr(0, i) ); 
      str = str.substr(i + sep_size, str.size() - i - sep_size);
      i = 0;
    }
  }

  answer->push_back(str);
  return answer;
}

vector<pair<string, vector<int>*>> *parce_train_events(string fname = "bnn_sm7_trainEvents.txt"){
  vector<pair<string, vector<int>*>> * answer = new vector<pair<string, vector<int>*>>;
  vector<int> * current_events;
  string current_name = "";

  regex name_pattern( ".+\.root" );

  regex sep(" ", regex_constants::egrep);
  smatch sm;

  cout << "parce_train_events() ... " << endl;
  int i = 0;
  std::ifstream infile(fname.c_str());
  for( std::string line; std::getline( infile, line ); ){
    if( regex_match(line, name_pattern) ){ // find new name line
      if(current_name != "") answer->push_back( make_pair(current_name, current_events) );
      current_events = new vector<int>();
      current_name = line;
      cout << "add " << current_name << endl;
      continue;
    }

    auto str = split_string(line, " ");
    if(str->size() > 8){
      for(auto s : *str) current_events->push_back( atoi(s.c_str()) );
      delete str;
    }
  }
  answer->push_back( make_pair(current_name, current_events) );

  cout << "parce_train_events() ... done" << endl;
  return answer;
}



