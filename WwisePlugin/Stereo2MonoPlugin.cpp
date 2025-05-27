/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Copyright (c) 2024 Audiokinetic Inc.
*******************************************************************************/

#include "stdafx.h"
#include "Stereo2MonoPlugin.h"

// Property names - doit correspondre au XML
const char* const szMonoBlend = "MonoBlend";

// Store current plugin settings into banks when asked to.
bool Stereo2MonoPlugin::GetBankParameters(const GUID& in_guidPlatform, AK::Wwise::Plugin::DataWriter& in_dataWriter) const
{
	// Pack parameters in bank - SEULEMENT MonoBlend comme dans l'ancien code
	in_dataWriter.WriteReal32(m_propertySet.GetReal32(in_guidPlatform, szMonoBlend));

	return true;
}

AK_DEFINE_PLUGIN_CONTAINER(Stereo2Mono);
AK_EXPORT_PLUGIN_CONTAINER(Stereo2Mono);
AK_ADD_PLUGIN_CLASS_TO_CONTAINER(Stereo2Mono, Stereo2MonoPlugin, Stereo2MonoFX);

DEFINE_PLUGIN_REGISTER_HOOK;
DEFINEDUMMYASSERTHOOK;