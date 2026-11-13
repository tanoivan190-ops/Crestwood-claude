#include "../main.h"
#include "../game/game.h"
#include "netgame.h"

extern CGame *pGame;

// 0.3.7
CObjectPool::CObjectPool()
{
    for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
    {
        m_bObjectSlotState[ObjectID] = false;
        m_pObjects[ObjectID] = nullptr;
    }

    m_iObjectCount = 0;
}
// 0.3.7
CObjectPool::~CObjectPool()
{
    for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
    {
        Delete(ObjectID);
    }
}

bool CObjectPool::New(OBJECTID ObjectID, int iModel, CVector vecPos, CVector vecRot, float fDrawDistance)
{
    // Never index the fixed pool with INVALID_OBJECT_ID or an out-of-range server ID.
    if (ObjectID >= MAX_OBJECTS || !pGame)
        return false;

    if (m_pObjects[ObjectID] != nullptr)
        Delete(ObjectID);

    CObject* pNewObject = pGame->NewObject(iModel, vecPos, vecRot, fDrawDistance);
    // CObject can be allocated even when GTA rejected the model/entity creation.
    // Do not register such a half-created object: Process() would later dereference it.
    if (!pNewObject || !pNewObject->m_pEntity) {
        delete pNewObject;
        m_pObjects[ObjectID] = nullptr;
        m_bObjectSlotState[ObjectID] = false;
        return false;
    }

    m_pObjects[ObjectID] = pNewObject;
    m_bObjectSlotState[ObjectID] = true;
    return true;
}

bool CObjectPool::Delete(OBJECTID ObjectID)
{
    if (ObjectID >= MAX_OBJECTS)
        return false;

    if (m_pObjects[ObjectID]) {
        delete m_pObjects[ObjectID];
        m_pObjects[ObjectID] = nullptr;
    }
    m_bObjectSlotState[ObjectID] = false;
    return true;
}

void CObjectPool::Process()
{
    static uint32_t s_dwLastTick = 0;

    if (s_dwLastTick == 0) {
        s_dwLastTick = GetTickCount();
    }

    uint32_t dwThisTick = GetTickCount();
    float fElapsedTime = (dwThisTick - s_dwLastTick) / 1000.0f;

    for (OBJECTID i = 0; i < MAX_OBJECTS; i++)
    {
        if (m_bObjectSlotState[i] && m_pObjects[i] && m_pObjects[i]->m_pEntity) {
            m_pObjects[i]->Process(fElapsedTime);
        }
    }

    s_dwLastTick = dwThisTick;
}

CObject* CObjectPool::FindObjectFromGtaPtr(CPhysical* pGtaObject)
{
    for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
    {
        if (m_bObjectSlotState[ObjectID] && m_pObjects[ObjectID] && m_pObjects[ObjectID]->m_pEntity && m_pObjects[ObjectID]->m_pEntity == pGtaObject)
            return m_pObjects[ObjectID];
    }

    return nullptr;
}

OBJECTID CObjectPool::FindIDFromGtaPtr(CPhysical* pGtaObject)
{
    for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
    {
        if (m_bObjectSlotState[ObjectID] && m_pObjects[ObjectID] && m_pObjects[ObjectID]->m_pEntity && m_pObjects[ObjectID]->m_pEntity == pGtaObject)
            return ObjectID;
    }

    return INVALID_OBJECT_ID;
}

void CObjectPool::ProcessMaterialText()
{
    for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
    {
        if (m_pObjects[ObjectID] && m_bObjectSlotState[ObjectID] && m_pObjects[ObjectID]->m_pEntity)
        {
            m_pObjects[ObjectID]->ProcessMaterialText();
        }
    }
}