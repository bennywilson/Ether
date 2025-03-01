/// kbComponent.h
///
/// 2016-2025 blk 1.0

#pragma once

#include <vector>
#include "kbGameEntityHeader.h"
#include "Quaternion.h"
#include "matrix.h"

class kbGameEntity;

///	kbBaseComponent
///
/// - Exists as the end point for children calling CollectAncestorTypeInfo() up their hierarchy
/// - Each child has a static m_TypeInfo which contains their own typeInfo as well as their ancestors'
/// - Should never be used as a base class pointer.  Use kbComponent* instead
class kbBaseComponent {
public:
	virtual	 ~kbBaseComponent() = 0 { }

	// Hack: use a void * instead of kbTypeInfoClass to work around mismatched type compile warning when passing in a type declared in the game project (as opposed to the
	// engine project).
	virtual	bool IsA(const void* const type) const { return false; }

protected:
	virtual void CollectAncestorTypeInfo_Internal(std::vector<class kbTypeInfoClass*>& collection) { }
};

/// kbComponent
/// 
/// Derived classes should provide default init values in their constructors and do actual initializations in their Initialize() function
/// A derived class' Initialize() function does NOT need to call their parent's Initialize()
class kbComponent : public kbBaseComponent {
	friend class kbEntity;

	KB_DECLARE_COMPONENT(kbComponent, kbBaseComponent);

public:
	virtual ~kbComponent() { enable_internal(false); }

	virtual void Enable(const bool setEnabled) { }
	bool IsEnabled() const { return m_IsEnabled; }

	// Called during level load after the owning entity is fully loaded
	virtual void post_load() { }

	virtual void editor_change(const std::string& propertyName) { }

	virtual void RenderSync() { }

	kbEntity* GetOwner() const { return m_pOwner; }

	void MarkAsDirty() { m_bIsDirty = true; }

	void SetOwner(kbEntity* const pGameEntity);

	// TODO: This is very hacky
	void SetOwningComponent(kbComponent* const pOwningComponent) { m_pOwningComponent = pOwningComponent; }

protected:
	virtual void enable_internal(const bool bIsEnabled) { }
	virtual void update_internal(const float DeltaTimeSeconds) { }
	virtual void LifeTimeExpired() { }

	kbComponent* GetOwningComponent() const { return m_pOwningComponent; }
	bool IsDirty() const { return m_bIsDirty; }

	bool m_bIsDirty;
	bool m_IsEnabled;

private:
	kbEntity* m_pOwner;
	kbComponent* m_pOwningComponent;
};

/// kbGameComponent
class kbGameComponent : public kbComponent {
	KB_DECLARE_COMPONENT(kbGameComponent, kbComponent);

public:
	virtual void Enable(const bool setEnabled) override;
	virtual void editor_change(const std::string& propertyName);

	void Update(const float DeltaTimeSeconds);

	kbGameEntity* GetOwner() const { return (kbGameEntity*)Super::GetOwner(); }
	kbString owner__name() const;
	Vec3 owner_position() const;
	Vec3 owner_scale() const;
	Quat4 owner_rotation() const;

	template<typename T>
	T* GetComponent() const {
		T* component = (T*)GetOwner()->GetComponentByType(T::GetType());
		return component;
	}

	void SetOwnerPosition(const Vec3& position);
	void SetOwnerRotation(const Quat4& rotation);

	float GetStartingLifeTime() const { return m_StartingLifeTime; }
	float GetLifeTimeRemaining() const { return m_LifeTimeRemaining; }

private:
	float m_StartingLifeTime;
	float m_LifeTimeRemaining;
};

/// kbTransformComponent
///
/// - Every game entity will have a kbTransformComponent as its first component to hold the entity's position/orientation/scale
/// - Some components will be derived from kbTransformComponent to represent the component's local position/orientation/scale
class kbTransformComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbTransformComponent, kbGameComponent);

public:
	void SetName(const std::string newName) { m_Name = newName; }
	void SetPosition(const Vec3& newPosition) { m_position = newPosition; }
	void SetScale(const Vec3& newScale) { m_Scale = newScale; }
	void SetOrientation(const Quat4& newOrientation) { m_Orientation = newOrientation; }

	const kbString& GetName() const { return m_Name; }
	const Vec3 GetPosition() const;
	const Vec3 GetScale() const;
	const Quat4 GetOrientation() const;

protected:
	kbString m_Name;
	Vec3 m_position;
	Vec3 m_Scale;
	Quat4 m_Orientation;
};

/// kbGameLogicComponent
///
/// This is a component for running game logic (AI, Player, etc).  
/// It's added to the end of a kbGameEntity's component list so that it will be run last
class kbGameLogicComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbGameLogicComponent, kbGameComponent);

protected:
	virtual void update_internal(const float DeltaTime) override;

private:
	int	m_DummyTemp;	// Hack: kbTypeInfoHierarchyIterator currently requires at least one element in a component
};

/// kbDamageComponent 
class kbDamageComponent : public kbGameLogicComponent {
	KB_DECLARE_COMPONENT(kbDamageComponent, kbGameLogicComponent);

public:
	float GetMinDamage() const { return m_MinDamage; }
	float GetMaxDamage() const { return m_MaxDamage; }

private:
	float m_MinDamage;
	float m_MaxDamage;
};

/// kbActorComponent 
class kbActorComponent : public kbGameLogicComponent {
	KB_DECLARE_COMPONENT(kbActorComponent, kbGameLogicComponent);

public:
	virtual void take_damage(const class kbDamageComponent* const damageComponent, const kbGameLogicComponent* const attackerComponent);

	float GetHealth() const { return m_CurrentHealth; }
	float GetMaxHealth() const { return m_MaxHealth; }

	bool sDead() const { return m_CurrentHealth <= 0.0f; }

protected:
	virtual void enable_internal(const bool bIsEnabled) override;

	float m_MaxHealth;
	float m_CurrentHealth;
};

/// kbDeleteEntityComponent
class kbDeleteEntityComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbDeleteEntityComponent, kbGameComponent);
protected:
	virtual void LifeTimeExpired();

private:
	float m_Dummy;
};


/// kbPlayerStartComponent
class kbPlayerStartComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbPlayerStartComponent, kbGameComponent);

	int m_DummyVar;
};

/// kbAnimEvent
class kbAnimEvent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbAnimEvent, kbGameComponent);

public:
	const kbString GetEventName() const { return m_EventName; }
	float GetEventTime() const { return m_EventTime; }
	float GetEventValue() const { return m_EventValue; }

	static float Evaluate(const std::vector<kbAnimEvent>& eventList, const float t);

private:
	kbString m_EventName;
	float m_EventValue;
	float m_EventTime;
};

/// kbAnimEventInfo_t
struct kbAnimEventInfo_t {
	kbAnimEventInfo_t(const kbAnimEvent& animEvent, const kbComponent* const pOwnerComponent) :
		m_AnimEvent(animEvent),
		m_pComponent(pOwnerComponent) { }

	const kbAnimEvent& m_AnimEvent;
	const kbComponent* m_pComponent;
};

/// IAnimEventListener
class IAnimEventListener abstract {
public:
	virtual void OnAnimEvent(const kbAnimEventInfo_t& animEvent) = 0;
};

/// kbVectorAnimEvent
class kbVectorAnimEvent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbVectorAnimEvent, kbGameComponent);

public:
	const kbString GetEventName() const { return m_EventName; }
	float GetEventTime() const { return m_EventTime; }
	Vec4 GetEventValue() const { return m_EventValue; }

	static Vec4	Evaluate(const std::vector<kbVectorAnimEvent>& eventList, const float t);

private:
	kbString m_EventName;
	Vec4 m_EventValue;
	float m_EventTime;
};

/// kbEditorGlobalSettingsComponent
class kbEditorGlobalSettingsComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbEditorGlobalSettingsComponent, kbGameComponent);

public:
	int	m_CameraSpeedIdx;
};

///  kbEditorLevelSettingsComponent
class kbEditorLevelSettingsComponent : public kbGameComponent {
	KB_DECLARE_COMPONENT(kbEditorLevelSettingsComponent, kbGameComponent);

public:
	Vec3 m_CameraPosition;
	Quat4 m_CameraRotation;
};

/// IStateMachine
template<typename StateEnum>
class StateMachineNode abstract {
public:

	StateMachineNode() :
		m_RequestedState((StateEnum)0),
		m_StateStartTime(-1.0f),
		m_bHasStateChangeRequest(false) { }

	void BeginState(const StateEnum previousState) {
		m_StateStartTime = g_GlobalTimer.TimeElapsedSeconds();
		BeginState_Internal(previousState);
	}

	void UpdateState() {
		UpdateState_Internal();
	}

	void EndState(const StateEnum nextState) {
		EndState_Internal(nextState);
	}

	float GetTimeSinceStateBegan() const {
		return g_GlobalTimer.TimeElapsedSeconds() - m_StateStartTime;
	}

	bool HasStateChangeRequest() const {
		return m_bHasStateChangeRequest;
	}

	StateEnum GetAndClearRequestedStateChange() {
		m_bHasStateChangeRequest = false;
		return m_RequestedState;
	}

protected:
	void RequestStateChange(const StateEnum requestedState) { m_bHasStateChangeRequest = true, m_RequestedState = requestedState; }

private:
	virtual void BeginState_Internal(const StateEnum previousState) { }
	virtual void UpdateState_Internal() { }
	virtual void EndState_Internal(const StateEnum nextState) { }

	StateEnum m_RequestedState;

	float m_StateStartTime;
	bool m_bHasStateChangeRequest;
};

template<typename StateClass, typename StateEnum>
class IStateMachine abstract {
public:
	IStateMachine() : m_CurrentState(StateEnum::NumStates) {
		ZeroMemory(m_States, sizeof(m_States));
		m_CurrentState = (StateEnum)StateEnum::NumStates;
		m_PreviousState = (StateEnum)StateEnum::NumStates;
	}

	virtual ~IStateMachine() {
		ShutdownStateMachine();

		for (int i = 0; i < StateEnum::NumStates; i++) {
			delete m_States[i];
		}
	}

	StateEnum GetPreviousState() const {
		return (StateEnum)m_PreviousState;
	}

	virtual void UpdateStateMachine() {
		// This condition is valid if state hasn't been set yet
		if (m_CurrentState >= StateEnum::NumStates) {
			return;
		}

		bool stateChanged = false;
		if (m_States[m_CurrentState]->HasStateChangeRequest()) {
			const StateEnum requestedState = m_States[m_CurrentState]->GetAndClearRequestedStateChange();
			if (requestedState != m_CurrentState) {
				m_States[m_CurrentState]->EndState(requestedState);

				m_PreviousState = m_CurrentState;
				m_CurrentState = requestedState;
				m_States[m_CurrentState]->BeginState(m_PreviousState);
				stateChanged = true;

				StateChangeCB(m_PreviousState, m_CurrentState);
			}
		}

		if (stateChanged == false) {
			m_States[m_CurrentState]->UpdateState();
		}
	}

	void InitializeStateMachine(StateClass* const stateNodes[StateEnum::NumStates]) {
		for (int i = 0; i < StateEnum::NumStates; i++) {
			delete m_States[i];
			m_States[i] = stateNodes[i];

			blk::error_check(stateNodes[i] != nullptr && m_States[i] != nullptr, "IStateMachine::InitializeStates() - NULL state.  Please call InitializeStates with proper values");
		}

		InitializeStateMachine_Internal();
	}

	void ShutdownStateMachine() {
		ShutdownStateMachine_Internal();
	}

	void RequestStateChange(const StateEnum newState) {
		if (newState < (StateEnum)(0) || newState >= StateEnum::NumStates) {
			blk::error("IStateMachine::RequestStateChange() - Invalid State requested");
		}

		if (newState == m_CurrentState) {
			return;
		}

		if (m_CurrentState != StateEnum::NumStates) {
			m_States[m_CurrentState]->EndState(newState);
		}

		blk::error_check(m_States[newState] != nullptr, "IStateMachine::RequestStateChange() - NULL state.  Please call InitializeStates with proper values");

		m_PreviousState = m_CurrentState;
		m_CurrentState = newState;
		m_States[m_CurrentState]->BeginState(m_PreviousState);

		StateChangeCB(m_PreviousState, m_CurrentState);
	}

	StateEnum GetCurrentState() const { return m_CurrentState; }

	bool IsInitialized() const { return m_CurrentState != StateEnum::NumStates; }

protected:
	virtual void StateChangeCB(const StateEnum previousState, const StateEnum nextState) { }

	virtual void InitializeStateMachine_Internal() { }
	virtual void ShutdownStateMachine_Internal() { }

	StateClass* m_States[StateEnum::NumStates];
	StateEnum m_CurrentState;
	StateEnum m_PreviousState;
};

/// ISingleton
template <typename T>
class ISingleton {
public:
	ISingleton() {
		blk::error_check(m_pInstance == nullptr, "Multiple instances of an ISingleton");
	}

	static T* Get() {
		if (m_pInstance == nullptr) {
			m_pInstance = new T();
		}

		return m_pInstance;
	}

	static void DeleteSingleton() {
		delete m_pInstance;
		m_pInstance = nullptr;
	}

private:
	inline static T* m_pInstance = nullptr;
};
