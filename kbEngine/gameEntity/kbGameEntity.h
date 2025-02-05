/// kbGameEntity.cpp
///
/// 2016-2025 blk 1.0

#pragma once

#define INVALID_ENTITYID UINT_MAX

bool operator<(const kbGUID& a, const kbGUID& b);

/// kbGameEntityPtr - All entities have a unique m_EntityId per game instance.  Entities loaded from disk (ex. from a .kblevel or .kbPkg file) will have m_GUID set
class kbGameEntityPtr {
public:
	kbGameEntityPtr() : m_EntityId(INVALID_ENTITYID) { }

	bool operator==(const kbGameEntityPtr op2) const { return m_EntityId == op2.m_EntityId; }

	void SetEntity(const kbGUID& guid);
	void SetEntity(kbGameEntity* const pGameEntity);
	kbGameEntity* GetEntity();
	const kbGameEntity* GetEntity() const;

	kbGUID GetGUID() const;

	int	GetEntityIndex() const { return m_EntityId; }

private:

	kbGUID m_GUID;
	uint m_EntityId;
};

/// kbEntity
class kbEntity {
public:
	kbEntity();
	virtual	~kbEntity() { }

	void PostLoad();

	virtual void AddComponent(kbComponent* const pComponent, int indexToInsertAt = -1);
	virtual void RemoveComponent(kbComponent* const pComponent);

	kbComponent* GetComponent(const size_t index) const { return m_Components[index]; }
	size_t NumComponents() const { return m_Components.size(); }

	const kbGUID& GetGUID() const { return m_GUID; }

	void MarkAsDirty() { m_bIsDirty = true; for (int i = 0; i < m_Components.size(); i++) { m_Components[i]->MarkAsDirty(); } }
	bool IsDirty() const { return m_bIsDirty; }

protected:
	void ClearDirty() { m_bIsDirty = false; }

	std::vector<kbComponent*> m_Components;

	// Entities that came from file (level, package, etc) have a GUID
	kbGUID m_GUID;

private:
	bool m_bIsDirty : 1;
};

/// kbGameEntity - kbGameEntities can only have kbGameComponents in their m_Components list
class kbGameEntity : public kbEntity {
public:

	explicit kbGameEntity(const kbGUID* const guid = nullptr, const bool bIsPrefab = false);
	explicit kbGameEntity(const kbGameEntity* const, const bool bIsPrefab, const kbGUID* const guid = nullptr);

	virtual	~kbGameEntity();
  
	void AddEntity(kbGameEntity* const pEntity);
	virtual void AddComponent(kbComponent* const pComponent, int indexToInsertAt = -1) override;
	kbGameComponent* GetComponent(const size_t index) const { return (kbGameComponent*)m_Components[index]; }

	// Updates
	void Update(const float DeltaTime);
		 
	void EnableAllComponents();
	void DisableAllComponents();
		 
	void RenderSync();

	// Accessors
	const kbString& GetName() const { return m_pTransformComponent->GetName(); }

	const kbVec3 GetPosition() const;
	void SetPosition(const kbVec3& newPosition) { m_pTransformComponent->SetPosition(newPosition); MarkAsDirty(); }

	const kbQuat GetOrientation() const;
	void SetOrientation(const kbQuat& newOrientation) { m_pTransformComponent->SetOrientation(newOrientation); MarkAsDirty(); }

	const kbVec3 GetScale() const { return m_pTransformComponent->GetScale(); }
	void SetScale(const kbVec3& newScale) { m_pTransformComponent->SetScale(newScale); MarkAsDirty(); }

	void CalculateWorldMatrix(kbMat4& worldMatrix) const;

	const kbBounds& GetBounds() const { return m_Bounds; }
	kbBounds GetWorldBounds() const;

	bool IsPrefab() const { return m_bIsPrefab; }

	void DeleteWhenComponentsAreInactive(const bool bDelete) { m_bDeleteWhenComponentsAreInactive = bDelete; }

	kbGameEntity* GetOwner() const { return m_pOwnerEntity; }

	kbActorComponent* GetActorComponent() const { return m_pActorComponent; }
	kbComponent* GetComponentByType(const void* const pTypeInfoClass) const;

	template<typename T>
	T* GetComponent() const {
		for (int i = 0; i < m_Components.size(); i++) {
			if (m_Components[i]->IsA(T::GetType())) {
				return (T*)m_Components[i];
			}
		}
		return nullptr;
	}

	const std::vector<kbGameEntity*>& GetChildEntities() const { return m_ChildEntities; }

	const uint GetEntityId() const { return m_EntityId; }

private:
	kbBounds m_Bounds;

	kbTransformComponent* m_pTransformComponent;		// For convenience.  This is always the first entry in the m_Components list
	kbActorComponent* m_pActorComponent;			// Only one kbActorComponent is allowed per kbGameEntity
	std::vector<kbGameEntity*> m_ChildEntities;
	kbGameEntity* m_pOwnerEntity;

	// All entities will have a m_EntityId.  They're temporary values that may differ between game instances
	uint m_EntityId;

	bool m_bIsPrefab : 1;
	bool m_bDeleteWhenComponentsAreInactive : 1;
};

/// kbPrefab
class kbPrefab {
	friend class kbEditor;
	friend class kbResourceManager;
	friend class kbFile;

public:
	~kbPrefab() { }

	const std::string& GetPrefabName() const { return m_PrefabName; }
	const size_t NumGameEntities() const { return m_GameEntities.size(); }
	const kbGameEntity* GetGameEntity(const int idx) const { return m_GameEntities[idx]; }

private:
	kbPrefab() { }

	UUID m_GUID;

	std::string	m_PrefabName;
	std::vector<kbGameEntity*> m_GameEntities;
};
