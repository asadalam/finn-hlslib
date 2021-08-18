#!/bin/bash
# Copyright © 2020 Syed Asad Alam. All rights reserved.
# Running the test_mvau_std.tcl using the following steps:
# 1) Remove old project directory
# 2) Generated weights
# 3) Run vivado_hls simulation
# 4) Copy the dumped files to RTL simulation folder
# 5) For weight files, only copy the first ${1} lines which equates (kernel_dim*kernel_dim*ifm_channels*ofm_channels)/(pe*simd)

WORKING_DIR=hls-syn-mvau-batch3-std
if [ -d ${WORKING_DIR} ]; then
    echo "Removing project folder";
    rm -Rf ${WORKING_DIR};
else
    echo "Project folder does not exist";
fi
echo "Generating weights"
ifm_ch=${1:-4}
ifm_dim=${2:-4}
ofm_ch=${3:-4}
kdim=${4:-2}
inp_wl=${5:-8}
inp_sgn=${6:-0}
wgt_wl=${7:-1}
wgt_sgn=${8:-0}
out_wl=${9:-16}
simd=${10:-2}
pe=${11:-2}
python gen_weights_fn.py --ifm_ch ${ifm_ch} --ifm_dim ${ifm_dim} --ofm_ch ${ofm_ch} --kdim ${kdim} --inp_wl ${inp_wl} --inp_sgn ${inp_sgn} --wgt_wl ${wgt_wl} --wgt_sgn ${wgt_sgn} --out_wl ${out_wl} --simd ${simd} --pe ${pe}
if [ $? -eq 0 ]; then
    echo "Weight generation successfull"
else
    echo "Weight generation failed"
    exit 0
fi
### temp copying of weights
cp params_batch3.h memdata.h

echo "Running HLS simulation"
vivado_hls -f test_mvau_std.tcl
if [ $? -eq 0 ]; then
    echo "HLS run successfull"
else
    echo "HLS run failed"
    exit 0
fi
echo "Copying dumped data"
cp ${WORKING_DIR}/sol1/csim/build/{inp_act.mem,out_act.mem} ${MVAU_RTL_ROOT}/proj/sim/
if [ $? -eq 0 ]; then
    echo "Data successfully copied"
else
    echo "Data not copied"
    exit 0
fi
exit 1
