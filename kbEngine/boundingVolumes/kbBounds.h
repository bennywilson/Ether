//===================================================================================================
// kbBounds.h
//
//
// 2016 blk 1.0
//===================================================================================================
#ifndef _KBBOUNDS_H_
#define _KBBOUNDS_H_

#include "kbVector.h"

class kbBounds {
public:
	kbBounds() { }
	explicit kbBounds(const bool bReset) {
		if (bReset) {
			Reset();
		}
	}

	kbBounds( const kbVec3 & Min, const kbVec3 & Max ) {
		SetMaxMin( Max, Min );
	}

	void Set( const float minX, const float minY, const float minZ, const float maxX, const float maxY, const float maxZ ) {
		m_Min.Set( minX, minY, minZ );
		m_Max.Set( maxX, maxY, maxZ );
	}

	void SetMaxMin( const kbVec3 & Max, const kbVec3 & Min ) {
		m_Max = Max;
		m_Min = Min;
	}

	const kbVec3 & Max() const { return m_Max; }
	const kbVec3 & Min() const { return m_Min; }

	kbVec3 Center() const { return ( m_Max + m_Min ) * 0.5f; }

	void AddPoint( const kbVec3 & point ) {
		if ( m_Max.x < point.x ) {
			m_Max.x = point.x;
		}
		if ( m_Max.y < point.y ) {
			m_Max.y = point.y;
		}
		if ( m_Max.z < point.z ) {
			m_Max.z = point.z;
		}
		
		if ( m_Min.x > point.x ) {
			m_Min.x = point.x;
		}
		if ( m_Min.y > point.y ) {
			m_Min.y = point.y;
		}
		if ( m_Min.z > point.z ) {
			m_Min.z = point.z;
		}
	}

	const kbBounds operator+( const kbBounds & op2 ) const {
		kbBounds returnBounds;
		returnBounds.m_Max.x = max( m_Max.x, op2.m_Max.x );
		returnBounds.m_Max.y = max( m_Max.y, op2.m_Max.y );
		returnBounds.m_Max.z = max( m_Max.z, op2.m_Max.z );

		returnBounds.m_Min.x = min( m_Min.x, op2.m_Min.x );
		returnBounds.m_Min.y = min( m_Min.y, op2.m_Min.y );
		returnBounds.m_Min.z = min( m_Min.z, op2.m_Min.z );
		return returnBounds;
	}

	void operator+=( const kbBounds & op2 ) {
		m_Max.x = max( m_Max.x, op2.m_Max.x );
		m_Max.y = max( m_Max.y, op2.m_Max.y );
		m_Max.z = max( m_Max.z, op2.m_Max.z );

		m_Min.x = min( m_Min.x, op2.m_Min.x );
		m_Min.y = min( m_Min.y, op2.m_Min.y );
		m_Min.z = min( m_Min.z, op2.m_Min.z );
	}

	void Reset() { 
		m_Max = kbVec3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		m_Min = kbVec3( FLT_MAX, FLT_MAX, FLT_MAX ); 
	}

	bool IntersectsBounds( const kbBounds & box ) const {
		if ( box.m_Max.x < m_Min.x || box.m_Min.x > m_Max.x ||
			 box.m_Max.y < m_Min.y || box.m_Min.y > m_Max.y ||
			 box.m_Max.z < m_Min.z || box.m_Min.z > m_Max.z ) {
			 return false;
		}

		return true;
	}

	bool ContainsPoint( const kbVec3 & point ) const {
		if ( point.x > m_Max.x || point.y > m_Max.y || point.z > m_Max.z ||
			 point.x < m_Min.x || point.y < m_Min.y || point.z < m_Min.z ) {
				 return false;
		}

		return true;
	}

	void Translate( const kbVec3 & translationVec ) {
		m_Max += translationVec;
		m_Min += translationVec;
	}

	void Scale( const kbVec3 & scale ) {
		m_Max.x *= scale.x;
		m_Max.y *= scale.y;
		m_Max.z *= scale.z;

		m_Min.x *= scale.x;
		m_Min.y *= scale.y;
		m_Min.z *= scale.z;

	}

private:
	kbVec3 m_Max, m_Min;
};
 

#endif