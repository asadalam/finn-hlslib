/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
/******************************************************************************
 *
 *  Authors: Syed Asad Alam <syed.asad.alam@tcd.ie>
 *           Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file mvau_tb_std.cpp
 *
 *  Testbench for the Matrix Vector Activation Batch Unit HLS block
 *
 *****************************************************************************/
#include <iostream>
#include <fstream>
#include <time.h>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#define AP_INT_MAX_W 16384
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"
#include "memdata.h"
#include "config.h"
#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"
#include "utils.hpp"
#include "dma.h"

using namespace hls;
using namespace std;

#define MAX_IMAGES 1
void Testbench_mvau(stream<ap_uint<IFM_Channels1*INPUT_PRECISION> > & in, stream<ap_uint<OFM_Channels1*ACTIVATION_PRECISION> > & out, unsigned int numReps);

int main()
{
  static	ap_uint<INPUT_PRECISION> IMAGE[MAX_IMAGES][IFMDim1*IFMDim1][IFM_Channels1];
  static	ap_uint<ACTIVATION_PRECISION> TEST[MAX_IMAGES][OFMDim1][OFMDim1][OFM_Channels1];
  stream<ap_uint<IFM_Channels1*INPUT_PRECISION> > input_stream("input_stream");
  stream<ap_uint<IFM_Channels1*INPUT_PRECISION> > input_stream_dump("input_stream_dump");
  stream<ap_uint<OFM_Channels1*ACTIVATION_PRECISION> > output_stream("output_stream");
  unsigned int counter = 0;
  for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
    for (unsigned int oy = 0; oy < IFMDim1; oy++) {
      for (unsigned int ox = 0; ox < IFMDim1; ox++) {
	ap_uint<INPUT_PRECISION*IFM_Channels1> input_channel = 0;
	for(unsigned int channel = 0; channel < IFM_Channels1; channel++)
	  {
	    ap_uint<INPUT_PRECISION> input = (ap_uint<INPUT_PRECISION>)(counter);
	    IMAGE[n_image][oy*IFMDim1+ox][channel]= input;
	    input_channel = input_channel >> INPUT_PRECISION;
	    input_channel(IFM_Channels1*INPUT_PRECISION-1,(IFM_Channels1-1)*INPUT_PRECISION)=input;
	    // Logging input data to a file
	    // InpAct_File << hex << (unsigned long long)input << "\n";
	    counter++;
	  }
	input_stream.write(input_channel);
	input_stream_dump.write(input_channel);
      }
    }
  }

  /*****************************************************************************************/
  // Lowering the input activation matrix for dumping data to be used by the MVAU Stream RTL
  hls::stream<ap_uint<SIMD1*INPUT_PRECISION> > convInp_temp;
  unsigned const InpPerImage = IFMDim1*IFMDim1;
  WidthAdjustedInputStream <IFM_Channels1*INPUT_PRECISION, SIMD1*INPUT_PRECISION, InpPerImage>  wa_in_temp (input_stream_dump,  MAX_IMAGES);
  ConvolutionInputGenerator<KERNEL_DIM, IFM_Channels1, INPUT_PRECISION, IFMDim1, OFMDim1, SIMD1, 1>(wa_in_temp, convInp_temp, MAX_IMAGES, ap_resource_dflt());
  logStringStream<SIMD1*INPUT_PRECISION>("inp_act.memh",convInp_temp);

  /***************************************/
  // Dumping the weights to file
  hls::stream<ap_uint<SIMD1*PE1*WIDTH> > paramStreamOutTemp;
  GenParamStream<TILE1, SIMD1, PE1, WIDTH>(PARAM::weights, paramStreamOutTemp, MAX_IMAGES * OFMDim1 * OFMDim1);
  logStringStream<SIMD1*PE1*WIDTH>("inp_wgt.memh",paramStreamOutTemp);
  /**************************************/
  
  static	ap_uint<WIDTH> W1[OFM_Channels1][KERNEL_DIM][KERNEL_DIM][IFM_Channels1];
  // initialize the weights
  constexpr int TX = (IFM_Channels1*KERNEL_DIM*KERNEL_DIM) / SIMD1;
  constexpr int TY = OFM_Channels1 / PE1;
  unsigned int kx=0;
  unsigned int ky=0;
  unsigned int chan_count=0;
  unsigned int out_chan_count=0;

  for (unsigned int oy = 0; oy < TY; oy++) {
    for(unsigned int pe=0;pe <PE1;pe++){
      for (unsigned int ox = 0; ox <TX; ox++) {
	for(unsigned int simd=0;simd<SIMD1;simd++){
	  W1[out_chan_count][kx][ky][chan_count] = PARAM::weights.weights(oy*TX + ox)[pe][simd];
	  //cout << "TILE " << oy*TX + ox << " PE " << pe << " SIMD " << simd << endl;
	  //cout << "IFM " << chan_count << " KX " << kx << " KY " << ky << " OFM " << out_chan_count << endl;
	  chan_count++;
	  if (chan_count==IFM_Channels1){
	    chan_count=0;
	    kx++;
	    if (kx==KERNEL_DIM){
	      kx=0;
	      ky++;
	      if (ky==KERNEL_DIM){
		ky=0;
		out_chan_count++;
		if (out_chan_count==OFM_Channels1){
		  out_chan_count=0;
		}
	      }
	    }
	  }
	}
      }
    }
  }

  // Preparing the inputs for the test bench
  unsigned const MatrixW = KERNEL_DIM * KERNEL_DIM * IFM_Channels1;
  unsigned const MatrixH = OFM_Channels1;
  unsigned const InpPerImage = IFMDim1*IFMDim1;

  hls::stream<ap_uint<SIMD1*INPUT_PRECISION> > convInp;
  hls::stream<ap_uint<SIMD1*PE1*WIDTH> > paramStreamOut;

  GenParamStream<TILE1, SIMD1, PE1, WIDTH>(PARAM::weights, paramStreamOut, MAX_IMAGES * OFMDim1 * OFMDim1);

  WidthAdjustedInputStream <IFM_Channels1*INPUT_PRECISION, SIMD1*INPUT_PRECISION, InpPerImage>  wa_in (input_stream,  numReps);
  WidthAdjustedOutputStream <PE1*ACTIVATION_PRECISION, OFM_Channels1*ACTIVATION_PRECISION, OFMDim1 * OFMDim1 * (OFM_Channels1 / PE1)>  mvOut (out,  MAX_IMAGES);
  
  ConvolutionInputGenerator<KERNEL_DIM, IFM_Channels1, INPUT_PRECISION, IFMDim1, OFMDim1, SIMD1, 1>(wa_in, convInp, numReps, ap_resource_dflt());

  // Performing Behavioral Convolution
  conv<MAX_IMAGES,IFMDim1,OFMDim1,IFM_Channels1,OFM_Channels1, KERNEL_DIM, 1, ap_uint<INPUT_PRECISION> >(IMAGE, W1, TEST);

  // Calling the HLS test bench
  Testbench_mvau(convInp, output_stream, MAX_IMAGES);
  // File initialization for dumping output activation
  std::ofstream OutAct_File;
  string out_act_fname = "out_act.memh";
  OutAct_File.open(out_act_fname.c_str());
  int err_counter = 0, err_perimage=0;
  ap_uint<ACTIVATION_PRECISION> out_chan;
  for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
    for (unsigned int oy = 0; oy < OFMDim1; oy++) {
      for (unsigned int ox = 0; ox < OFMDim1; ox++) {
	for(int e=0;e<1;e++){
	  ap_uint<OFM_Channels1*ACTIVATION_PRECISION> outElem = output_stream.read();
	  for(unsigned int channel = 0; channel < OFM_Channels1; channel++){
	    ap_uint<ACTIVATION_PRECISION> EXP = TEST[n_image][ox][oy][channel + e * OFM_Channels1];
	    out_chan(ACTIVATION_PRECISION-1,0) = outElem((channel + 1)*ACTIVATION_PRECISION-1,channel*ACTIVATION_PRECISION);

	    if (EXP != out_chan){
	      std::cout << "ERROR: Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;
	      err_counter ++;
	      err_perimage++;
	    }else{
	      std::cout << "Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;
	      // Logging HLS output to file
	      OutAct_File << hex << (unsigned long long)out_chan << "\n";
	    }
	  }
	}
      }
    }
    if(err_perimage == 0){
      std::cout << "Image # " << n_image << " passed the testing."<< std::endl;
    }
    else{
      err_perimage=0;
      std::cout << "Image # " << n_image << " failed the testing."<< std::endl;
    }
  }
  if(err_counter == 0){
    return 0;
  }
  else{
    return 1;
  }

}


