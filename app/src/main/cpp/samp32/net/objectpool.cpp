#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include <unordered_map>

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
	if (m_pObjects[ObjectID] != nullptr) {
		Delete(ObjectID);
	}

	m_pObjects[ObjectID] = pGame->NewObject(iModel, vecPos, vecRot, fDrawDistance);
	if (!m_pObjects[ObjectID]) return false;

	m_bObjectSlotState[ObjectID] = true;
	return true;
}

bool CObjectPool::Delete(OBJECTID ObjectID)
{
	if (ObjectID < MAX_OBJECTS && m_bObjectSlotState[ObjectID])
	{
		CObject* pObject = m_pObjects[ObjectID];
		if (pObject)
		{
			delete m_pObjects[ObjectID];
			m_pObjects[ObjectID] = nullptr;
			m_bObjectSlotState[ObjectID] = false;
		}
	}

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
		if (m_bObjectSlotState[i]) {
			m_pObjects[i]->Process(fElapsedTime);
		}
	}

	s_dwLastTick = dwThisTick;
}

CObject* CObjectPool::FindObjectFromGtaPtr(CPhysical* pGtaObject)
{
	for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
	{
		if (m_pObjects[ObjectID] && m_pObjects[ObjectID]->m_pEntity && m_pObjects[ObjectID]->m_pEntity == pGtaObject)
			return m_pObjects[ObjectID];
	}

	return nullptr;
}

OBJECTID CObjectPool::FindIDFromGtaPtr(CPhysical* pGtaObject)
{
	for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
	{
		if (m_pObjects[ObjectID] && m_pObjects[ObjectID]->m_pEntity && m_pObjects[ObjectID]->m_pEntity == pGtaObject)
			return ObjectID;
	}

	return INVALID_OBJECT_ID;
}

void CObjectPool::ProcessMaterialText()
{
	for (OBJECTID ObjectID = 0; ObjectID < MAX_OBJECTS; ObjectID++)
	{
		if (m_pObjects[ObjectID] && m_bObjectSlotState[ObjectID] == true)
		{
			m_pObjects[ObjectID]->ProcessMaterialText();
		}
	}
} 

// [v51] OPTI : le rendu de chaque objet (CObject_Render_hook) cherchait l'objet SA-MP en parcourant les
// 1000 emplacements, a chaque image. Ici : cache pointeur GTA -> id, verifie en O(1) a chaque appel ;
// les entites qui ne sont pas des objets SA-MP sont memorisees 500 ms.
CObject* CObjectPool::FindObjectFromGtaPtrFast(const void* pGtaObject)
{
	struct Entry { uint16_t id; uint32_t until; };
	static std::unordered_map<const void*, Entry> s_cache;
	if (!pGtaObject) return nullptr;
	const uint32_t now = GetTickCount();
	auto it = s_cache.find(pGtaObject);
	if (it != s_cache.end()) {
		const Entry& e = it->second;
		if (e.id != 0xFFFF) {
			if (e.id < MAX_OBJECTS && m_bObjectSlotState[e.id] && m_pObjects[e.id] && (const void*) m_pObjects[e.id]->m_pEntity == pGtaObject)
				return m_pObjects[e.id];
		} else if ((int32_t)(e.until - now) > 0) {
			return nullptr;
		}
	}
	if (s_cache.size() > 8192) s_cache.clear();
	for (OBJECTID id = 0; id < MAX_OBJECTS; id++) {
		if (m_bObjectSlotState[id] && m_pObjects[id] && (const void*) m_pObjects[id]->m_pEntity == pGtaObject) {
			s_cache[pGtaObject] = { (uint16_t) id, 0 };
			return m_pObjects[id];
		}
	}
	s_cache[pGtaObject] = { 0xFFFF, now + 500 };
	return nullptr;
}
