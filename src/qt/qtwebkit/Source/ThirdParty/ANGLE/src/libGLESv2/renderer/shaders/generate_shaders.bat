@ECHO OFF
REM
REM Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
REM Use of this source code is governed by a BSD-style license that can be
REM found in the LICENSE file.
REM

PATH %PATH%;%DXSDK_DIR%\Utilities\bin\x86

fxc /E standardvs /T vs_2_0 /Fh compiled/standardvs.h Blit.vs
fxc /E flipyvs /T vs_2_0 /Fh compiled/flipyvs.h Blit.vs
fxc /E passthroughps /T ps_2_0 /Fh compiled/passthroughps.h Blit.ps
fxc /E luminanceps /T ps_2_0 /Fh compiled/luminanceps.h Blit.ps
fxc /E componentmaskps /T ps_2_0 /Fh compiled/componentmaskps.h Blit.ps

fxc /E VS_Passthrough /T vs_4_0 /Fh compiled/passthrough11vs.h Passthrough11.hlsl
fxc /E PS_PassthroughRGBA /T ps_4_0 /Fh compiled/passthroughrgba11ps.h Passthrough11.hlsl
fxc /E PS_PassthroughRGB /T ps_4_0 /Fh compiled/passthroughrgb11ps.h Passthrough11.hlsl
fxc /E PS_PassthroughLum /T ps_4_0 /Fh compiled/passthroughlum11ps.h Passthrough11.hlsl
fxc /E PS_PassthroughLumAlpha /T ps_4_0 /Fh compiled/passthroughlumalpha11ps.h Passthrough11.hlsl

fxc /E VS_Clear /T vs_4_0 /Fh compiled/clear11vs.h Clear11.hlsl
fxc /E PS_ClearSingle /T ps_4_0 /Fh compiled/clearsingle11ps.h Clear11.hlsl
fxc /E PS_ClearMultiple /T ps_4_0 /Fh compiled/clearmultiple11ps.h Clear11.hlsl
