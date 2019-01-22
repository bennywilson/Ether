//===================================================================================================
// EtherAI.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERAI_H_
#define _ETHERAI_H_

#include "EtherActor.h"
#include "kbInputManager.h"


/**
 *	EtherAIComponent
 */
class EtherAIComponent : public EtherActorComponent {

	friend class EtherAIManager;
	KB_DECLARE_COMPONENT( EtherAIComponent, EtherActorComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual void								StartDeath( const kbDamageComponent *const damageComponent, const kbGameLogicComponent *const attackerComponent );

protected:
	virtual void								Update_Internal( const float DeltaTime ) override;

	enum eEnemyAIState_t {
		Enemy_Idle,
		Enemy_Pursue,
		Enemy_CloseCombat,
		Enemy_Dead
	}											m_AIState;

	virtual void								State_Pursue( const float DeltaTime ) { }
	virtual void								State_CloseCombat( const float DeltaTime ) { }
	virtual void								State_Dead( const float DeltaTime );

	virtual void								UpdateMovementAndAnimation( const float DeltaTimeSeconds ) { }

	// TODO was in old class
	int											m_UpdateFrequency;
	float										m_BaseMoveSpeed;
	float										m_LastNetPositionUpdateTime;
	std::vector<int>							m_TargetList;
	// *****************/

	kbVec3										m_TargetLocation;
	kbQuat										m_TargetRotation;

	float										m_PursueMinDistance;

	int											m_TempDummy1;

	// Death
	float										m_DeathStartTimer;
	kbVec3										m_DeathVelocity;


private:
	// Commands for the AI Manager to control this pawn

};

/**
 *	EtherEnemySoldierAIComponent
 */
class EtherEnemySoldierAIComponent : public EtherAIComponent {

	KB_DECLARE_COMPONENT( EtherEnemySoldierAIComponent, EtherAIComponent );

//---------------------------------------------------------------------------------------------------
public:

	virtual void								ClientNetUpdate( const kbNetMsg_t * NetMsg ) override;

protected:

	virtual void								SetEnable_Internal( const bool isEnabled ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

	virtual void								State_Pursue( const float DeltaTime ) override;
	virtual void								State_CloseCombat( const float DeltaTime ) override;

	virtual void								UpdateMovementAndAnimation( const float DeltaTimeSeconds ) override;

	void										UpdateFiringBehavior( const float DeltaSeconds );
	bool										Fire();

	kbGameEntityPtr								m_TargetEntity;

	// Transient
	bool										m_bIsSpraying;
	float										m_LastSprayTime;
	float										m_CurSprayStartTime;
	float										m_NextShotTime;
	float										m_NextSprayTime;

	// Data
	float										m_SprayDurationSec;
	float										m_SecTimeBetweenSprays;
	float										m_SecBetweenShots;
	kbGameEntityPtr								m_Projectile;
};

/**
 *	EtherAIManager
 */
class EtherAIManager : public kbIInputCallback {

//---------------------------------------------------------------------------------------------------
public:
												EtherAIManager();
												~EtherAIManager();

	void										Initialize();

	void										RegisterCombatant( EtherActorComponent *const ActorComponent );
	void										UnregisterCombatant( EtherActorComponent *const ActorComponent );

	bool										GetCloseCombatSpotOnTarget( kbVec3 & out_Position, const EtherActorComponent *const attacker, const EtherActorComponent *const target );

	void										Update( const float DeltaTime );

private:

	virtual void								InputKeyPressedCB( const int cbParam ) override;
	virtual const char *						GetInputCBName() const { return "EtherAIManager"; }

	const static int NUM_CLOSE_RANGE_SPOTS = 10;
	struct CombatantInfo_t {
		CombatantInfo_t();

		const static float						CLOSE_COMBAT_DISTANCE;
		static kbVec3							GetCloseOffset( const int iCloseSpot );

		int										m_CloseRangeSpots[NUM_CLOSE_RANGE_SPOTS];
		int										m_MyTargetId;
	};
	std::map<kbGameEntityPtr, CombatantInfo_t>	m_CombatantMap;
	std::vector<kbGameEntityPtr>				m_AIList;

	int											m_DebugAIIdx;
};

#endif