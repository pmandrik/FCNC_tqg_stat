# -*- coding: utf-8 -*-

"""
/////////////////////////////////////////////////////////////////////////////
  Author      :     P.Mandrik, IHEP
  Date        :     12/07/17
  Version     :     0.5.5
/////////////////////////////////////////////////////////////////////////////

  Python module AutoDatacart.py provide function to create datacards for
  for bayesian discrete template-based analyses for
    1. theta
    2. CombinedLimit
  ROOT-only, skip some of theta configurations, single output file
  in addition create a latex human-readable table for defined configuration file
    
/////////////////////////////////////////////////////////////////////////////
  Changelog : 
    10/03/17, 13/03/17 v0.5.0
    days of creation, version 0.5.0

    12/07/17 v0.5.5
    add support for CombinedLimit
"""

import TemplateMaster

#====================================================================== TEMPLATE FOR THETA
def_theta_template = """
// generated with AutoDatacart.py and TemplateMaster.py at {% DATE %}

observables = {
  obs_name = {
    range = (0.0, 1.0);
    nbins = {% N_BINS %};
  };	
};

// input histograms definitions ----------------
{% for CHANAL in CHANALS %}
  // {% CHANAL.name %} ----------------
  // nominal
  {% CHANAL.name %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE_MC %}";
    histoname = "{% CHANAL.toy_hist_name %}";
    use_errors = true; 
  };
  // alternative
  {% CHANAL.name %}_alt = {
    type = "root_histogram";
    filename = "{% CHANAL.toy_file_name %}";
    histoname = "{% CHANAL.alt_hist_name %}";
    use_errors = true; 
  };
{% for PARAMETER in CHANAL.get_shape_params %}
  // {% CHANAL.name %} + {% PARAMETER %}
  {% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE_MC %}";
    histoname = "{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %}";
    use_errors = false; 
  };
  {% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE_MC %}";
    histoname = "{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %}";
    use_errors = false; 
  };

{% endfor %}

{% endfor %}

// parameters definition ----------------
  parameters = {% func theta_set_like PARAMETERS_NAMES_ALL %}{% endfunc %};

// model definition ----------------
  {% MODEL_NAME %} = {
    obs_name = {
{% for CHANAL in CHANALS %}
{% if CHANAL.used_in_fit %}
        {% CHANAL.name %} = {
          coefficient-function = {type = "multiply"; factors = {% func theta_set_like CHANAL.get_mult_params %}{% endfunc %}; };
          histogram = { 
            type = "interpolating_histo";
            parameters = {% func theta_set_like CHANAL.get_shape_params %}{% endfunc %};
            nominal-histogram = "@{% CHANAL.name %}";
{% for PARAMETER in CHANAL.get_shape_params %}
            {% PARAMETER %}-plus-histogram  = "@{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %}";
            {% PARAMETER %}-minus-histogram = "@{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %}";
{% endfor %}          };
        };
{% endif %}
{% endfor %}
    };

    parameter-distribution = {
      type ="product_distribution";
      distributions ={% func theta_set_like_distr PARAMETERS_NAMES_FIT %}{% endfunc %};
    };

    bb_uncertainties = {% BARLOW_BESTON %};
  };

// alternative model definition ----------------
{% if MODEL_NAME_TOY %}
  {% if AZIMOV %}
    {% MODEL_NAME_TOY %} = {
      obs_name = {
  {% for CHANAL in CHANALS %}
  {% if CHANAL.used_in_toydata %}
          {% CHANAL.name %} = {
            coefficient-function = {type ="multiply"; factors = ("{% CHANAL.name %}_toy_norm");};
	          histogram = "@{% CHANAL.name %}_alt";
          };
  {% endif %}
  {% endfor %}
      };
      parameter-distribution = {
        type = "delta_distribution";
  {% for CHANAL in CHANALS %}
  {% if CHANAL.used_in_toydata %}
        {% CHANAL.name %}_toy_norm = {% CHANAL.toy_normalization %};
  {% endif %}
  {% endfor %}
     };

      bb_uncertainties = false;
    };
  {% else %}
    {% MODEL_NAME_TOY %} = {
      obs_name = {
  {% for CHANAL in CHANALS %}
  {% if CHANAL.used_in_toydata %}
          {% CHANAL.name %} = {
            coefficient-function = {type = "multiply"; factors = {% func theta_set_like CHANAL.get_mult_params_fix %}{% endfunc %}; };
            histogram = { 
              type = "interpolating_histo";
              parameters = {% func theta_set_like CHANAL.get_shape_params_fix %}{% endfunc %};
              nominal-histogram = "@{% CHANAL.base_hist_name %}_alt";
  {% for PARAMETER in CHANAL.get_shape_params %}
              {% PARAMETER %}_fix-plus-histogram  = "@{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %}";
              {% PARAMETER %}_fix-minus-histogram = "@{% CHANAL.base_hist_name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %}";
  {% endfor %}          };
          };
  {% endif %}
  {% endfor %}
      };

      parameter-distribution = {
        type ="product_distribution";
        distributions ={% func theta_set_like_distr PARAMETERS_NAMES_TOYDATA %}{% endfunc %};
      };

      bb_uncertainties = false;
    };
  {% endif %}
{% endif %}

// parameters distributions definitions ----------------
{% for PARAMETER in PARAMETERS %}
  {% PARAMETER.name %}-dist = {
    type = "{% PARAMETER.theta_distr %}";
{% if PARAMETER.is_flat %}
    {% PARAMETER.name %} = {
{% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
    };
{% else %} 
{% if PARAMETER.is_delta %}
    {% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
    {% endfor %}  };
{% else %}
    parameter = "{% PARAMETER.name %}";
{% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
{% endif %}
{% endif %}
{% endfor %}

{% if AZIMOV %}{% else %}
{% if MODEL_NAME_TOY %}
{% for PARAMETER in PARAMETERS_TOYDATA %}
  {% PARAMETER.name %}-dist = {
    type = "{% PARAMETER.theta_distr %}";
{% if PARAMETER.is_flat %}
    {% PARAMETER.name %} = {
{% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
    };
{% else %}
{% if PARAMETER.is_delta %}
    {% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
    {% endfor %}  };
{% else %}
    parameter = "{% PARAMETER.name %}";
{% for OTHER_PAIRS in PARAMETER.theta_other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
{% endif %}
{% endif %}
{% endfor %}
{% endif %}
{% endif %}


// hypotest definition ----------------
hypotest = {
  type = "mcmc_chain";
  name = "mcmc_chain";
  iterations = {% MCMC_ITERATION_N %};
  burn-in    = 0; 
  re-init    = 0;
  output_database = {
    type = "rootfile_database";
    filename = "{% MODEL_NAME %}_theta.root";
  };
};

// main definition ----------------
  main = {
    {% if MODEL_NAME_TOY %}
    data_source = {
      type = "model_source";
      model = "@{% MODEL_NAME_TOY %}";
      dice_poisson = {% DICE_POISSON %};
      dice_template_uncertainties = {% DICE_SYSTEMATIC %};
      name = "source";
      rnd_gen = { seed = {% SEED %}; }; 
    };
    {% else %}
    data_source = {
      type = "histo_source";
      obs_name = {
        type= "root_histogram";
        filename ="{% INPUT_FILE_DATA %}";
        histoname = "{% DATA_HIST_NAME %}";
      };
      name = "source";
    };
    {% endif %}
    
    model = "@{% MODEL_NAME %}";
    producers = ("@hypotest");
    n-events = {% MCMC_CHAINS_N %};

    output_database = {
      type = "rootfile_database";
      filename = "{% MODEL_NAME %}.root";
      products_data = "*";
    };
  };

// final options  ----------------
  options = {
    plugin_files = ("$THETA_DIR/lib/root.so", "$THETA_DIR/lib/core-plugins.so" );
  };

"""

#====================================================================== TEMPLATE FOR CFG TABLE
def_theta_table="""
  % generated with AutoDatacart.py and TemplateMaster.py at {% DATE %}
\documentclass{article} 
\usepackage{graphicx} 
\usepackage{xcolor}
\usepackage{multirow}

\\begin{document} 
  \\begin{center} 

  % table of parameter vx chanals
  Parameters vs Chanals table:\\\\
  \\begin{small}
    \\setlength{\\tabcolsep}{2pt} % General space between cols (6pt standard)
    \\renewcommand{\\arraystretch}{1}
    \\begin{tabular}{ | c | {% for CHANAL in CHANALS %} c |{% endfor %}  }
    \hline
    \multirow{2}{*}{Parameters} & \multicolumn{ {% CHANALS.__len__ %}}{ |c| }{ Chanals } \\\\ 
    \cline{2-{% CHANALS.__len__ %}}
   {% for CHANAL in CHANALS %} & {% CHANAL.name %} {% endfor %} \\\\ 
    \hline
    \hline
  {% for PARAMETER in PARAMETERS %}
      {% PARAMETER.name %} {% for CHANAL in CHANALS %} & {% func tabulate_check_intersection CHANAL PARAMETER %}{% endfunc %} {% endfor %} \\\\
      \hline
  {% endfor %}
    \hline
    \\end{tabular}


  *N - template normalization parameter,\\\\
  *V - shape variation parameter\\
  \\end{small}
  
  \\newpage
  % table of parameter vx chanals
  Parameters descriptions table:\\\\
  \\begin{scriptsize}
    \\setlength{\\tabcolsep}{3pt} % General space between cols (6pt standard)
    \\renewcommand{\\arraystretch}{2}
    \\begin{tabular}{ | c | c | c | c | c |  }
    \hline
      Parameters & Prior Theta & Options Theta & Prior CL & Options CL \\\\
    \hline
    \hline
  {% for PARAMETER in PARAMETERS %}
      {% PARAMETER.name %} & {% PARAMETER.theta_distr %} & \shortstack{{% func tabulate_get_stack PARAMETER.theta_other %}{% endfunc %}} & {% PARAMETER.cl_distr %} & \shortstack{{% func tabulate_get_stack PARAMETER.cl_other %}{% endfunc %}} \\\\
      \hline
  {% endfor %}
    \hline
    \\end{tabular}
  \\end{scriptsize}

  \end{center} 
\end{document} 
"""

#====================================================================== TEMPLATE FOR COMBINEDLIMIT
def_combinedlimit_template="""
# generated with AutoDatacart.py and TemplateMaster.py at {% DATE %}, to run type:
# {% CL_RUN_COMMAND %}
# {% CL_PAR_VS_UNCERT %}

# defines the number of final states analyzed, number of channels in a counting experiment or the number of bins in a binned shape fit
imax * 

# defines the number of independent physical processes whose yields are provided to the code, minus one
jmax {% CL_NUMBER_OF_CHANALS %}

# defines the number of independent systematical uncertainties (nuisance parameters)
kmax {% CL_NUMBER_OF_PARAMETERS %}

------------
{% func tabulate %}#*shapes*  process    channel file                   histogram-name  histogram-name-for-systematics
shapes      *          *       {% INPUT_FILE_MC %}     $PROCESS        $PROCESS_$SYSTEMATIC
shapes      data_obs   *       {% INPUT_FILE_DATA %}   {% DATA_HIST_NAME %}{% endfunc %}

bin channel
observation {% CL_DATA_RATE %}

------------
{% func tabulate TABULATE_SYMBOL %}bin     |{% for CHANAL in CHANALS %}|channel{% endfor %}
process |{% for CHANAL in CHANALS %}|{% CHANAL.name %}{% endfor %}
process |{% for CHANAL in CHANALS %}|{% CHANAL.cl_id %}{% endfor %}
rate    |{% for CHANAL in CHANALS %}|{% CHANAL.cl_rate %}{% endfor %}

{% for PARAMETER in PARAMETERS %}
{% PARAMETER.name %} | {% PARAMETER.cl_distr %}{% for CL_WEIGHT in PARAMETER.cl_weights %}|{% CL_WEIGHT %} {% endfor %}
{% endfor %}{% endfunc %}
"""

def_mroot_template="""
{% for CHANAL in CHANALS %}{% CHANAL.name %} |{% for PARAMETER in CHANAL.get_shape_params %}{% PARAMETER %} {% endfor %} |{% for PARAMETER in CHANAL.get_mult_params %}{% PARAMETER %} {% endfor %}
{% endfor %}
"""

import os.path
import copy

def tabulate_check_intersection(text, chanal, parameter):
  if parameter.name in chanal.get_mult_params() : return "N"
  if parameter.name in chanal.get_shape_params() : return "V"
  else : return "-"

def tabulate_get_stack(text, list):
  answer = ""
  for i, item in enumerate(list):
    if type(item[1]) == type(1.1):
      answer += str(item[0]) + " = " + "{0:.4e}".format( item[1] )
    else : answer += str(item[0]) + " = " + str(item[1])
    if i != len(list)-1 : answer += " \\\\ "
  return answer

def theta_set_like(text, list=[]):
  answer = "("
  for i, item in enumerate(list):
    answer += "\"" + text + item + "\""
    if i != len(list)-1 : answer += ", "
  return answer + ")"

def theta_set_like_distr(text, list=[]):
  answer = "("
  for i, item in enumerate(list):
    answer += "\"@"+ item + "-dist\""
    if i != len(list)-1 : answer += ", "
  return answer + ")"

def theta_expand_pair(text, list=[]):
  return str(list[0]) + " = " + str(list[1]);

class Chanal():
  def __init__(self, name):
    self.name = name
    self.parameters = []

    self.cl_id = -9999.999
    self.cl_rate = -9999.999

    self.toy_hist_name     = name
    self.base_hist_name    = name
    self.alt_hist_name     = name
    self.toy_file_name     = ""
    self.toy_normalization = 1.0

    self.used_in_toydata = True
    self.used_in_fit = True

  def has_parameter(self, name):
    if name in self.parameters : return True
    return False

  def get_mult_params(self):
    return [ item.name for item in self.parameters if item.type != "shape" ]
    
  def get_shape_params(self):
    return [ item.name for item in self.parameters if item.type == "shape" ]

  def get_mult_params_fix(self):
    return [ item.name + "_fix" for item in self.parameters if item.type != "shape" ]

  def get_shape_params_fix(self):
    return [ item.name + "_fix" for item in self.parameters if item.type == "shape" ]


class Parameter():
  def __init__(self, name, distribution, type):
    self.name = name
    self.distr = distribution
    self.type = type
    self.options = {}

    self.theta_distr = "unknow"
    self.theta_other = []

    self.cl_distr     = "unknow"
    self.cl_other    = []
    self.cl_weights  = []
    self.cl_phys_range = ""

  def is_flat(self):
    if self.distr == "flat_distribution" : return True
    return False

  def is_delta(self):
    if self.distr == "delta_distribution" : return True
    return False

  def init(self):
    self.theta_other = []
    self.cl_other    = []
    
    if self.type == "shape" and not self.distr == "delta_distribution":
      self.theta_distr = "gauss"
      self.theta_other += [ ["mean", "0.0"] ]
      self.theta_other += [ ["width", "1.0"] ]
      self.theta_other += [ ["range", "(\"-inf\", \"inf\")"]]
      
      self.cl_distr = "shape"

    if self.type == "shape" and self.distr == "delta_distribution" : 
      self.theta_distr  = self.distr
      self.theta_other += [[self.name, self.options["mean"]]]

    if self.type == "mult" and self.distr == "log_normal": 
      self.theta_distr = self.distr
      self.theta_other += [ ["mu",    self.options["mean"]] ]
      self.theta_other += [ ["sigma", gauss_to_theta_lognormal(self.options["width"])] ]

      self.cl_distr = "lnN"
      self.cl_other += [ ["width", gauss_to_cl_lognormal(self.options["width"])] ]
      self.cl_phys_range = self.options["range"][1:-1]
      self.cl_other += [ ["range", self.cl_phys_range] ]

    if self.type == "mult" and self.distr == "flat_distribution" : 
      self.theta_distr = self.distr
      self.theta_other += [ ["fix-sample-value", self.options["mean"]] ]
      self.theta_other += [ ["range", '(0.0,"inf")'] ]

      self.cl_phys_range = self.options["range"][1:-1]
      self.cl_distr = "unif " + " ".join( self.options["range"][1:-1].split(",") )
      self.cl_other += [ ["range", self.cl_phys_range] ]

    if self.type == "mult" and self.distr == "delta_distribution" : 
      self.theta_distr  = self.distr
      self.theta_other += [[self.name, self.options["mean"]]]
    #print self.cl_distr

class DatacardMaster():
  def __init__(self, name, n_bins=50, path=""):
    # options
    self.chanals = []
    self.input_file_mc   = "histograms.root"
    self.input_file_data = ""
    self.enable_barlow_beston = "true"
    self.name = name
    self.path = ""
    self.mcmc_iters = 10000
    self.mcmc_chains = 1
    self.n_nibs = n_bins
    self.data_hist_name = "data"
    self.interpolating_postfixes = ["Up", "Down"]
    self.cl_run_command = "combine --help"
    self.cl_par_vs_uncert = ""
    self.parameters_order_list = []
    self.model_name_toy  = False
    self.dice_poisson    = False
    self.dice_systematic = False
    self.seed = 0
    self.azimov = True

  def init(self, mode):
    # collect parameters
    params = []
    for chanal in self.chanals: params += chanal.parameters
    params = list(set(params))
    
    # sort parameters
    params.sort(key=lambda x: x.cl_distr + x.name, reverse=True)
    self.parameters = []
    for par_name in self.parameters_order_list:
      par = next((par for par in params if par.name == par_name), None)
      if not par : continue
      self.parameters += [ par ]
      params.remove( par )
    self.parameters += params

    params = []
    for chanal in self.chanals:
      if chanal.used_in_fit: params += chanal.parameters
    params = list(set(params))
    self.parameters_fit = params

    params = []
    for chanal in self.chanals: 
      if chanal.used_in_toydata : params += chanal.parameters
    params = list(set(params))
    self.parameters_toydata = copy.deepcopy( params )
    for par in self.parameters_toydata:
      par.name += "_fix"
    #print "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    #print [ x.name for x in self.parameters ], self.parameters_order_list
    #print "\n\n\n\n\n\n\n"

    # init parameters
    for par in self.parameters : par.init()
    for par in self.parameters_toydata : par.init()

    # set cl command to run analyse
    # combine test_01_datacart.txt -M MarkovChainMC -i 10000000 --tries 1 --saveChain --noSlimChain --setPhysicsModelParameterRanges r=0.0,10.0:sigma_s_ch=0.0,10.0:sigma_tW_ch=0.0,10.0
    if "cl" in mode:
      self.cl_run_command  = "combine " + self.name+"_cl.txt -M MarkovChainMC -i " + str(self.mcmc_iters) + " --tries 1 --saveChain --noSlimChain "
      self.cl_run_command += "--burnInSteps 0 --noDefaultPrior=0 "
      self.cl_run_command += "--setParameterRanges "
      for par in self.parameters : 
        if par.cl_phys_range : 
          self.cl_run_command += par.name + "="
          self.cl_run_command += par.cl_phys_range + ":"
      self.cl_run_command = self.cl_run_command[:-1] + " "
      self.cl_run_command += "--freezeParameters r "
      self.cl_run_command += "--redefineSignalPOIs " + ",".join([par.name for par in self.parameters if par.is_flat()])

      # set list of used uncertanties in parameters - only for log-normal at the moment
      self.cl_par_vs_uncert = " ".join([ par.name + ":" + str(par.options["width"]) for par in self.parameters if par.distr == "log_normal"])
      self.cl_par_vs_uncert += " " + " ".join([ par.name + ":unif" for par in self.parameters if par.distr == "flat_distribution"])

    # check if datacard configuration is not wrong
    # 1. all chanals probably has parameters
    for chanal in self.chanals:
      if not len(chanal.parameters): print "DatacardMaster(): warning : ", chanal.name, " do not affected by any parameters"

    # 2. all distributions has known type
    if "cl" in mode:
      for par in self.parameters:
        if par.cl_distr == "unknow":
          print "DatacardMaster(): warning : ", par.name, " have unknow theta type for using in CombinedLimit"

    return

  def get_def_dic(self, mode):
    self.init(mode)
    dic = {}
    from time import gmtime, strftime

    # global options ----------------------------
    dic["DATE"] = strftime("%Y-%m-%d %H:%M:%S", gmtime())
    dic["CHANALS"] = self.chanals
    dic["PARAMETERS"] = self.parameters
    dic["PARAMETERS_TOYDATA"] = self.parameters_toydata
    dic["PARAMETERS_NAMES_FIT"] = [ par.name for par in self.parameters_fit ]
    dic["PARAMETERS_NAMES_ALL"] = [ par.name for par in self.parameters ]
    dic["PARAMETERS_NAMES_TOYDATA"] = [ par.name for par in self.parameters_toydata ]
    dic["INPUT_FILE_MC"]   = self.input_file_mc
    dic["INPUT_FILE_DATA"] = self.input_file_data if self.input_file_data else self.input_file_mc
    dic["DATA_HIST_NAME"] = self.data_hist_name
    dic["INTER_POSTFIX_PLUS"]  = self.interpolating_postfixes[0]
    dic["INTER_POSTFIX_MINUS"] = self.interpolating_postfixes[1]
    dic["BARLOW_BESTON"] = "true" if self.enable_barlow_beston else "false"
    dic["N_BINS"] = self.n_nibs
    dic["MODEL_NAME"] = self.name
    dic["MCMC_ITERATION_N"] = self.mcmc_iters
    dic["MCMC_CHAINS_N"] = self.mcmc_chains
    dic["TABULATE_SYMBOL"] = "|"
    dic["MODEL_NAME_TOY"] = self.model_name_toy
    dic["DICE_POISSON"] = "true" if self.dice_poisson else "false"
    dic["DICE_SYSTEMATIC"] = "true" if self.dice_systematic else "false"
    dic["SEED"] = self.seed
    dic["AZIMOV"] = self.azimov

    # set default chanal toy configuratonif not specified
    if  self.model_name_toy:
      for chanal in self.chanals:
        if not chanal.toy_file_name: chanal.toy_file_name = self.input_file_mc
      dic["PARAMETERS_NAMES_ALL"] += [ chanal.name + "_toy_norm" for chanal in self.chanals ]
      dic["PARAMETERS_NAMES_ALL"] += dic["PARAMETERS_NAMES_TOYDATA"]

     # CL specific options ----------------------------
    if "cl" in mode:
      dic["CL_NUMBER_OF_CHANALS"]    = len(self.chanals) - 1
      dic["CL_NUMBER_OF_PARAMETERS"] = len(dic["PARAMETERS"])
      dic["CL_RUN_COMMAND"]   = self.cl_run_command
      dic["CL_PAR_VS_UNCERT"] = self.cl_par_vs_uncert

      # rates
      try:
        import ROOT as root
        def get_rates_from_hist(hist_name, file_name):
          file = root.TFile(file_name, "READ")
          hist = file.Get(hist_name)
          try    : hist.Integral()
          except : print "DatacardMaster(): warning : can't gate rate for ", "\""+hist_name+"\"", hist, file_name
          return hist.Integral()

        for chanal in self.chanals:
          chanal.cl_rate = get_rates_from_hist(chanal.name, self.input_file_mc)

        dic["CL_DATA_RATE"] = get_rates_from_hist(self.data_hist_name, dic["INPUT_FILE_DATA"])
      except:
        print "DatacardMaster(): warning : can't process dictionary initialisation for CombinedLimit datacard"

      # ids
      for chanal in self.chanals:
        chanal.cl_id = self.chanals.index(chanal)

      # chanals weights
      def get_cl_weight(parameter, chanal):
        if parameter not in chanal.parameters : return "-"
        if parameter.type == "shape" : return 1.0
        if parameter.type == "mult"  :
          if parameter.distr == "log_normal" : 
            return [item[1] for item in parameter.cl_other if item[0] == "width"][0]
          if parameter.distr == "flat_distribution" : 
            return 1.
        return "?"

      for parameter in self.parameters:
        for chanal in self.chanals:
          parameter.cl_weights += [ get_cl_weight(parameter, chanal) ]

    # set some functions ----------------------------
    TemplateMaster.add_TemplateMaster_dictionary(dic)

    dic["theta_set_like"] = theta_set_like
    dic["theta_set_like_distr"] = theta_set_like_distr
    dic["theta_expand_pair"] = theta_expand_pair
    dic["tabulate_check_intersection"] = tabulate_check_intersection
    dic["tabulate_get_stack"] = tabulate_get_stack

    return dic

  def parce_template(self, template, dic):
    # apply magic
    return TemplateMaster.parce_template(template, dic)

  def save(self, mode, out_name=""):
    dic = self.get_def_dic(mode)
    if not out_name : out_name = self.name

    # THETA ----------------------------
    if "theta" in mode:
      datacard = self.parce_template(def_theta_template, dic)
      full_name = os.path.join(self.path, out_name+"_theta.cfg")
      f = open(full_name, "w")
      f.write(datacard)
      f.close()

    # LATEX SUPPORT TABLE ----------------------------
    if "latex" in mode:
      table = self.parce_template(def_theta_table, dic)
      table = table.replace("_", "\\_")
      full_name = os.path.join(self.path, "model_"+out_name+".tex")
      f = open(full_name, "w")
      f.write(table)
      f.close()

    # COMBINEDLIMIT ----------------------------
    if "cl" in mode:
      datacard = self.parce_template(def_combinedlimit_template, dic)
      full_name = os.path.join(self.path, out_name+"_cl.txt")
      f = open(full_name, "w")
      f.write(datacard)
      f.close()

    # mRoot ----------------------------
    if "mRoot" in mode:
      datacard = self.parce_template(def_mroot_template, dic)
      full_name = os.path.join(self.path, out_name+"_mroot.txt")
      f = open(full_name, "w")
      f.write(datacard)
      f.close()

import math
def gauss_to_theta_lognormal(val):
  tval = math.sqrt( math.log(0.5 + 0.5 * math.sqrt(1 + 4 * val*val)) )
  return tval

def gauss_to_cl_lognormal(val):
  # for log-normal rules is 1+Δx/x
  tval = 1. + val
  return tval

#====================================================================== DEFAULT MAIN FOR TESTING
def test():
  datacard = DatacardMaster("test_datacard", n_bins=20)

  chanals_names = [
    "t_ch",    0.10,
    "s_ch",    0.10,
    "tW_ch",   0.15,
    "ttbar",   0.15,
    "Diboson", 0.20,
    "DY",      0.20,
    "WQQ",     0.30,
    "Wc",      0.30,
    "Wb",      0.30,
    "Wother",  0.30,
    "QCD",     0.40,
  ]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.062]

  interp_pars  = ["btag_jes", "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2"]
  interp_pars += ["PileUp", "JER", "UnclMET", "JEC"]

  datacard.parameters_order_list = [ "sigma_" + name for name, err in zip( chanals_names[::2], chanals_names[1::2] ) ] + mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  1.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(-4.0,4.0)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  # define chanals
  for name, err in zip( chanals_names[::2], chanals_names[1::2] ):
    chanal        = Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)

    if name != "t_ch":
      norm_parameter = Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]    =  0.0
      norm_parameter.options["width"]   =  err
      norm_parameter.options["range"] = '(-4.0,4.0)' # this is only CL range in poweres of sigma
      chanal.parameters += [ norm_parameter ]
    else :
      norm_parameter = Parameter( "sigma_" + name, "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 1.0
      norm_parameter.options["range"] = '(-4.0,4.0)' # this is both for theta and CL
      chanal.parameters += [ norm_parameter ]

    if name != "QCD" : chanal.parameters += common_interp_pars

    datacard.chanals += [ chanal ]

  # set some options
  datacard.input_file_mc = "hists_SM.root"
  datacard.mcmc_iters  = 500000
  datacard.mcmc_chains = 1

  datacard.save()

if __name__ == "__main__": 
  print "AutoDatacard.py test run ..."
  test()








