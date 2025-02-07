//===================================================================================================
// kbLevelDirector.h
//
// 2019 blk 1.0
//===================================================================================================
#ifndef _KBLEVELDIRECTOR_H_
#define _KBLEVELDIRECTOR_H_


/**
 *	kbLevelDirector
 */
template<typename T, typename C>
class kbLevelDirector : public IStateMachine<T,C> {

//---------------------------------------------------------------------------------------------------
public:
	kbLevelDirector() {

	}

	virtual	~kbLevelDirector() { }

	virtual void UpdateStateMachine() { IStateMachine<T,C>::UpdateStateMachine(); }
};

#endif