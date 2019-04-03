/*
 * Copyright (c) 2018
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 * This is a sample pixel filter to do edge detection
 */

#include <UT/UT_DSOVersion.h>

#include "RAY_VarianceFilter.h"
#include <RAY/RAY_SpecialChannel.h>
#include <UT/UT_Args.h>
#include <UT/UT_StackBuffer.h>
#include <SYS/SYS_Floor.h>
#include <SYS/SYS_Math.h>

using namespace HDK_Sample;

RAY_PixelFilter *
allocPixelFilter(const char *name)
{
    // NOTE: We could use name to distinguish between multiple pixel filters,
    //       in the same library, but we only have one.
    return new RAY_VarianceFilter();
}

RAY_VarianceFilter::RAY_VarianceFilter()
    : mySamplesPerPixelX(1) // Initialized just in case; value shouldn't be used
    , mySamplesPerPixelY(1) // Initialized just in case; value shouldn't be used
    , myUseColourGradient(true)
    , myUseZGradient(true)
    , myUseOpID(true)
    , myColourGradientThreshold(0.1f)
    , myZGradientThreshold(0.005f)
    , myColourGradientWidth(3.0f)
    , myZGradientWidth(3.0f)
    , myOpIDWidth(3.0f)
{
}

RAY_VarianceFilter::~RAY_VarianceFilter()
{
}

RAY_PixelFilter *
RAY_VarianceFilter::clone() const
{
    // In this case, all of our members can be default-copy-constructed,
    // so we don't need to write a copy constructor implementation.
    RAY_VarianceFilter *pf = new RAY_VarianceFilter(*this);
    return pf;
}

void
RAY_VarianceFilter::setArgs(int argc, const char *const argv[])
{
    UT_Args args;
    args.initialize(argc, argv);
    args.stripOptions("c:o:s:w:z:");

    // e.g. default values correspond with:
    // -c 0.1 -w 3.0 -z 0.005 -s 3.0 -o 3.0
    // To disable any of the 3 detections, set one of the corresponding
    // parameters to a negative number, like -1

    if (args.found('c'))
    {
        myColourGradientThreshold = args.fargp('c');
        myUseColourGradient = (myColourGradientThreshold >= 0);
        if (myUseColourGradient && args.found('w'))
        {
            myColourGradientWidth = args.fargp('w');
            if (myColourGradientWidth < 0)
                myUseColourGradient = false;
            // NOTE: You could add support for widths < 1.0 by taking the max
            //       of the gradients within a pixel.
            //       The upper limit is just to avoid accidents.
            myColourGradientWidth = SYSclamp(myColourGradientWidth, 1.0f, 1024.0f);
        }
    }
    if (args.found('o'))
    {
        myOpIDWidth = args.fargp('o');
        myUseOpID = (myOpIDWidth >= 0);
        myOpIDWidth = SYSclamp(myOpIDWidth, 1.0f, 1024.0f);
    }
    if (args.found('z'))
    {
        myZGradientThreshold = args.fargp('z');
        myUseZGradient = (myZGradientThreshold >= 0);
        if (myUseZGradient && args.found('s'))
        {
            myZGradientWidth = args.fargp('s');
            if (myZGradientWidth < 0)
                myUseZGradient = false;
            // NOTE: You could add support for widths < 1.0 by taking the max
            //       of the gradients within a pixel.
            //       The upper limit is just to avoid accidents.
            myZGradientWidth = SYSclamp(myZGradientWidth, 1.0f, 1024.0f);
        }
    }
}

void
RAY_VarianceFilter::getFilterWidth(float &x, float &y) const
{
    // NOTE: You could add support for different x and y filter widths,
    //       which might be useful for non-square pixels.
    float filterwidth = SYSmax(myUseColourGradient ? myColourGradientWidth : 1,
                               myUseZGradient ? myZGradientWidth : 1,
                               myUseOpID ? myOpIDWidth : 1);
    x = filterwidth;
    y = filterwidth;
}

void
RAY_VarianceFilter::addNeededSpecialChannels(RAY_Imager &imager)
{
    if (myUseOpID)
        addSpecialChannel(imager, RAY_SPECIAL_OPID);
    if (myUseZGradient)
        addSpecialChannel(imager, RAY_SPECIAL_PZ);
}

namespace {
float RAYcomputeSumX2(int samplesperpixel,float width,int &halfsamplewidth)
{
    float sumx2 = 0;
    if (samplesperpixel & 1)
    {
        // NOTE: This omits the middle sample
        halfsamplewidth = (int)SYSfloor(float(samplesperpixel)*0.5f*width);

        // There's a close form for this sum, but I figured I'd write it out in full,
        // since it's not a bottleneck.
        for (int i = -halfsamplewidth; i <= halfsamplewidth; ++i)
        {
            float x = float(i)/float(samplesperpixel);
            sumx2 += x*x;
        }
    }
    else
    {
        halfsamplewidth = (int)SYSfloor(float(samplesperpixel)*0.5f*width + 0.5f);

        // There's a close form for this sum, but I figured I'd write it out in full,
        // since it's not a bottleneck.
        for (int i = -halfsamplewidth; i < halfsamplewidth; ++i)
        {
            float x = (float(i)+0.5f)/float(samplesperpixel);
            sumx2 += x*x;
        }
    }
    return sumx2;
}
}

void
RAY_VarianceFilter::prepFilter(int samplesperpixelx, int samplesperpixely)
{
    mySamplesPerPixelX = samplesperpixelx;
    mySamplesPerPixelY = samplesperpixely;

    // We can precompute coefficients here
    myColourSumX2 = RAYcomputeSumX2(mySamplesPerPixelX, myColourGradientWidth, myColourSamplesHalfX);
    myColourSumY2 = RAYcomputeSumX2(mySamplesPerPixelY, myColourGradientWidth, myColourSamplesHalfY);
    myZSumX2 = RAYcomputeSumX2(mySamplesPerPixelX, myZGradientWidth, myZSamplesHalfX);
    myZSumY2 = RAYcomputeSumX2(mySamplesPerPixelY, myZGradientWidth, myZSamplesHalfY);
    myOpIDSamplesHalfX = (int)SYSfloor(float(mySamplesPerPixelX)*0.5f*myOpIDWidth + ((mySamplesPerPixelX & 1) ? 0.0f : 0.5f));
    myOpIDSamplesHalfY = (int)SYSfloor(float(mySamplesPerPixelY)*0.5f*myOpIDWidth + ((mySamplesPerPixelY & 1) ? 0.0f : 0.5f));
}

void
RAY_VarianceFilter::filter(
    float *destination,
    int vectorsize,
    const RAY_SampleBuffer &source,
    int channel,
    int sourcewidth,
    int sourceheight,
    int destwidth,
    int destheight,
    int destxoffsetinsource,
    int destyoffsetinsource,
    const RAY_Imager &imager) const
{
    const float *const colourdata = getSampleData(source, channel);
//    const float *const colourdata = myUseColourGradient
//        ? getSampleData(source, channel)
//        : NULL;
//    const float *const zdata = myUseZGradient
//        ? getSampleData(source, getSpecialChannelIdx(imager, RAY_SPECIAL_PZ))
//        : NULL;
//    const float *const opiddata = myUseOpID
//        ? getSampleData(source, getSpecialChannelIdx(imager, RAY_SPECIAL_OPID))
//        : NULL;

//    UT_ASSERT(myUseColourGradient == (colourdata != NULL));
//    UT_ASSERT(myUseZGradient == (zdata != NULL));
//    UT_ASSERT(myUseOpID == (opiddata != NULL));

    // The gradient computations can be made much faster by separating x and y,
    // using a temporary buffer, but this is implemented the slow way.
    // In fact, nothing in here is optimized.

    for (int desty = 0; desty < destheight; ++desty)
    {
        for (int destx = 0; destx < destwidth; ++destx)
        {
            bool isedge = false;

            // First, compute the sample bounds of the pixel
            const int sourcefirstx = destxoffsetinsource + destx*mySamplesPerPixelX;
            const int sourcefirsty = destyoffsetinsource + desty*mySamplesPerPixelY;
            const int sourcelastx = sourcefirstx + mySamplesPerPixelX-1;
            const int sourcelasty = sourcefirsty + mySamplesPerPixelY-1;
            // Find the first sample to read for colour and z gradients
            const int sourcefirstcx = sourcefirstx + (mySamplesPerPixelX>>1) - myColourSamplesHalfX;
            const int sourcefirstcy = sourcefirsty + (mySamplesPerPixelY>>1) - myColourSamplesHalfY;
            const int sourcefirstzx = sourcefirstx + (mySamplesPerPixelX>>1) - myZSamplesHalfX;
            const int sourcefirstzy = sourcefirsty + (mySamplesPerPixelY>>1) - myZSamplesHalfY;
            const int sourcefirstox = sourcefirstx + (mySamplesPerPixelX>>1) - myOpIDSamplesHalfX;
            const int sourcefirstoy = sourcefirsty + (mySamplesPerPixelY>>1) - myOpIDSamplesHalfY;
            // Find the last sample to read for colour and z gradients
            const int sourcelastcx = sourcefirstx + ((mySamplesPerPixelX-1)>>1) + myColourSamplesHalfX;
            const int sourcelastcy = sourcefirsty + ((mySamplesPerPixelY-1)>>1) + myColourSamplesHalfY;
            const int sourcelastzx = sourcefirstx + ((mySamplesPerPixelX-1)>>1) + myZSamplesHalfX;
            const int sourcelastzy = sourcefirsty + ((mySamplesPerPixelY-1)>>1) + myZSamplesHalfY;
            const int sourcelastox = sourcefirstx + ((mySamplesPerPixelX-1)>>1) + myOpIDSamplesHalfX;
            const int sourcelastoy = sourcefirsty + ((mySamplesPerPixelY-1)>>1) + myOpIDSamplesHalfY;
            // Find the first and last that will be read
            int sourcefirstrx = sourcefirstx;
            int sourcefirstry = sourcefirsty;
            int sourcelastrx = sourcelastx;
            int sourcelastry = sourcelasty;
//            if (myUseColourGradient)
//            {
            sourcefirstrx = SYSmin(sourcefirstrx, sourcefirstcx);
            sourcefirstry = SYSmin(sourcefirstry, sourcefirstcy);
            sourcelastrx = SYSmax(sourcelastrx, sourcelastcx);
            sourcelastry = SYSmax(sourcelastry, sourcelastcy);
//            }
//            if (myUseZGradient)
//            {
//                sourcefirstrx = SYSmin(sourcefirstrx, sourcefirstzx);
//                sourcefirstry = SYSmin(sourcefirstry, sourcefirstzy);
//                sourcelastrx = SYSmax(sourcelastrx, sourcelastzx);
//                sourcelastry = SYSmax(sourcelastry, sourcelastzy);
//            }
//            if (myUseOpID)
//            {
//                sourcefirstrx = SYSmin(sourcefirstrx, sourcefirstox);
//                sourcefirstry = SYSmin(sourcefirstry, sourcefirstoy);
//                sourcelastrx = SYSmax(sourcelastrx, sourcelastox);
//                sourcelastry = SYSmax(sourcelastry, sourcelastoy);
//            }

            UT_StackBuffer<float> minRGB(vectorsize);
            UT_StackBuffer<float> maxRGB(vectorsize);
//            UT_StackBuffer<float> varianceRGB(vectorsize);

            for (int i = 0; i < vectorsize; ++i) {
                minRGB[i] =  1000.0f;
                maxRGB[i] = -1000.0f;
//                varianceRGB[i] = 0;
            }

            for (int sourcey = sourcefirstry; sourcey <= sourcelastry && !isedge; ++sourcey)
            {
                for (int sourcex = sourcefirstrx; sourcex <= sourcelastrx; ++sourcex)
                {
                    int sourcei = sourcex + sourcewidth*sourcey;

                    // Find (x,y) of sample relative to *middle* of pixel
//                    float x = (float(sourcex) - 0.5f*float(sourcelastx + sourcefirstx))/float(mySamplesPerPixelX);
//                    float y = (float(sourcey) - 0.5f*float(sourcelasty + sourcefirsty))/float(mySamplesPerPixelY);

                    if (sourcex >= sourcefirstcx && sourcex <= sourcelastcx && sourcey >= sourcefirstcy && sourcey <= sourcelastcy)
                    {
                        for (int i = 0; i < vectorsize; ++i) {
                            minRGB[i] = SYSmin(minRGB[i], colourdata[vectorsize*sourcei + i]);
                            maxRGB[i] = SYSmax(maxRGB[i], colourdata[vectorsize*sourcei + i]);
                        }
                    }
                }
            }



//            if (!isedge)
//            {
//                if (myUseColourGradient)
//                {
//                    int nx = sourcelastcx-sourcefirstcx+1;
//                    int ny = sourcelastcy-sourcefirstcy+1;
//                    for (int i = 0; i < vectorsize; ++i)
//                        varianceRGB[i] /= (ny*myColourSumX2);
//                    float mag2x = 0;
//                    for (int i = 0; i < vectorsize; ++i)
//                        mag2x += varianceRGB[i]*varianceRGB[i];

//                    for (int i = 0; i < vectorsize; ++i)
//                        colourgradienty[i] /= (nx*myColourSumY2);
//                    float mag2y = 0;
//                    for (int i = 0; i < vectorsize; ++i)
//                        mag2y += colourgradienty[i]*colourgradienty[i];

//                    if ((mag2x + mag2y) >= myColourGradientThreshold*myColourGradientThreshold)
//                        isedge = true;
//                }
//                if (!isedge && myUseZGradient && hasnonfarz)
//                {
//                    int nx = sourcelastzx-sourcefirstzx+1;
//                    int ny = sourcelastzy-sourcefirstzy+1;
//                    zaverage /= float(nx)*float(ny);
//                    zgradientx /= (ny*myZSumX2*zaverage);
//                    zgradienty /= (nx*myZSumY2*zaverage);
//                    float mag2x = zgradientx*zgradientx;
//                    float mag2y = zgradienty*zgradienty;

//                    if ((mag2x + mag2y) >= myZGradientThreshold*myZGradientThreshold)
//                        isedge = true;
//                }
//            }

//            float value = isedge ? 1.0f : 0.0f;
            for (int i = 0; i < vectorsize; ++i, ++destination)
                *destination = maxRGB[i]-minRGB[i];
        }
    }
}
