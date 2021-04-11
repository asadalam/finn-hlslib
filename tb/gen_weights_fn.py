#   Copyright (c) 2019, Xilinx, Inc.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in the 
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
#   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  

import numpy as np
import pandas as pd
import subprocess
import sys
import argparse
import os


def gen_weights(kdim,iwl,ifmc,ofmc,ifmd,owl,wwl,simd,pe,stride=1,mmv=1):
    outFileWeights = open("memdata.h" , "wt")
    outFileConfig = open("config.h" , "wt")
    outFileWeightsFull = open("../../sim/weights_mem_full.memh","wt")
    outFileWeightsFullSrc_Sim = open("../../sim/weights_mem_full_src.mem","wt")
    outFileWeightsFullSrc = open("../../src/mvau_top/weights_mem_full_src.mem","wt")

    kernel_dim = kdim
    input_precision = iwl
    ifm_channels = ifmc
    ofm_channels = ofmc
    ifm_dimension = ifmd
    ofm_dimension = (ifm_dimension-kernel_dim)/stride+1
    
    activation_precision = owl
    expand = 1
    w_precision = wwl
    mmv=1
    wgt_sign = 0
    inp_sign = 0
    out_sign = 0#wgt_sign or inp_sign
    
    tile = ifm_channels *kernel_dim*kernel_dim * ofm_channels // (simd*pe)
    
    outFileConfig.write("#define KERNEL_DIM %d \n" % kernel_dim)
    outFileConfig.write("#define SIMD1 %d \n" % simd)
    outFileConfig.write("#define PE1 %d \n" % pe)
    outFileConfig.write("#define MMV1 %d \n" % mmv)
    outFileConfig.write("#define WIDTH %d \n" % w_precision)
    
    outFileConfig.write("#define IFM_Channels1 %d \n" % ifm_channels)
    outFileConfig.write("#define OFM_Channels1 %d \n" % ofm_channels)
    outFileConfig.write("#define IFMDim1 %d \n" % ifm_dimension)
    outFileConfig.write("#define OFMDim1 %d \n" % ofm_dimension)
    outFileConfig.write("#define STRIDE %d \n" % stride)
    outFileConfig.write("#define INPUT_PRECISION %d \n" % input_precision)
    outFileConfig.write("#define TILE1 %d \n" % tile)
    #outFileConfig.write("#define WGT_SIGN %d \n" % wgt_sign)
    
    outFileConfig.write("#define ACTIVATION_PRECISION %d \n" % activation_precision)
    if(wgt_sign == 1):
        outFileConfig.write("typedef ap_int<

    outFileConfig.close()
    outFileWeights.write("#ifndef PARAMS_HPP\n")
    outFileWeights.write("#define PARAMS_HPP\n")

    if (w_precision == 1):
        outFileWeights.write("static BinaryWeights<%d,%d,%d> weights= {\n{\n" %(simd,pe,tile))
    elif (wgt_sign == 1):
        outFileWeights.write("static FixedPointWeights<%d,ap_int<%d>,%d,%d> weights= {\n{\n" %(simd,w_precision,pe,tile))
    elif (wgt_sign == 0):
        outFileWeights.write("static FixedPointWeights<%d,ap_uint<%d>,%d,%d> weights= {\n{\n" %(simd,w_precision,pe,tile))

    weights_dict = dict()
    for p in range(pe):
	weights_list = []
	dict_key = p
	fname_src = "../../proj/src/mvau_top/weight_mem"+str(p)+".memh"
	fname_sim = "../../proj/sim/weight_mem"+str(p)+".memh"
	outFileWeightsHexSrc = open(fname_src,"wt")
	outFileWeightsHexSim = open(fname_sim,"wt")
	outFileWeights.write("{ \n")
	for t in range(tile):
	    width = simd*w_precision;
	    val = random.randint(0, 1<<width-1)
	    outFileWeights.write("%s" % hex(val))
	    formatted_string = format(val,"x")
	    outFileWeightsHexSrc.write("%s" % formatted_string)
	    outFileWeightsHexSim.write("%s" % formatted_string)
	    weights_list.append(formatted_string)
	    if t!=tile-1:
		outFileWeights.write(",\n")
		outFileWeightsHexSrc.write("\n")
		outFileWeightsHexSim.write("\n")
	outFileWeights.write("} \n")
	if p!=pe-1:
	    outFileWeights.write(",")
	weights_dict[dict_key] = weights_list
	outFileWeightsHexSrc.close()
	outFileWeightsHexSim.close()

    outFileWeights.write("}\n};\n } \n")
    outFileWeights.write("#endif \n")
    outFileWeights.close()

    nf = ofm_channels/pe
    matrix_w = kernel_dim*kernel_dim*ifm_channels // (simd)

    for n in np.arange(nf):
        for p in weights_dict:
	    r = int(len(weights_dict[p])/nf)
	    for wt in np.arange(r):
	        k = int(wt+matrix_w*n)
	        outFileWeightsFull.write("%s\n" % weights_dict[p][k])

    outFileWeightsFull.close()

    return 0


def parser():
    parser = argparse.ArgumentParser(description='Python data script for Toom Cook 1D convolution using Chebyshev nodes')
    parser.add_argument('-k','--kdim',default=,type=int,
			help="2")
    parser.add_argument('-i','--inp_wl',default=,type=int,
			help="8")
    parser.add_argument('--ifm_ch', default=1.,type=int,
			help="4")
    parser.add_argument('--ofm_ch', default=2., type=int,
			help="4")
    parser.add_argument('--ifm_dim', default=1, type=int,
			help="4")
    parser.add_argument('-o','--out_wl', default=3, type=int,
			help="16")
    parser.add_argument('-s','--simd',default=5000,type=int,
			help="2")
    parser.add_argument('-p', '--pe', default=0,type=int,
			help="2")
    parser.add_argument('-w','--wgt_wl',default=,type=int,
                        help="1")
    return parser



if __name__ == "__main__":

    ## Reading the argument list passed to this script
    args = parser().parse_args()

    ## Calling the weight generation function
    gen_weights(args.kdim,args.inp_wl,args.ifm_ch,args.ofm_ch,args.ifm_dim,args.out_wl,args.simd,args.pe,args.wgt_wl)

    sys.exit(0)


    
