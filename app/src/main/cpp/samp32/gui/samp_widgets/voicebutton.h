#pragma once
#include "../../game/game.h"
#include "../../game/util.h"

#include "../../main.h"

#include "../../settings.h"
#include "util/CUtil.h"


extern CSettings* pSettings;
#include <atomic>
// [v15] bouton micro : icone "VOCAL" Crestwood (assets/models/buttons/vocal*.png), affiche meme si
// settings.ini dit VoiceChatEnable=false (le launcher l'ecrivait a false -> le bouton n'apparaissait jamais).
// Seule l'option "Bouton micro" des parametres en jeu le masque.
extern std::atomic<bool> g_cwVoiceButton;
RwTexture* Crestwood_LoadApkTexture(const char* path);

class VoiceButton : public Button
{
public:
	VoiceButton() : Button("TALK", UISettings::fontSize() / 2) {
		m_recording = false;
		/* 5:3 aspect ratio */
		//m_texture_micro_on = (RwTexture*)CUtil::LoadTextureFromDB("samp", "icon_micro_on");
		//m_texture_micro_off = (RwTexture*)CUtil::LoadTextureFromDB("samp", "icon_micro_off");
		m_texture_micro_on = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voiceactive"); //default
		//if (Server == 40) m_texture_micro_on = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voiceactive");
		//if (Server == 40) m_texture_micro_off = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voicepassive");
		m_texture_micro_off = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voicepassive"); //default
		//if (Server == 13) m_texture_micro_off = (RwTexture*)CUtil::LoadTextureFromDB("samp", "icon_micro_on");
		//if (Server == 13) m_texture_micro_on = (RwTexture*)CUtil::LoadTextureFromDB("samp", "icon_micro_off");
	}

	virtual void draw(ImGuiRenderer* renderer) override
	{
		if (!g_cwVoiceButton.load()) return;

		if (countdown > 0 && recording() == 1) countdown--;
		if (countdown == 0 && recording() == 1) setRecording(0);
		RwTexture* tex = Crestwood_LoadApkTexture(recording() ? "models/buttons/vocal_on.png" : "models/buttons/vocal.png");
		if (!tex) tex = recording() ? m_texture_micro_on : m_texture_micro_off;
		if (tex && tex->raster) {
			renderer->drawImage(absolutePosition(), absolutePosition() + size(), tex->raster);
		}
	}

	void touchPopEvent() override
	{
		countdown = 500;
		setRecording(0);
	}

	void touchPushEvent() override
	{
		if (!g_cwVoiceButton.load()) return;
			//setRecording(recording() ^ 1);
		setRecording(1);
		countdown = 500;
	}

	void setRecording(bool recording) 
	{ 
		if (recording == 1) countdown = 200;
		m_recording = recording;
		this->setCaptionColor(m_recording ? ImColor(1.0f, 0.0f, 0.0f) : ImColor(1.0f, 1.0f, 1.0f));
	}

	bool recording() const { return m_recording; }

private:
	bool m_recording;
	RwTexture* m_texture_micro_on;
	RwTexture* m_texture_micro_off;
	//int countdown = 1000;
public:
	int countdown = 1000;
};