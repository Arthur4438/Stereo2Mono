/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the
"Apache License"); you may not use this file except in compliance with the
Apache License. You may obtain a copy of the Apache License at
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Copyright (c) 2024 Audiokinetic Inc.
*******************************************************************************/

#include "Stereo2MonoFX.h"
#include "../Stereo2MonoConfig.h"

#include <AK/AkWwiseSDKVersion.h>

AK::IAkPlugin* CreateStereo2MonoFX(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, Stereo2MonoFX());
}

AK::IAkPluginParam* CreateStereo2MonoFXParams(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, Stereo2MonoFXParams());
}

AK_IMPLEMENT_PLUGIN_FACTORY(Stereo2MonoFX, AkPluginTypeEffect, Stereo2MonoConfig::CompanyID, Stereo2MonoConfig::PluginID)

Stereo2MonoFX::Stereo2MonoFX()
    : m_pParams(nullptr)
    , m_pAllocator(nullptr)
    , m_pContext(nullptr)
{
}

Stereo2MonoFX::~Stereo2MonoFX()
{
}

AKRESULT Stereo2MonoFX::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkEffectPluginContext* in_pContext, AK::IAkPluginParam* in_pParams, AkAudioFormat& in_rFormat)
{
    m_pParams = (Stereo2MonoFXParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pContext;

    return AK_Success;
}

AKRESULT Stereo2MonoFX::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    AK_PLUGIN_DELETE(in_pAllocator, this);
    return AK_Success;
}

AKRESULT Stereo2MonoFX::Reset()
{
    return AK_Success;
}

AKRESULT Stereo2MonoFX::GetPluginInfo(AkPluginInfo& out_rPluginInfo)
{
    out_rPluginInfo.eType = AkPluginTypeEffect;
    out_rPluginInfo.bIsInPlace = true;
    out_rPluginInfo.bCanProcessObjects = false;
    out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
    return AK_Success;
}

void Stereo2MonoFX::Execute(AkAudioBuffer* io_pBuffer)
{
    const AkUInt32 uNumChannels = io_pBuffer->NumChannels();

    // Check if effect is enabled first
    if (!m_pParams->NonRTPC.bEnabled)
    {
        // Effect is disabled/muted - pass audio through unchanged
        return;
    }

    // Only process stereo sources (2 channels)
    if (uNumChannels != 2)
        return;

    // Get the current MonoBlend parameter value (0.0 to 100.0)
    AkReal32 fMonoBlendPercent = m_pParams->RTPC.fMonoBlend;

    // Convert percentage to blend factor (0.0 = full stereo, 1.0 = full mono)
    AkReal32 fBlendFactor = fMonoBlendPercent / 100.0f;

    // Get pointers to left and right channel buffers
    AkReal32* AK_RESTRICT pLeftChannel = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(0);
    AkReal32* AK_RESTRICT pRightChannel = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(1);

    // Process each audio frame
    for (AkUInt16 uFrame = 0; uFrame < io_pBuffer->uValidFrames; ++uFrame)
    {
        // Get current left and right samples
        AkReal32 fLeftSample = pLeftChannel[uFrame];
        AkReal32 fRightSample = pRightChannel[uFrame];

        // Calculate mono signal (average of L+R)
        AkReal32 fMonoSample = (fLeftSample + fRightSample) * 0.5f;

        // Apply blend between stereo and mono
        // When fBlendFactor = 0.0: output = original stereo
        // When fBlendFactor = 1.0: output = mono (both channels identical)
        pLeftChannel[uFrame] = fLeftSample * (1.0f - fBlendFactor) + fMonoSample * fBlendFactor;
        pRightChannel[uFrame] = fRightSample * (1.0f - fBlendFactor) + fMonoSample * fBlendFactor;
    }
}

AKRESULT Stereo2MonoFX::TimeSkip(AkUInt32 in_uFrames)
{
    return AK_DataReady;
}