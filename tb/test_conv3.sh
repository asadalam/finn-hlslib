#!/bin/bash
# Copyright © 2020 Syed Asad Alam. All rights reserved.
# Running the test_conv3.tcl using the following steps:
# 1) Remove old project directory
# 2) Generated weights
# 3) Run vivado_hls simulation
# 4) Copy the dumped files to RTL simulation folder
# 5) For weight files, only copy the first ${1} lines which equates (kernel_dim*kernel_dim*ifm_channels*ofm_channels)/(pe*simd)

WORKING_DIR=hls-syn-conv
if [ -d ${WORKING_DIR} ]; then
    echo "Removing project folder";
    rm -Rf ${WORKING_DIR};
else
    echo "Project folder does not exist";
fi
echo "Generating weights"
python gen_weigths.py
echo "Running HLS simulation"
vivado_hls test_conv3.tcl
echo "Copying dumped data"
cp hls-syn-conv/sol1/csim/build/{inp_act.memh,out_act.memh} ../../proj/sim/
exit 1
