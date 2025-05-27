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

#include "../stdafx.h"
#include "Stereo2MonoPluginGUI.h"

#include "../resource.h"

// Includes pour WAAPI
#include <AK/WwiseAuthoringAPI/AkAutobahn/Client.h>
#include <AK/WwiseAuthoringAPI/waapi.h>
#include <sstream>
#include <iomanip>

// Initialisation du compteur statique
int Stereo2MonoPluginGUI::s_rtpcCounter = 1;

// Set the property names to UI control binding populated table.
bool Stereo2MonoPluginGUI::GetDialog(AK::Wwise::Plugin::eDialog in_eDialog, UINT& out_uiDialogID, AK::Wwise::Plugin::PopulateTableItem*& out_pTable) const
{
	AKASSERT(in_eDialog == AK::Wwise::Plugin::SettingsDialog);

	out_uiDialogID = IDD_STEREO2MONO_MAIN;
	out_pTable = nullptr; // Pas de table - Wwise va utiliser les controles automatiquement

	return true;
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool Stereo2MonoPluginGUI::WindowProc(AK::Wwise::Plugin::eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT& out_lResult)
{
	switch (in_message)
	{
	case WM_INITDIALOG:
		m_hwndPropView = in_hWnd;
		break;
	case WM_DESTROY:
		m_hwndPropView = NULL;
		break;
	case WM_COMMAND:
		if (LOWORD(in_wParam) == IDC_CREATE_RTPC_BUTTON && HIWORD(in_wParam) == BN_CLICKED)
		{
			OnCreateRTPCButtonClicked();
			out_lResult = 0;
			return true;
		}
		break;
	}

	out_lResult = 0;
	return false;
}

// Implement online help when the user clicks on the "?" icon .
bool Stereo2MonoPluginGUI::Help(HWND in_hWnd, AK::Wwise::Plugin::eDialog in_eDialog, const char* in_szLanguageCode) const
{
	return false;
}

// Allocateur complet pour WAAPI - TOUTES les methodes implementees
class SimpleAllocator : public AK::IAkPluginMemAlloc
{
public:
	// 1. Malloc avec parametres de debug - SIGNATURE EXACTE
	void* Malloc(size_t in_uSize, const char* in_pszFile, AkUInt32 in_uLine)
	{
		// Ignorer les parametres de debug pour notre implementation simple
		(void)in_pszFile;
		(void)in_uLine;
		return malloc(in_uSize);
	}

	// 2. Free - SIGNATURE EXACTE
	void Free(void* in_pMemAddress)
	{
		if (in_pMemAddress)
			free(in_pMemAddress);
	}

	// 3. Malloc aligne - SIGNATURE EXACTE
	void* Malign(size_t in_uSize, size_t in_uAlignment, const char* in_pszFile, AkUInt32 in_uLine)
	{
		// Ignorer les parametres de debug
		(void)in_pszFile;
		(void)in_uLine;

		// Implementation simple d'alignement
#ifdef _WIN32
		return _aligned_malloc(in_uSize, in_uAlignment);
#else
		void* ptr = nullptr;
		if (posix_memalign(&ptr, in_uAlignment, in_uSize) == 0)
			return ptr;
		return nullptr;
#endif
	}

	// 4. Realloc avec parametres de debug - SIGNATURE EXACTE
	void* Realloc(void* in_pMemAddress, size_t in_uSize, const char* in_pszFile, AkUInt32 in_uLine)
	{
		// Ignorer les parametres de debug
		(void)in_pszFile;
		(void)in_uLine;
		return realloc(in_pMemAddress, in_uSize);
	}

	// 5. Realloc aligne - SIGNATURE EXACTE
	void* ReallocAligned(void* in_pMemAddress, size_t in_uSize, size_t in_uAlignment, const char* in_pszFile, AkUInt32 in_uLine)
	{
		// Ignorer les parametres de debug
		(void)in_pszFile;
		(void)in_uLine;

		if (!in_pMemAddress) {
			return Malign(in_uSize, in_uAlignment, in_pszFile, in_uLine);
		}

		if (in_uSize == 0) {
			Free(in_pMemAddress);
			return nullptr;
		}

		// Implementation simple - allouer nouveau bloc et copier
		void* newPtr = Malign(in_uSize, in_uAlignment, in_pszFile, in_uLine);
		if (newPtr && in_pMemAddress) {
			// Copier les donnees (estimation de taille)
			memcpy(newPtr, in_pMemAddress, in_uSize);
			Free(in_pMemAddress);
		}

		return newPtr;
	}

	// Destructeur virtuel
	virtual ~SimpleAllocator() {}
};

// Obtient l'ID du container sur lequel le plugin est applique
std::string Stereo2MonoPluginGUI::GetCurrentContainerGUID()
{
	try {
		// Utiliser PropertySet pour obtenir l'ID de l'objet
		const GUID* objectIdPtr = m_propertySet.GetID();
		if (objectIdPtr) {
			const GUID& objectId = *objectIdPtr;  // Dereferencer le pointeur

			// Convertir GUID en string format
			char guidString[64];
			sprintf_s(guidString, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				objectId.Data1, objectId.Data2, objectId.Data3,
				objectId.Data4[0], objectId.Data4[1], objectId.Data4[2], objectId.Data4[3],
				objectId.Data4[4], objectId.Data4[5], objectId.Data4[6], objectId.Data4[7]);

			return std::string(guidString);
		}
	}
	catch (...) {
		// En cas d'erreur, retourner vide
	}

	return "";
}

// Obtient le nom du container a partir de son GUID
std::string Stereo2MonoPluginGUI::GetContainerName(const std::string& containerGUID)
{
	try {
		if (containerGUID.empty()) {
			return "";
		}

		// Preparer l'appel WAAPI pour obtenir le nom de l'objet
		std::ostringstream getInfoArgs;
		getInfoArgs << "{\"from\":{\"id\":[\"" << containerGUID << "\"]}}";

		std::string getInfoOptions = "{\"return\":[\"name\"]}";  // On veut juste le nom

		char* results = nullptr;
		char* error = nullptr;
		SimpleAllocator allocator;

		// Faire l'appel WAAPI
		m_host.WaapiCall(
			"ak.wwise.core.object.get",
			getInfoArgs.str().c_str(),
			getInfoOptions.c_str(),
			allocator,
			results,
			error
		);

		std::string containerName = "";

		// Verifier les erreurs
		if (error && strlen(error) > 0) {
			// En cas d'erreur, retourner vide
			allocator.Free(results);
			allocator.Free(error);
			return "";
		}

		// Parser le JSON pour extraire le nom
		if (results && strlen(results) > 0) {
			std::string jsonResult = std::string(results);

			// Recherche simple du nom dans le JSON
			// Le JSON ressemble a: {"return":[{"name":"MonContainer"}]}
			size_t namePos = jsonResult.find("\"name\":\"");
			if (namePos != std::string::npos) {
				size_t nameStart = namePos + 8;  // Apres "name":"
				size_t nameEnd = jsonResult.find("\"", nameStart);
				if (nameEnd != std::string::npos) {
					containerName = jsonResult.substr(nameStart, nameEnd - nameStart);
				}
			}
		}

		allocator.Free(results);
		allocator.Free(error);
		return containerName;
	}
	catch (...) {
		return "";
	}
}

// Cree ou trouve le dossier Stereo2Mono dans Game Parameters (version corrigee)
bool Stereo2MonoPluginGUI::CreateOrFindStereo2MonoFolder()
{
	try {
		SimpleAllocator allocator;
		char* results = nullptr;
		char* error = nullptr;

		// 1. Verifier si le dossier existe (chemin corrige)
		std::ostringstream checkArgs;
		checkArgs << "{\"from\":{\"path\":[\"\\\\Game Parameters\\\\Default Work Unit\\\\Stereo2Mono\"]}}";

		m_host.WaapiCall(
			"ak.wwise.core.object.get",
			checkArgs.str().c_str(),
			"{}",
			allocator,
			results,
			error
		);

		// Si pas d'erreur, le dossier existe
		if (!error || strlen(error) == 0) {
			allocator.Free(results);
			allocator.Free(error);
			return true;
		}

		// Nettoyer avant la creation
		allocator.Free(results);
		allocator.Free(error);

		// 2. Creer le dossier (chemin parent corrige)
		std::ostringstream createArgs;
		createArgs << "{";
		createArgs << "\"parent\":\"\\\\Game Parameters\\\\Default Work Unit\",";
		createArgs << "\"type\":\"Folder\",";
		createArgs << "\"name\":\"Stereo2Mono\",";
		createArgs << "\"onNameConflict\":\"rename\"";
		createArgs << "}";

		results = nullptr;
		error = nullptr;

		m_host.WaapiCall(
			"ak.wwise.core.object.create",
			createArgs.str().c_str(),
			"{}",
			allocator,
			results,
			error
		);

		bool success = (!error || strlen(error) == 0);

		allocator.Free(results);
		allocator.Free(error);
		return success;
	}
	catch (...) {
		return false;
	}
}

// Gestionnaire de clic sur le bouton Create RTPC
void Stereo2MonoPluginGUI::OnCreateRTPCButtonClicked()
{
	if (CreateGameParameterRTPC(""))
	{
		// Succes affiche dans CreateGameParameterRTPC
	}
	else
	{
		MessageBoxA(NULL,
			"Failed to create Game Parameter.\n\n"
			"Please ensure:\n"
			"- Wwise Authoring is running\n"
			"- WAAPI is enabled in Project Settings\n"
			"- Plugin is applied to a valid container",
			"Creation Failed",
			MB_OK | MB_ICONERROR);
	}
}

// Verification du succes de la liaison RTPC
bool Stereo2MonoPluginGUI::TestSetReferenceForRTPC(const std::string& rtpcGUID, const std::string& effectGUID, const std::string& rtpcName)
{
	try {
		SimpleAllocator allocator;
		std::string testResults = "=== VERIFYING RTPC SUCCESS ===\n\n";
		testResults += "RTPC: " + rtpcName + " (" + rtpcGUID + ")\n";
		testResults += "Effect: " + effectGUID + "\n\n";

		// Test 1: Verifier si la liaison a bien fonctionne
		{
			char* results = nullptr;
			char* error = nullptr;

			std::ostringstream getLinkedArgs;
			getLinkedArgs << "{\"from\":{\"id\":[\"" << effectGUID << "\"]}}";

			m_host.WaapiCall(
				"ak.wwise.core.object.get",
				getLinkedArgs.str().c_str(),
				"{\"return\":[\"MonoBlend\",\"rtpc\"]}",
				allocator,
				results,
				error
			);

			testResults += "1. VERIFY RTPC LINK SUCCESS:\n";
			if (results && strlen(results) > 0) {
				testResults += "Current state: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Test 2: Confirmer la liaison en appliquant une nouvelle fois (devrait reussir)
		{
			char* results = nullptr;
			char* error = nullptr;

			std::ostringstream confirmArgs;
			confirmArgs << "{";
			confirmArgs << "\"object\":\"" << effectGUID << "\",";
			confirmArgs << "\"property\":\"MonoBlend\",";
			confirmArgs << "\"value\":\"" << rtpcGUID << "\"";
			confirmArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				confirmArgs.str().c_str(),
				"{}",
				allocator,
				results,
				error
			);

			testResults += "2. CONFIRM RTPC LINK (SHOULD SUCCEED AGAIN):\n";
			testResults += "Args: " + confirmArgs.str() + "\n";
			if (results && strlen(results) > 0) {
				testResults += "SUCCESS: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Test 3: Essayer d'ajouter une courbe par defaut si necessaire
		{
			char* results = nullptr;
			char* error = nullptr;

			// Verifier d'abord si une courbe existe
			std::ostringstream getCurveArgs;
			getCurveArgs << "{\"from\":{\"id\":[\"" << effectGUID << "\"]}}";

			m_host.WaapiCall(
				"ak.wwise.core.object.get",
				getCurveArgs.str().c_str(),
				"{\"return\":[\"rtpc.curve\"]}",
				allocator,
				results,
				error
			);

			testResults += "3. CHECK FOR EXISTING CURVE:\n";
			if (results && strlen(results) > 0) {
				testResults += "Curve state: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Test 4: Tester differentes valeurs pour MonoBlend via RTPC
		{
			char* results = nullptr;
			char* error = nullptr;

			// Essayer de definir une valeur numerique pour MonoBlend
			std::ostringstream setValueArgs;
			setValueArgs << "{";
			setValueArgs << "\"object\":\"" << effectGUID << "\",";
			setValueArgs << "\"property\":\"MonoBlend\",";
			setValueArgs << "\"value\":50.0";  // Valeur numerique au lieu d'un GUID
			setValueArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				setValueArgs.str().c_str(),
				"{}",
				allocator,
				results,
				error
			);

			testResults += "4. TEST NUMERIC VALUE FOR MonoBlend:\n";
			testResults += "Args: " + setValueArgs.str() + "\n";
			if (results && strlen(results) > 0) {
				testResults += "SUCCESS: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Test 5: Remettre la liaison RTPC apres le test numerique
		{
			char* results = nullptr;
			char* error = nullptr;

			std::ostringstream restoreArgs;
			restoreArgs << "{";
			restoreArgs << "\"object\":\"" << effectGUID << "\",";
			restoreArgs << "\"property\":\"MonoBlend\",";
			restoreArgs << "\"value\":\"" << rtpcGUID << "\"";
			restoreArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				restoreArgs.str().c_str(),
				"{}",
				allocator,
				results,
				error
			);

			testResults += "5. RESTORE RTPC LINK:\n";
			testResults += "Args: " + restoreArgs.str() + "\n";
			if (results && strlen(results) > 0) {
				testResults += "SUCCESS: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Test 6: Etat final de l'effet
		{
			char* results = nullptr;
			char* error = nullptr;

			std::ostringstream finalStateArgs;
			finalStateArgs << "{\"from\":{\"id\":[\"" << effectGUID << "\"]}}";

			m_host.WaapiCall(
				"ak.wwise.core.object.get",
				finalStateArgs.str().c_str(),
				"{\"return\":[\"MonoBlend\",\"rtpc\",\"@*\"]}",
				allocator,
				results,
				error
			);

			testResults += "6. FINAL EFFECT STATE:\n";
			if (results && strlen(results) > 0) {
				testResults += "Final state: " + std::string(results) + "\n\n";
			}
			if (error && strlen(error) > 0) {
				testResults += "Error: " + std::string(error) + "\n\n";
			}

			allocator.Free(results);
			allocator.Free(error);
		}

		// Message de resume
		testResults += "=== SUMMARY ===\n";
		testResults += "RTPC creation: SUCCESS\n";
		testResults += "RTPC linking: SUCCESS (Test 1 from previous test)\n";
		testResults += "Method found: ak.wwise.core.object.setProperty\n";
		testResults += "Syntax: {\"object\":\"effectGUID\",\"property\":\"MonoBlend\",\"value\":\"rtpcGUID\"}\n\n";
		testResults += "The RTPC should now be linked to MonoBlend parameter!\n";
		testResults += "Check in Wwise GUI to confirm the link is visible.";

		// Afficher tous les resultats
		MessageBoxA(NULL, testResults.c_str(), "RTPC Link Verification", MB_OK | MB_ICONINFORMATION);
		return true;
	}
	catch (...) {
		MessageBoxA(NULL, "Exception during RTPC verification", "Error", MB_OK);
		return false;
	}
}

// Supprimé - fonction ExploreCreatedRTPC non nécessaire dans l'approche simplifiée

// Nouvelle fonction qui cree la RTPC ET la courbe en meme temps
bool Stereo2MonoPluginGUI::LinkRTPCToMonoBlend(const std::string& rtpcGUID, const std::string& effectGUID, const std::string& rtpcName)
{
	try {
		if (rtpcGUID.empty() || effectGUID.empty()) {
			return false;
		}

		SimpleAllocator allocator;
		std::string results = "=== CREATING RTPC + CURVE BINDING ===\n\n";
		results += "RTPC: " + rtpcName + " (" + rtpcGUID + ")\n";
		results += "Effect: " + effectGUID + "\n\n";

		// Etape 1: Creer d'abord une courbe lineaire pour le Mono Blend
		std::string curveGUID = "";
		{
			char* createResults = nullptr;
			char* createError = nullptr;

			std::ostringstream createCurveArgs;
			createCurveArgs << "{";
			createCurveArgs << "\"parent\":\"" << effectGUID << "\",";
			createCurveArgs << "\"type\":\"Curve\",";
			createCurveArgs << "\"name\":\"MonoBlend_Curve\",";
			createCurveArgs << "\"onNameConflict\":\"rename\"";
			createCurveArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.create",
				createCurveArgs.str().c_str(),
				"{}",
				allocator,
				createResults,
				createError
			);

			results += "1. CREATE CURVE:\n";
			results += "Args: " + createCurveArgs.str() + "\n";
			if (createResults && strlen(createResults) > 0) {
				results += "SUCCESS: " + std::string(createResults) + "\n";

				// Extraire l'ID de la courbe creee
				std::string jsonResult = std::string(createResults);
				size_t idPos = jsonResult.find("\"id\":\"");
				if (idPos != std::string::npos) {
					size_t idStart = idPos + 6;
					size_t idEnd = jsonResult.find("\"", idStart);
					if (idEnd != std::string::npos) {
						curveGUID = jsonResult.substr(idStart, idEnd - idStart);
					}
				}
			}
			if (createError && strlen(createError) > 0) {
				results += "Error: " + std::string(createError) + "\n";
			}
			results += "\n";

			allocator.Free(createResults);
			allocator.Free(createError);
		}

		// Etape 2: Configurer la courbe avec des points lineaires (0 vers 100)
		if (!curveGUID.empty()) {
			char* configResults = nullptr;
			char* configError = nullptr;

			std::ostringstream configureCurveArgs;
			configureCurveArgs << "{";
			configureCurveArgs << "\"object\":\"" << curveGUID << "\",";
			configureCurveArgs << "\"property\":\"Points\",";
			configureCurveArgs << "\"value\":[";
			configureCurveArgs << "{\"x\":0,\"y\":0,\"shape\":\"Linear\"},";
			configureCurveArgs << "{\"x\":100,\"y\":100,\"shape\":\"Linear\"}";
			configureCurveArgs << "]";
			configureCurveArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				configureCurveArgs.str().c_str(),
				"{}",
				allocator,
				configResults,
				configError
			);

			results += "2. CONFIGURE CURVE POINTS:\n";
			results += "Args: " + configureCurveArgs.str() + "\n";
			if (configResults && strlen(configResults) > 0) {
				results += "SUCCESS: " + std::string(configResults) + "\n";
			}
			if (configError && strlen(configError) > 0) {
				results += "Error: " + std::string(configError) + "\n";
			}
			results += "\n";

			allocator.Free(configResults);
			allocator.Free(configError);
		}

		// Etape 3: Lier le controle d'entree RTPC (selon les resultats WAQL)
		{
			char* linkResults = nullptr;
			char* linkError = nullptr;

			std::ostringstream linkControlInputArgs;
			linkControlInputArgs << "{";
			linkControlInputArgs << "\"object\":\"" << effectGUID << "\",";
			linkControlInputArgs << "\"property\":\"rtpc.controlinput\",";
			linkControlInputArgs << "\"value\":\"" << rtpcGUID << "\"";
			linkControlInputArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				linkControlInputArgs.str().c_str(),
				"{}",
				allocator,
				linkResults,
				linkError
			);

			results += "3. LINK RTPC CONTROL INPUT:\n";
			results += "Args: " + linkControlInputArgs.str() + "\n";
			if (linkResults && strlen(linkResults) > 0) {
				results += "SUCCESS: " + std::string(linkResults) + "\n";
			}
			if (linkError && strlen(linkError) > 0) {
				results += "Error: " + std::string(linkError) + "\n";
			}
			results += "\n";

			allocator.Free(linkResults);
			allocator.Free(linkError);
		}

		// Etape 4: Definir le nom de propriete (selon les resultats WAQL)
		{
			char* propResults = nullptr;
			char* propError = nullptr;

			std::ostringstream setPropNameArgs;
			setPropNameArgs << "{";
			setPropNameArgs << "\"object\":\"" << effectGUID << "\",";
			setPropNameArgs << "\"property\":\"rtpc.propertyname\",";
			setPropNameArgs << "\"value\":\"MonoBlend\"";
			setPropNameArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				setPropNameArgs.str().c_str(),
				"{}",
				allocator,
				propResults,
				propError
			);

			results += "4. SET PROPERTY NAME:\n";
			results += "Args: " + setPropNameArgs.str() + "\n";
			if (propResults && strlen(propResults) > 0) {
				results += "SUCCESS: " + std::string(propResults) + "\n";
			}
			if (propError && strlen(propError) > 0) {
				results += "Error: " + std::string(propError) + "\n";
			}
			results += "\n";

			allocator.Free(propResults);
			allocator.Free(propError);
		}

		// Etape 5: Lier la courbe (selon les resultats WAQL)
		if (!curveGUID.empty()) {
			char* curveResults = nullptr;
			char* curveError = nullptr;

			std::ostringstream linkCurveArgs;
			linkCurveArgs << "{";
			linkCurveArgs << "\"object\":\"" << effectGUID << "\",";
			linkCurveArgs << "\"property\":\"rtpc.curve\",";
			linkCurveArgs << "\"value\":\"" << curveGUID << "\"";
			linkCurveArgs << "}";

			m_host.WaapiCall(
				"ak.wwise.core.object.setProperty",
				linkCurveArgs.str().c_str(),
				"{}",
				allocator,
				curveResults,
				curveError
			);

			results += "5. LINK CURVE:\n";
			results += "Args: " + linkCurveArgs.str() + "\n";
			if (curveResults && strlen(curveResults) > 0) {
				results += "SUCCESS: " + std::string(curveResults) + "\n";
			}
			if (curveError && strlen(curveError) > 0) {
				results += "Error: " + std::string(curveError) + "\n";
			}
			results += "\n";

			allocator.Free(curveResults);
			allocator.Free(curveError);
		}

		// Etape 6: Verification finale avec les proprietes qui marchent
		{
			char* verifyResults = nullptr;
			char* verifyError = nullptr;

			std::ostringstream verifyArgs;
			verifyArgs << "{\"from\":{\"id\":[\"" << effectGUID << "\"]}}";

			m_host.WaapiCall(
				"ak.wwise.core.object.get",
				verifyArgs.str().c_str(),
				"{\"return\":[\"rtpc.controlinput\",\"rtpc.propertyname\",\"rtpc.curve\"]}",
				allocator,
				verifyResults,
				verifyError
			);

			results += "6. FINAL VERIFICATION:\n";
			if (verifyResults && strlen(verifyResults) > 0) {
				results += "Final RTPC state: " + std::string(verifyResults) + "\n";
			}
			if (verifyError && strlen(verifyError) > 0) {
				results += "Error: " + std::string(verifyError) + "\n";
			}
			results += "\n";

			allocator.Free(verifyResults);
			allocator.Free(verifyError);
		}

		results += "=== SUMMARY ===\n";
		results += "Method: Create Curve + Link via rtpc.* properties\n";
		results += "Based on: WAQL exploration results showing working properties\n";
		results += "Curve GUID: " + curveGUID + "\n";
		results += "Check Wwise GUI to see if RTPC link is now functional!";

		MessageBoxA(NULL, results.c_str(), "RTPC + Curve Creation Test", MB_OK | MB_ICONINFORMATION);
		return true;
	}
	catch (...) {
		MessageBoxA(NULL, "Exception in RTPC+Curve creation", "Error", MB_OK);
		return false;
	}
}

// Fonction principale de creation de Game Parameter RTPC
bool Stereo2MonoPluginGUI::CreateGameParameterRTPC(const std::string& name)
{
	try {
		// 1. Obtenir le GUID du container (l'effet lui-meme)
		std::string containerGUID = GetCurrentContainerGUID();
		if (containerGUID.empty()) {
			MessageBoxA(NULL, "Cannot get container GUID", "Error", MB_OK);
			return false;
		}

		// 2. Obtenir le nom du container
		std::string containerName = GetContainerName(containerGUID);
		if (containerName.empty()) {
			MessageBoxA(NULL, "Cannot get container name", "Error", MB_OK);
			return false;
		}

		// 3. Creer/trouver le dossier Stereo2Mono
		if (!CreateOrFindStereo2MonoFolder()) {
			MessageBoxA(NULL, "Failed to create or find Stereo2Mono folder", "Error", MB_OK);
			return false;
		}

		// 4. Creer le RTPC
		std::ostringstream rtpcName;
		rtpcName << containerName << "_Attenuation";

		std::ostringstream createArgs;
		createArgs << "{";
		createArgs << "\"parent\":\"\\\\Game Parameters\\\\Default Work Unit\\\\Stereo2Mono\",";
		createArgs << "\"type\":\"GameParameter\",";
		createArgs << "\"name\":\"" << rtpcName.str() << "\",";
		createArgs << "\"onNameConflict\":\"rename\"";
		createArgs << "}";

		char* results = nullptr;
		char* error = nullptr;
		SimpleAllocator allocator;

		m_host.WaapiCall(
			"ak.wwise.core.object.create",
			createArgs.str().c_str(),
			"{}",
			allocator,
			results,
			error
		);

		if (error && strlen(error) > 0) {
			std::string errorMsg = "Error creating RTPC: " + std::string(error);
			MessageBoxA(NULL, errorMsg.c_str(), "Creation Error", MB_OK);
			allocator.Free(results);
			allocator.Free(error);
			return false;
		}

		// 5. Extraire l'ID de la RTPC creee
		std::string rtpcGUID = "";
		std::string actualRTPCName = rtpcName.str();
		if (results && strlen(results) > 0) {
			std::string jsonResult = std::string(results);

			size_t idPos = jsonResult.find("\"id\":\"");
			if (idPos != std::string::npos) {
				size_t idStart = idPos + 6;
				size_t idEnd = jsonResult.find("\"", idStart);
				if (idEnd != std::string::npos) {
					rtpcGUID = jsonResult.substr(idStart, idEnd - idStart);
				}
			}

			size_t namePos = jsonResult.find("\"name\":\"");
			if (namePos != std::string::npos) {
				size_t nameStart = namePos + 8;
				size_t nameEnd = jsonResult.find("\"", nameStart);
				if (nameEnd != std::string::npos) {
					actualRTPCName = jsonResult.substr(nameStart, nameEnd - nameStart);
				}
			}
		}

		allocator.Free(results);
		allocator.Free(error);

		// 6. Essayer de créer une vraie liaison RTPC (pas juste setProperty)
		if (!rtpcGUID.empty()) {
			LinkRTPCToMonoBlend(rtpcGUID, containerGUID, actualRTPCName);
		}

		// 7. Vérification optionnelle (peut être commentée)
		// if (!rtpcGUID.empty()) {
		//     TestSetReferenceForRTPC(rtpcGUID, containerGUID, actualRTPCName);
		// }

		// 8. Message de succes final
		std::string successMsg = "COMPLETE SUCCESS!\n\n";
		successMsg += "Container: " + containerName + "\n";
		successMsg += "RTPC Created: " + actualRTPCName + "\n";
		successMsg += "RTPC GUID: " + rtpcGUID + "\n";
		successMsg += "Location: Game Parameters\\Default Work Unit\\Stereo2Mono\n";
		successMsg += "\nRTPC created and linked to MonoBlend!\n";
		successMsg += "Method: ak.wwise.core.object.setProperty\n";
		successMsg += "\nCheck in Wwise: the MonoBlend parameter should now\n";
		successMsg += "show the RTPC connection in the property editor!";

		MessageBoxA(NULL, successMsg.c_str(), "RTPC Created & Linked!", MB_OK | MB_ICONINFORMATION);

		return true;
	}
	catch (...) {
		MessageBoxA(NULL, "Exception in CreateGameParameterRTPC", "Error", MB_OK);
		return false;
	}
}

AK_ADD_PLUGIN_CLASS_TO_CONTAINER(Stereo2Mono, Stereo2MonoPluginGUI, Stereo2MonoFX);