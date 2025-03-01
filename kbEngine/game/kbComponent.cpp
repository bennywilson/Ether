/// kbComponent.cpp
///
/// 2016-2025 blk 1.0

#include "blk_core.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "kbGameEntityHeader.h"
#include "kbGame.h"

KB_DEFINE_COMPONENT(kbComponent)
KB_DEFINE_COMPONENT(kbTransformComponent)
KB_DEFINE_COMPONENT(kbGameLogicComponent)
KB_DEFINE_COMPONENT(kbDamageComponent)
KB_DEFINE_COMPONENT(kbActorComponent)

/*
void CopyVarToComponent( const kbComponent * Src, kbComponent * Dst, const kbTypeInfoVar * currentVar ) {
	byte * DstByte = ( ( byte * ) Dst );
	byte * SrcByte = ( ( byte * ) Src );

	if ( currentVar->IsArray() ) {
		switch( currentVar->Type() ) {
			case KBTYPEINFO_SHADER : {
				std::vector< class kbShader * >	& DestShaderList = *( std::vector< class kbShader * > *)( &DstByte[currentVar->Offset()] );
				std::vector< class kbShader * >	& SrcShaderList = *( std::vector< class kbShader * > *)( &SrcByte[currentVar->Offset()] );

				DestShaderList = SrcShaderList;
				break;
			}

			default: {
				byte *const SrcArrayPtr = &SrcByte[currentVar->Offset()];
				byte *const DestArrayPtr = &DstByte[currentVar->Offset()];
				const int arraySize = g_NameToTypeInfoMap->GetVectorSize( SrcByte, currentVar->GetStructName() );
				g_NameToTypeInfoMap->ResizeVector( DestArrayPtr, currentVar->GetStructName(), arraySize );
				for ( int i = 0; i < arraySize; i++ ) {

					byte *const Destin = (byte*)g_NameToTypeInfoMap->GetVectorElement( arrayBytePtr, currentVar->GetStructName(), i );

					if ( currentVar->Type() == KBTYPEINFO_STRUCT ) {
						while ( m_Buffer[m_CurrentReadPos] != '{' ) {
							m_CurrentReadPos++;
						}
						ReadComponent( pGameEntity, currentVar->GetStructName(), (kbComponent*)arrayElem );
					} else {
						m_CurrentReadPos = nextStringPos + 1;
						nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
						nextToken = m_Buffer.substr( m_CurrentReadPos, nextStringPos - m_CurrentReadPos );
						ReadProperty( currentVar, arrayElem, nextToken, nextStringPos );
						nextStringPos = m_Buffer.find_first_of( " {\n\r\t", m_CurrentReadPos );
					}
					nextStringPos = m_Buffer.find_first_of( " }{\n\r\t", m_CurrentReadPos );
				}
				if ( arraySize == 0 ) {
					m_CurrentReadPos = m_Buffer.find_first_of( "}", m_CurrentReadPos );
				}
				break;
			}
		}	else {
		switch( currentVar->Type() ) {
			case KBTYPEINFO_BOOL :
			{
				const bool & srcBool = *(const bool*)&SrcByte[currentVar->Offset()];
				bool & dstBool = *(bool*)&DstByte[currentVar->Offset()];
				dstBool = srcBool;
				break;
			}

			case KBTYPEINFO_FLOAT : {
				const float & srcFloat = *( const float* )&SrcByte[currentVar->Offset()];
				float & dstFloat = *( float* )&DstByte[currentVar->Offset()];
				dstFloat = srcFloat;
				break;
			}

			case KBTYPEINFO_INT :
			{
				const int & srcInt = *( const int* )&SrcByte[currentVar->Offset()];
				int & dstInt = *( int* )&DstByte[currentVar->Offset()];
				dstInt = srcInt;
				break;
			}

			case KBTYPEINFO_STRING :
			{
				const std::string & srcString =  *(const std::string*)&SrcByte[currentVar->Offset()];
				std::string & dstString = *(std::string*)&DstByte[currentVar->Offset()];
				dstString = srcString;
				break;
			}

			case KBTYPEINFO_VECTOR4 :
			{
				const Vec4 & srcVec = *(Vec4*)&SrcByte[currentVar->Offset()];
				Vec4 & dstVec = *(Vec4*)&DstByte[currentVar->Offset()];
				dstVec = srcVec;
				break;
			}

			case KBTYPEINFO_VECTOR :
			{
				const Vec3 & srcVec = *(Vec3*)&SrcByte[currentVar->Offset()];
				Vec3 & dstVec = *(Vec3*)&DstByte[currentVar->Offset()];
				dstVec = srcVec;
				break;
			}

			case KBTYPEINFO_PTR :
			case KBTYPEINFO_TEXTURE :
			case KBTYPEINFO_STATICMODEL :
			case KBTYPEINFO_SHADER :
			{
				INT_PTR * destPtr = ( INT_PTR * )&DstByte[currentVar->Offset()];
				INT_PTR & destRef = *destPtr;
				const INT_PTR * srcPtr = ( const INT_PTR * )&SrcByte[currentVar->Offset()];
				const INT_PTR & srcRef = *srcPtr;
				destRef = srcRef;
				break;
			}

			case KBTYPEINFO_ENUM : {
				int & srcEnum = *(int*)&SrcByte[currentVar->Offset()];
				int & destEnum = *(int*)&DstByte[currentVar->Offset()];
				destEnum = srcEnum;
			}
		}
	}
}*/

/// kbComponent::Constructor
	void kbComponent::Constructor() {
	m_pOwner = nullptr;
	m_pOwningComponent = nullptr;
	m_bIsDirty = false;
	m_IsEnabled = false;
}

/// kbComponent::SetOwner
void kbComponent::SetOwner(kbEntity* const pGameEntity) {

	if (pGameEntity == nullptr) {
		blk::error("Initializing a kbComponent with a NULL game entity", GetComponentClassName());
	}

	m_pOwner = pGameEntity;
}

/// kbGameComponent::Constructor
void kbGameComponent::Constructor() {
	m_StartingLifeTime = -1.0f;
	m_LifeTimeRemaining = -1.0f;
}

/// kbGameComponent::Enable
void kbGameComponent::Enable(const bool setEnabled) {

	if (m_IsEnabled == setEnabled) {
		return;
	}

	m_IsEnabled = setEnabled;

	if (GetOwner() == nullptr || GetOwner()->IsPrefab() == true) {
		return;
	}

	enable_internal(setEnabled);

	if (setEnabled) {
		m_LifeTimeRemaining = m_StartingLifeTime;
	}
}

/// kbGameComponent::EditorChange
void kbGameComponent::editor_change(const std::string& propertyName) {
	Super::editor_change(propertyName);

	if (propertyName == "Enabled") {
		if (GetOwner() == nullptr || GetOwner()->IsPrefab() == true) {
			return;
		}

		enable_internal(m_IsEnabled);
	}
}

/// kbGameComponent::Update
void kbGameComponent::Update(const float DeltaTimeSeconds) {
	if (m_LifeTimeRemaining >= 0.0f) {
		m_LifeTimeRemaining -= DeltaTimeSeconds;
		if (m_LifeTimeRemaining < 0) {
			Enable(false);
			LifeTimeExpired();
			return;
		}
	}

	update_internal(DeltaTimeSeconds);
	m_bIsDirty = false;
}

/// kbGameComponent::GetOwnerName
kbString kbGameComponent::owner__name() const {
	return GetOwner()->GetName();
}

/// kbGameComponent::GetOwnerPosition
Vec3 kbGameComponent::owner_position() const {
	return ((kbGameEntity*)GetOwner())->GetPosition();
}

/// kbGameComponent::GetOwnerScale
Vec3 kbGameComponent::owner_scale() const {
	return ((kbGameEntity*)GetOwner())->GetScale();
}

/// kbGameComponent::GetOwnerRotation
Quat4 kbGameComponent::owner_rotation() const {
	return ((kbGameEntity*)GetOwner())->GetOrientation();
}

/// kbGameComponent::SetOwnerPosition
void kbGameComponent::SetOwnerPosition(const Vec3& position) {
	GetOwner()->SetPosition(position);
}

/// kbGameComponent::SetOwnerRotation
void kbGameComponent::SetOwnerRotation(const Quat4& rotation) {
	GetOwner()->SetOrientation(rotation);
}

/// kbTransformComponent::Constructor
void kbTransformComponent::Constructor() {
	m_position.set(0.0f, 0.0f, 0.0f);
	m_Scale.set(1.0f, 1.0f, 1.0f);
	m_Orientation.set(0.0f, 0.0f, 0.0f, 1.0f);
}

/// kbTransformComponent::GetPosition
const Vec3 kbTransformComponent::GetPosition() const {
	if (GetOwner()->GetComponent(0) == this) {
		return m_position;
	}

	const Mat4 parentRotation = GetOwner()->GetOrientation().to_mat4();
	const Vec3 worldPosition = parentRotation.transform_point(m_position);

	return parentRotation.transform_point(m_position) + GetOwner()->GetPosition();
}

/// kbTransformComponent::GetScale
const Vec3 kbTransformComponent::GetScale() const {
	if (GetOwner()->GetComponent(0) == this) {
		return m_Scale;
	}

	return m_Scale;
}

/// kbTransformComponent::GetOrientation
const Quat4 kbTransformComponent::GetOrientation() const {
	if (GetOwner()->GetComponent(0) == this) {
		return m_Orientation;
	}

	return m_Orientation;
}

/// kbGameLogicComponent::Constructor
void kbGameLogicComponent::Constructor() {
	m_DummyTemp = 0;
}

/// kbGameLogicComponent::update_internal
void kbGameLogicComponent::update_internal(const float DeltaTime) {
	START_SCOPED_TIMER(CLOTH_COMPONENT);
	Super::update_internal(DeltaTime);
}

/// kbDamageComponent::Constructor
void kbDamageComponent::Constructor() {
	m_MinDamage = 10.0f;
	m_MaxDamage = 10.0f;
}

/// kbActorComponent::Constructor
void kbActorComponent::Constructor() {
	m_MaxHealth = 10.0f;
	m_CurrentHealth = m_MaxHealth;
}

/// kbActorComponent::Constructor
void kbActorComponent::enable_internal(const bool bIsEnabled) {
	Super::enable_internal(bIsEnabled);

	if (bIsEnabled) {
		m_CurrentHealth = m_MaxHealth;
	}
}

/// kbActorComponent::take_damage
void kbActorComponent::take_damage(const class kbDamageComponent* const pDamageComponent, const kbGameLogicComponent* const attackerComponent) {
	if (pDamageComponent == nullptr) {
		return;
	}

	m_CurrentHealth -= pDamageComponent->GetMaxDamage();
}

/// kbDeleteEntityComponent::Constructor
void kbDeleteEntityComponent::Constructor() {
	m_Dummy = 1.0f;
}

/// kbDeleteEntityComponent::LifeTimeExpired
void kbDeleteEntityComponent::LifeTimeExpired() {
	g_pGame->RemoveGameEntity(GetOwner());
}

/// kbPlayerStartComponent::Constructor
void kbPlayerStartComponent::Constructor() {
	m_DummyVar = 0;
}

/// kbAnimEvent::Constructor()
void kbAnimEvent::Constructor() {
	m_EventTime = 0.0f;
	m_EventValue = 0.0f;
}

/// kbVectorAnimEvent::Constructor()
void kbVectorAnimEvent::Constructor() {
	m_EventTime = 0.0f;
	m_EventValue = Vec3::zero;
}

/// kbAnimEvent::Evaluate
float kbAnimEvent::Evaluate(const std::vector<kbAnimEvent>& eventList, const float t) {
	if (eventList.size() == 0) {
		blk::warn("kbAnimEvent::Evaluate() - Empty event list");
		return 0;
	}

	for (size_t i = 0; i < eventList.size(); i++) {
		if (t < eventList[i].GetEventTime()) {
			if (i == 0) {
				return eventList[0].GetEventValue();
			}

			const float lerp = (t - eventList[i - 1].GetEventTime()) / (eventList[i].GetEventTime() - eventList[i - 1].GetEventTime());
			return kbLerp(eventList[i - 1].GetEventValue(), eventList[i].GetEventValue(), lerp);
		}
	}

	return eventList.back().GetEventValue();
}

/// kbVectorAnimEvent::Evaluate
Vec4 kbVectorAnimEvent::Evaluate(const std::vector<kbVectorAnimEvent>& eventList, const float t) {
	if (eventList.size() == 0) {
		blk::warn("kbVectorAnimEvent::Evaluate() - Empty event list");
		return Vec3::zero;
	}

	for (int i = 0; i < eventList.size(); i++) {
		if (t < eventList[i].GetEventTime()) {
			if (i == 0) {
				return eventList[0].GetEventValue();
			}

			const float lerp = (t - eventList[i - 1].GetEventTime()) / (eventList[i].GetEventTime() - eventList[i - 1].GetEventTime());
			//	blk::log( "i = %d, lerp = %f.  time1 = %f, time2 = %f", i, lerp, eventList[i-1].GetEventTime(), t - eventList[i-1].GetEventTime() );
			return kbLerp(eventList[i - 1].GetEventValue(), eventList[i].GetEventValue(), lerp);
		}
	}

	//	blk::log( "Gah!");
	return eventList.back().GetEventValue();
}

/// kbEditorGlobalSettingsComponent::Constructor
void kbEditorGlobalSettingsComponent::Constructor() {
	m_CameraSpeedIdx = 0;
}

/// kbEditorLevelSettingsComponent::Constructor
void kbEditorLevelSettingsComponent::Constructor() {
	m_CameraPosition = Vec3::zero;
	m_CameraRotation = Quat4::identity;
}
