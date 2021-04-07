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
 *****************************************************************************/

/******************************************************************************
 *
 *  Authors: Giulio Gambardella <giuliog@xilinx.com>
 *           Thomas B. Preusser <thomas.preusser@utexas.edu>
 *             Marie-Curie Fellow, Xilinx Ireland, Grant Agreement No. 751339
 *           Christoph Doehring <cdoehrin@xilinx.com>
 *
 *  \file convlayer.h
 *
 *  Library of templated HLS functions for BNN deployment.
 *  This file lists a set of convenience functions used to implement
 *  convolutional layers.
 *
 *****************************************************************************/

#ifndef CONVLAYER_H
#define CONVLAYER_H

#include <ap_int.h>
#include <hls_stream.h>

#include "streamtools.h"
#include "mvau.hpp"
/**
 * \brief 	Convolutional layer implementation
 *
 * The function implements a generic convolutional layer, and it's basically composed of the sliding window generator
 * implemeting the im2col algorithm and the Matrix_Vector_Activate_Batch function to perform computation.
 * 
 * \tparam ConvKernelDim 	Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels 		Number of Input Feature Maps
 * \tparam IFMDim 			Width and Height of the Input Feature Map (assumed square)
 * \tparam OFMChannels 		Number of Output Feature Maps
 * \tparam OFMDim 			Width and Height of the Output Feature Map (assumed square)
 * \tparam SIMD 			Number of input columns computed in parallel
 * \tparam PE 				Number of output rows computed in parallel
 * \tparam TSrcI 			DataType of the input activation (as used in the MAC)
 * \tparam TDstI 			DataType of the output activation (as generated by the activation)
 * \tparam TWeightI 		DataType of the weights (as used in the MAC)
 * \tparam InStreamW 		Width of the input stream
 * \tparam OutStreamW 		Width of the output stream
 * \tparam TW 				DataType of the weights matrix - safely deducible from the paramaters
 * \tparam TA 				DataType of the activation class (e.g. thresholds) - safely deducible from the paramaters
 * \tparam R 				DataType for the resource used for FPGA implementation of the MAC  - safely deducible from the paramaters
 *
 * \param in 				Input stream
 * \param out 				Output stream
 * \param weights 			Weights matrix (currently supports BinaryWeights or FixedPointWeights)
 * \param activation 		Activation class
 * \param reps 				Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r 				Resource type for the hardware implementation of the MAC block
 */

template<
		unsigned int ConvKernelDim,		
		unsigned int IFMChannels,		
		unsigned int IFMDim,			
		unsigned int OFMChannels,		
		unsigned int OFMDim,			
		
		unsigned int SIMD, 				// number of SIMD lanes
		unsigned int PE,				// number of PEs
		
		typename TSrcI = Identity,      // redefine I/O interpretation as needed for input activations
		typename TDstI = Identity,		// redefine I/O interpretation as needed for output activations
		typename TWeightI = Identity,	// redefine I/O interpretation as needed for weigths

		int InStreamW, int OutStreamW,  // safely deducible (stream width must be int though!)
		typename TW,   typename TA,  typename R
>
void ConvLayer_Batch(hls::stream<ap_uint<InStreamW>>  &in,
			    hls::stream<ap_uint<OutStreamW>> &out,
			    TW const        &weights,
			    TA const        &activation,
			    unsigned const   reps,
				R const &r) {
#pragma HLS INLINE
  unsigned const MatrixW = ConvKernelDim * ConvKernelDim * IFMChannels;
  unsigned const MatrixH = OFMChannels;
  unsigned const InpPerImage = IFMDim*IFMDim*IFMChannels*TSrcI::width/InStreamW;
  WidthAdjustedInputStream <InStreamW, SIMD*TSrcI::width, InpPerImage>  wa_in (in,  reps);
  WidthAdjustedOutputStream <PE*TDstI::width, OutStreamW, OFMDim * OFMDim * (OFMChannels / PE)>  mvOut (out,  reps);
  hls::stream<ap_uint<SIMD*TSrcI::width> > convInp("StreamingConvLayer_Batch.convInp");
  ConvolutionInputGenerator<ConvKernelDim, IFMChannels, TSrcI::width, IFMDim,
			OFMDim, SIMD,1>(wa_in, convInp, reps, ap_resource_dflt());
  // Logging input activation matrix
  // logStringStream<SIMD*TSrcI::width>("inp_act.memh",convInp);
  Matrix_Vector_Activate_Batch<MatrixW, MatrixH, SIMD, PE, 1, TSrcI, TDstI, TWeightI>
    (static_cast<hls::stream<ap_uint<SIMD*TSrcI::width>>&>(convInp),
     static_cast<hls::stream<ap_uint<PE*TDstI::width>>&>  (mvOut),
     weights, activation, reps* OFMDim * OFMDim, r);
  // Logging output activation matrix
  // logStringStream<PE*TDstI::width>("out_act.memh",mvOut);
}


/**
 * \brief 	Convolutional layer implementation
 *
 * The function implements a generic convolutional layer, and it's basically composed of the sliding window generator
 * implemeting the im2col algorithm and the Matrix_Vector_Activate_Batch function to perform computation.
 *
 * \tparam ConvKernelDim 	Dimension of the convolutional kernel (assumed square)
 * \tparam IFMChannels 		Number of Input Feature Maps
 * \tparam IFMDim 			Width and Height of the Input Feature Map (assumed square)
 * \tparam OFMChannels 		Number of Output Feature Maps
 * \tparam OFMDim 			Width and Height of the Output Feature Map (assumed square)
 * \tparam STRIDE 			Stride of the convolutional kernel
 *
 * \tparam SIMD 			Number of input columns computed in parallel
 * \tparam PE 				Number of output rows computed in parallel
 * \tparam MMV 				Number of output pixels computed in parallel
 *
 * \tparam TSrcI 			DataType of the input activation (as used in the MAC)
 * \tparam TDstI 			DataType of the output activation (as generated by the activation)
 * \tparam TWeightI 		DataType of the weights (as used in the MAC)
 * \tparam InStreamW 		Width of the input stream
 * \tparam OutStreamW 		Width of the output stream
 * \tparam TW 				DataType of the weights matrix - safely deducible from the paramaters
 * \tparam TA 				DataType of the activation class (e.g. thresholds) - safely deducible from the paramaters
 * \tparam R 				DataType for the resource used for FPGA implementation of the MAC  - safely deducible from the paramaters
 *
 * \param in 				Input stream
 * \param out 				Output stream
 * \param weights 			Weights matrix (currently supports BinaryWeights or FixedPointWeights)
 * \param activation 		Activation class
 * \param reps 				Number of time the function has to be repeatedly executed (e.g. number of images)
 * \param r 				Resource type for the hardware implementation of the MAC block
 */
template<
		unsigned int ConvKernelDim,
		unsigned int IFMChannels,
		unsigned int IFMDim,
		unsigned int OFMChannels,
		unsigned int OFMDim,
		unsigned int STRIDE,

		unsigned int SIMD,				// number of SIMD lanes
		unsigned int PE,				// number of PEs
		unsigned int MMV,

		typename TSrcI = Identity,      // redefine I/O interpretation as needed for input activations
		typename TDstI = Identity,		// redefine I/O interpretation as needed for output activations
		typename TWeightI = Identity,	// redefine I/O interpretation as needed for weigths

		int InStreamW, int OutStreamW,  // safely deducible (stream width must be int though!)
		typename TW,   typename TA,  typename R
>
void ConvLayer_Batch_MMV(hls::stream<ap_uint<InStreamW>>  &in,
			    hls::stream<ap_uint<OutStreamW>> &out,
			    TW const        &weights,
			    TA const        &activation,
			    unsigned const   reps,
				R const &r) {
#pragma HLS INLINE
  unsigned const MatrixW = ConvKernelDim * ConvKernelDim * IFMChannels;
  unsigned const MatrixH = OFMChannels;
  unsigned const InpPerImage = IFMDim*IFMDim*IFMChannels*TSrcI::width/InStreamW;
  const unsigned int mmvReps = (reps * OFMDim * OFMDim) / MMV;
  WidthAdjustedInputStream <InStreamW, SIMD*TSrcI::width, InpPerImage>  wa_in (in,  reps);
  WidthAdjustedOutputStream <OFMChannels*TDstI::width*MMV, OutStreamW, OFMDim * OFMDim * (OFMChannels / PE)/MMV>  mvOut (out,  reps);
  stream<MultiChanData<MMV, PE * TDstI::width> > mmv2dwc("mmv2dwc");
  stream<MultiChanData<MMV, OFMChannels * TDstI::width> > dwc2flat("dwc2flat");
  hls::stream<MultiChanData<MMV, SIMD *TSrcI::width> > convInp("StreamingConvLayer_Batch.convInp");
  ConvolutionInputGenerator_MMV<ConvKernelDim, IFMChannels, TSrcI::width, IFMDim,
			OFMDim, SIMD, STRIDE, MMV>(wa_in, convInp, reps, ap_resource_dflt());
  Matrix_Vector_Activate_Batch<MatrixW, MatrixH, SIMD, PE, MMV, TSrcI, TDstI, TWeightI>
    (static_cast<hls::stream<MultiChanData<MMV,SIMD*TSrcI::width>>&>(convInp),
     static_cast<hls::stream<MultiChanData<MMV,PE*TDstI::width>>&>(mmv2dwc),
     weights, activation, mmvReps, r);
  MultiChanDataWidthConverter_Batch<PE * TDstI::width, OFMChannels * TDstI::width,  OFMDim * OFMDim * (OFMChannels / PE), MMV>(mmv2dwc, dwc2flat, reps);
  FlattenMultiChanData<MMV, OFMChannels * TDstI::width>(dwc2flat, mvOut, mmvReps);

}

#endif
