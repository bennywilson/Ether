//===================================================================================================
// EtherPlayer.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERPLAYER_H_
#define _ETHERPLAYER_H_

#include "EtherActor.h"

/**
 *	EtherPlayerComponent - Base Component for player game logic
 */
class EtherPlayerComponent : public EtherActorComponent {

	KB_DECLARE_COMPONENT( EtherPlayerComponent, EtherActorComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

	virtual void								TakeDamage( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) override;
	virtual void								StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent ) override;

	void										HandleMovement( const struct kbInput_t & Input, const float DT );
	void										HandleAction( const struct kbInput_t & Input );

	int											GetNumStimPacks() const { return m_NumStimPacks; }
	void										UseStimPack() { m_NumStimPacks--; }

	int											GetNumAirstrikes() const { return m_NumAirstrikes; }
	void										UseAirstrike() { m_NumAirstrikes--; }

	int											GetNumOLC() const { return m_NumOLC; }
	void										UseOLC() { m_NumOLC--; }

	class kbSkeletalModelComponent *			GetFPHands() { return m_pFPHands; }

protected:

	virtual void								Update_Internal( const float DeltaTime ) override;
	void										UpdateDeath( const float DeltaTimeSec );

	void										Action_Fire( const bool bActivatedThisFrame );
	void										Action_ThrowGrenade( const bool bActivatedThisFrame );

	float										m_LastTimeHit;
	float										m_DeathStartTime;
	int											m_PlayerDummy;
	bool										m_bHasHitGround;

	int											m_NumStimPacks;
	int											m_NumAirstrikes;
	int											m_NumOLC;

	class kbSkeletalModelComponent	*			m_pFPHands;
	float										m_GrenadeCoolddownSec;
};

#endif
