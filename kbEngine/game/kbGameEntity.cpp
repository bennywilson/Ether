/// kbGameEntity.cpp
///
/// 2016-2025 blk 1.0
/// 
#include "blk_core.h"
#include "blk_containers.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbBounds.h"
#include "kbGame.h"
#include "kbGameEntityHeader.h"

/// kbEntity::kbEntity
kbEntity::kbEntity() :
	m_bIsDirty(false) {
}

/// kbEntity::PostLoad
void kbEntity::post_load() {
	for (int i = 0; i < m_Components.size(); i++) {
		m_Components[i]->post_load();
	}
}

/// kbEntity::AddComponent
void kbEntity::AddComponent(kbComponent* const pComponent, int indexToInsertAt) {
	pComponent->SetOwner(this);

	const int lastComponentIdx = (int)m_Components.size();
	if (indexToInsertAt == -1) {
		indexToInsertAt = lastComponentIdx;
	}

	if (pComponent->IsA(kbGameLogicComponent::GetType())) {
		indexToInsertAt = lastComponentIdx;
	} else if (m_Components.size() > 0 && indexToInsertAt == lastComponentIdx) {
		while (indexToInsertAt > 0 && m_Components[indexToInsertAt - 1]->IsA(kbGameLogicComponent::GetType())) {
			indexToInsertAt--;
		}
	}

	if (indexToInsertAt < 0 || indexToInsertAt >= m_Components.size()) {
		m_Components.push_back(pComponent);
	} else {
		m_Components.insert(m_Components.begin() + indexToInsertAt, pComponent);
	}
}

/// kbEntity::RemoveComponent
void kbEntity::RemoveComponent(kbComponent* const pComponent) {

	// TODO: This might move the game logic component from the back of the list!
	blk::std_remove_swap(m_Components, pComponent);
}

//===================================================================================================
//	kbGameEntityPtr
//===================================================================================================
bool operator<(const kbGUID& a, const kbGUID& b) {
	return memcmp(&a, &b, sizeof(b)) < 0;
}

struct EntPtrHash {
	size_t operator()(const kbGUID& op) const {
		return std::hash<int>()(op.m_iGuid[0]) ^ std::hash<int>()(op.m_iGuid[1]) ^ std::hash<int>()(op.m_iGuid[2]) ^ std::hash<int>()(op.m_iGuid[3]);
	}
};

std::unordered_map<kbGUID, kbGameEntity*, EntPtrHash> g_GUIDToEntityMap;
std::unordered_map<int, kbGameEntity*> g_IndexToEntityMap;

/// kbGameEntityPtr::SetEntity
void kbGameEntityPtr::SetEntity(const kbGUID& guid) {

	m_GUID = guid;
	m_EntityId = INVALID_ENTITYID;

	std::unordered_map<kbGUID, kbGameEntity*, EntPtrHash>::const_iterator GUIDToEntityIt = g_GUIDToEntityMap.find(m_GUID);
	if (GUIDToEntityIt == g_GUIDToEntityMap.end()) {
		g_GUIDToEntityMap[guid] = nullptr;
	} else {
		if (GUIDToEntityIt->second != nullptr) {
			m_EntityId = GUIDToEntityIt->second->GetEntityId();
		}
	}
}

/// kbGameEntityPtr::SetEntity
void kbGameEntityPtr::SetEntity(kbGameEntity* const pGameEntity) {

	if (pGameEntity == nullptr) {
		m_EntityId = INVALID_ENTITYID;
		ZeroMemory(&m_GUID, sizeof(m_GUID));
		return;
	}

	// Enter into guid map
	m_GUID = pGameEntity->GetGUID();

	if (m_GUID.IsValid()) {
		std::unordered_map<kbGUID, kbGameEntity*, EntPtrHash>::const_iterator GUIDToEntityIt = g_GUIDToEntityMap.find(m_GUID);

		if (GUIDToEntityIt != g_GUIDToEntityMap.cend() && GUIDToEntityIt->second != pGameEntity && GUIDToEntityIt->second != nullptr) {

			blk::error("kbGameEntityPtr::SetEntity() - Entities %s && %s share the same guid - %u %u %u %u",
					  pGameEntity->GetName().c_str(), GUIDToEntityIt->second->GetName().c_str(),
					  m_GUID.m_iGuid[0], m_GUID.m_iGuid[1], m_GUID.m_iGuid[2], m_GUID.m_iGuid[3]);
		}

		g_GUIDToEntityMap[m_GUID] = pGameEntity;
	}

	// Enter into entity index map
	m_EntityId = pGameEntity->GetEntityId();

	std::unordered_map<int, kbGameEntity*>::const_iterator IDToEntityIt = g_IndexToEntityMap.find(m_EntityId);

	if (IDToEntityIt != g_IndexToEntityMap.cend() && IDToEntityIt->second != pGameEntity && IDToEntityIt->second != nullptr) {
		blk::error("kbGameEntityPtr::SetEntity() - Entities %s && %s share the same guid - %u %u %u %u",
				 pGameEntity->GetName().c_str(), IDToEntityIt->second->GetName().c_str(),
				 m_GUID.m_iGuid[0], m_GUID.m_iGuid[1], m_GUID.m_iGuid[2], m_GUID.m_iGuid[3]);
	}

	g_IndexToEntityMap[m_EntityId] = pGameEntity;
}

/// kbGameEntityPtr::GetEntity
kbGameEntity* kbGameEntityPtr::GetEntity() {

	if (m_EntityId != INVALID_ENTITYID) {

		std::unordered_map<int, kbGameEntity*>::const_iterator it = g_IndexToEntityMap.find(m_EntityId);
		if (it != g_IndexToEntityMap.cend()) {
			return it->second;
		}
	}

	std::unordered_map<kbGUID, kbGameEntity*, EntPtrHash>::const_iterator it = g_GUIDToEntityMap.find(m_GUID);
	if (it == g_GUIDToEntityMap.cend()) {
		return nullptr;
	}

	return it->second;
}

const kbGameEntity* kbGameEntityPtr::GetEntity() const {
	kbGameEntityPtr* const pThisNoConst = const_cast<kbGameEntityPtr*>(this);
	return pThisNoConst->GetEntity();
}

/// kbGameEntityPtr::GetGUID
kbGUID kbGameEntityPtr::GetGUID() const {

	return m_GUID;
}

//===================================================================================================
//	kbGameEntity
//===================================================================================================

uint g_EntityNumber = 0;

/// kbGameEntity
kbGameEntity::kbGameEntity(const kbGUID* const guid, const bool bIsPrefab) :
	m_Bounds(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f)),
	m_pActorComponent(nullptr),
	m_pOwnerEntity(nullptr),
	m_EntityId(g_EntityNumber++),
	m_bIsPrefab(bIsPrefab),
	m_bDeleteWhenComponentsAreInactive(false) {

	m_pTransformComponent = new kbTransformComponent();
	m_pTransformComponent->SetPosition(Vec3::zero);
	AddComponent(m_pTransformComponent);

	if (bIsPrefab == false) {
		const std::string newName = "Entity_" + std::to_string(m_EntityId);
		m_pTransformComponent->SetName(newName.c_str());

		if (guid != nullptr) {
			m_GUID = *guid;
		}
	} else {
		m_pTransformComponent->SetName("Prefab");

		if (guid != nullptr) {
			m_GUID = *guid;
		} else if (g_UseEditor == true) {
			CoCreateGuid(&m_GUID.m_Guid);
		} else {
			blk::error("kbGameEntity::kbGameEntity() - Prefab created with invalid GUID");
		}
	}

	if (m_GUID.IsValid() == false) {
		CoCreateGuid(&m_GUID.m_Guid);
	}

	kbGameEntityPtr entityPtr;
	entityPtr.SetEntity(this);
}

/// kbGameEntity::kbGameEntity( const kbGameEntity * )
kbGameEntity::kbGameEntity(const kbGameEntity* pGameEntity, const bool bIsPrefab, const kbGUID* const guid) :
	m_Bounds(pGameEntity->GetBounds()),
	m_pActorComponent(nullptr),
	m_pOwnerEntity(nullptr),
	m_EntityId(g_EntityNumber++),
	m_bIsPrefab(bIsPrefab),
	m_bDeleteWhenComponentsAreInactive(false) {

	for (int i = 0; i < pGameEntity->m_Components.size(); i++) {
		const kbTypeInfoClass* const pTypeInfoClass = g_NameToTypeInfoMap->GetTypeInfoFromClassName(pGameEntity->m_Components[i]->GetComponentClassName());
		kbComponent* newComponent = pTypeInfoClass->ConstructInstance(pGameEntity->m_Components[i]);
		AddComponent(newComponent);

		if (i == 0) {
			m_pTransformComponent = (kbTransformComponent*)newComponent;
			if (m_pTransformComponent->IsA(kbTransformComponent::GetType()) == false) {
				blk::error("kbGameEntity::kbGameEntity() - Somehow the first component is not the transform component");
			}
		}
	}

	if (m_bIsPrefab == false) {
		for (int i = 0; i < m_Components.size(); i++) {
			if (pGameEntity->m_Components[i]->IsEnabled()) {
				m_Components[i]->Enable(false);
				m_Components[i]->Enable(true);
			} else {
				m_Components[i]->Enable(false);
			}
		}
	}

	if (bIsPrefab == false) {
		if (guid != nullptr) {
			m_GUID = *guid;
		}
	} else {
		if (guid != nullptr) {
			m_GUID = *guid;
		} else if (g_UseEditor == true) {
			CoCreateGuid(&m_GUID.m_Guid);
		} else {
			blk::error("kbGameEntity::kbGameEntity() - Prefab created with invalid GUID");
		}
	}

	if (m_GUID.IsValid() == false) {
		CoCreateGuid(&m_GUID.m_Guid);
	}

	kbGameEntityPtr entityPtr;
	entityPtr.SetEntity(this);
}

/// kbGameEntity::~kbGameEntity
kbGameEntity::~kbGameEntity() {

	// Disable components first so they can clean up any cross references before destruction
	for (int i = 0; i < m_Components.size(); i++) {
		m_Components[i]->Enable(false);
	}

	for (int i = 0; i < m_Components.size(); i++) {
		delete m_Components[i];
	}

	for (int i = 0; i < m_ChildEntities.size(); i++) {
		delete m_ChildEntities[i];
	}

	if (m_GUID.IsValid()) {
		g_GUIDToEntityMap.erase(m_GUID);
	}

	blk::error_check(m_EntityId != INVALID_ENTITYID, "kbGameEntity::~kbGameEntity() - Destroying entity with an invalid entity id");
	g_IndexToEntityMap.erase(m_EntityId);
}

/// kbGameEntity::AddComponent
void kbGameEntity::AddComponent(kbComponent* const pComponent, int indexToInsertAt) {

	if (pComponent == nullptr || pComponent->IsA(kbGameComponent::GetType()) == false) {
		blk::error("%s is trying to add a null component or one that is not a kbGameComponent.", GetName().c_str());
	}

	if (pComponent->IsA(kbActorComponent::GetType())) {
		if (m_pActorComponent != nullptr) {
			blk::error("%s is trying to add multiple kbGameLogicComponent.", GetName().c_str());
			return;
		}

		m_pActorComponent = static_cast<kbActorComponent*>(pComponent);
	}

	kbEntity::AddComponent(pComponent, indexToInsertAt);
}

/// kbGameEntity::AddEntity
void kbGameEntity::AddEntity(kbGameEntity* const pEntity) {
	pEntity->m_pOwnerEntity = this;
	m_ChildEntities.push_back(pEntity);

	// Make sure pEntity is not in kbGame's list as it will now be managed by this
	g_pGame->RemoveGameEntity(pEntity);
}

/// kbGameEntity::Update
void kbGameEntity::Update(const float DeltaTime) {
	START_SCOPED_TIMER(GAME_ENTITY_UPDATE)

	{
		START_SCOPED_TIMER(COMPONENT_UPDATE)

			for (int i = 0; i < m_Components.size(); i++) {
				// todo: make sure entity is still valid before updating the next component (ex. projectile may have removed the entity)
				if (GetComponent(i)->IsEnabled()) {
					GetComponent(i)->Update(DeltaTime);
				}
			}
	}

	for (int i = 0; i < m_ChildEntities.size(); i++) {
		m_ChildEntities[i]->Update(DeltaTime);
	}

	if (m_bDeleteWhenComponentsAreInactive) {
		bool bActiveComponentsExist = false;
		for (int i = 1; i < m_Components.size(); i++) {
			if (m_Components[i]->IsEnabled()) {
				bActiveComponentsExist = true;
				break;
			}
		}

		if (bActiveComponentsExist == false) {
			g_pGame->RemoveGameEntity(this);
			return;
		}
	}

	ClearDirty();
}

/// kbGameEntity::EnableAllComponents
void kbGameEntity::EnableAllComponents() {
	for (int i = 0; i < m_Components.size(); i++) {
		m_Components[i]->Enable(true);
	}
}

/// kbGameEntity::DisableAllComponents
void kbGameEntity::DisableAllComponents() {
	for (int i = 0; i < m_Components.size(); i++) {
		m_Components[i]->Enable(false);
	}
}

/// kbGameEntity::RenderSync
void kbGameEntity::RenderSync() {

	for (int i = 0; i < m_Components.size(); i++) {
		m_Components[i]->RenderSync();
	}

	for (int i = 0; i < m_ChildEntities.size(); i++) {
		m_ChildEntities[i]->RenderSync();
	}
}

/// kbGameEntity::CalculateWorldMatrix
void kbGameEntity::CalculateWorldMatrix(Mat4& inOutMatrix) const {

	Mat4 scaleMat(Mat4::identity);

	const float modelScale = kbLevelComponent::GetGlobalModelScale();
	scaleMat[0].x = GetScale().x * modelScale;
	scaleMat[1].y = GetScale().y * modelScale;
	scaleMat[2].z = GetScale().z * modelScale;

	inOutMatrix = scaleMat * GetOrientation().to_mat4();
	inOutMatrix[3] = GetPosition();
	inOutMatrix[3].w = 1.0f;
}

/// kbGameEntity::GetWorldBounds
kbBounds kbGameEntity::GetWorldBounds() const {
	kbBounds returnBounds = m_Bounds;
	returnBounds.Scale(GetScale());
	returnBounds.Translate(GetPosition());
	return returnBounds;
}

/// kbGameEntity::GetOrientation
const Quat4 kbGameEntity::GetOrientation() const {
	if (m_pOwnerEntity != nullptr) {
		// This entity's orientation is in model space while the parent's is in world
		return  m_pTransformComponent->GetOrientation() * m_pOwnerEntity->GetOrientation();
	}

	return m_pTransformComponent->GetOrientation();
}

/// kbGameEntity::GetPosition
const Vec3 kbGameEntity::GetPosition() const {
	if (m_pOwnerEntity != nullptr) {
		const Quat4 entityOrientation = GetOrientation();
		const Vec3 worldSpaceOffset = entityOrientation.to_mat4().transform_point(m_pTransformComponent->GetPosition());
		return worldSpaceOffset + m_pOwnerEntity->GetPosition();
	}

	return m_pTransformComponent->GetPosition();
}

/// kbGameEntity::GetComponentByType
kbComponent* kbGameEntity::GetComponentByType(const void* const pTypeInfoClass) const {
	if (pTypeInfoClass == nullptr) {
		return nullptr;
	}

	for (int i = 0; i < m_Components.size(); i++) {
		if (m_Components[i]->IsA(pTypeInfoClass)) {
			return m_Components[i];
		}
	}

	return nullptr;
}
