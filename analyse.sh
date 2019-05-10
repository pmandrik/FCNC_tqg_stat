#!/bin/bash

#---------- 0. Please Set Parameters

nbins=10
niters=500000
release="PUPPI_JID_BTAG"  # "PUPPI_JID" "CHS_JID" "PUPPI_JID_BTAG" "PUPPI_JID_JECB" "PUPPI_JID_JECF"
burn_in_frac=0.1

mode=$1
#  possible modes:
#    start  - just clean and create folder
#    qcd    - find qcd normalization
#    hists  - produce all related root files with hists
#    sm     - sm analyse
#    fcnc   - both fcnc 1d analyse
#    unmarg - unmarg error
#    full   - full chain

package=$2
#  supported packages:
#    theta
#    cl
#    all

#---------- 1. Setup
myname=`basename "$0"`
echo "$myname, setup ... "

source /cvmfs/sft.cern.ch/lcg/app/releases/ROOT/6.06.08/x86_64-slc6-gcc49-opt/root/bin/thisroot.sh
source /cvmfs/sft.cern.ch/lcg/contrib/gcc/4.9/x86_64-slc6-gcc49-opt/setup.sh

export PATH=/cvmfs/sft.cern.ch/lcg/external/Python/2.7.3/x86_64-slc6-gcc48-opt/bin:$PATH
export LD_LIBRARY_PATH=/cvmfs/sft.cern.ch/lcg/external/Python/2.7.3/x86_64-slc6-gcc48-opt/lib:$LD_LIBRARY_PATH

srcdir=/afs/cern.ch/user/p/pmandrik/public/analysis/MSU_statistics_13TEV/theta_13tev_global
cfgdir=$(pwd)
workdir=$(pwd)/$release

#set -e
#set -o xtrace

if [ "$mode" = "start" ] || [ "$mode" = "full" ]; then
  echo "$myname, recreate work directory $workdir"
  rm -rf $workdir
  mkdir -p $workdir
  if [ "$mode" = "start" ]; then exit; fi
else echo "$myname, skip recreating work directory $workdir"; fi

cd $workdir

#---------- 2. Find QCD normalization
if [ "$mode" = "qcd" ] || [ "$mode" = "full" ]; then
  rm -rf "$workdir/qcd" && mkdir "$_" && cd "$_"
  echo "$myname, find QCD normalization ... "
  root -q -b -l "$cfgdir/tree_to_hists.C(\"QCD\",\"$release\",\"hists_QCD.root\",$nbins)"
  root -q -b -l "$srcdir/histsPlot.cpp(\"QCD_before\", \"hists_QCD.root\")"

  python $cfgdir/create_card.py --fname="qcd_jul" --nbins=$nbins --input="hists_QCD.root" --mode="theta"
  $srcdir/run_theta.sh qcd_jul_theta.cfg

  root -q -b -l "$srcdir/getQuantiles.cpp(\"qcd_jul_theta.root\", \"f_Other\")"
  IFS=" " read QCD_low QCD_norm QCD_upp <<< "`cat getQuantiles_temp.txt`"
  echo "$myname, Other norm factors = $QCD_low $QCD_norm $QCD_upp ..."

  root -q -b -l "$srcdir/getQuantiles.cpp(\"qcd_jul_theta.root\", \"f_QCD\")"
  IFS=" " read QCD_low QCD_norm QCD_upp <<< "`cat getQuantiles_temp.txt`"
  echo "$myname, QCD norm factors = $QCD_low $QCD_norm $QCD_upp ..."

  root -q -b -l "$srcdir/histsPlot.cpp(\"QCD_after\",\"hists_QCD.root\","$QCD_norm")"
else echo "$myname, skip qcd normalization calcullations"; fi

IFS=" " read QCD_low QCD_norm QCD_upp <<< "`cat $workdir/qcd/getQuantiles_temp.txt`"
echo "$myname, QCD norm factors = $QCD_low $QCD_norm $QCD_upp ..."

#---------- 3. Create histogramms file
if [ "$mode" = "hists" ] || [ "$mode" = "full" ]; then
  mkdir -p "$workdir/hists" && cd "$_"
  echo "$myname, create histogramms file ... "
  if [ "$package" = "sm" ] || [ "$package" = "all" ]; then
    root -q -b -l "$cfgdir/tree_to_hists.C(\"SM\",\"$release\",\"hists_SM.root\",$nbins, $QCD_norm)"
    root -q -b -l "$srcdir/histsPlot.cpp(\"SM_before\",\"hists_SM.root\")"
    root -q -b -l "$srcdir/histsChecker.cpp(\"hists_SM.root\",\"SM_\")"
    #python $srcdir/create_toy.py --input="hists_SM.root" --output="toy_hists_SM.root" --fname="simple"
  fi

  if [ "$package" = "unmarg_sm" ] || [ "$package" = "all" ]; then
    root -q -b -l "$cfgdir/tree_to_hists.C(\"SM_unmarg\",\"$release\",\"hists_SM_unmarg.root\",$nbins, $QCD_norm)"
    root -q -b -l "$srcdir/histsCompUnmarg.cpp(\"hists_SM.root\", \"hists_SM_unmarg.root\",\"SM_unmarg_\")"
  fi

  if [ "$package" = "fcnc" ] || [ "$package" = "all" ]; then
    root -q -b -l "$cfgdir/tree_to_hists.C(\"FCNC_tcg\",\"$release\",\"hists_FCNC_tcg.root\",$nbins, $QCD_norm)"
    root -q -b -l "$cfgdir/tree_to_hists.C(\"FCNC_tug\",\"$release\",\"hists_FCNC_tug.root\",$nbins, $QCD_norm)"

    root -q -b -l "$srcdir/histsPlot.cpp(\"FCNC_tug\",\"hists_FCNC_tug.root\")"
    root -q -b -l "$srcdir/histsChecker.cpp(\"hists_FCNC_tug.root\",\"FCNC_tug_\")"

    root -q -b -l "$srcdir/histsPlot.cpp(\"FCNC_tcg\",\"hists_FCNC_tcg.root\")"
    root -q -b -l "$srcdir/histsChecker.cpp(\"hists_FCNC_tcg.root\",\"FCNC_tcg_\")"
  fi

  if [ "$mode" = "hists" ]; then exit; fi
else echo "$myname, skip recreating histogramms files"; fi

#---------- 4. Run SM analyse
if [ "$mode" = "sm" ] || [ "$mode" = "full" ]; then
  echo "$myname, SM ... "
  mkdir -p "$workdir/sm" && cd "$_"

  if [ "$package" = "theta" ] || [ "$package" = "all" ]; then
    python $cfgdir/create_card.py --fname="sm_jul" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_SM.root" --mode="latex theta mRoot"
    $srcdir/run_theta.sh sm_jul_theta.cfg
    root -q -b -l "$srcdir/burnInStudy.cpp(\"sm_jul_theta.root\", \"sigma_t_ch\", \"sm_theta\")"
    root -q -b -l "$srcdir/getTable.cpp(\"sm_jul_theta.root\", \"sm_theta\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_sm_theta.tex

    root -q -b -l "$srcdir/getPostHists.cpp(\"$workdir/hists/hists_SM.root\", \"sm_jul_mroot.txt\", \"sm_jul_theta.root\")"
    root -q -b -l "$srcdir/histsPlot.cpp(\"SM_after\",\"postfit_hists/posthists.root\")"
    root -q -b -l "$srcdir/histsChecker.cpp(\"$workdir/hists/hists_SM.root\",\"./postfit_hists/posthists.root\", \"SM_comp_\")"
  fi

  python $cfgdir/create_card.py --fname="sm_jul" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_SM.root" --mode="latex"
  pdflatex -interaction=batchmode model_sm_jul.tex

  if [ "$mode" = "sm" ]; then exit; fi
else echo "$myname, skip sm analyse"; fi

#---------- 5. Run FCNC analyse
set -x
if [ "$mode" = "fcnc" ] || [ "$mode" = "full" ]; then
  echo "$myname, FCNC ... "
  mkdir -p "$workdir/fcnc" && cd "$_"
    python $cfgdir/create_card.py --fname="fcnc_tug_jul" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_FCNC_tug.root" --mode="latex theta"
    $srcdir/run_theta.sh fcnc_tug_jul_theta.cfg
    root -q -b -l "$srcdir/burnInStudy.cpp(\"fcnc_tug_jul_theta.root\", \"KU\", \"fcnc_tug_theta\")"
    root -q -b -l "$srcdir/getTable.cpp(\"fcnc_tug_jul_theta.root\", \"fcnc_tug_theta\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_fcnc_tug_theta.tex

    python $cfgdir/create_card.py --fname="fcnc_tcg_jul" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_FCNC_tcg.root" --mode="latex theta"
    $srcdir/run_theta.sh fcnc_tcg_jul_theta.cfg
    root -q -b -l "$srcdir/burnInStudy.cpp(\"fcnc_tcg_jul_theta.root\", \"KC\", \"fcnc_tcg_theta\")"
    root -q -b -l "$srcdir/getTable.cpp(\"fcnc_tcg_jul_theta.root\", \"fcnc_tcg_theta\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_fcnc_tcg_theta.tex

    python $cfgdir/create_card.py --fname="fcnc_tug_jul_expected" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_FCNC_tug.root" --mode="latex theta"
    $srcdir/run_theta.sh fcnc_tug_jul_expected_theta.cfg
    root -q -b -l "$srcdir/burnInStudy.cpp(\"fcnc_tug_jul_expected_theta.root\", \"KU\", \"fcnc_tug_theta\")"
    root -q -b -l "$srcdir/getTable.cpp(\"fcnc_tug_jul_expected_theta.root\", \"fcnc_tug_expected_theta\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_fcnc_tug_expected_theta.tex

    python $cfgdir/create_card.py --fname="fcnc_tcg_jul_expected" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_FCNC_tcg.root" --mode="latex theta"
    $srcdir/run_theta.sh fcnc_tcg_jul_expected_theta.cfg
    root -q -b -l "$srcdir/burnInStudy.cpp(\"fcnc_tcg_jul_expected_theta.root\", \"KC\", \"fcnc_tcg_theta\")"
    root -q -b -l "$srcdir/getTable.cpp(\"fcnc_tcg_jul_expected_theta.root\", \"fcnc_tcg_expected_theta\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_fcnc_tcg_expected_theta.tex

  pdflatex -interaction=batchmode model_fcnc_tcg_jul.tex
  pdflatex -interaction=batchmode model_fcnc_tug_jul.tex
  pdflatex -interaction=batchmode model_fcnc_tcg_jul_expected.tex
  pdflatex -interaction=batchmode model_fcnc_tug_jul_expected.tex

  if [ "$mode" = "sm" ]; then exit; fi
else echo "$myname, skip sm analyse"; fi

if [ "$mode" = "fcnc_var" ] || [ "$mode" = "full" ]; then
  mkdir -p "$workdir/fcnc_var" && cd "$_"
  python $cfgdir/create_card.py --fname="fcnc_tug_jul_expected_variation" --nbins=$nbins --niters=$niters --input="$workdir/hists/hists_FCNC_tug.root" --mode="latex theta"
  
  for name in fcnc_*cfg; do
    echo $name
    $srcdir/run_theta.sh $name

    filename="${name%.*}"
    root -q -b -l "$srcdir/getTable.cpp(\"$filename.root\", \"$filename\", $burn_in_frac)"
    pdflatex -interaction=batchmode getTable_$filename.tex
  done;
fi

















