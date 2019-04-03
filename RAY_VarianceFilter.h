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

#pragma once

#ifndef __RAY_VarianceFilter__
#define __RAY_VarianceFilter__

#include <RAY/RAY_PixelFilter.h>

namespace HDK_Sample {

class RAY_VarianceFilter : public RAY_PixelFilter {
public:
    RAY_VarianceFilter();
    virtual ~RAY_VarianceFilter();

    virtual RAY_PixelFilter *clone() const;

    /// setArgs is called with the options specified after the pixel filter
    /// name in the Pixel Filter parameter on the Mantra ROP.
    /// This filter accepts 5 options:
    /// -c 0.1      Consider a colour gradient of 0.1 colour units / pixel
    ///             to be an edge.  Make -1 to disable colour gradient check.
    /// -w 3.0      Make the width of the region to fit lines to for the
    ///             colour gradient 3.0 pixels, i.e. each pixel may depend on
    ///             samples 1.5 pixels from its centre.  It gets clamped to
    ///             a minimum width of 1.0.
    /// -z 0.005    Consider a z-depth gradient of a factor of 0.005 change
    ///             in the z-depth per pixel to be an edge.  For example,
    ///             a gradient of 0.51 distance units per pixel at a depth
    ///             of 100 units would be considered an edge.  Make -1 to
    ///             disable z-depth gradient check.
    /// -s 3.0      Make the width of the region to fit lines to for the
    ///             z-depth gradient 3.0 pixels, i.e. each pixel may depend on
    ///             samples 1.5 pixels from its centre.  It gets clamped to
    ///             a minimum width of 1.0.
    /// -o 3.0      Make the width of the region to search for varying Op IDs
    ///             3.0 pixels, i.e. each pixel may depend on
    ///             samples 1.5 pixels from its centre.  It gets clamped to
    ///             a minimum width of 1.0.  Make -1 to disable Op ID check.
    virtual void setArgs(int argc, const char *const argv[]);

    /// getFilterWidth is called after setArgs when Mantra needs to know
    /// how far to expand the render region.
    virtual void getFilterWidth(float &x, float &y) const;

    /// addNeededSpecialChannels is called after setArgs so that this filter
    /// can indicate that it depends on having special channels like z-depths
    /// or Op IDs.
    virtual void addNeededSpecialChannels(RAY_Imager &imager);

    /// prepFilter is called after setArgs so that this filter can
    /// precompute data structures or values for use in filtering that
    /// depend on the number of samples per pixel in the x or y directions.
    virtual void prepFilter(int samplesperpixelx, int samplesperpixely);

    /// filter is called for each destination tile region with a source
    /// that is at least as large as is needed by this filter, based on
    /// the filter widths returned by getFilterWidth.
    virtual void filter(
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
        const RAY_Imager &imager) const;

private:
    /// These must be saved in prepFilter.
    /// Each pixel has mySamplesPerPixelX*mySamplesPerPixelY samples.
    /// @{
    int mySamplesPerPixelX;
    int mySamplesPerPixelY;
    /// @}

    /// true iff detecting edges using the magnitude of the colour gradient
    bool myUseColourGradient;

    /// true iff detecting edges using the magnitude of the z-depth gradient
    bool myUseZGradient;

    /// true iff detecting edges using changes in the Operator ID
    bool myUseOpID;

    /// Min magnitude of the colour gradient that will be considered an edge
    /// Units are: colour units / pixel
    float myColourGradientThreshold;

    /// Min magnitude of the z-depth gradient that will be considered an edge
    /// Units are: distance units / pixel
    float myZGradientThreshold;

    /// Width in pixels of filter to determine colour gradient
    float myColourGradientWidth;

    /// Width in pixels of filter to determine z-depth gradient
    float myZGradientWidth;

    /// Width in pixels of filter to check for different Operator IDs
    float myOpIDWidth;

    /// Normalizing coefficients computed in prepFilter
    /// @{
    float myColourSumX2;
    float myColourSumY2;
    float myZSumX2;
    float myZSumY2;
    /// @}

    /// Filter half-widths (rounded down) in sample counts
    /// @{
    int myColourSamplesHalfX;
    int myColourSamplesHalfY;
    int myZSamplesHalfX;
    int myZSamplesHalfY;
    int myOpIDSamplesHalfX;
    int myOpIDSamplesHalfY;
    /// @}
};

} // End HDK_Sample namespace

#endif
