# -*- coding: utf-8 -*-

import sys
import copy

sys.path.append( "/afs/cern.ch/user/p/pmandrik/public/analysis/MSU_statistics_13TEV/theta_13tev_global/" )

import AutoDatacard as atd


import math
def qcd_jul(args):
  datacard = atd.DatacardMaster("qcd_jul", args.nbins)

  chanal_qcd   = atd.Chanal( "QCD" )
  chanal_other = atd.Chanal( "other" )

  datacard.chanals  += [ chanal_qcd, chanal_other ]

  f_qcd = atd.Parameter( "f_QCD", "flat_distribution", "mult" )
  f_qcd.options["mean"]  = 1.0
  f_qcd.options["range"] = '(0.0,"inf")'
  chanal_qcd.parameters = [ f_qcd ]

  f_other = atd.Parameter( "f_Other", "flat_distribution", "mult")
  f_other.options["mean"]  = 1.0
  f_other.options["range"] = '(0.0,"inf")'
  chanal_other.parameters = [ f_other ]

  datacard.use_mcmc = True
  datacard.input_file_mc   = args.input
  datacard.mcmc_iters  = args.niters
  return datacard

def sm_jul(args):
  datacard = atd.DatacardMaster("sm_jul", args.nbins)

  chanals_names = [
    "t_ch",    31./207., '(-0.5,0.5)', # 0.10
    "s_ch",    0.10, '(-2.85,3.0)',
    "tW_ch",   0.15, '(-6.0,-1.5)',
    "ttbar",   0.15, '(-4.0,5.0)',
    "Diboson", 0.20, '(-3.0,3.0)',
    "DY",      0.20, '(-3.0,3.0)',
    "WQQ",     0.30, '(-1.0,4.5)',
    "Wc",      0.30, '(-2.5,2.0)',
    "Wb",      0.30, '(-2.5,3.5)',
    "Wother",  0.30, '(-3.5,1.5)',
    #"Wjets",   0.30, '(-3.5,1.5)',
    "QCD",     0.80, '(-3.0,1.5)',
  ]

  mult_pars = ["lumi"]
  mult_errs = [0.025]

  interp_pars  = ["TagRate", "MistagRate"]
  interp_pars += ["PileUp", "JER", "UnclMET", "JEC"]
  # interp_pars += ["LepId", "LepTrig", "LepIso"]
  ren_pars     = ["Fac", "Ren", "RenFac"]
  interp_pars += ren_pars
  xsr_pars     = ["Isr_red", "Fsr_red"]
  interp_pars += xsr_pars

  pss = ["_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"]
  pss = []
  pss_names = []

  for item in ["isr", "fsr"] :
    for ps in pss:
      interp_pars += [ item + ps ]
      pss_names += [ item + ps ]

  has_red = ["s_ch", "t_ch", "ttbar", "WQQ", "Wb", "Wc", "Wother", "DY"]
  has_isr = ["s_ch", "t_ch", "ttbar", "tW_ch"]

  datacard.parameters_order_list  = [ "sigma_" + name for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ) ] 
  datacard.parameters_order_list += mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = atd.Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  0.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(-2.5,2.5)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = atd.Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  # define chanals
  for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ):
    chanal        = atd.Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)

    interp_pars = []
    for param in common_interp_pars:
      if param.name in ren_pars  and name not in has_red : continue
      if param.name in xsr_pars  and name not in has_isr : continue
      if param.name in pss_names and name not in has_isr : continue
      interp_pars += [ param ]

    if name != "t_ch":
      norm_parameter = atd.Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]  =  0.0
      norm_parameter.options["width"] =  err
      norm_parameter.options["range"] = rang
      chanal.parameters += [ norm_parameter ]
    else :
      norm_parameter = atd.Parameter( "sigma_" + name, "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 1.0
      norm_parameter.options["range"] = rang
      chanal.parameters += [ norm_parameter ]

    if name != "QCD" : chanal.parameters += interp_pars

    datacard.chanals += [ chanal ]

  return datacard

def sm_jul_unmarg(args):
  # get default sm analyses
  datacard = sm_jul(args)

  datacard.input_file_mc   = args.input
  datacard.input_file_data = args.input_data
  datacard.mcmc_iters      = args.niters
  datacard.mcmc_chains     = 1
  datacard.model_name_toy = "sm_unmarg"

  # nominal
  dcard = copy.deepcopy(datacard)
  dcard.name = "sm_nominal"
  for chanal in dcard.chanals:
    chanal.toy_normalization = 1.0 if chanal.name != "t_ch" else args.signal_norm
  dcard.save( args.mode.split(" ") )

  # will use toy as data input JEC
  dcard = copy.deepcopy(datacard)
  dcard.name = "sm_JECUp"
  for chanal in dcard.chanals:
    if chanal.name != "QCD": chanal.toy_hist_name     = chanal.name + "_JECUp" # will use this hist name
    chanal.toy_file_name     = "" # in this case will use same as for nominal model
    chanal.toy_normalization = 1.0 if chanal.name != "t_ch" else args.signal_norm
  dcard.save( args.mode.split(" ") )

  dcard = copy.deepcopy(datacard)
  dcard.name = "sm_JECDown"
  for chanal in dcard.chanals:
    if chanal.name != "QCD": chanal.toy_hist_name     = chanal.name + "_JECDown" # will use this hist name
    chanal.toy_file_name     = "" # in this case will use same as for nominal model
    chanal.toy_normalization = 1.0 if chanal.name != "t_ch" else args.signal_norm
  dcard.save( args.mode.split(" ") )

  # ttbar, t_ch, tW_ch scale up, scale down
  chanal_names = ["ttbar", "t_ch", "tW_ch"]
  shift_names   = ["scale_up", "scale_down"]
  for chanal_name in chanal_names:
    for shift_name in shift_names:
      dcard = copy.deepcopy(datacard)
      dcard.name = "sm_" + chanal_name + "_" + shift_name
      for chanal in dcard.chanals:
        chanal.toy_normalization = 1.0 if chanal.name != "t_ch" else args.signal_norm
        if chanal.name == chanal_name: 
          chanal.toy_hist_name     = chanal.name + "_" + shift_name 
          chanal.toy_file_name     = args.input_unmarg
        else:
          chanal.toy_file_name     = ""
      dcard.save( args.mode.split(" ") )

  return None;

def fcnc_1d_jul(args, coupling_hist_name):
  datacard = atd.DatacardMaster("fcnc_1d_jul_CHANGE_THIS_NAME", args.nbins)

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
    coupling_hist_name, 0.0,
  ]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.062]

  #interp_pars = ["PileUp", "JEC", "JER", "UnclMET", 
  #interp_pars = ["PileUp", "JER", "UnclMET", 
  #               "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" ]
  interp_pars  = ["TagRate", "MistagRate"]
  interp_pars += ["PileUp", "JER", "UnclMET", "JECF", "JECB"]

  datacard.parameters_order_list = [ "sigma_" + name for name, err in zip( chanals_names[::2], chanals_names[1::2] ) ] + mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = atd.Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  0.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(0.0,5.0)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = atd.Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  # define chanals
  for name, err in zip( chanals_names[::2], chanals_names[1::2] ):
    chanal        = atd.Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)

    if name not in ["fcnc_tug", "fcnc_tcg"]:
      norm_parameter = atd.Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]    =  0.0
      norm_parameter.options["width"]   =  err
      norm_parameter.options["range"] = '(-5.0,5.0)'
      chanal.parameters += [ norm_parameter ]
    if name == "fcnc_tcg" :
      norm_parameter = atd.Parameter("KC", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
    if name == "fcnc_tug" :
      norm_parameter = atd.Parameter("KU", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
    if name != "QCD" : chanal.parameters += common_interp_pars
    datacard.chanals += [ chanal ]

  return datacard

def fcnc_tug_jul(args):
  datacard = fcnc_1d_jul(args, "fcnc_tug")
  datacard.name = "fcnc_tug_jul"
  return datacard

def fcnc_tcg_jul(args):
  datacard = fcnc_1d_jul(args, "fcnc_tcg")
  datacard.name = "fcnc_tcg_jul"
  return datacard

def fcnc_1d_jul_expected(args, coupling_hist_name):
  datacard = atd.DatacardMaster("fcnc_1d_jul_CHANGE_THIS_NAME", args.nbins)

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
    coupling_hist_name, 0.0,
  ]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.062]

  #interp_pars = ["PileUp", "JEC", "JER", "UnclMET", 
  #interp_pars = ["PileUp", "JER", "UnclMET", 
  #               "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" ]
  interp_pars  = ["TagRate", "MistagRate"]
  interp_pars += ["PileUp", "JER", "UnclMET", "JECF", "JECB"]

  datacard.parameters_order_list = [ "sigma_" + name for name, err in zip( chanals_names[::2], chanals_names[1::2] ) ] + mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = atd.Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  0.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(0.0,5.0)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = atd.Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  # define chanals
  chanal_fcnc_tcg, chanal_fcnc_tug = None, None
  for name, err in zip( chanals_names[::2], chanals_names[1::2] ):
    chanal        = atd.Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)
    if name ==  "fcnc_tcg": chanal_fcnc_tcg = chanal
    if name ==  "fcnc_tug": chanal_fcnc_tug = chanal

    if name not in ["fcnc_tug", "fcnc_tcg"]:
      norm_parameter = atd.Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]    =  0.0
      norm_parameter.options["width"]   =  err
      norm_parameter.options["range"] = '(-5.0,5.0)'
      chanal.parameters += [ norm_parameter ]
    if name == "fcnc_tcg" :
      norm_parameter = atd.Parameter("KC", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
    if name == "fcnc_tug" :
      norm_parameter = atd.Parameter("KU", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
    if name != "QCD" : chanal.parameters += common_interp_pars
    datacard.chanals += [ chanal ]

  #  define expected data model
  print datacard.model_name_toy
  datacard.model_name_toy = "expected_data"
  if chanal_fcnc_tcg : chanal_fcnc_tcg.used_in_toydata = False
  if chanal_fcnc_tug : chanal_fcnc_tug.used_in_toydata = False

  return datacard

def fcnc_tug_jul_expected(args):
  datacard = fcnc_1d_jul_expected(args, "fcnc_tug")
  datacard.name = "fcnc_tug_jul_expected"
  return datacard

def fcnc_tcg_jul_expected(args):
  datacard = fcnc_1d_jul_expected(args, "fcnc_tcg")
  datacard.name = "fcnc_tcg_jul_expected"
  return datacard

def fcnc_tug_jul_expected_variation(args):
  datacard = fcnc_1d_jul_expected(args, "fcnc_tug")
  datacard.name = "fcnc_tug_jul_expected"

  datacard.input_file_mc   = args.input
  datacard.input_file_data = args.input_data
  datacard.mcmc_iters      = args.niters
  datacard.enable_barlow_beston = False
  datacard.seed            = 0
  datacard.dice_poisson    = False
  datacard.dice_systematic = False
  datacard.azimov          = True
  datacard.mcmc_chains     = 1

  chanals_names = {
    "t_ch":    0.10,
    "s_ch":    0.10,
    "tW_ch":   0.15,
    "ttbar":   0.15,
    "Diboson": 0.20,
    "DY":      0.20,
    "WQQ":     0.30,
    "Wc":      0.30,
    "Wb":      0.30,
    "Wother" :  0.30,
    "QCD" :     0.40,
  }

  for channel in datacard.chanals:
    dcard = copy.deepcopy(datacard)
    dcard.name = dcard.name + "_" + channel.name
    if channel.name not in chanals_names.keys() : continue

    for chan in dcard.chanals:
      #if chan.name != channel.name : continue

      chan.toy_normalization = 0.60
      dcard_minus = copy.deepcopy(dcard)
      dcard_minus.name = dcard.name + "_minus"
      dcard_minus.save( args.mode.split(" ") )

      chan.toy_normalization = 1.40
      dcard_plus  = copy.deepcopy(dcard)
      dcard_plus.name = dcard.name + "_plus"
      dcard_plus.save( args.mode.split(" ") )

  return None









def fcnc_jul_unmarg(args, datacard, coupling_name):
  datacard.input_file_mc   = args.input
  datacard.input_file_data = args.input_data
  datacard.mcmc_iters      = args.niters
  datacard.mcmc_chains     = 1
  datacard.model_name_toy = "fcnc_unmarg"



  # nominal
  dcard = copy.deepcopy(datacard)
  dcard.name = "fcnc_" + coupling_name +"_nominal"
  for chanal in dcard.chanals:
    chanal.toy_normalization = 1.0 if chanal.name != ("fcnc_"+coupling_name) else args.signal_norm
  dcard.save( args.mode.split(" ") )

  # will use toy as data input JEC
  for sysfix in ["Up", "Down"]:
    dcard = copy.deepcopy(datacard)
    dcard.name = "fcnc_" + coupling_name + "_JEC" + sysfix
    for chanal in dcard.chanals:
      if chanal.name != "QCD": chanal.toy_hist_name = chanal.name + "_JEC"  + sysfix # will use this hist name
      chanal.toy_normalization = 1.0 if chanal.name != ("fcnc_"+coupling_name) else args.signal_norm
    dcard.save( args.mode.split(" ") )

  # ttbar, t_ch, tW_ch scale up, scale down
  chanal_names = ["ttbar", "t_ch", "tW_ch"]
  shift_names   = ["scale_up", "scale_down"]
  for chanal_name in chanal_names:
    for shift_name in shift_names:
      dcard = copy.deepcopy(datacard)
      dcard.name = "fcnc_" + coupling_name + "_" + chanal_name + "_" + shift_name
      for chanal in dcard.chanals:
        chanal.toy_normalization = 1.0 if chanal.name != ("fcnc_"+coupling_name) else args.signal_norm
        if chanal.name == chanal_name: 
          chanal.toy_hist_name     = chanal.name + "_" + shift_name 
          chanal.toy_file_name     = args.input_unmarg
        else:
          chanal.toy_file_name     = ""
      dcard.save( args.mode.split(" ") )

  return None;

def fcnc_tug_jul_unmarg(args):
  # get default fcnc analyses
  datacard = fcnc_tug_jul(args)
  fcnc_jul_unmarg(args, datacard, "tug")

def fcnc_tcg_jul_unmarg(args):
  # get default fcnc analyses
  datacard = fcnc_tcg_jul(args)
  fcnc_jul_unmarg(args, datacard, "tcg")

if __name__ == "__main__": 
  import argparse
  parser = argparse.ArgumentParser(description='Produce some theta .cfg datacards ... ')
  parser.add_argument('--fcnc_fix',   dest='fcnc_fix',   type=str,   default='', help='')
  parser.add_argument('--fname',      dest='fname',      type=str,   default='test', help='predifined function name to call')
  parser.add_argument('--nbins',      dest='nbins',      type=int,   default=10,     help='number of bins in histograms')
  parser.add_argument('--niters',     dest='niters',     type=int,   default=50000,     help='number of iters')
  parser.add_argument('--input',      dest='input',      type=str,   default='input.root', help='root input file name')
  parser.add_argument('--input_data', dest='input_data', type=str,   default='', help='root input toys file name')
  parser.add_argument('--input_unmarg', dest='input_unmarg', type=str,   default='', help='root input unmarginal systematic file name')
  parser.add_argument('--mode',       dest='mode',       type=str,   default="latex cl theta", help='')
  parser.add_argument('--signal_norm',dest='signal_norm',type=float, default=1.0, help='')
  args = parser.parse_args()

  print "create_card.py call ..."
  flist = [f for f in globals().values() if (type(f) == type(atd.test) and f.__name__ == args.fname) ]

  if flist : 
    print "create_card.py will use ", args.fname, " -> ", flist, "with parameters ", args.input, args
    datacard = flist[0](args)

    # set some options
    if datacard: 
      datacard.input_file_mc   = args.input
      datacard.input_file_data = args.input_data
      datacard.mcmc_iters      = args.niters
      datacard.mcmc_chains     = 1
      datacard.dice_poisson    = False
      datacard.dice_systematic = False
      datacard.azimov          = True
      datacard.enable_barlow_beston = True

      datacard.save( args.mode.split(" ") )
  else     : atd.test()























