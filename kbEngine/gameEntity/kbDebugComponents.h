//===================================================================================================
// kbDebugComponents.h
//
//
// 2018 kbEngine 2.0
//===================================================================================================
#ifndef _KBDEBUGCOMPONENTS_H_
#define _KBDEBUGCOMPONENTS_H_

/**
 *	kbDebugSphereCollision
 */
class kbDebugSphereCollision : public kbGameComponent {

	KB_DECLARE_COMPONENT( kbDebugSphereCollision, kbGameComponent );

//---------------------------------------------------------------------------------------------------
public:

protected:
	virtual void								SetEnable_Internal( const bool bEnable ) override;
	virtual void								Update_Internal( const float DeltaTime ) override;

private:
	class kbModel *								m_pCollisionModel;

	kbRenderObject								m_RenderObject;
};

#endif