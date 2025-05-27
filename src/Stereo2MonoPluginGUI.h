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

#pragma once

#include "../Stereo2MonoPlugin.h"

// Includes pour WAAPI
#include <AK/WwiseAuthoringAPI/AkAutobahn/Client.h>
#include <AK/WwiseAuthoringAPI/waapi.h>
#include <string>

class Stereo2MonoPluginGUI final
	: public AK::Wwise::Plugin::PluginMFCWindows<>
	, public AK::Wwise::Plugin::GUIWindows
	, public AK::Wwise::Plugin::RequestHost
	, public AK::Wwise::Plugin::RequestPropertySet
{
public:
	Stereo2MonoPluginGUI() {}

	virtual bool GetDialog(AK::Wwise::Plugin::eDialog in_eDialog, UINT& out_uiDialogID, AK::Wwise::Plugin::PopulateTableItem*& out_pTable) const override;
	virtual bool WindowProc(AK::Wwise::Plugin::eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT& out_lResult) override;
	virtual bool Help(HWND in_hWnd, AK::Wwise::Plugin::eDialog in_eDialog, const char* in_szLanguageCode) const override;

private:
	// Fonctions principales
	void OnCreateRTPCButtonClicked();
	bool CreateGameParameterRTPC(const std::string& name);

	// Fonctions utilitaires
	std::string GetCurrentContainerGUID();
	std::string GetContainerName(const std::string& containerGUID);
	bool CreateOrFindStereo2MonoFolder();

	// Fonctions d'exploration et de liaison
	bool ExploreCreatedRTPC(const std::string& rtpcGUID, const std::string& rtpcName);
	bool TestSetReferenceForRTPC(const std::string& rtpcGUID, const std::string& effectGUID, const std::string& rtpcName);
	bool LinkRTPCToMonoBlend(const std::string& rtpcGUID, const std::string& effectGUID, const std::string& rtpcName);
	bool TestSchemaBasedRTPCLinking(const std::string& rtpcGUID, const std::string& effectGUID, const std::string& rtpcName);

	HWND m_hwndPropView = nullptr;

	// Compteur statique pour les noms d'RTPC uniques
	static int s_rtpcCounter;
};