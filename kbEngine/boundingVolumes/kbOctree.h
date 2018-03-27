//===================================================================================================
// kbOctree.h
//
//
// 2016 kbEngine 2.0
//===================================================================================================
#ifndef _KBOCTREE_H_
#define _KBOCTREE_H_

#include "kbBounds.h"

template<class T>
class kbOctreeHelper {
public:
	bool IntersectsBounds( const kbBounds & bounds, const T * element ) const {
		return true;
	}
};

template<class T, const int MAX_DEPTH, const int MAX_ELEMENTS_PER_NODE >
class kbOctree {
public:
	
	kbOctree() { children = NULL; currentDepth = 0; const float MaxExtent = 9999.0f; nodeBounds.Set( -MaxExtent, -MaxExtent, -MaxExtent, MaxExtent, MaxExtent, MaxExtent ); }
	~kbOctree()	{ DeleteTree(); }


	bool	AddElement( kbOctreeHelper<T> & octreeHelper, T & element ) {

		if ( !octreeHelper.IntersectsBounds( nodeBounds, element ) ) {
			return false;
		}

		if ( ( data.size() >= MAX_ELEMENTS_PER_NODE && currentDepth < MAX_DEPTH ) || children != NULL ) {
			if ( children == NULL ) {
				children = new kbOctree[8];

				for ( int i = 0; i < 8; i++ ) {
					children[i].currentDepth = currentDepth + 1;
				}

				const kbVec3 minPt = nodeBounds.Min();
				const kbVec3 midPt = nodeBounds.Center();
				const kbVec3 maxPt = nodeBounds.Max();

				children[0].nodeBounds.Set( minPt.x, minPt.y, midPt.z, midPt.x, midPt.y, maxPt.z );
				children[1].nodeBounds.Set( midPt.x, minPt.y, midPt.z, maxPt.x, midPt.y, maxPt.z );
				children[2].nodeBounds.Set( minPt.x, midPt.y, midPt.z, midPt.x, maxPt.y, maxPt.z );
				children[3].nodeBounds.Set( midPt.x, midPt.y, midPt.z, maxPt.x, maxPt.y, maxPt.z );
				children[4].nodeBounds.Set( minPt.x, minPt.y, minPt.z, midPt.x, midPt.y, midPt.z );
				children[5].nodeBounds.Set( midPt.x, minPt.y, minPt.z, maxPt.x, midPt.y, midPt.z );
				children[6].nodeBounds.Set( minPt.x, midPt.y, minPt.z, midPt.x, maxPt.y, midPt.z );
				children[7].nodeBounds.Set( midPt.x, midPt.y, minPt.z, maxPt.x, maxPt.y, midPt.z );

				// push data down to leaves
				for ( unsigned int i = 0; i < data.size(); i++ ) {
					for ( int l = 0; l < 8; l++ ) {
						if ( children[l].AddElement( octreeHelper, data[i] ) ) {
							break;
						}
					}
				}
				data.clear();
			}

			int i = 0;
			for ( i = 0; i < 8; i++ ) {
				if ( children[i].AddElement( octreeHelper, element ) ) {
					break;
				}
			}

			if ( i == 8 ) {
				//idLib::Error( "Error, yo!" );
				// error
			}
		} else {
			data.push_back( element );
		}
		return true;
	}

	int GetElementsWithinBounds( const kbBounds & bounds, kbOctreeHelper<T> & octreeHelper, std::vector< T * > & items ) {
	
		int numTests = 1;
		if ( !bounds.IntersectsBounds( nodeBounds ) ) {
			return numTests;
		}

		if ( children != NULL ) {
			for ( int i = 0; i < 8; i++ ) {
				numTests += children[i].GetElementsWithinBounds( bounds, octreeHelper, items );
			}
		} else {
			for ( unsigned int i = 0; i < data.size(); i++ ) {
				numTests++;
				if ( octreeHelper.IntersectsBounds( bounds, data[i] ) ) {
					items.push_back( &data[i] );
				}
			}
		}

		return numTests;
	}

	void DeleteTree() {
		delete[] children;
	}

	void DebugDraw() {
		if ( children != NULL ) {
			for ( int i = 0; i < 8; i++ ) {
				children[i].DebugDraw();
			}
			assert( data.Num() == 0 );

		} else {
			kbVec4 color( gameLocal->random.RandomFloat(), gameLocal->random.RandomFloat(), gameLocal->random.RandomFloat(), 0.0f );
			color.Normalize();
			color *= 0.5f;
			color.w = 1;
			//gameLocal->GetRenderDebug()->DebugBounds( color, nodeBounds, vec3_origin, 60000 );

		/*	for ( int i = 0; i < data.Num(); i++ ) {
				gameLocal->GetRenderDebug()->DebugPoint( color, data[i].position, 60000 );
			}*/
		}
	}

private:


	kbBounds				nodeBounds;
	kbOctree *				children;
	std::vector< T >		data;
	int						currentDepth;
};

#endif
