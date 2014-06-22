//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

struct VS_OUTPUT
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD0;
};

uniform float4 halfPixelSize : c0;

// Standard Vertex Shader
// Input 0 is the homogenous position.
// Outputs the homogenous position as-is.
// Outputs a tex coord with (0,0) in the upper-left corner of the screen and (1,1) in the bottom right.
// C0.X must be negative half-pixel width, C0.Y must be half-pixel height. C0.ZW must be 0.
VS_OUTPUT standardvs(in float4 position : POSITION)
{
    VS_OUTPUT Out;

    Out.position = position + halfPixelSize;
    Out.texcoord = position * float4(0.5, -0.5, 1.0, 1.0) + float4(0.5, 0.5, 0, 0);

    return Out;
};

// Flip Y Vertex Shader
// Input 0 is the homogenous position.
// Outputs the homogenous position as-is.
// Outputs a tex coord with (0,1) in the upper-left corner of the screen and (1,0) in the bottom right.
// C0.XY must be the half-pixel width and height. C0.ZW must be 0.
VS_OUTPUT flipyvs(in float4 position : POSITION)
{
    VS_OUTPUT Out;

    Out.position = position + halfPixelSize;
    Out.texcoord = position * float4(0.5, 0.5, 1.0, 1.0) + float4(0.5, 0.5, 0, 0);

    return Out;
};
