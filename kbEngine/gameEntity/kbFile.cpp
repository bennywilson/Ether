//===================================================================================================
// kbFile.cpp
//
//
// 2016-2019 kbEngine 2.0
//===================================================================================================
#include <vector>
#include <string>
#include "kbCore.h"
#include "kbVector.h"
#include "kbQuaternion.h"
#include "kbBounds.h"
#include "kbGameEntityHeader.h"
#include "kbFile.h"

/**
 *	kbFile::kbFile
 */
kbFile::kbFile() :
	m_FileType( FT_None ),
	m_CurrentReadPos( 0 ),
	m_NextReadPos( 0 ),
	m_bIsPackageFile( false ),
	m_bLoadAssetsImmediately( true ) {
}

/**
 *	kbFile::~kbFile
 */
kbFile::~kbFile() {
}

/**
 *	kbFile::Open
 */
bool kbFile::Open( const std::string & fileName, const kbFileType_t fileType ) {

	if ( fileName.empty() ) {
		kbWarning( "kbFile::Open() - Empty file name" );
		return false;
	}

	if ( fileType != FT_Read && fileType != FT_Write ) {
		kbWarning( "kbFile::Open() - %s has an invalid file type", fileName.c_str() );
		return false;
	}

	m_FileType = fileType;
	m_FileName = fileName;

	if ( m_FileType == FT_Write ) {
		std::string tempFileName = m_FileName.c_str();
		tempFileName += "_tmp";

		m_File.open( tempFileName.c_str(), std::fstream::out );
	} else {
		m_File.open( m_FileName.c_str(), std::fstream::in );

		if ( m_File.fail() ) {
			return false;
		}
		m_File.seekg( 0, m_File.end );
		size_t length = m_File.tellg();
		m_File.seekg( 0, m_File.beg );

		char * readBuffer = new char[ length + 1 ];
		m_File.read( readBuffer, length );
		std::streamsize charsRead = m_File.gcount();

		readBuffer[charsRead] = '\0';
		m_Buffer = readBuffer;

		delete[] readBuffer;

		m_File.close();

	}

	if ( GetFileExtension( fileName ) == "kbPkg" ) {
		m_bIsPackageFile = true;
	}
	return true;
}

/**
 *	kbFile::Close
 */
void kbFile::Close() {
	if ( m_FileType != FT_Write && m_FileType != FT_Read ) {
		kbWarning( "kbFile::Close() - Tried to close %s with an invalid file type", m_FileName.c_str() );
		return;
	}

	if ( m_FileType == FT_Write ) {		// note: read files are already closed
		m_File.close();
		
		std::string tempFileName = m_FileName.c_str();
		tempFileName += "_tmp";
		CopyFile( tempFileName.c_str(), m_FileName.c_str(), false );
		DeleteFile( tempFileName.c_str() );	
	}

	m_FileType = FT_None;
}

/**
 *	kbFile::ReadGameEntity
 */
kbGameEntity *kbFile::ReadGameEntity() {
	if ( m_FileType != FT_Read ) {
		kbWarning( "kbFile::ReadGameEntity() - Tried to read from file %s, but the file does not have the correct type.", m_FileName.c_str() );
		return false;
	}
	return ReadGameEntity_Internal();
}


/**
 *	kbFile::ReadGameEntity_Internal
 */
kbGameEntity * kbFile::ReadGameEntity_Internal() {
	size_t nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
	if ( nextStringPos == std::string::npos ) {
		return nullptr;
	}

	// Read GUID
	m_CurrentReadPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
	if ( m_CurrentReadPos == std::string::npos ) {
		return nullptr;
	}
	m_CurrentReadPos += 1;

	nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
	const std::string guid1 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

	m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
	nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
	const std::string guid2 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

	m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
	nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
	const std::string guid3 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

	m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
	nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
	const std::string guid4 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
	
	m_CurrentReadPos = m_Buffer.find_first_of( "{", m_CurrentReadPos ) + 1;
	int bracketCount = 1;
	nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );

	kbGUID entityGUID;
	entityGUID.m_iGuid[0] = strtoul( &guid1[0], nullptr, 0 );
	entityGUID.m_iGuid[1] = strtoul( &guid2[0], nullptr, 0 );
	entityGUID.m_iGuid[2] = strtoul( &guid3[0], nullptr, 0 );
	entityGUID.m_iGuid[3] = strtoul( &guid4[0], nullptr, 0 );

	kbGameEntity *const pGameEntity = new kbGameEntity( &entityGUID, m_bIsPackageFile );

	do {
		std::string nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
		char nextChar = m_Buffer[m_CurrentReadPos];
		m_CurrentReadPos = nextStringPos + 1;

		if ( nextChar == '}' ) {
			bracketCount--;
		} else if ( nextChar == '{') { 
			bracketCount++;
		} else if ( nextChar == ' ' || nextChar == '{' || nextChar == '\n' || nextChar == '\r' || nextChar == '\t' || nextChar == '=' ) {

		} else if ( nextToken.find( "Component", 0 ) != std::string::npos ) {
			ReadComponent( pGameEntity, nextToken, NULL );
		}
		
		nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
	} while( bracketCount != 0 && nextStringPos != std::string::npos );

	pGameEntity->PostLoad();

	for ( int i = 0; i < pGameEntity->NumComponents(); i++ ) {
		kbGameComponent *const pComponent = pGameEntity->GetComponent(i);
		if ( pComponent->IsEnabled() && m_bIsPackageFile == false ) {
			pComponent->Enable( false );
			pComponent->Enable( true );
		}
	}
	return pGameEntity;
}

/**
 *	kbFile::ReadComponent
 */
kbComponent * kbFile::ReadComponent( kbGameEntity *const pGameEntity, const std::string & componentType, kbComponent * ComponentToFill ) {
	kbComponent * pComponent = nullptr;
	if ( ComponentToFill != nullptr ) {
		pComponent = ComponentToFill;
	} else if ( componentType == "kbTransformComponent" ) {
		pComponent = (kbComponent*)( pGameEntity->GetComponent(0) );
	} else {
		pComponent = ConstructClassFromName( componentType );
		pGameEntity->AddComponent( pComponent );
	}

	size_t nextStringPos = m_CurrentReadPos + 1;
	int bracketState = 0;

	do {
		std::string nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
		char nextChar = m_Buffer[m_CurrentReadPos];
		m_CurrentReadPos = nextStringPos + 1;

		if ( nextChar == '{' ) {
			bracketState++;
		} else if ( nextChar == ' ' || nextChar == '\n' || nextChar == '\r' || nextChar == '\t' || nextChar == '=' ) {

		} else if ( nextChar == '}' ) {
			bracketState--;
			if ( bracketState == 0 ) {
				break;
			}
		} else {
			const std::vector< class kbTypeInfoClass * > & typeInfo = pComponent->GetTypeInfo();
			const kbTypeInfoVar * currentVar = nullptr;

			for ( int i = 0; i < typeInfo.size(); i++ ) {
				const kbTypeInfoVar * typeInfoVar = typeInfo[i]->GetField( nextToken );
				if ( typeInfoVar != nullptr ) {
					currentVar = typeInfoVar;
					break;
				}
			}

            // Unrecognized var.  Go to the next line
            if ( currentVar == nullptr ) {
		        while ( m_Buffer[m_CurrentReadPos] != '\n' ) {
			        m_CurrentReadPos++;
		        }
		        while (m_Buffer[m_CurrentReadPos] == ' ' || m_Buffer[m_CurrentReadPos] == '{' || m_Buffer[m_CurrentReadPos] == '\n' || m_Buffer[m_CurrentReadPos] == '\r' || m_Buffer[m_CurrentReadPos] == '\t' ) {
			        m_CurrentReadPos++;
		        }
		        nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
                continue;
            }

			m_CurrentReadPos++;
			while (m_Buffer[m_CurrentReadPos] == ' ' ) m_CurrentReadPos++;

			if ( m_Buffer[m_CurrentReadPos] == '"' ) {
				// Reading a name in quuotes, get the whole thing
				nextStringPos = m_Buffer.find_first_of( "\"", m_CurrentReadPos + 1 );
			} else {
				nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			}

			byte * pCurrentComponentAsBytePtr = ( ( byte * ) pComponent );
			
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			if ( currentVar->IsArray() ) {
				switch( currentVar->Type() ) {
					case KBTYPEINFO_SHADER : {
						std::vector< class kbShader * >	& shaderList = *( std::vector< class kbShader * > *)( &pCurrentComponentAsBytePtr[currentVar->Offset()] );
			
						shaderList.resize( atoi( nextToken.c_str() ) );
						int size = (int)shaderList.size();
						while( size > 0 ) {
							m_CurrentReadPos++;
							size /= 10;
						}

						for ( int i = 0; i < shaderList.size(); i++ ) {
							while ( m_Buffer[m_CurrentReadPos] == ' ' || m_Buffer[m_CurrentReadPos] == '\n' || m_Buffer[m_CurrentReadPos] == '\r' || m_Buffer[m_CurrentReadPos] == '\t' ) {
								m_CurrentReadPos++;
							}
							nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
							nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			
							const kbTypeInfoVar * pVar = currentVar;
							shaderList[i] = (kbShader*)g_ResourceManager.GetResource( nextToken, m_bLoadAssetsImmediately, true );
							m_CurrentReadPos = nextStringPos;
						}
			
						currentVar = nullptr;
						break;
					}

					default: {
						byte *const arrayBytePtr = &pCurrentComponentAsBytePtr[currentVar->Offset()];
						
						const size_t arraySize = atoi( nextToken.c_str() );
						g_NameToTypeInfoMap->ResizeVector( arrayBytePtr, currentVar->GetStructName(), arraySize );
						for ( int i = 0; i < arraySize; i++ ) {

							byte *const arrayElem = (byte*)g_NameToTypeInfoMap->GetVectorElement( arrayBytePtr, currentVar->GetStructName(), i );

							if ( currentVar->Type() == KBTYPEINFO_STRUCT ) {
								while ( m_Buffer[m_CurrentReadPos] != '{' ) {
									m_CurrentReadPos++;
								}
								kbComponent *const pNewComponent = ReadComponent( pGameEntity, currentVar->GetStructName(), (kbComponent*)arrayElem );
								pNewComponent->SetOwningComponent( pComponent );
							} else {
								// hack
								if ( currentVar->Type() == KBTYPEINFO_FLOAT ) {
									m_CurrentReadPos = nextStringPos + 2;
								} else {
									m_CurrentReadPos = nextStringPos + 1;
								}
								nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
								nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
								ReadProperty( currentVar, arrayElem, nextToken, nextStringPos );
								nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
							}
							nextStringPos = m_Buffer.find_first_of( " }{\n\r\t", m_CurrentReadPos );
						}
						break;
					}
				}
			} else {
				ReadProperty( currentVar, &pCurrentComponentAsBytePtr[currentVar->Offset()], nextToken, nextStringPos );
			}

			if ( m_Buffer[m_CurrentReadPos] == '}' ) {
				bracketState--;
				if ( bracketState == 0) {
					m_CurrentReadPos++;
					break;
				}
			}
			m_CurrentReadPos = nextStringPos + 1;
		}

		while (m_Buffer[m_CurrentReadPos] == ' ' || m_Buffer[m_CurrentReadPos] == '{' || m_Buffer[m_CurrentReadPos] == '\n' || m_Buffer[m_CurrentReadPos] == '\r' || m_Buffer[m_CurrentReadPos] == '\t' ) {
			m_CurrentReadPos++;
		}
		nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
	} while ( bracketState != 0 );

	return pComponent;
}

/**
 *	kbFile::ReadProperty
 */
void kbFile::ReadProperty( const kbTypeInfoVar *const pTypeInfoVar, byte *const byteOffset, std::string & nextToken, size_t & nextStringPos ) {

	switch( pTypeInfoVar->Type() ) {
		case KBTYPEINFO_BOOL : 
		{
			bool & pComponentBool = *(bool*)byteOffset;
			pComponentBool = (nextToken[0] - '0') == 1;
			break;
		}
		
		case KBTYPEINFO_FLOAT : {
			float & pComponentFloat = *( float* )byteOffset;
			pComponentFloat = (float)atof( nextToken.c_str() );
			break;
		}
		
		case KBTYPEINFO_INT : 
		{
			int & pComponentInt = *(int*)byteOffset;
			pComponentInt = atoi(nextToken.c_str());
			break;
		}
		
		case KBTYPEINFO_KBSTRING : {
			kbString & string = *(kbString*)byteOffset;
			std::string strippedString = nextToken;
			strippedString.erase( std::remove( strippedString.begin(), strippedString.end(), '"'), strippedString.end() );
			string = strippedString;
			break;
		}
		
		case KBTYPEINFO_STRING :
		{
			std::string & theString = *(std::string*)byteOffset;
			break;
		}
		
		case KBTYPEINFO_VECTOR4 :
		{
			kbVec4 & theVec = *(kbVec4*)byteOffset;
		
			theVec[0] = (float)atof(nextToken.c_str());
		
			m_CurrentReadPos = nextStringPos + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			theVec[1] = (float)atof(nextToken.c_str());
		
			m_CurrentReadPos = nextStringPos + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			theVec[2] = (float)atof(nextToken.c_str());
		
			m_CurrentReadPos = nextStringPos + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			theVec[3] = (float)atof(nextToken.c_str());
		
			break;
		}
		
		case KBTYPEINFO_VECTOR :
		{
			kbVec3 & theVec = *(kbVec3*)byteOffset;
		
			theVec[0] = (float)atof(nextToken.c_str());
		
			m_CurrentReadPos = nextStringPos + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			theVec[1] = (float)atof(nextToken.c_str());
		
			m_CurrentReadPos = nextStringPos + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
			theVec[2] = (float)atof(nextToken.c_str());
		
		
			break;
		}
		
		case KBTYPEINFO_GAMEENTITY : {
			kbGameEntityPtr & entityPtr = *(kbGameEntityPtr*)byteOffset;

			// Read GUID
			const std::string guid1 = nextToken;

			m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
			nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
			const std::string guid2 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

			m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
			nextStringPos = m_Buffer.find_first_of( " ", m_CurrentReadPos );
			const std::string guid3 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

			m_CurrentReadPos = m_Buffer.find_first_of( " ", nextStringPos ) + 1;
			nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
			const std::string guid4 = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );

			kbGUID entityGUID;
			entityGUID.m_iGuid[0] = strtoul( &guid1[0], nullptr, 0 );
			entityGUID.m_iGuid[1] = strtoul( &guid2[0], nullptr, 0 );
			entityGUID.m_iGuid[2] = strtoul( &guid3[0], nullptr, 0 );
			entityGUID.m_iGuid[3] = strtoul( &guid4[0], nullptr, 0 );
			entityPtr.SetEntity( entityGUID );
			break;
		}

		case KBTYPEINFO_SOUNDWAVE :
		case KBTYPEINFO_ANIMATION :
		case KBTYPEINFO_PTR :
		case KBTYPEINFO_TEXTURE :
		case KBTYPEINFO_STATICMODEL :
		case KBTYPEINFO_SHADER :
		{
			INT_PTR * intPtr = ( INT_PTR * )byteOffset;
			INT_PTR & intRef = *intPtr;
			if ( nextToken != "NULL" ) {
				intRef = (INT_PTR)(g_ResourceManager.GetResource( nextToken, m_bLoadAssetsImmediately, true ));
			}
			break;
		}
				
		case KBTYPEINFO_ENUM : {
			int & pComponentInt = *(int*)byteOffset;
		
			const std::vector< std::string > * enumList = g_NameToTypeInfoMap->GetEnum( pTypeInfoVar->GetStructName() );
		
			pComponentInt = 0;
			int i = 0;
			for ( i = 0; i < enumList->size(); i++ ) {
				if ( (*enumList)[i] == nextToken ) {
					pComponentInt = i;
					break;
				}
			}
		
			if ( i == enumList->size() ) {
				kbWarning( "Enum value out of range" );
			}
		}
	}
}

/**
 *	kbFile::WriteGamEntity
 */
bool kbFile::WriteGameEntity( const kbGameEntity * pGameObject ) {
	std::string curTab = "";
	if ( this->m_bIsPackageFile ) {
		curTab += "\t";
	}

	return WriteGameEntity_Internal( pGameObject, curTab );
}

/**
 *	kbFile::WriteGamEntity_Internal
 */
bool kbFile::WriteGameEntity_Internal( const kbGameEntity * pGameObject, std::string & curTab ) {
	if ( m_FileType != FT_Write ) {
		kbWarning( "kbFile::WriteGameEntity() - Tried to write to file %s, but the file does not have the correct type.", m_FileName.c_str() );
		return false;
	}

	if ( pGameObject == NULL ) {
		kbWarning( "kbFile::WriteGameEntity() - Tried to write to file %s, but the game object passed in is null.", m_FileName.c_str() );
		return false;
	}

	const kbGUID & guid = pGameObject->GetGUID();
	m_Buffer += curTab + "kbGameEntity " + std::to_string(guid.m_iGuid[0]) + " " + std::to_string(guid.m_iGuid[1])  + " " + std::to_string(guid.m_iGuid[2])  + " " + std::to_string(guid.m_iGuid[3]) + " {\n";

	curTab += "\t";

	for ( int i = 0; i < pGameObject->NumComponents(); i++ ) {
		const kbComponent *const pCurComponent = pGameObject->GetComponent( i );
		WriteComponent( pCurComponent, curTab );
		m_Buffer += "\n";
	}

	curTab.resize( curTab.size() - 1 );
	m_Buffer += curTab + "}\n";

	// hack
	if ( curTab.size() > 0 ) {
		curTab.resize( curTab.size() - 1 );
		m_Buffer += curTab + "}\n";
	}

	m_File.write( m_Buffer.c_str(), m_Buffer.length() );
	m_Buffer.clear();

	return true;
}

/**
 *	kbFile::WriteComponent
 */
void kbFile::WriteComponent( const kbComponent *const pCurComponent, std::string & curTab ) {
	m_Buffer += curTab + pCurComponent->GetComponentClassName() + " { \n";
	curTab += "\t";

	kbTypeInfoHierarchyIterator iterator( pCurComponent );
	byte * componentBytePtr = ( byte* ) pCurComponent;

	// Write out variables
	for ( kbTypeInfoHierarchyIterator::iteratorType pNextField = iterator.Begin(); iterator.IsDone() == false; pNextField = iterator.GetNextTypeInfoField() )
	{
		byte * byteOffsetToVar = componentBytePtr + pNextField->second.Offset();

		m_Buffer += curTab + pNextField->first.c_str();		// Write out var name
		m_Buffer += " = ";

		// Write out arrays
		if ( pNextField->second.IsArray() ) {
			switch( pNextField->second.Type() ) {

				case KBTYPEINFO_SHADER : {
					std::vector< class kbShader * >	* shaderList = ( std::vector< class kbShader * > *)( byteOffsetToVar );
					m_Buffer += std::to_string( shaderList->size() ) + "\n\t" + curTab;
		
					for ( int i = 0; i < shaderList->size(); i++ ) {
						WriteProperty( pNextField->second.Type(), pNextField->second.GetStructName(), (byte*)&(*shaderList)[i], m_Buffer );
						m_Buffer += "\n" ;
					}
					break;
				}

				default : {
					const size_t vectorSize = g_NameToTypeInfoMap->GetVectorSize( byteOffsetToVar, pNextField->second.GetStructName() );
					m_Buffer += std::to_string( vectorSize );
					for ( int i = 0; i < vectorSize; i++ ) {
						m_Buffer += "\n";
						byte *const arrayElem = (byte*)g_NameToTypeInfoMap->GetVectorElement( byteOffsetToVar, pNextField->second.GetStructName(), i );
						if ( pNextField->second.Type() == KBTYPEINFO_STRUCT ) {
							curTab += "\t";
							WriteComponent( (kbComponent*)arrayElem, curTab );
							curTab.resize( curTab.size() - 1 );
						} else {
							WriteProperty( pNextField->second.Type(), pNextField->second.GetStructName(), arrayElem, m_Buffer );
						}
					}
					break;
				}
			}
		} else {
			WriteProperty( pNextField->second.Type(), pNextField->second.GetStructName(), byteOffsetToVar, m_Buffer );
		}

		m_Buffer += "\n";
	}
	curTab.resize( curTab.size() - 1 );
	m_Buffer += curTab + "}";
}

/**
 *	kbFile::WriteProperty
 */
void kbFile::WriteProperty( const kbTypeInfoType_t propertyType, const std::string & structName, byte * byteOffsetToVar, std::string & writeBuffer ) {
	static 	char charBuffer[256];

	switch( propertyType ) {
		case KBTYPEINFO_BOOL :
		{
			bool *const boolVal = (bool*)byteOffsetToVar;
			if ( *boolVal == 0 ) {
				writeBuffer += "0";
			}
			else {
				writeBuffer += "1";
			}
			break;
		}
	
		case KBTYPEINFO_STRING : {
			std::string & string = * ( ( std::string*) byteOffsetToVar );
			writeBuffer += "\"";
			writeBuffer += string.c_str();
			writeBuffer += "\"";
			break;
		}
	
		case KBTYPEINFO_KBSTRING : {
			kbString& string = * ( ( kbString*) byteOffsetToVar );
			writeBuffer += "\"";
			writeBuffer += string.c_str();
			writeBuffer += "\"";
			break;
		}

		case KBTYPEINFO_VECTOR4 :
		{
			kbVec4 & vector = *( kbVec4 * )byteOffsetToVar;
			
			sprintf_s( charBuffer, "%f %f %f %f", vector.x, vector.y, vector.z, vector.w );
			writeBuffer += charBuffer;
			break;
		}
	
		case KBTYPEINFO_VECTOR :
		{
			kbVec3 & vector = *( kbVec3* )byteOffsetToVar;
			sprintf_s( charBuffer, "%f %f %f", vector.x, vector.y, vector.z );
			writeBuffer += charBuffer;
			break;
		}
	
		case KBTYPEINFO_FLOAT : {
			float theFloat = *( float* )byteOffsetToVar;
			sprintf_s( charBuffer, " %f", theFloat );
			writeBuffer += charBuffer;
			break;
		}
		
		case KBTYPEINFO_INT :
		{
			int theInt = *( int* )byteOffsetToVar;
			sprintf_s( charBuffer, "%d", theInt );
			writeBuffer += charBuffer;
			break;
		}
	
		case KBTYPEINFO_SOUNDWAVE :
		case KBTYPEINFO_ANIMATION :
		case KBTYPEINFO_PTR :
		case KBTYPEINFO_TEXTURE :
		case KBTYPEINFO_STATICMODEL :
		case KBTYPEINFO_SHADER : {
			kbResource * pResource = *( ( kbResource** ) byteOffsetToVar );
			if ( pResource != NULL ) {
				const char * fullFileName = pResource->GetFullFileName().c_str();
				sprintf_s( charBuffer, "%s",  fullFileName);
				writeBuffer += charBuffer;
			} else {
				writeBuffer += "NULL";
			}
			writeBuffer += "\0";
			break;
		}

		case KBTYPEINFO_GAMEENTITY : {
			const kbGameEntityPtr entityPtr = *(kbGameEntityPtr*)byteOffsetToVar;

			if ( entityPtr.GetEntity() != nullptr ) {
				const kbGUID entityGUID = entityPtr.GetGUID();
				writeBuffer += std::to_string( entityGUID.m_iGuid[0] ) + " ";
				writeBuffer += std::to_string( entityGUID.m_iGuid[1] ) + " ";
				writeBuffer += std::to_string( entityGUID.m_iGuid[2] ) + " ";
				writeBuffer += std::to_string( entityGUID.m_iGuid[3] );
			} else {
				writeBuffer += "0 0 0 0";
			}
			break;
		}
		case KBTYPEINFO_ENUM : {
			const std::vector< std::string > * enumList = g_NameToTypeInfoMap->GetEnum( structName );
			int & enumIntValue = * ( (int*) byteOffsetToVar );

			if ( enumIntValue < 0 || enumIntValue >= enumList->size() ) {
				kbWarning( "Enum value out of range!" );
				enumIntValue = 0;
			}

			writeBuffer += (*enumList)[enumIntValue].c_str();
			break;
		}
	}
}

/**
 *	kbFile::WritePackage
 */
bool kbFile::WritePackage( const kbPackage & package ) {
	if ( m_FileType != FT_Write ) {
		kbWarning( "kbFile::WritePackage() - Tried to write to file %s, but the file does not have the correct type.", m_FileName.c_str() );
		return false;
	}

	if ( package.NumFolders() == 0 ) {
		kbWarning( "kbFile::WritePackage() - Tried to write to file %s with no folders, m_FileName.c_str() " );
		return false;
	}

	std::string curTab = "\t";

	for ( int i = 0; i < package.NumFolders(); i++ ) {

		const std::vector< class kbPrefab * > & prefabs = package.GetPrefabsForFolder(i);

		m_Buffer += package.GetFolderName(i) + " " + std::to_string( prefabs.size() ) + "\n";

		for ( int j = 0; j < prefabs.size(); j++ ) {
			m_Buffer += "kbPrefab ";
			m_Buffer += std::to_string( prefabs[j]->NumGameEntities() );
			m_Buffer += " {\n";

			m_Buffer += "\t" + prefabs[j]->GetPrefabName() + "\n";
			for ( int l = 0; l < prefabs[j]->NumGameEntities(); l++ ) {
				// Refresh guid table
				kbGameEntityPtr entityPtr;
				entityPtr.SetEntity( const_cast<kbGameEntity*>( prefabs[j]->GetGameEntity( l ) ) );
				WriteGameEntity_Internal( prefabs[j]->GetGameEntity( l ), curTab );
			}
			m_Buffer += "}\n";
		}
	}

	m_File.write( m_Buffer.c_str(), m_Buffer.length() );
	m_Buffer.clear();

	return true;
}

/**
 *	kbFile::ReadToken
 */
void kbFile::ReadToken( std::string & token ) {

	// Find Start of next token
	m_NextReadPos = m_Buffer.find_first_not_of( " \t{\n\r}" , m_CurrentReadPos );
	if ( m_NextReadPos == std::string::npos ) {
		return;
	}
	m_CurrentReadPos = m_NextReadPos;

	// Get next token
	m_NextReadPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
	token = m_Buffer.substr( m_CurrentReadPos, m_NextReadPos - m_CurrentReadPos );
	m_CurrentReadPos = m_NextReadPos + 1;
}

/**
 *	kbFile::ReadPackage
 */
kbPackage * kbFile::ReadPackage( const bool bLoadAssetsImmediately ) {

	m_bLoadAssetsImmediately = bLoadAssetsImmediately;

	if ( m_FileType != FT_Read ) {
		kbWarning( "kbFile::ReadPackage() - Tried to read to file %s, but the file does not have the correct type.", m_FileName.c_str() );
		return false;
	}

	std::string nextToken;
	ReadToken( nextToken );

	kbPackage * newPackage = new kbPackage();
	const size_t packageNamePos = m_FileName.find_last_of( "/" );
	newPackage->m_PackageName = m_FileName.substr( packageNamePos + 1 );
	int folderIdx = -1;

	while( m_NextReadPos != std::string::npos ) {

		kbPackage::kbFolder newFolder;
		newFolder.m_FolderName = nextToken;

		ReadToken( nextToken );

		const unsigned int NumPrefabsInFolder = std::stoi( nextToken );

		if ( NumPrefabsInFolder > 256 ) {
			kbError( "Too many prefabs in folder" );
		}

		for ( unsigned int prefabIdx = 0; prefabIdx < NumPrefabsInFolder; prefabIdx++ ) {

			ReadToken( nextToken );
			if ( nextToken != "kbPrefab" ) {
				kbError( "Expected 'kbPrefab' while reading file" );
			}

			ReadToken( nextToken );
			const unsigned int NumEntitiesInPrefab = std::stoi( nextToken );
			if ( NumEntitiesInPrefab > 16 ) {
				kbError( "Too many entities in prefab" );
			}

			kbPrefab * pPrefab = new kbPrefab();
			ReadToken( pPrefab->m_PrefabName );
			newFolder.m_pPrefabs.push_back( pPrefab );

			for ( unsigned entityIdx = 0; entityIdx < NumEntitiesInPrefab; entityIdx++ ) {
				kbGameEntity * pEntity = ReadGameEntity();
				pPrefab->m_GameEntities.push_back( pEntity );
			}
		}

		newPackage->m_Folders.push_back( newFolder );
		folderIdx++;

		ReadToken( nextToken );
	}

	return newPackage;
}