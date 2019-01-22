//===================================================================================================
// kbString.cpp
//
// Instead of storing a string, kbString stores an index into the string table for fast look ups
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include <map>
#include <string>
#include <vector>
#include "kbString.h"

std::map<std::string, int> * g_StringTable = nullptr;
std::vector<std::string> * g_StringList = nullptr;
std::string * g_EmptyString = nullptr;

kbString kbString::EmptyString( "" );

/**
 *	kbString::ShutDown
 */
void kbString::ShutDown() {
	delete g_StringTable;
	g_StringTable = nullptr;

	delete g_StringList;
	g_StringList = nullptr;

	delete g_EmptyString;
	g_EmptyString = nullptr;
}

/**
 *	kbString::kbString
 */
kbString::kbString( const std::string & InString ) {

	if ( g_StringTable == nullptr ) {
		g_StringTable= new std::map<std::string, int>();
		g_StringList = new std::vector<std::string>();
		g_EmptyString = new std::string;
	}

	std::map<std::string, int>::const_iterator it = g_StringTable->find( InString );
	if ( it != g_StringTable->end() ) {
		m_StringTableIndex = it->second;
	}
	else {
		g_StringList->push_back( InString );
		m_StringTableIndex = (int)(g_StringList->size() - 1);
		(*g_StringTable)[InString] = m_StringTableIndex;
	}
}

/**
 *	kbString::operator==
 */
bool kbString::operator==( const kbString & Op2 ) const {
	return m_StringTableIndex == Op2.m_StringTableIndex;
}

/**
 *	kbString::operator==
 */
bool kbString::operator==( const char * op2 ) const {
	return *this == kbString( op2 );
}

/**
 *	kbString::operator!=
 */
bool kbString::operator!=( const kbString & Op2 ) const {
	return m_StringTableIndex != Op2.m_StringTableIndex;
}

/**
 *	kbString::operator=
 */
kbString & kbString::operator=( const kbString & Op2 ) {
	m_StringTableIndex = Op2.m_StringTableIndex;
	return *this;
}

/**
 *	kbString::operator=
 */
kbString & kbString::operator=( const std::string & Op2 ) {
	std::map<std::string, int>::const_iterator it = g_StringTable->find( Op2 );
	if ( it != g_StringTable->end() ) {
		m_StringTableIndex = it->second;
	}
	else {
		g_StringList->push_back( Op2 );
		m_StringTableIndex = (int)(g_StringList->size() - 1);
		(*g_StringTable)[Op2] = m_StringTableIndex;
	}
	return *this;
}

/**
 *	kbString::stl_string
 */
const std::string & kbString::stl_str() const
{
	if ( m_StringTableIndex < 0 || m_StringTableIndex >= g_StringList->size() ) {
		return *g_EmptyString;
	}

	return (*g_StringList)[m_StringTableIndex];
}

/**
 *	kbString::c_str
 */
const char * kbString::c_str() const
{
	if ( m_StringTableIndex < 0 || m_StringTableIndex >= g_StringList->size() ) {
		return g_EmptyString->c_str();
	}

	return (*g_StringList)[m_StringTableIndex].c_str();
}

