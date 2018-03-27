//===================================================================================================
// EtherActor.h
//
//
// 2016-2017 kbEngine 2.0
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

#endif