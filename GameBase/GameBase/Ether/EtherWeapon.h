//===================================================================================================
// EtherWeapon.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _ETHERWEAPON_H_
#define _ETHERWEAPON_H_

#include "kbSoundManager.h"

/**
 *	EtherProjectileComponent
 */
class EtherProjectileComponent : public kbDamageComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	friend class EtherWeaponComponent;

	KB_DECLARE_COMPONENT( EtherProjectileComponent, kbGameLogicComponent );

	void									SetOwner( const kbGameEntityPtr newOwner ) { m_OwnerEntity = newOwner; }

	void									Launch();

protected:

	virtual void							LifeTimeExpired() override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	void									DealRadiusDamage();

private:

	std::vector<kbSoundData>				m_ImpactCharacterSoundData;
	std::vector<kbSoundData>				m_ImpactEnvironmentSoundData;
	std::vector<kbSoundData>				m_ImpactWoodSoundData;
	std::vector<kbSoundData>				m_ExplosionSoundData;
	std::vector<kbSoundData>				m_LaunchSoundData;

	float									m_Damage;
	float									m_Velocity;
	float									m_LifeTime;
	float									m_TracerLength;
	float									m_TraceWidth;
	kbGameEntityPtr							m_DefaultImpactFX;
	kbGameEntityPtr							m_WoodImpactFX;

	float									m_DetonationTimer;		// Todo: remove in favor of kbComponent's lifetime var
	float									m_DamageRadius;
	bool									m_bUseBillboard;
	bool									m_bExplodeOnImpact;

	kbGameEntityPtr							m_OwnerEntity;
};

/**
 *	kbVec3TimePointComponent
 */
class kbVec3TimePointComponent : public kbGameLogicComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( kbVec3TimePointComponent, kbGameLogicComponent );

	const kbVec3 &							GetVectorValue() const { return m_Vector; }
	float									GetTime() const { return m_Time; }

private:
	kbVec3									m_Vector;
	float									m_Time;
};


/**
 *	kbAnimatedQuadComponent
 */
class kbAnimatedQuadComponent : public kbGameLogicComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( kbAnimatedQuadComponent, kbGameLogicComponent );

	void									StartAnimation( const kbVec3 & position );
	void									UpdateAnimation( const kbVec3 & position );

	bool									AnimationIsFinished() const;

private:

	// Editor properties
	kbTexture *								m_pTexture;
	kbVec3									m_UVStart;
	kbVec3									m_UVEnd;
	kbVec3									m_MinStartScale;
	kbVec3									m_MaxStartScale;
	std::vector<kbVec3TimePointComponent>	m_ScaleOverTime;
	float									m_MinLifeTime;
	float									m_MaxLifeTime;
	bool									m_bRandomizeStartingRotation;

	// Run time
	kbVec3									m_StartScale;
	float									m_StartingRotation;
	float									m_LifeTime;
	float									m_StartTime;
};


/**
 *	EtherWeaponComponent
 */
class EtherWeaponComponent : public kbGameLogicComponent {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
	KB_DECLARE_COMPONENT( EtherWeaponComponent, kbGameLogicComponent );

	virtual bool							Fire( const bool bActivatedThisFrame );

	void									PlayAnimation( const kbString & animationName, const float transitionLenSec );

protected:

	virtual void							SetEnable_Internal( const bool bEnable ) override;
	virtual void							Update_Internal( const float DeltaTime ) override;

	void									UpdateShells( const float DeltaTime );

	bool									Fire_Internal();

private:

	float									m_SecondsBetweenShots;
	std::vector<kbAnimatedQuadComponent>	m_MuzzleFlashAnimData;
	kbGameEntityPtr							m_MuzzleFlashEntity;
	kbGameEntityPtr							m_Projectile;

	kbModel	*								m_pShellModel;
	kbVec3									m_MinShellVelocity;
	kbVec3									m_MaxShellVelocity;
	kbVec3									m_MinAxisVelocity;
	kbVec3									m_MaxAxisVelocity;
	float									m_ShellLifeTime;

	kbMaterial *							m_pShellTrailMaterial;
	kbTexture *								m_pShellTrailShader;

	int										m_BurstCount;
	float									m_SecondsBetweenBursts;

	// Run time
	kbSkeletalModelComponent *				m_pWeaponModel;
	int										m_CurrentBurstCount;
	float									m_BurstTimer;
	float									m_ShotTimer;

	
	std::vector<kbAnimatedQuadComponent>	m_ActiveMuzzleFlashAnims;

	bool									m_bInstantHit;
	bool									m_bIsFiring;

	struct BulletShell {
											BulletShell() : m_bAvailable( true ) { }

		kbVec3								m_Velocity;
		kbVec3								m_RotationAxis;
		float								m_RotationMag;
		float								m_LifeTimeLeft;
		kbRenderObject						m_RenderObject;
		kbTransformComponent				m_Component;
		int									m_AtlasIdx;
		float								m_NormalizedAnimStartTime;
		bool								m_bAvailable;
	};
	std::vector<BulletShell>				m_ShellPool;
};

#endif
