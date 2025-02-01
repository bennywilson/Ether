/// kbTypeInfo.h
///
/// 2016-2025 kbEngine 2.0

#pragma once

/// kbTypeInfoVar
class kbTypeInfoVar {
public:
	kbTypeInfoVar() :
		m_Type(KBTYPEINFO_NONE),
		m_Offset(0),
		m_bIsArray(false) {
	}

	kbTypeInfoVar(const kbTypeInfoType_t fieldType, const size_t fieldOffset, const bool bIsArray, const std::string& structName) :
		m_Type(fieldType),
		m_Offset(fieldOffset),
		m_bIsArray(bIsArray),
		m_StructName(structName) {
	}

	const kbTypeInfoType_t Type() const { return m_Type; }
	const size_t Offset() const { return m_Offset; }
	const bool IsArray() const { return m_bIsArray; }
	const std::string& GetStructName() const { return m_StructName; }

private:
	kbTypeInfoType_t m_Type;
	size_t m_Offset;
	std::string	m_StructName;
	bool m_bIsArray;
};

/// kbTypeInfoClass - Maps a class' member's names to its kbTypeInfoField
class kbTypeInfoClass {
public:

	void AddMember(const std::string& memberName, const kbTypeInfoVar& fieldInfo) {
		memberFieldsMap[memberName] = fieldInfo;
	}

	const kbTypeInfoVar* GetField(const std::string& memberName) const {
		std::map< std::string, kbTypeInfoVar >::const_iterator it = memberFieldsMap.find(memberName);
		if (it == memberFieldsMap.end()) {
			return nullptr;
		}
		return &it->second;
	}

	const std::map< std::string, kbTypeInfoVar >& GetMemberFieldsMap() const { return memberFieldsMap; }

	const std::string& GetClassName() const { return m_ClassName; }

	virtual kbComponent* ConstructInstance() const = 0;
	virtual kbComponent* ConstructInstance(const kbComponent* const) const = 0;

protected:
	std::string m_ClassName;

private:
	std::map<std::string, kbTypeInfoVar> memberFieldsMap;
};

/// kbNameToTypeInfoMap - This class maps all type info class' to string names.  Code can create an instance of a kbComponent with it's name
class kbNameToTypeInfoMap {
public:
	kbNameToTypeInfoMap();
	~kbNameToTypeInfoMap();

	void AddTypeInfo(const kbTypeInfoClass* const classToAdd);
	void AddEnum(const std::string& enumName, const std::vector<std::string>& enumFields);

	const kbTypeInfoClass* GetTypeInfoFromClassName(const std::string& name) { return m_Map[name]; }

	std::map<std::string, const kbTypeInfoClass*> GetClassMap() const { return m_Map; }

	const std::vector<std::string>* GetEnum(const std::string& name) { return &m_EnumMap[name]; }

	template<typename t>
	void RegisterVectorOperations(const std::string& vectorTypeString) {
		std::map<std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t)>::const_iterator it = m_ResizeVectorPtr.find(vectorTypeString);
		if (it == m_ResizeVectorPtr.end()) {
			m_ResizeVectorPtr[vectorTypeString] = &kbNameToTypeInfoMap::ResizeVector_Internal<t>;
			m_GetVectorElementPtr[vectorTypeString] = &kbNameToTypeInfoMap::GetVectorElement_Internal<t>;
			m_GetVectorSizePtr[vectorTypeString] = &kbNameToTypeInfoMap::GetVectorSize_Internal<t>;
			m_InsertVectorElementPtr[vectorTypeString] = &kbNameToTypeInfoMap::InsertVectorElement_Internal<t>;
			m_RemoveVectorElementPtr[vectorTypeString] = &kbNameToTypeInfoMap::RemoveVectorElement_Internal<t>;
		}
	}

	void ResizeVector(const void* const vectorPtr, const std::string& vectorStringType, const size_t newVectorSize) {
		void (kbNameToTypeInfoMap:: * pFunc)(const void*, const size_t) = nullptr;

		std::map< std::string, void (kbNameToTypeInfoMap::*)(const void* const, const size_t) >::const_iterator it = m_ResizeVectorPtr.find(vectorStringType);
		if (it != m_ResizeVectorPtr.end()) {
			pFunc = it->second;
			(this->*pFunc)(vectorPtr, newVectorSize);
		}
	}

	void* GetVectorElement(const void* const vectorPtr, const std::string& vectorStringType, const size_t index) {
		void* (kbNameToTypeInfoMap:: * pFunc)(const void*, const size_t) = nullptr;

		std::map< std::string, void* (kbNameToTypeInfoMap::*)(const void*, const size_t) >::const_iterator it = m_GetVectorElementPtr.find(vectorStringType);
		if (it != m_GetVectorElementPtr.end()) {
			pFunc = it->second;
			return (this->*pFunc)(vectorPtr, index);
		}
		return nullptr;
	}

	size_t GetVectorSize(const void* const vectorPtr, const std::string& vectorStringType) {
		size_t(kbNameToTypeInfoMap:: * pFunc)(const void*) = nullptr;

		std::map< std::string, size_t(kbNameToTypeInfoMap::*)(const void*) >::const_iterator it = m_GetVectorSizePtr.find(vectorStringType);
		if (it != m_GetVectorSizePtr.end()) {
			pFunc = it->second;
			return (this->*pFunc)(vectorPtr);
		}
		return 0;
	}

	void InsertVectorElement(const void* const vectorPtr, const std::string& vectorStringType, const size_t index) {
		void (kbNameToTypeInfoMap:: * pFunc)(const void*, const size_t) = nullptr;

		std::map< std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t) >::const_iterator it = m_InsertVectorElementPtr.find(vectorStringType);
		if (it != m_InsertVectorElementPtr.end()) {
			pFunc = it->second;
			(this->*pFunc)(vectorPtr, index);
		}
	}

	void RemoveVectorElement(const void* const vectorPtr, const std::string& vectorStringType, const size_t index) {
		void (kbNameToTypeInfoMap:: * pFunc)(const void*, const size_t) = nullptr;

		std::map< std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t) >::const_iterator it = m_RemoveVectorElementPtr.find(vectorStringType);
		if (it != m_RemoveVectorElementPtr.end()) {
			pFunc = it->second;
			(this->*pFunc)(vectorPtr, index);
		}
	}

private:
	std::map<std::string, const kbTypeInfoClass*>	m_Map;
	std::map<std::string, std::vector< std::string>> m_EnumMap;

	template<typename t>
	void ResizeVector_Internal(const void* const vectorPtr, const size_t vectorSize = 0) {
		// todo: kbComponents are reconstructed on resize		
		std::vector<t>& vec = *(std::vector<t>*)vectorPtr;
		std::vector<t> backUp = vec;

		for (int i = 0; i < vec.size() && i < backUp.size(); i++)
			backUp[i] = vec[i];

		vec.resize(vectorSize);

		for (int i = 0; i < vec.size() && i < backUp.size(); i++)
			vec[i] = backUp[i];
	}

	template<typename t>
	void* GetVectorElement_Internal(const void* const vectorPtr = nullptr, const size_t index = 0) {
		std::vector<t>& vec = *(std::vector<t>*)vectorPtr;
		return (void*)&vec[index];
	}

	template<typename t>
	size_t GetVectorSize_Internal(const void* const vectorPtr = nullptr) {
		std::vector<t>& vec = *(std::vector<t>*)vectorPtr;
		return vec.size();
	}

	template<typename t>
	void InsertVectorElement_Internal(const void* const vectorPtr = nullptr, const size_t index = 0) {
		// todo: kbComponents are reconstructed on insert
		std::vector<t>& vec = *(std::vector<t>*)vectorPtr;
		std::vector<t> backUp = vec;

		for (int i = 0; i < vec.size() && i < backUp.size(); i++)
			backUp[i] = vec[i];

		vec.insert(vec.begin() + index, t());

		for (int i = 0; i < vec.size() && i < backUp.size(); i++)
		{
			if (i == index)
				continue;
			else if (i < index)
				vec[i] = backUp[i];
			else
				vec[i] = backUp[i - 1];
		}
	}

	template<typename t>
	void RemoveVectorElement_Internal(const void* const vectorPtr = nullptr, const size_t index = 0) {
		std::vector<t>& vec = *(std::vector<t>*)vectorPtr;
		vec.erase(vec.begin() + index);
	}

	std::map<std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t)> m_ResizeVectorPtr;
	std::map<std::string, void* (kbNameToTypeInfoMap::*)(const void*, const size_t)> m_GetVectorElementPtr;
	std::map<std::string, size_t(kbNameToTypeInfoMap::*)(const void*)> m_GetVectorSizePtr;
	std::map<std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t)> m_InsertVectorElementPtr;
	std::map<std::string, void (kbNameToTypeInfoMap::*)(const void*, const size_t)> m_RemoveVectorElementPtr;
};
extern kbNameToTypeInfoMap* g_NameToTypeInfoMap;

kbComponent* ConstructClassFromName(const std::string& className);

#define AddEnumField( ENUM_FIELD_NAME, ENUM_STRING_NAME ) \
	enumFields.push_back( ENUM_STRING_NAME );

#define AddField( FIELD_NAME, FIELD_TYPE, CLASS_TYPE, MEMBER_NAME, IS_ARRAY, STRUCT_NAME ) \
{ \
	kbTypeInfoVar newField( FIELD_TYPE, (size_t)&((CLASS_TYPE*)(0))->MEMBER_NAME, IS_ARRAY, STRUCT_NAME ); \
	AddMember( FIELD_NAME, newField ); \
	if ( g_NameToTypeInfoMap == nullptr ) { g_NameToTypeInfoMap = new kbNameToTypeInfoMap(); } \
	g_NameToTypeInfoMap->RegisterVectorOperations<CLASS_TYPE>(#CLASS_TYPE); \
}


#define GenerateEnum( ENUM_TYPE, ENUM_NAME, ADD_ENUM_FIELDS ) \
	class ENUM_TYPE##_Enum { \
	public: \
		ENUM_TYPE##_Enum() { \
			std::vector< std::string > enumFields; \
			ADD_ENUM_FIELDS \
			if ( g_NameToTypeInfoMap == nullptr ) { g_NameToTypeInfoMap = new kbNameToTypeInfoMap(); } \
			g_NameToTypeInfoMap->AddEnum( ENUM_NAME, enumFields ); \
			} \
	};

#define GenerateClass( CLASS_TYPE, ADD_FIELDS ) \
	class CLASS_TYPE##_TypeInfo : public kbTypeInfoClass { \
public:\
	CLASS_TYPE##_TypeInfo() { \
		ADD_FIELDS \
		m_ClassName = #CLASS_TYPE; \
		if ( g_NameToTypeInfoMap == nullptr ) { g_NameToTypeInfoMap = new kbNameToTypeInfoMap(); } \
		g_NameToTypeInfoMap->AddTypeInfo(this); \
	} \
	~CLASS_TYPE##_TypeInfo() { \
		/* Go ahead and delete the name-to-typeinfo mapping here.  All typeinfos are deleted together anyways when the program terminates. */ \
		delete g_NameToTypeInfoMap; \
		g_NameToTypeInfoMap = nullptr; \
	} \
	virtual kbComponent * ConstructInstance() const { return new CLASS_TYPE; } \
	virtual kbComponent * ConstructInstance( const kbComponent *const pComponentToCopy ) const { return new CLASS_TYPE( *static_cast<const CLASS_TYPE*>( pComponentToCopy )); } \
};

/// Helper for iterating over a class and its ancestor's type info
class kbTypeInfoHierarchyIterator {
public:
	typedef std::map< std::string, kbTypeInfoVar >::const_iterator iteratorType;

	kbTypeInfoHierarchyIterator(const kbComponent* pComponent) :
		m_pComponent(pComponent),
		m_CurrentIndex(0) {
		const std::vector< class kbTypeInfoClass* >& pClass = m_pComponent->GetTypeInfo();
		m_Iterator = pClass[0]->GetMemberFieldsMap().begin();
	}


	iteratorType Begin() {
		m_CurrentIndex = 0;
		m_Iterator = m_pComponent->GetTypeInfo()[0]->GetMemberFieldsMap().begin();

		return m_Iterator;
	}

	bool IsDone() const {
		return m_CurrentIndex >= m_pComponent->GetTypeInfo().size();
	}

	const iteratorType GetNextTypeInfoField() {
		m_Iterator++;

		if (m_Iterator == m_pComponent->GetTypeInfo()[m_CurrentIndex]->GetMemberFieldsMap().end()) {
			m_CurrentIndex++;

			if (m_CurrentIndex < m_pComponent->GetTypeInfo().size()) {
				m_Iterator = m_pComponent->GetTypeInfo()[m_CurrentIndex]->GetMemberFieldsMap().begin();
			}

		}

		return m_Iterator;
	}

private:
	const kbComponent* m_pComponent;
	iteratorType				m_Iterator;
	int							m_CurrentIndex;
};

#include "kbTypeInfoGeneratedClasses.h"
