//===================================================================================================
// KungFuSnolaf.cpp
//
// 2019 kbEngine 2.0
//===================================================================================================
#include <math.h>
#include "CannonGame.h"
#include "CannonPlayer.h"
#include "KungFuSnolaf.h"
#include "kbEditor.h"
#include "kbEditorEntity.h"

/**
 *	KungFuSnolafStateIdle
 */
template<typename T>
class KungFuSnolafStateIdle : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateIdle( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSnolafStateRun
 */
template<typename T>
class KungFuSnolafStateRun : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateRun( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }
};

/**
 *	KungFuSnolafStateHug
 */
template<typename T>
class KungFuSnolafStateHug : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateHug( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }
};


/**
 *	KungFuSnolafStateDead
 */
template<typename T>
class KungFuSnolafStateDead : public KungFuSnolafStateBase<T> {

//---------------------------------------------------------------------------------------------------
public:
	KungFuSnolafStateDead( CannonActorComponent *const pPlayerComponent ) : KungFuSnolafStateBase( pPlayerComponent ) { }
};


/**
 *	KungFuSnolafComponent::Constructor
 */
void KungFuSnolafComponent::Constructor() {

}


/**
 *	KungFuSnolafComponent::SetEnable_Internal
 */
void KungFuSnolafComponent::SetEnable_Internal( const bool bEnable ) {
	Super::SetEnable_Internal( bEnable );

	// Make sure sheep package is loaded
	g_ResourceManager.GetPackage( "./assets/Packages/Sheep.kbPkg" );

	if ( bEnable ) {
		if ( m_SkelModelsList.size() > 1 ) {
			const static kbString smearParam = "smearParams";
			m_SkelModelsList[1]->SetMaterialParamVector( 0, smearParam.stl_str(), kbVec4::zero );
		}

		KungFuSnolafStateBase<KungFuSnolafState::SnolafState_T> * snolafStates[] = {
			new KungFuSnolafStateIdle<KungFuSnolafState::SnolafState_T>( this ),
			new KungFuSnolafStateRun<KungFuSnolafState::SnolafState_T>( this ),
			new KungFuSnolafStateHug<KungFuSnolafState::SnolafState_T>( this ),
			new KungFuSnolafStateDead<KungFuSnolafState::SnolafState_T>( this )
		};

		InitializeStates( snolafStates );
		RequestStateChange( KungFuSnolafState::Idle );
	}
}

/**
*	KungFuSnolafComponent::OnAnimEvent
*/
void KungFuSnolafComponent::OnAnimEvent( const kbAnimEvent & animEvent ) {

}

 /**
  *	KungFuSnolafComponent::Update_Internal
  */
void KungFuSnolafComponent::Update_Internal( const float DT ) {
	Super::Update_Internal( DT );

	UpdateStateMachine();

}
