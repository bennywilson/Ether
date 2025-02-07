/// kbFile.h
///
/// 2016-2025 blk 1.0

#pragma once

#include <fstream>

/// kbFile
class kbFile {
public:
	enum kbFileType_t {
		FT_None,
		FT_Read,
		FT_Write,
	};

	kbFile();
	virtual	~kbFile();

	bool Open(const std::string& fileName, const kbFileType_t fileType);
	void Close();

	bool WritePackage(const kbPackage& Package);
	kbPackage* ReadPackage(const bool bLoadAssetsImmediately = true);

	bool WriteGameEntity(const kbGameEntity* pGameEntity);
	kbGameEntity* ReadGameEntity();

private:
	bool WriteGameEntity_Internal(const kbGameEntity* pGameEntity, std::string& curTab);
	void WriteComponent(const kbComponent* const pComponent, std::string& curTab);
	void WriteProperty(const kbTypeInfoType_t propertyType, const std::string& structName, byte* byteOffsetToVar, std::string& writeBuffer);

	kbGameEntity* ReadGameEntity_Internal();
	kbComponent* ReadComponent(kbGameEntity* const pEntity, const std::string& className, kbComponent* ComponentToFill);
	void ReadProperty(const kbTypeInfoVar* const pTypeInfoVar, byte* const byteOffset, std::string& nextToken, size_t& nextStringPos);
	void ReadToken(std::string& token);

	std::fstream m_File;

	kbFileType_t m_FileType;
	std::string	m_FileName;

	std::string	m_Buffer;
	size_t m_CurrentReadPos;
	size_t m_NextReadPos;

	bool m_bIsPackageFile;
	bool m_bLoadAssetsImmediately;
};
