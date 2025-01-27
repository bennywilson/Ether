//===================================================================================================
// DQuadWeapon.h
//
//
// 2016-2017 kbEngine 2.0
//===================================================================================================
#ifndef _DQUADWEAPON_H_
#define _DQUADWEAPON_H_

/**
 *	kbDQuadProjectile
 */
class kbDQuadProjectileComponent : public kbDamageComponent {
public:
	friend class kbDQuadWeaponComponent;

	KB_DECLARE_COMPONENT( kbDQuadProjectileComponent, kbGameLogicComponent );

	virtual void							Update( const float DeltaTime );

	int										GetOwnerNetId() const { return m_OwnerNetId; }

private:

	float									m_Damage;
	float									m_Velocity;
	float									m_LifeTime;
	float									m_TracerLength;
	float									m_TraceWidth;
	kbGameEntityPtr							m_ExplosionFX;

	int										m_OwnerNetId;
};


/**
 *	kbDQuadWeaponComponent
 */
class kbDQuadWeaponComponent : public kbGameLogicComponent {
public:
	KB_DECLARE_COMPONENT( kbDQuadWeaponComponent, kbGameLogicComponent );

	virtual bool							Fire();

	virtual void							Update( const float DeltaTime );

private:

	float									m_ShotsPerSecond;
	kbGameEntityPtr							m_MuzzleFlashEntity;
	kbGameEntityPtr							m_Projectile;
	bool									m_bInstantHit;
};

#endif
