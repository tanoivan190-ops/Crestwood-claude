#pragma once

#define INVALID_OBJECT_ID	0xFFFF

class CObjectPool
{
public:
    CObjectPool();
    ~CObjectPool();

    bool New(OBJECTID ObjectID, int iModel, CVector vecPos, CVector vecRot, float fDrawDistance);
    bool Delete(OBJECTID ObjectID);

    void Process();

    // 0.3.7
    CObject* GetAt(OBJECTID ObjectID) {
        // MAX_OBJECTS is a count: valid indexes are 0..MAX_OBJECTS-1.
        if (ObjectID < MAX_OBJECTS && m_bObjectSlotState[ObjectID])
            return m_pObjects[ObjectID];

        return nullptr;
    }

    CObject *GetObjectFromGtaPtr(CEntityGTA *pGtaObject)
    {
        for (OBJECTID x = 0; x < MAX_OBJECTS; ++x)
        {
            if (m_bObjectSlotState[x] && m_pObjects[x] && m_pObjects[x]->m_pEntity == pGtaObject)
                return m_pObjects[x];
        }

        return nullptr;
    }

    OBJECTID FindIDFromGtaPtr(CPhysical* pGtaObject);

    CObject* FindObjectFromGtaPtr(CPhysical* pGtaObject);

    void ProcessMaterialText();
    int GetCount() { return m_iObjectCount; }

private:
    int			m_iObjectCount;
    bool		m_bObjectSlotState[MAX_OBJECTS];
    CObject*	m_pObjects[MAX_OBJECTS];
};