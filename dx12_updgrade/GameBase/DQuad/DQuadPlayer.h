//===================================================================================================
// DQuadPlayer.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef DQUADPLAYER_H_
#define DQUADPLAYER_H_

#include "DQuadActor.h"

/**
 *	kbDQuadPlayerComponent - Base Component for player game logic
 */
class kbDQuadPlayerComponent : public kbDQuadActorComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadPlayerComponent, kbDQuadActorComponent );

	virtual void										Update( const float DeltaTime );

	void												HandleMovement( const struct kbInput_t & Input, const float DT );
	void												HandleAction( const struct kbInput_t & Input );

	void												NetUpdate( const kbNetMsg_t * NetMsg );

protected:

	void												Action_Fire();

	int													m_PlayerDummy;


	// Networking
	float												m_LastMovementTime;
};

#endif