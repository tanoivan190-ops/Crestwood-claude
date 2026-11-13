#include "jniutil.h"
#include "game/game.h"
extern CGame *pGame;

CJavaWrapper::CJavaWrapper(JNIEnv *env, jobject activity)
{
    this->activity = env->NewGlobalRef(activity);

    jclass clas = env->GetObjectClass(activity);
    if(!clas)
    {
        FLog("no clas");
        return;
    }

    s_showTab = env->GetMethodID(clas, "showTab", "()V");
    s_hideTab = env->GetMethodID(clas, "hideTab", "()V");
    s_clearTab = env->GetMethodID(clas, "clearTab", "()V");
    s_setTab = env->GetMethodID(clas, "setTab", "(ILjava/lang/String;II)V");

    s_showLoadingScreen = env->GetMethodID(clas, "showLoadingScreen", "()V");
    s_hideLoadingScreen = env->GetMethodID(clas, "hideLoadingScreen", "()V");

    s_setPauseState = env->GetMethodID(clas, "setPauseState", "(Z)V");
    
    s_ShowDialog = env->GetMethodID(clas, "showDialog", "(II[B[B[B[B)V");

	s_showInputLayout = env->GetMethodID(clas, "showKeyboard", "()V");
    s_hideInputLayout = env->GetMethodID(clas, "hideKeyboard", "()V");

    s_exitGame = env->GetMethodID(clas, "exitGame", "()V");

    s_showEditObject = env->GetMethodID(clas, "showEditObject", "()V");
    s_hideEditObject = env->GetMethodID(clas, "hideEditObject", "()V");

    // [Crestwood] HUD HTML (absent d'une ancienne activite : on efface l'exception NoSuchMethodError)
    s_updateHudState = env->GetMethodID(clas, "updateHudState", "(FFFFFZZZZFFZFFFFF)V");
    if (env->ExceptionCheck()) env->ExceptionClear();
    s_updateHudMoney = env->GetMethodID(clas, "updateHudMoney", "(I)V");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_updateHudMoney = nullptr; }
    s_hudChat = env->GetMethodID(clas, "hudChatMessage", "([B[BI)V");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_hudChat = nullptr; }
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_updateHudState = nullptr; }
    s_clothesOpen = env->GetMethodID(clas, "clothesOpen", "(Ljava/lang/String;)V");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_clothesOpen = nullptr; }
    s_clothesState = env->GetMethodID(clas, "clothesState", "(Ljava/lang/String;)V");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_clothesState = nullptr; }
    s_clothesClose = env->GetMethodID(clas, "clothesClose", "()V");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_clothesClose = nullptr; }

    env->DeleteLocalRef(clas);
}

void CJavaWrapper::ShowKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showInputLayout);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideInputLayout);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ShowLoadingScreen()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showLoadingScreen);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideLoadingScreen()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideLoadingScreen);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::SetPauseState(bool pause)
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_setPauseState, pause);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::SetTab(int id, char* names, int score, int pings)
{
    JNIEnv* global_env;
    javaVM->GetEnv((void**)&global_env, JNI_VERSION_1_6);

	if (!global_env)
	{
		LOGI("No env");
		return;
	}

	jclass strClass = global_env->FindClass("java/lang/String"); 
	jmethodID ctorID = global_env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V"); 
	jstring encoding = global_env->NewStringUTF("UTF-8"); 

	jbyteArray bytes = global_env->NewByteArray(strlen(names)); 
	global_env->SetByteArrayRegion(bytes, 0, strlen(names), (jbyte*)names); 
	jstring str1 = (jstring)global_env->NewObject(strClass, ctorID, bytes, encoding);

    global_env->CallVoidMethod(activity, s_setTab, id, str1, score, pings);

    EXCEPTION_CHECK(global_env);
}

void CJavaWrapper::ShowTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ClearTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_clearTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ShowDialog(int dialogStyle, int dialogID, char* title, char* text, char* button1, char* button2)
{
	JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		FLog("No env");
		return;
	}

	std::string sTitle(title);
	std::string sText(text);
	std::string sButton1(button1);
	std::string sButton2(button2);

	jbyteArray jstrTitle = as_byte_array((unsigned char*)sTitle.c_str(), sTitle.length());
	jbyteArray jstrText = as_byte_array((unsigned char*)sText.c_str(), sText.length());
	jbyteArray jstrButton1 = as_byte_array((unsigned char*)sButton1.c_str(), sButton1.length());
	jbyteArray jstrButton2 = as_byte_array((unsigned char*)sButton2.c_str(), sButton2.length());

	env->CallVoidMethod(activity, s_ShowDialog, dialogID, dialogStyle, jstrTitle, jstrText, jstrButton1, jstrButton2);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::exitGame() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_exitGame);

    EXCEPTION_CHECK(env);   // [v19]
}

void CJavaWrapper::ShowEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_showEditObject);

    EXCEPTION_CHECK(env);   // [v19]
}

void CJavaWrapper::HideEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_hideEditObject);

    EXCEPTION_CHECK(env);   // [v19]
}

// Argent en poche -> HUD HTML (avant, le HUD restait a 0 FCFA : rien ne lui envoyait l'argent)
void CJavaWrapper::UpdateHudMoney(int money)
{
	if (!javaVM || !activity || !s_updateHudMoney) return;
	JNIEnv* env = nullptr;
	javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if (!env) return;
	env->CallVoidMethod(activity, s_updateHudMoney, (jint) money);
	if (env->ExceptionCheck()) env->ExceptionClear();
}

void CJavaWrapper::UpdateHudState(float health, float armour, float thirst, float hunger, float energy,
	bool dead, bool running, bool exhausted, bool inVehicle, float speed, float vehHealth, bool handbrake,
	float posX, float posY, float camFx, float camFy, float heading)
{
	if (!javaVM || !activity || !s_updateHudState) return;
	JNIEnv* env = nullptr;
	javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if (!env) return;
	env->CallVoidMethod(activity, s_updateHudState, health, armour, thirst, hunger, energy,
		(jboolean)dead, (jboolean)running, (jboolean)exhausted, (jboolean)inVehicle, speed, vehHealth, (jboolean)handbrake,
		posX, posY, camFx, camFy, heading);
	EXCEPTION_CHECK(env);
}

static void CallClothesString(CJavaWrapper* w, jmethodID mid, const std::string& json)
{
	if (!javaVM || !w->activity || !mid) return;
	JNIEnv* p = nullptr;
	javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
	if (!p) return;
	jstring js = p->NewStringUTF(json.c_str());
	if (!js) { if (p->ExceptionCheck()) p->ExceptionClear(); return; }
	p->CallVoidMethod(w->activity, mid, js);
	p->DeleteLocalRef(js);
	if (p->ExceptionCheck()) { p->ExceptionDescribe(); p->ExceptionClear(); }
}

void CJavaWrapper::ClothesPanelOpen(const std::string& json)  { CallClothesString(this, s_clothesOpen, json); }
void CJavaWrapper::ClothesPanelState(const std::string& json) { CallClothesString(this, s_clothesState, json); }

void CJavaWrapper::ClothesPanelClose()
{
	if (!javaVM || !activity || !s_clothesClose) return;
	JNIEnv* p = nullptr;
	javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
	if (!p) return;
	p->CallVoidMethod(activity, s_clothesClose);
	if (p->ExceptionCheck()) { p->ExceptionDescribe(); p->ExceptionClear(); }
}

// [Crestwood] chaque ligne du chat (joueurs, serveur, infos) -> chat HTML du HUD (assets/hud)
void CJavaWrapper::HudChatMessage(const std::string& nick, const std::string& text, uint32_t rgb)
{
	if (!javaVM || !activity || !s_hudChat) return;
	JNIEnv* env = nullptr;
	javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if (!env) return;
	jbyteArray jn = env->NewByteArray((jsize)nick.size());
	jbyteArray jt = env->NewByteArray((jsize)text.size());
	if (!jn || !jt) { if (env->ExceptionCheck()) env->ExceptionClear(); return; }
	env->SetByteArrayRegion(jn, 0, (jsize)nick.size(), (const jbyte*)nick.data());
	env->SetByteArrayRegion(jt, 0, (jsize)text.size(), (const jbyte*)text.data());
	env->CallVoidMethod(activity, s_hudChat, jn, jt, (jint)rgb);
	if (env->ExceptionCheck()) env->ExceptionClear();
	env->DeleteLocalRef(jn);
	env->DeleteLocalRef(jt);
}
