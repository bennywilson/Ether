//===================================================================================================
// DQuadActor.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef DQUADACTOR_H_
#define DQUADACTOR_H_

/**
 *	kbDQuadActorComponent - Base Component for AI logic
 */
class kbDQuadActorComponent : public kbActorComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadActorComponent, kbActorComponent );

	virtual void						SetEnable_Internal( const bool isEnabled );

	virtual void						TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent );

	kbGameEntity *						GetEquippedItem() const { return m_pEquippedItem; }
	virtual void						SetEquippedItem( kbGameEntity *const pNewItem ) { m_pEquippedItem = pNewItem; }

	virtual void						NetUpdate( const struct kbNetMsg_t * NetMsg ) { }

protected:

	void								PlaceOnGround();

	enum UpperBodyState_t {
		UBS_Idle,
		UBS_Firing,
	}									m_UpperBodyState;

	kbGameEntity *						m_pEquippedItem;

	int									m_Dummy;
};

#endif