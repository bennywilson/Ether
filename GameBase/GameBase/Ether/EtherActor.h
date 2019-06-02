//===================================================================================================
// EtherActor.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef ETHERACTOR_H_
#define ETHERACTOR_H_

/**
 *	EtherActorComponent - Base Component for AI logic
 */
class EtherActorComponent : public kbActorComponent {

	KB_DECLARE_COMPONENT( EtherActorComponent, kbActorComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void								TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) override;

	kbGameEntity *								GetEquippedItem() const { return m_pEquippedItem; }
	virtual void								SetEquippedItem( kbGameEntity *const pNewItem ) { m_pEquippedItem = pNewItem; }

	virtual void								ClientNetUpdate( const struct kbNetMsg_t * NetMsg ) { }

	virtual void								StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent );

protected:

	virtual void								SetEnable_Internal( const bool bIsEnabled ) override;

	void										PlaceOnGround( const float Offset );

	enum UpperBodyState_t {
		UBS_Idle,
		UBS_Firing,
	}											m_UpperBodyState;

	kbGameEntity *								m_pEquippedItem;

	float										m_GroundHoverDist;
	int											m_Dummy;
};

/**
 *	EtherComponentToggler
 */
class EtherComponentToggler : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherComponentToggler, kbGameComponent );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

protected:
	virtual void								SetEnable_Internal( const bool bIsEnabled ) override;
	virtual void								Update_Internal( const float DeltaTimeSeconds ) override;

private:

	void										ToggleComponents( const bool bToggleOn );

	float										m_MinFirstBurstDelaySec;
	float										m_MaxFirstBurstDelaySec;
	float										m_MinOnSeconds;
	float										m_MaxOnSeconds;
	float										m_MinOffSeconds;
	float										m_MaxOffSeconds;
	float										m_MinSecBetweenBursts;
	float										m_MaxSecBetweenBursts;
	int											m_MinNumOnBursts;
	int											m_MaxNumOnBursts;

	float										m_NextOnOffStartTime;
	int											m_NumBurstsLeft;
	enum TogglerState {
		WaitingToBurst,
		Bursting
	};
	TogglerState								m_State;
	bool										m_bComponentsEnabled;
};

/**
 *	EtherLightAnimatorComponent
 */
class EtherLightAnimatorComponent : public kbGameComponent {

	KB_DECLARE_COMPONENT( EtherLightAnimatorComponent, kbGameComponent );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

protected:
	virtual void								SetEnable_Internal( const bool bIsEnabled ) override;
	virtual void								Update_Internal( const float DeltaTimeSeconds ) override;

	virtual void								EditorChange( const std::string & propertyName ) override;
private:

	std::vector<kbVectorAnimEvent>				m_LightColorCurve;

	float										m_StartTime;
};

#endif