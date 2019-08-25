//==============================================================================
// KungFuLevelComponent.h
//
// 2019 kbEngine 2.0
//==============================================================================
#ifndef _KUNGFULEVELCOMPONENT_H_
#define _KUNGFULEVELCOMPONENT_H_


/**
 *	KungFuLevelComponent
 */
class KungFuLevelComponent : public CannonLevelComponent {

	KB_DECLARE_COMPONENT( KungFuLevelComponent, CannonLevelComponent );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:

protected:

	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:

	int											m_Dummy;
};


#endif