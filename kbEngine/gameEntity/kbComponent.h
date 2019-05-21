//===================================================================================================
// kbComponent.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBCOMPONENT_H_
#define _KBCOMPONENT_H_

class kbGameEntity;

/**
 *	kbBaseComponent - Exists as the end point for children calling CollectAncestorTypeInfo() up their hierarchy
 *					- Each child has a static m_TypeInfo which contains their own typeInfo as well as their ancestors'
 *					- Should never be used as a base class pointer.  Use kbComponent* instead
 */
class kbBaseComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	virtual										~kbBaseComponent() = 0 { }

	// Hack: use a void * instead of kbTypeInfoClass to work around mismatched type compile warning when passing in a type declared in the game project (as opposed to the
	// engine project).
	virtual	bool								IsA( const void *const type ) const { return false; }

protected:
	virtual void								CollectAncestorTypeInfo_Internal( std::vector< class kbTypeInfoClass * > & collection ) { }
};

/**
 *	kbComponent - Derived classes should provide default init values in their constructors and do actual initializations in their Initialize() function
 *			    - A derived class' Initialize() function does NOT need to call their parent's Initialize()
 */
class kbComponent : public kbBaseComponent {

	friend class kbEntity;

	KB_DECLARE_COMPONENT( kbComponent, kbBaseComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual										~kbComponent() { SetEnable_Internal( false ); }

	virtual void								Enable( const bool setEnabled ) { }
	bool										IsEnabled() const { return m_IsEnabled; }

	// Called during level load after the owning entity is fully loaded
	virtual void								PostLoad() { }

	virtual void								EditorChange( const std::string & propertyName ) { }

	virtual void								RenderSync() { }

	kbEntity *									GetOwner() const { return m_pOwner; }

	void										MarkAsDirty() { m_bIsDirty = true; }

	void										SetOwner( kbEntity *const pGameEntity );

	// TODO: This is very hacky
	void										SetOwningComponent( kbComponent *const pOwningComponent ) { m_pOwningComponent = pOwningComponent; }

protected:

	virtual void								SetEnable_Internal( const bool bIsEnabled ) { }
	virtual void								Update_Internal( const float DeltaTimeSeconds ) { }
	virtual void								LifeTimeExpired() { }

	kbComponent *								GetOwningComponent() const { return m_pOwningComponent; }
	bool										IsDirty() const { return m_bIsDirty; }

	bool										m_bIsDirty;
	bool										m_IsEnabled;

private:

	kbEntity *									m_pOwner;
	kbComponent *								m_pOwningComponent;
};

/**
 *	kbGameComponent
 */
class kbGameComponent : public kbComponent {

	KB_DECLARE_COMPONENT( kbGameComponent, kbComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void								Enable( const bool setEnabled ) override;
	virtual void								EditorChange( const std::string & propertyName );

	void										Update( const float DeltaTimeSeconds );

	kbGameEntity *								GetOwner() const { return (kbGameEntity *) Super::GetOwner(); }
	kbString									GetOwnerName() const;

	float										GetStartingLifeTime() const { return m_StartingLifeTime; }
	float										GetLifeTimeRemaining() const { return m_LifeTimeRemaining; }

private:

	float										m_StartingLifeTime;
	float										m_LifeTimeRemaining;
};

/**
 *	kbTransformComponent - Every game entity will have a kbTransformComponent as its first component to hold the entity's position/orientation/scale
 *						 - Some components will be derived from kbTransformComponent to represent the component's local position/orientation/scale
 */
class kbTransformComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbTransformComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	void										SetName( const std::string newName ) { m_Name = newName; }
	void										SetPosition( const kbVec3 & newPosition ) { m_Position = newPosition; }
	void										SetScale( const kbVec3 & newScale ) { m_Scale = newScale; }
	void										SetOrientation( const kbQuat & newOrientation ) { m_Orientation = newOrientation; }

	const std::string &							GetName() const { return m_Name.stl_str(); }
	const kbVec3								GetPosition() const;
	const kbVec3								GetScale() const;
	const kbQuat								GetOrientation() const;

protected:

	kbString									m_Name;

	kbVec3										m_Position;
	kbVec3										m_Scale;
	kbQuat										m_Orientation;
};

/**
 *	kbGameLogicComponent - This is a component for running game logic (AI, Player, etc).  It's added to the end of a kbGameEntity's component list so that it will be run last
 */
class kbGameLogicComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbGameLogicComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

protected:
	virtual void								Update_Internal( const float DeltaTime ) override;

private:
	int											m_DummyTemp;	// Hack: kbTypeInfoHierarchyIterator currently requires at least one element in a component
};

/**
 *	kbDamageComponent 
 */
class kbDamageComponent : public kbGameLogicComponent {

	KB_DECLARE_COMPONENT( kbDamageComponent, kbGameLogicComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	float										GetMinDamage() const { return m_MinDamage; }
	float										GetMaxDamage() const { return m_MaxDamage; }

private:
	float										m_MinDamage;
	float										m_MaxDamage;
};

/**
 *	kbActorComponent 
 */
class kbActorComponent : public kbGameLogicComponent {

	KB_DECLARE_COMPONENT( kbActorComponent, kbGameLogicComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void								TakeDamage( const class kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent );

	float										GetHealth() const { return m_CurrentHealth; }
	float										GetMaxHealth() const { return m_MaxHealth; }

	bool										IsDead() const { return m_CurrentHealth <= 0.0f; }

protected:

	virtual void								SetEnable_Internal( const bool bIsEnabled ) override;

	float										m_MaxHealth;
	float										m_CurrentHealth;
};

/**
 *	kbPlayerStartComponent
 */
class kbPlayerStartComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT( kbPlayerStartComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	int m_DummyVar;
};

/**
 *	kbAnimEvent
 */
class kbAnimEvent : public kbGameComponent {
	KB_DECLARE_COMPONENT( kbAnimEvent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	const kbString							GetEventName() const { return m_EventName; }
	float									GetEventTime() const { return m_EventTime; }
	float									GetEventValue() const { return m_EventValue; }

	static float							Evaluate( const std::vector<kbAnimEvent> & eventList, const float t );

private:
	kbString								m_EventName;
	float									m_EventValue;
	float									m_EventTime;
};

/**
 *	kbVectorAnimEvent
 */
class kbVectorAnimEvent : public kbGameComponent {
	KB_DECLARE_COMPONENT( kbVectorAnimEvent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	const kbString							GetEventName() const { return m_EventName; }
	float									GetEventTime() const { return m_EventTime; }
	kbVec3									GetEventValue() const { return m_EventValue; }

	static kbVec3							Evaluate( const std::vector<kbVectorAnimEvent> & eventList, const float t );

private:
	kbString								m_EventName;
	kbVec3									m_EventValue;
	float									m_EventTime;
};

/**
 *  kbEditorLevelComponent
 */
class kbEditorLevelComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT( kbEditorLevelComponent, kbGameComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	const kbVec3 &								GetCameraPosition() const { return m_CameraPosition; }
	const kbQuat &								GetCameraRotation() const { return m_CameraRotation; }

	void										SetCameraPosition( const kbVec3 & newPos ) { m_CameraPosition = newPos; }
	void										SetCameraRotation( const kbQuat & newRot ) { m_CameraRotation = newRot; }

private:
	kbVec3										m_CameraPosition;
	kbQuat										m_CameraRotation;
};

#endif