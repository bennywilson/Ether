//===================================================================================================
// kbString.h
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#ifndef _KBSTRING_H_
#define _KBSTRING_H_

/**
 *	Instead of storing a string, kbString stores an index into the string table for fast look ups
 */
#define INVALID_KBSTRING -1

class kbString {

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
public:
												kbString() { m_StringTableIndex = INVALID_KBSTRING; }
												kbString( const std::string & InString );

	bool										operator==( const kbString & Op2 ) const;
	bool										operator==( const char * string ) const;

	bool										operator!=( const kbString & Op2 ) const;

	kbString &									operator=( const kbString & Op2 );
	kbString &									operator=( const std::string & InString );

	bool										operator <( const kbString & op2 ) const { return stl_str() < op2.stl_str(); }

	bool										IsEmptyString() const { return c_str()[0] == '\0'; }

	int											GetStringTableIndex() const { return m_StringTableIndex; }
	size_t										GetLength() const { return stl_str().length(); }

	const std::string &							stl_str() const;
	const char *								c_str() const;
	
	static void									ShutDown();

	static kbString								EmptyString;

private:

	int											m_StringTableIndex;
};

struct kbStringHash
{
	 size_t operator()( const kbString & key ) const
	 {
		 size_t hash = key.GetStringTableIndex();
		 return hash;
	 }
};

#endif
