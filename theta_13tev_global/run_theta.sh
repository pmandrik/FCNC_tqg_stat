#!/bin/bash

datacard=$1

#source /afs/cern.ch/user/p/pmandrik/public/global_cfg/theta/theta_slc6/theta-env.sh

source /cvmfs/sft.cern.ch/lcg/external/gcc/4.7.2/x86_64-slc6-gcc47-opt/setup.sh
source /cvmfs/sft.cern.ch/lcg/app/releases/ROOT/5.34.26/x86_64-slc6-gcc47-opt/root/bin/thisroot.sh
export OPTIONS_BOOSTDIR=/cvmfs/sft.cern.ch/lcg/external/Boost/1.53.0_python2.7/x86_64-slc6-gcc47-opt
export OPTIONS_SQLITEDIR=/cvmfs/sft.cern.ch/lcg/external/sqlite/3070900/x86_64-slc6-gcc47-opt
export LD_LIBRARY_PATH="/cvmfs/sft.cern.ch/lcg/external/Boost/1.53.0_python2.7/x86_64-slc6-gcc47-opt/lib:/afs/cern.ch/user/p/pmandrik/public/global_cfg/theta/theta/theta/bin:/cvmfs/sft.cern.ch/lcg/app/releases/ROOT/5.34.26/x86_64-slc6-gcc47-opt/root/lib:/cvmfs/sft.cern.ch/lcg/external/gcc/4.7.2/x86_64-slc6-gcc47-opt/lib64:/cvmfs/sft.cern.ch/lcg/external/mpfr/2.4.2/x86_64-slc6-gcc47-opt/lib:/cvmfs/sft.cern.ch/lcg/external/gmp/4.3.2/x86_64-slc6-gcc47-opt/lib
"

echo $LD_LIBRARY_PATH
echo "run_theta over "$1" datacard ... "
time /afs/cern.ch/user/p/pmandrik/public/global_cfg/theta/theta_slc6/theta/bin/theta $datacard
# time /afs/cern.ch/work/n/ntsirova/public/theta/theta/bin/theta $datacard
