//===================================================================================================
// DQuadAI.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef DQUADAI_H_
#define DQUADAI_H_

#include "DQuadActor.h"

/**
 *	kbDQuadAIComponent - Base Component for AI logic
 */
class kbDQuadAIComponent : public kbDQuadActorComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadAIComponent, kbDQuadActorComponent );

	virtual void						Update( const float DeltaTime );

	const std::vector<int> &			GetTargetList() const { return m_TargetList; }

protected:
	int									m_UpdateFrequency;
	float								m_BaseMoveSpeed;

	std::vector<int>					m_TargetList;
};

/**
 *	kbEnemyAIComponent
 */
class kbEnemyAIComponent : public kbDQuadAIComponent {
public:
	KB_DECLARE_COMPONENT( kbEnemyAIComponent, kbDQuadAIComponent );

	virtual void						Update( const float DeltaTime );

protected:
	enum eEnemyAIState_t {
		Enemy_Idle,
		Enemy_Pursue,
		Enemy_CircleStrafe
	}									m_AIState;

	virtual void						State_Pursue( const float DeltaTime ) { }
	virtual void						State_CircleStrafe( const float DeltaTime ) { }
 
	kbVec3								m_TargetLocation;
	kbQuat								m_TargetRotation;

	float								m_PursueMinDistance;

	int									m_TempDummy1;
};

/**
 *	kbEnemySoldierAIComponent
 */
class kbEnemySoldierAIComponent : public kbEnemyAIComponent {
public:
	KB_DECLARE_COMPONENT( kbEnemySoldierAIComponent, kbEnemyAIComponent );

	virtual void						Update( const float DeltaTime );

	virtual void						NetUpdate( const kbNetMsg_t * NetMsg );

protected:

	virtual void						SetEnable_Internal( const bool isEnabled );

	virtual void						State_Pursue( const float DeltaTime );
	virtual void						State_CircleStrafe( const float DeltaTime );

	class kbModel *						m_pEyeBall;
	bool								m_bEyeballAdded;
	int									m_TempDummy2;


};

/**
 *	DQuadAIManager
 */
class DQuadAIManager {
public:
													DQuadAIManager();

	void											RegisterActor( kbDQuadActorComponent *const AIComponent );
	void											UnregisterActor( kbDQuadActorComponent *const AIComponent );

	int												GetTarget( kbDQuadActorComponent *const requestor, kbVec3 * goalPosition );

	void											Update( const float DeltaTime );

private:
	std::vector<int>								m_TargetList;

	const static int NUM_STRAFE_SPOTS = 4;
	struct AITarget_t {
		AITarget_t() {
			memset( m_StrafeSpots, -1, sizeof( m_StrafeSpots ) );
		}
		int											m_StrafeSpots[NUM_STRAFE_SPOTS];
	};

	std::map<int, AITarget_t>						m_TargetToEnemiesMap;			// Maps a target to its enemies
};

extern DQuadAIManager g_AIManager;

#endif