#if defined _geckoju_included
    #endinput
#endif
#define _geckoju_included

#define GECKOJU_VERSION             "1.0.0"
#define GECKOJU_PACKET_ID           (0xFA)

#define GECKOJU_POS_CENTER          (-1)
#define GECKOJU_POS_END             (-2)
#define GECKOJU_SIZE_FULL           (0)

#define GECKOJU_EVENT_LOADED        (101)
#define GECKOJU_EVENT_URL_CHANGED   (102)
#define GECKOJU_EVENT_TITLE_CHANGED (103)
#define GECKOJU_EVENT_CONSOLE       (104)
#define GECKOJU_EVENT_ERROR         (105)
#define GECKOJU_EVENT_CLICK         (106)
#define GECKOJU_EVENT_LOADING_START (107)
#define GECKOJU_EVENT_LOADING_END   (108)
#define GECKOJU_EVENT_BACK_PRESSED  (109)
#define GECKOJU_EVENT_CUSTOM        (110)

#define GECKOJU_CMD_CREATE          (1)
#define GECKOJU_CMD_DESTROY         (2)
#define GECKOJU_CMD_LOAD_URL        (3)
#define GECKOJU_CMD_RELOAD          (4)
#define GECKOJU_CMD_STOP            (5)
#define GECKOJU_CMD_EVAL_JS         (6)
#define GECKOJU_CMD_SET_BOUNDS      (7)
#define GECKOJU_CMD_SET_VISIBLE     (8)
#define GECKOJU_CMD_SET_FOCUS       (9)
#define GECKOJU_CMD_SET_INTERACTIVE (18)
#define GECKOJU_CMD_CUSTOM          (100)

// Platform identifiers
#define GECKOJU_PLATFORM_UNKNOWN    (0)
#define GECKOJU_PLATFORM_MOBILE     (1)  // Android GeckoView
#define GECKOJU_PLATFORM_PC         (2)  // PC CEF (original SA-MP)

forward OnGeckojuEvent(playerid, browserid, eventType, const data[]);
forward OnGeckojuReady(playerid);

static bool:geckoju_PlayerReady[MAX_PLAYERS];
static geckoju_PlayerPlatform[MAX_PLAYERS]; // 0=unknown, 1=mobile, 2=pc

stock Geckoju_CreateBrowser(playerid, browserid, x, y, width, height, const url[], bool:visible = true, bool:showExitButton = false) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_CREATE,
        PR_INT32, browserid,
        PR_INT32, x,
        PR_INT32, y,
        PR_INT32, width,
        PR_INT32, height,
        PR_UINT8, visible ? 1 : 0,
        PR_UINT8, showExitButton ? 1 : 0,
        PR_STRING8, url
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_DestroyBrowser(playerid, browserid) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_DESTROY,
        PR_INT32, browserid
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_LoadUrl(playerid, browserid, const url[]) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_LOAD_URL,
        PR_INT32, browserid,
        PR_STRING8, url
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_Reload(playerid, browserid) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_RELOAD,
        PR_INT32, browserid
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_Stop(playerid, browserid) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_STOP,
        PR_INT32, browserid
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_ExecuteJS(playerid, browserid, const script[]) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_EVAL_JS,
        PR_INT32, browserid,
        PR_STRING8, script
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_SetBounds(playerid, browserid, x, y, width, height) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_SET_BOUNDS,
        PR_INT32, browserid,
        PR_INT32, x,
        PR_INT32, y,
        PR_INT32, width,
        PR_INT32, height
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_SetVisible(playerid, browserid, bool:visible) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_SET_VISIBLE,
        PR_INT32, browserid,
        PR_UINT8, visible ? 1 : 0
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_SetInteractive(playerid, browserid, bool:interactive) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_SET_INTERACTIVE,
        PR_INT32, browserid,
        PR_UINT8, interactive ? 1 : 0
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Geckoju_SetFocus(playerid, browserid, bool:focused) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_SET_FOCUS,
        PR_INT32, browserid,
        PR_UINT8, focused ? 1 : 0
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

// Geckoju_SetBgAlpha removed - background transparency is now handled via HTML/CSS
// Use transparent background in your web content: body { background: transparent; }

stock Geckoju_SendData(playerid, browserid, const data[]) {
    if (!IsPlayerConnected(playerid)) return 0;
    
    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, GECKOJU_PACKET_ID,
        PR_UINT8, GECKOJU_CMD_CUSTOM,
        PR_INT32, browserid,
        PR_STRING8, data
    );
    BS_Send(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock bool:Geckoju_IsPlayerReady(playerid) {
    if (!IsPlayerConnected(playerid)) return false;
    return geckoju_PlayerReady[playerid];
}

/**
 * Check if player is using mobile client (Android GeckoView)
 * @return true if mobile, false if PC or unknown
 */
stock bool:Geckoju_IsMobileClient(playerid) {
    if (!IsPlayerConnected(playerid)) return false;
    return geckoju_PlayerPlatform[playerid] == GECKOJU_PLATFORM_MOBILE;
}

/**
 * Check if player is using PC client (original SA-MP CEF)
 * @return true if PC, false if mobile or unknown
 */
stock bool:Geckoju_IsPCClient(playerid) {
    if (!IsPlayerConnected(playerid)) return false;
    return geckoju_PlayerPlatform[playerid] == GECKOJU_PLATFORM_PC;
}

/**
 * Get player's platform type
 * @return GECKOJU_PLATFORM_UNKNOWN, GECKOJU_PLATFORM_MOBILE, or GECKOJU_PLATFORM_PC
 */
stock Geckoju_GetPlayerPlatform(playerid) {
    if (!IsPlayerConnected(playerid)) return GECKOJU_PLATFORM_UNKNOWN;
    return geckoju_PlayerPlatform[playerid];
}

stock Geckoju_Internal_OnConnect(playerid) {
    geckoju_PlayerReady[playerid] = false;
    geckoju_PlayerPlatform[playerid] = GECKOJU_PLATFORM_UNKNOWN;
}

stock Geckoju_Internal_OnDisconnect(playerid) {
    geckoju_PlayerReady[playerid] = false;
    geckoju_PlayerPlatform[playerid] = GECKOJU_PLATFORM_UNKNOWN;
}

stock Geckoju_Internal_ProcessPacket(playerid, BitStream:bs) {
    new packetId;
    BS_ReadValue(bs, PR_UINT8, packetId);
    
    if (packetId != GECKOJU_PACKET_ID) return 0;
    
    new eventType;
    BS_ReadValue(bs, PR_UINT8, eventType);
    
    // Event type 0 = Ready signal with platform info
    if (eventType == 0) {
        // Read platform identifier (mobile sends "MOBILE", PC would send "PC")
        new browserid;
        BS_ReadValue(bs, PR_INT32, browserid);
        
        new platformStr[16];
        BS_ReadValue(bs, PR_STRING8, platformStr, sizeof(platformStr));
        
        // Determine platform from string
        if (strcmp(platformStr, "MOBILE", true) == 0) {
            geckoju_PlayerPlatform[playerid] = GECKOJU_PLATFORM_MOBILE;
        } else if (strcmp(platformStr, "PC", true) == 0) {
            geckoju_PlayerPlatform[playerid] = GECKOJU_PLATFORM_PC;
        } else {
            geckoju_PlayerPlatform[playerid] = GECKOJU_PLATFORM_UNKNOWN;
        }
        
        geckoju_PlayerReady[playerid] = true;
        CallLocalFunction("OnGeckojuReady", "i", playerid);
        return 1;
    }
    
    new browserid;
    BS_ReadValue(bs, PR_INT32, browserid);
    
    new data[256];
    BS_ReadValue(bs, PR_STRING8, data, sizeof(data));
    
    CallLocalFunction("OnGeckojuEvent", "iiis", playerid, browserid, eventType, data);
    return 1;
}

#if defined FILTERSCRIPT
    public OnFilterScriptInit() {
        #if defined Geckoju_OnFilterScriptInit
            Geckoju_OnFilterScriptInit();
        #endif
        return 1;
    }
    #if defined _ALS_OnFilterScriptInit
        #undef OnFilterScriptInit
    #else
        #define _ALS_OnFilterScriptInit
    #endif
    #define OnFilterScriptInit Geckoju_OnFilterScriptInit
    #if defined Geckoju_OnFilterScriptInit
        forward Geckoju_OnFilterScriptInit();
    #endif
#else
    public OnGameModeInit() {
        #if defined Geckoju_OnGameModeInit
            Geckoju_OnGameModeInit();
        #endif
        return 1;
    }
    #if defined _ALS_OnGameModeInit
        #undef OnGameModeInit
    #else
        #define _ALS_OnGameModeInit
    #endif
    #define OnGameModeInit Geckoju_OnGameModeInit
    #if defined Geckoju_OnGameModeInit
        forward Geckoju_OnGameModeInit();
    #endif
#endif

public OnPlayerConnect(playerid) {
    Geckoju_Internal_OnConnect(playerid);
    #if defined Geckoju_OnPlayerConnect
        return Geckoju_OnPlayerConnect(playerid);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerConnect
    #undef OnPlayerConnect
#else
    #define _ALS_OnPlayerConnect
#endif
#define OnPlayerConnect Geckoju_OnPlayerConnect
#if defined Geckoju_OnPlayerConnect
    forward Geckoju_OnPlayerConnect(playerid);
#endif

public OnPlayerDisconnect(playerid, reason) {
    Geckoju_Internal_OnDisconnect(playerid);
    #if defined Geckoju_OnPlayerDisconnect
        return Geckoju_OnPlayerDisconnect(playerid, reason);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerDisconnect
    #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect Geckoju_OnPlayerDisconnect
#if defined Geckoju_OnPlayerDisconnect
    forward Geckoju_OnPlayerDisconnect(playerid, reason);
#endif

public OnIncomingPacket(playerid, packetid, BitStream:bs) {
    if (packetid == GECKOJU_PACKET_ID) {
        Geckoju_Internal_ProcessPacket(playerid, bs);
        return 0;
    }
    #if defined Geckoju_OnIncomingPacket
        return Geckoju_OnIncomingPacket(playerid, packetid, bs);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnIncomingPacket
    #undef OnIncomingPacket
#else
    #define _ALS_OnIncomingPacket
#endif
#define OnIncomingPacket Geckoju_OnIncomingPacket
#if defined Geckoju_OnIncomingPacket
    forward Geckoju_OnIncomingPacket(playerid, packetid, BitStream:bs);
#endif
