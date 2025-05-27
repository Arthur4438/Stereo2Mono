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

#include "Stereo2MonoFXParams.h"

#include <AK/Tools/Common/AkBankReadHelpers.h>

Stereo2MonoFXParams::Stereo2MonoFXParams()
{
}

Stereo2MonoFXParams::~Stereo2MonoFXParams()
{
}

Stereo2MonoFXParams::Stereo2MonoFXParams(const Stereo2MonoFXParams& in_rParams)
{
    RTPC = in_rParams.RTPC;
    NonRTPC = in_rParams.NonRTPC;
    m_paramChangeHandler.SetAllParamChanges();
}

AK::IAkPluginParam* Stereo2MonoFXParams::Clone(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, Stereo2MonoFXParams(*this));
}

AKRESULT Stereo2MonoFXParams::Init(AK::IAkPluginMemAlloc* in_pAllocator, const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    if (in_ulBlockSize == 0)
    {
        // Initialize default parameters
        RTPC.fMonoBlend = 0.0f;        // Start with full stereo (0% mono blend)
        NonRTPC.bEnabled = true;       // Effect enabled by default
        m_paramChangeHandler.SetAllParamChanges();
        return AK_Success;
    }

    return SetParamsBlock(in_pParamsBlock, in_ulBlockSize);
}

AKRESULT Stereo2MonoFXParams::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    AK_PLUGIN_DELETE(in_pAllocator, this);
    return AK_Success;
}

AKRESULT Stereo2MonoFXParams::SetParamsBlock(const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    AKRESULT eResult = AK_Success;
    AkUInt8* pParamsBlock = (AkUInt8*)in_pParamsBlock;

    // Read mono blend parameter from bank data (0-100%)
    RTPC.fMonoBlend = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    CHECKBANKDATASIZE(in_ulBlockSize, eResult);

    // Read enabled parameter from bank data
    NonRTPC.bEnabled = READBANKDATA(bool, pParamsBlock, in_ulBlockSize);
    CHECKBANKDATASIZE(in_ulBlockSize, eResult);

    m_paramChangeHandler.SetAllParamChanges();

    return eResult;
}

AKRESULT Stereo2MonoFXParams::SetParam(AkPluginParamID in_paramID, const void* in_pValue, AkUInt32 in_ulParamSize)
{
    AKRESULT eResult = AK_Success;

    // Handle parameter changes from Wwise authoring or RTPC
    switch (in_paramID)
    {
    case PARAM_MONOBLEND_ID:
        // Update mono blend value (0.0 = full stereo, 100.0 = full mono)
        RTPC.fMonoBlend = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_MONOBLEND_ID);
        break;
    case PARAM_ENABLED_ID:
        // Update enabled state (true = effect active, false = bypass)
        NonRTPC.bEnabled = *((bool*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_ENABLED_ID);
        break;
    default:
        eResult = AK_InvalidParameter;
        break;
    }

    return eResult;
}