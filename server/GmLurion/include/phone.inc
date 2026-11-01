#if defined PHONE_SYNC_INC_
    #endinput
#endif
#define PHONE_SYNC_INC_

#if !defined PAWNRAKNET_INC_
    #error "Pawn.RakNet not found. Include Pawn.RakNet first."
#endif

#define PHONE_PACKET_ID (0xFC)
#define PHONE_CMD_SHOW (1)
#define PHONE_CMD_HIDE (2)

stock Phone_Show(playerid)
{
    if (!IsPlayerConnected(playerid)) return 0;

    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, PHONE_PACKET_ID,
        PR_UINT8, PHONE_CMD_SHOW
    );
    PR_SendPacket(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}

stock Phone_Hide(playerid)
{
    if (!IsPlayerConnected(playerid)) return 0;

    new BitStream:bs = BS_New();
    BS_WriteValue(bs,
        PR_UINT8, PHONE_PACKET_ID,
        PR_UINT8, PHONE_CMD_HIDE
    );
    PR_SendPacket(bs, playerid, PR_HIGH_PRIORITY, PR_RELIABLE_ORDERED);
    BS_Delete(bs);
    return 1;
}
