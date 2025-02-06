//==============================================================================
// kbLightRendering.cpp
//
//
// 2017-2018 blk 1.0
//==============================================================================
#include <stdio.h>
#include "kbCore.h"
#include "kbConsole.h"
#include "kbRenderer_DX11.h"
#include "kbModel.h"
#include "kbGameEntityHeader.h"
#include "kbComponent.h"
#include "kbSkeletalModelComponent.h"
#include "kbPlane.h"

kbConsoleVariable g_DebugShadowBounds( "debugshadowbounds", false, kbConsoleVariable::Console_Bool, "Freeze shadow position and draw the bounds.", "" );
kbConsoleVariable g_ShowShadows( "showshadows", true, kbConsoleVariable::Console_Bool, "Toggle Shadows on/off.", "" );
kbConsoleVariable g_ShowLightShafts( "showlightshafts", true, kbConsoleVariable::Console_Bool, "Toggle light shafts on/off.", "" );

/**
 *	kbRenderer_DX11::RenderLights
 */
void kbRenderer_DX11::RenderLights() {

	if ( m_ViewMode == ViewMode_Wireframe ) {
		return;
	}

	START_SCOPED_RENDER_TIMER( RENDER_LIGHTING );

	for ( auto iter = m_pCurrentRenderWindow->GetRenderLightMap().begin(); iter != m_pCurrentRenderWindow->GetRenderLightMap().end(); iter++ ) {
		RenderLight( iter->second );
		ID3D11ShaderResourceView * const nullRTViews[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		m_pDeviceContext->PSSetShaderResources( 0, 8, nullRTViews );
	}

	ID3D11ShaderResourceView * const nullRTViews[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pDeviceContext->PSSetShaderResources( 0, 8, nullRTViews );
	m_RenderState.SetBlendState();
}

/**
 *	kbRenderer_DX11::RenderLight
 */
kbVec3 frozenCameraPosition;

void kbRenderer_DX11::RenderLight( const kbRenderLight *const pLight ) {

	// Matrices that are scaled to 0 will produce a 0 depth in the projection shader and not shadow the pixel
	kbMat4 splitMatrices[4] = { kbMat4::identity, kbMat4::identity, kbMat4::identity, kbMat4::identity };
	splitMatrices[0].make_scale( kbVec3::zero );
	splitMatrices[1].make_scale( kbVec3::zero );
	splitMatrices[2].make_scale( kbVec3::zero );
	splitMatrices[3].make_scale( kbVec3::zero );

	if ( pLight->m_bCastsShadow ) {
		RenderShadow( pLight, splitMatrices );
	}

	START_SCOPED_RENDER_TIMER( RENDER_LIGHT );

	// Render Light
	if ( pLight->m_pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		m_pDeviceContext->OMSetRenderTargets( 1, &GetAccumBuffer( m_iAccumBuffer )->m_pRenderTargetView, nullptr );
		m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareAlways, false );
	} else {
		m_pDeviceContext->OMSetRenderTargets( 1, &GetAccumBuffer( m_iAccumBuffer )->m_pRenderTargetView, m_pDepthStencilView );
		m_RenderState.SetDepthStencilState(	true,
											kbRenderState::DepthWriteMaskZero,
											kbRenderState::CompareLess,
											true,
											0xff,
											0x0,
											kbRenderState::StencilKeep,
											kbRenderState::StencilKeep,
											kbRenderState::StencilReplace,
											kbRenderState::CompareNotEqual,
											kbRenderState::StencilKeep,
											kbRenderState::StencilKeep,
											kbRenderState::StencilReplace,
											kbRenderState::CompareNotEqual,
											1);

	}

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;
	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	ID3D11ShaderResourceView *const  RenderTargetViews[] = {	GetRenderTarget_DX11(COLOR_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(NORMAL_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(SPECULAR_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(DEPTH_BUFFER)->m_pShaderResourceView,
																GetRenderTarget_DX11(SHADOW_BUFFER)->m_pShaderResourceView };

	ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState, m_pNormalMapSamplerState, m_pShadowMapSamplerState, m_pShadowMapSamplerState };
	m_pDeviceContext->PSSetShaderResources( 0, 5, RenderTargetViews );
	m_pDeviceContext->PSSetSamplers( 0, 4, SamplerStates );

	const kbLightComponent *const pLightComponent = pLight->m_pLightComponent;
	const kbShader * pShader = nullptr;

	if ( pLightComponent->GetMaterialList().size() > 0 ) {
		pShader = pLightComponent->GetMaterialList()[0].GetShader();
	}

	if ( pShader == nullptr || pShader->GetVertexShader() == nullptr || pShader->GetPixelShader() == nullptr ) {

		if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
			pShader = m_pDirectionalLightShader;
		} else if ( pLightComponent->IsA( kbCylindricalLightComponent::GetType() ) ) {
			pShader = m_pCylindricalLightShader;
		} else {
			pShader = m_pPointLightShader;
		}
	}

	m_RenderState.SetBlendState( pShader );
	m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), nullptr, 0 );
	m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), nullptr, 0 );
    m_pDeviceContext->GSSetShader( (ID3D11GeometryShader *) pShader->GetGeometryShader(), nullptr, 0 );

	const auto & varBindings = pShader->GetShaderVarBindings();
	ID3D11Buffer * pConstBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_pDeviceContext->Map( pConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	blk::error_check( SUCCEEDED(hr), "kbRenderer_DX11::RenderLight() - Failed to map matrix buffer" );

	byte * pMappedData = (byte*)mappedResource.pData;
	if ( pLightComponent->GetMaterialList().size() > 0 ) {
		const kbMaterialComponent & matComp = pLightComponent->GetMaterialList()[0];
		auto & shaderParms = matComp.GetShaderParams();
	
		kbShaderParamOverrides_t overrides;
		for ( int j = 0; j < shaderParms.size(); j++ ) {
			if ( shaderParms[j].GetTexture() != nullptr ) {
				overrides.SetTexture( shaderParms[j].GetParamName().stl_str(), shaderParms[j].GetTexture() );
			} else if ( shaderParms[j].GetRenderTexture() != nullptr ) {
	
				overrides.SetTexture( shaderParms[j].GetParamName().stl_str(), shaderParms[j].GetRenderTexture() );
			} else {
				overrides.SetVec4( shaderParms[j].GetParamName().stl_str(), shaderParms[j].GetVector() );
			}
		}

		if ( matComp.GetShader() != nullptr ) {
			SetConstantBuffer( matComp.GetShader()->GetShaderVarBindings(), &overrides, nullptr, pMappedData );
		}
	} 

	SetShaderVec4( "lightDirection", kbVec4( -pLight->m_Orientation.ToMat4()[2].ToVec3(), pLight->m_Length ), pMappedData, varBindings );
	SetShaderVec4( "lightColor", pLight->m_Color, pMappedData, varBindings );
	SetShaderMat4( "inverseViewProjection", m_pCurrentRenderWindow->GetInverseViewProjection(), pMappedData, varBindings );
	SetShaderVec4( "cameraPosition", m_pCurrentRenderWindow->GetCameraPosition(), pMappedData, varBindings );

	kbMat4 lightMatrix[4];
	kbVec4 splitDistances;
	for ( int i = 0; i < 4; i++ ) {
		lightMatrix[i] = splitMatrices[i];
		splitDistances[i] = pLight->m_CascadedShadowSplits[i];
	}

	SetShaderMat4Array( "lightMatrix", lightMatrix, 4, pMappedData, varBindings );
	SetShaderVec4( "splitDistances", splitDistances, pMappedData, varBindings );
	SetShaderVec4( "lightPosition", kbVec4( pLight->m_Position.x, pLight->m_Position.y, pLight->m_Position.z, pLight->m_Radius ), pMappedData, varBindings );

	kbMat4 mvpMatrix;
	mvpMatrix.make_identity();

	SetShaderMat4( "mvpMatrix", mvpMatrix, pMappedData, varBindings );

	m_pDeviceContext->Unmap( pConstBuffer, 0 );
	m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstBuffer );
	m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstBuffer );

	m_pDeviceContext->Draw( 6, 0 );

	ID3D11ShaderResourceView *const nullArray[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pDeviceContext->PSSetShaderResources( 0, 5, nullArray );

	m_pDeviceContext->OMSetRenderTargets( 1, &GetAccumBuffer( m_iAccumBuffer )->m_pRenderTargetView, nullptr );
	//m_RenderState.SetDepthStencilState();
	m_RenderState.SetDepthStencilState( false, kbRenderState::DepthWriteMaskZero, kbRenderState::CompareAlways, false );
}

/**
 *	kbRenderer_DX11::RenderShadow
 */
void kbRenderer_DX11::RenderShadow( const kbRenderLight *const pLight, kbMat4 splitMatrices[] ) {

	ID3D11ShaderResourceView *const pNullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
											   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	// Unbind all textures
	m_pDeviceContext->VSSetShaderResources( 0, 16, pNullSRVs );
	m_pDeviceContext->GSSetShaderResources( 0, 16, pNullSRVs );
	m_pDeviceContext->PSSetShaderResources( 0, 16, pNullSRVs );

	if ( g_ShowShadows.GetBool() == false ) {
		return;
	}

	START_SCOPED_RENDER_TIMER( RENDER_SHADOW_DEPTH );

	const float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(SHADOW_BUFFER)->m_pRenderTargetView, clearColor );
	m_pDeviceContext->ClearDepthStencilView( GetRenderTarget_DX11(SHADOW_BUFFER_DEPTH)->m_pDepthStencilView , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );

	const float shadowBufferSize = (float) GetRenderTarget_DX11(SHADOW_BUFFER)->GetWidth();
	const float halfShadowBufferSize = shadowBufferSize * 0.5f;

	kbMat4 textureMatrix;
	textureMatrix.make_identity();
	textureMatrix[0].x = 0.5f;
	textureMatrix[1].y = -0.5f;
	textureMatrix[3].x = 0.5f + ( 0.5f / shadowBufferSize );
	textureMatrix[3].y = 0.5f + ( 0.5f / shadowBufferSize );

	// Render Shadow map
	m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(SHADOW_BUFFER)->m_pRenderTargetView, GetRenderTarget_DX11(SHADOW_BUFFER_DEPTH)->m_pDepthStencilView );

	const kbMat4 oldViewMatrix = m_pCurrentRenderWindow->GetViewMatrix();
	const kbMat4 oldProjectionMatrix = m_pCurrentRenderWindow->GetProjectionMatrix();
	const kbMat4 oldViewProjMatrix = m_pCurrentRenderWindow->GetViewProjectionMatrix();

	kbPlane frustumPlanes[6];
	kbVec3 upperLeft, upperRight, lowerRight, lowerLeft, dummyPoint;

	static kbMat4 frozenMatrix = m_pCurrentRenderWindow->GetViewProjectionMatrix();;
	static kbVec3 camDir = m_pCurrentRenderWindow->GetCameraRotation().ToMat4()[2].ToVec3();

	// Toggle between normal cam and debug cam
	static bool bKeyDown = false;
	static bool debuggingShadowBounds = false;
	if ( g_DebugShadowBounds.GetBool() != debuggingShadowBounds ) {
		debuggingShadowBounds = g_DebugShadowBounds.GetBool();
		if ( debuggingShadowBounds ) {
			debuggingShadowBounds = true;
			frozenMatrix = m_pCurrentRenderWindow->GetViewProjectionMatrix();
			frozenCameraPosition = m_pCurrentRenderWindow->GetCameraPosition();
			camDir = m_pCurrentRenderWindow->GetCameraRotation().ToMat4()[2].ToVec3();
		}
	}

	if ( debuggingShadowBounds == false ) {
		frozenMatrix = m_pCurrentRenderWindow->GetViewProjectionMatrix();
		frozenCameraPosition = m_pCurrentRenderWindow->GetCameraPosition();
		camDir = m_pCurrentRenderWindow->GetCameraRotation().ToMat4()[2].ToVec3();
	}

	frozenMatrix.left_clip_plane( frustumPlanes[0] );
	frozenMatrix.top_clip_plane( frustumPlanes[1] );
	frozenMatrix.right_clip_plane( frustumPlanes[2]);
	frozenMatrix.bottom_clip_plane( frustumPlanes[3] );
	frozenMatrix.near_clip_plane( frustumPlanes[4] );
	frozenMatrix.far_clip_plane( frustumPlanes[5] );

	frustumPlanes[1].PlanesIntersect( dummyPoint, upperLeft, frustumPlanes[0] );
	frustumPlanes[2].PlanesIntersect( dummyPoint, upperRight, frustumPlanes[1] );
	frustumPlanes[3].PlanesIntersect( dummyPoint, lowerRight, frustumPlanes[2] );
	frustumPlanes[0].PlanesIntersect( dummyPoint, lowerLeft, frustumPlanes[3] );

	const float DistToFarCorner = m_RenderWindowList[0]->GetFarPlane() / ( camDir.dot( upperLeft ) );
	const float NearCornerDist = ( ( m_RenderWindowList[0]->GetNearPlane() ) * DistToFarCorner ) / m_RenderWindowList[0]->GetFarPlane();

	if ( debuggingShadowBounds ) {
		vertexLayout lines[8];
		lines[0].position = frozenCameraPosition;
		lines[0].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );
		lines[1].position = frozenCameraPosition + upperLeft * DistToFarCorner;
		lines[1].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

		lines[2].position = frozenCameraPosition;
		lines[2].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );
		lines[3].position = frozenCameraPosition + upperRight * DistToFarCorner;
		lines[3].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

		lines[4].position = frozenCameraPosition;
		lines[4].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );
		lines[5].position = frozenCameraPosition + lowerLeft * DistToFarCorner;
		lines[5].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

		lines[6].position = frozenCameraPosition;
		lines[6].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );
		lines[7].position = frozenCameraPosition + lowerRight * DistToFarCorner;
		lines[7].SetColor( kbVec4( 1.0f, 0.0f, 1.0f, 1.0f ) );

		m_DepthLines_RenderThread.push_back( lines[0] );
		m_DepthLines_RenderThread.push_back( lines[1] );
		m_DepthLines_RenderThread.push_back( lines[2] );
		m_DepthLines_RenderThread.push_back( lines[3] );
		m_DepthLines_RenderThread.push_back( lines[4] );
		m_DepthLines_RenderThread.push_back( lines[5] );
		m_DepthLines_RenderThread.push_back( lines[6] );
		m_DepthLines_RenderThread.push_back( lines[7] );
	}

	m_RenderState.SetDepthStencilState();

	for ( int i = 0; i < 4 && pLight->m_CascadedShadowSplits[i] < FLT_MAX; i++ ) {
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = ( i % 2 ) * halfShadowBufferSize;
		viewport.TopLeftY = ( i / 2 ) * halfShadowBufferSize;
		viewport.Width = halfShadowBufferSize;
		viewport.Height = halfShadowBufferSize;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_pDeviceContext->RSSetViewports( 1, &viewport );

		// Calculate shadow map bounds
		const float prevDist = ( i == 0 ) ? ( 0.0f ) : ( pLight->m_CascadedShadowSplits[i] - 1 );
		const kbVec3 lookAtPoint = frozenCameraPosition + camDir * ( prevDist + ( pLight->m_CascadedShadowSplits[i] - prevDist ) * 0.5f );
		const float halfFOV = kbToRadians ( 75.0f ) * 0.5f;
		const float distToCorner = pLight->m_CascadedShadowSplits[i] / ( cos( halfFOV ) );
		kbVec3 cornerVert = frozenCameraPosition + distToCorner * upperLeft;
		const float boundsLength = ( lookAtPoint - cornerVert ).length();

		// Debug Drawing -
		if ( debuggingShadowBounds ) {
			vertexLayout start, end;
			start.SetColor( kbVec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
			end.SetColor( kbVec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
	
			start.position = frozenCameraPosition + distToCorner * upperLeft;
			end.position = frozenCameraPosition + distToCorner * upperRight;
			m_DepthLines_RenderThread.push_back( start );
			m_DepthLines_RenderThread.push_back( end );
		
			start.position = frozenCameraPosition + distToCorner * upperRight;
			end.position = frozenCameraPosition + distToCorner * lowerRight;
			m_DepthLines_RenderThread.push_back( start );
			m_DepthLines_RenderThread.push_back( end );
	
			start.position = frozenCameraPosition + distToCorner * lowerRight;
			end.position = frozenCameraPosition + distToCorner * lowerLeft;
			m_DepthLines_RenderThread.push_back( start );
			m_DepthLines_RenderThread.push_back( end );
	
			start.position = frozenCameraPosition + distToCorner * lowerLeft;
			end.position = frozenCameraPosition + distToCorner * upperLeft;
			m_DepthLines_RenderThread.push_back( start );
			m_DepthLines_RenderThread.push_back( end );
		}
		// --------------------------------------------------
		kbVec3 lightDir = -pLight->m_Orientation.ToMat4()[2].ToVec3();

		kbMat4 lightViewMatrix;
		lightViewMatrix.look_at( lookAtPoint + lightDir * boundsLength * 10.0f, lookAtPoint, kbVec3( 0.0f, 1.0f, 0.0f ) );

		kbMat4 lightProjMatrix;
		lightProjMatrix.ortho_lh( boundsLength * 2.0f, boundsLength * 2.0f, 10.0f, boundsLength * 40.0f );

		const kbMat4 lightViewProjMatrix = lightViewMatrix * lightProjMatrix;
		const float texelSize = 2.0f / (shadowBufferSize * 0.5f);
		kbVec4 projCenter( 0.0f, 0.0f, 0.0f, 1.0f );
		projCenter = projCenter.transform_point( lightViewProjMatrix, true);

		const float fracX = fmod( projCenter.x, texelSize );
		const float fracY = fmod( projCenter.y, texelSize );
        
		kbMat4 offset;
		offset.make_identity();
		offset[3][0] = -fracX;
		offset[3][1] = -fracY;

		splitMatrices[i] = lightViewProjMatrix * offset * textureMatrix;

		m_pCurrentRenderWindow->HackSetViewMatrix( lightViewMatrix );
		m_pCurrentRenderWindow->HackSetProjectionMatrix( lightProjMatrix );
		m_pCurrentRenderWindow->HackSetViewProjectionMatrix( lightViewProjMatrix * offset );
		
		std::vector<kbRenderSubmesh> & LightingPassVisibleList = m_pCurrentRenderWindow->GetVisibleSubMeshes( RP_Lighting );
		for ( int i = 0; i < LightingPassVisibleList.size(); i++ ) {
			if ( LightingPassVisibleList[i].GetRenderObject()->m_bCastsShadow ) {
				RenderMesh( &LightingPassVisibleList[i], true );
			}
		}
	}

	D3D11_VIEWPORT viewport;
	viewport.Width = (float)Back_Buffer_Width;
	viewport.Height = (float)Back_Buffer_Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	m_pDeviceContext->RSSetViewports( 1, &viewport );

	m_pCurrentRenderWindow->HackSetViewMatrix( oldViewMatrix );
	m_pCurrentRenderWindow->HackSetProjectionMatrix( oldProjectionMatrix );
	m_pCurrentRenderWindow->HackSetViewProjectionMatrix( oldViewProjMatrix );

	PLACE_GPU_TIME_STAMP( "Shadow Depth" );
}

/**
 *	kbRenderer_DX11::RenderLightShaft
 */
void kbRenderer_DX11::RenderLightShafts() {

	if ( g_ShowLightShafts.GetBool() == false ) {
		return;
	}

	START_SCOPED_RENDER_TIMER( RENDER_LIGHTSHAFTS );

	m_RenderState.SetBlendState();

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;
	m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pDeviceContext->RSSetState( m_pDefaultRasterizerState );

	for ( int i = 0; i < m_LightShafts_RenderThread.size(); i++ ) {

		const kbLightShafts & CurLightShafts = m_LightShafts_RenderThread[i];
		const float HeightMultiplier = (float)kbRenderer_DX11::Back_Buffer_Width / (float)kbRenderer_DX11::Back_Buffer_Height;
		const float HalfBaseHeight = CurLightShafts.m_Height * HeightMultiplier * 0.5f;
		const float HalfIterationHeight = CurLightShafts.m_IterationHeight * HeightMultiplier;

		const kbVec3 worldVecToShaft = m_pCurrentRenderWindow->GetCameraPosition() + CurLightShafts.m_Rotation.ToMat4()[2].ToVec3() * -3000.0f;
		kbVec4 shaftScreenPos = kbVec4( worldVecToShaft ).transform_point( m_pCurrentRenderWindow->GetViewProjectionMatrix(), false );
		if ( shaftScreenPos.w < 0.0f ) {
			continue;
		}
		shaftScreenPos /= shaftScreenPos.w;

		// Generate Flare Mask
		{
			kbMat4 mvpMatrix;
			mvpMatrix.make_identity();
			mvpMatrix.make_scale( kbVec3( CurLightShafts.m_Width * 0.5f, HalfBaseHeight, 1.0f ) );
			mvpMatrix[3] = kbVec3((shaftScreenPos.x * 0.5f) + 0.5f, (shaftScreenPos.y * -0.5f) + 0.5f, 0.0f );
			mvpMatrix[3].x -= CurLightShafts.m_Width * 0.25f;
			mvpMatrix[3].y -= HalfBaseHeight * 0.5f;

			m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(SCRATCH_BUFFER)->m_pRenderTargetView, nullptr );
		
			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = ( float )GetRenderTarget_DX11(SCRATCH_BUFFER)->GetWidth();
			viewport.Height = ( float )GetRenderTarget_DX11(SCRATCH_BUFFER)->GetHeight();
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
			m_pDeviceContext->RSSetViewports( 1, &viewport );

			ID3D11ShaderResourceView *const  RenderTargetViews[] = { (ID3D11ShaderResourceView*)CurLightShafts.m_pTexture->GetGPUTexture(), GetRenderTarget_DX11(DEPTH_BUFFER)->m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState, m_pShadowMapSamplerState };
			m_pDeviceContext->PSSetShaderResources( 0, 2, RenderTargetViews );
			m_pDeviceContext->PSSetSamplers( 0, 2, SamplerStates );

			m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*)m_pLightShaftsShader->GetVertexLayout() );
			m_pDeviceContext->VSSetShader( (ID3D11VertexShader *)m_pLightShaftsShader->GetVertexShader(), nullptr, 0 );
			m_pDeviceContext->PSSetShader( (ID3D11PixelShader *)m_pLightShaftsShader->GetPixelShader(), nullptr, 0 );

			const auto & varBindings = m_pLightShaftsShader->GetShaderVarBindings();
			ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			blk::error_check( SUCCEEDED(hr), "kbRenderer_DX11::RenderLightShafts() - Failed to map matrix buffer" );

			SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );
			SetShaderVec4( "color", CurLightShafts.m_Color, mappedResource.pData, varBindings );

			m_pDeviceContext->Unmap( pConstantBuffer, 0 );
			m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
			m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

			m_pDeviceContext->Draw( 6, 0 );
		}

		// Flare out here
		{
			const float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			m_pDeviceContext->ClearRenderTargetView( GetRenderTarget_DX11(DOWN_RES_BUFFER)->m_pRenderTargetView, color );
			m_pDeviceContext->OMSetRenderTargets( 1, &GetRenderTarget_DX11(DOWN_RES_BUFFER)->m_pRenderTargetView, nullptr );

			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = ( float )GetRenderTarget_DX11(DOWN_RES_BUFFER)->GetWidth();
			viewport.Height = ( float )GetRenderTarget_DX11(DOWN_RES_BUFFER)->GetHeight();
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
			m_pDeviceContext->RSSetViewports( 1, &viewport );	

			ID3D11ShaderResourceView *const  RenderTargetViews[] = { GetRenderTarget_DX11(SCRATCH_BUFFER)->m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
			m_pDeviceContext->PSSetShaderResources( 0, 1, RenderTargetViews );
			m_pDeviceContext->PSSetSamplers( 0, 1, SamplerStates );

			m_pDeviceContext->IASetInputLayout( (ID3D11InputLayout*) m_pGodRayIterationShader->GetVertexLayout() );
			m_pDeviceContext->VSSetShader( (ID3D11VertexShader *) m_pGodRayIterationShader->GetVertexShader(), nullptr, 0 );
			m_pDeviceContext->PSSetShader( (ID3D11PixelShader *) m_pGodRayIterationShader->GetPixelShader(), nullptr, 0 );

			m_RenderState.SetBlendState( m_pGodRayIterationShader );

			const auto & varBindings = m_pGodRayIterationShader->GetShaderVarBindings();
			ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

			kbMat4 mvpMatrix;
			mvpMatrix.make_identity();
			mvpMatrix.make_scale( kbVec3( CurLightShafts.m_Width * 0.5f, HalfBaseHeight, 1.0f ) );
			mvpMatrix[3] = kbVec3(shaftScreenPos.x, shaftScreenPos.y, 0.0f );

			for ( int itr = 0; itr < CurLightShafts.m_NumIterations; itr++ ) {

				kbVec3 curScale( CurLightShafts.m_IterationWidth * (float)itr, HalfIterationHeight * (float)itr, 1.0f );
				mvpMatrix[0].x += curScale.x;
				mvpMatrix[1].y += curScale.y;

				D3D11_MAPPED_SUBRESOURCE mappedResource;
				HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
				blk::error_check( SUCCEEDED(hr), "kbRenderer_DX11::RenderLightShafts() - Failed to map matrix buffer" );

				SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );

				m_pDeviceContext->Unmap( pConstantBuffer, 0 );
				m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
				m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

				m_pDeviceContext->Draw( 6, 0 );
			}
		}

		// Final Render To Screen
		{
			const kbMat4 mvpMatrix = kbMat4::identity;

			D3D11_VIEWPORT viewport;
			viewport.Width = (float)Back_Buffer_Width;
			viewport.Height = (float)Back_Buffer_Height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;

			m_pDeviceContext->RSSetViewports( 1, &viewport );
			m_pDeviceContext->OMSetRenderTargets( 1, &GetAccumBuffer( m_iAccumBuffer )->m_pRenderTargetView, nullptr );

			ID3D11ShaderResourceView *const  RenderTargetViews[] = { GetRenderTarget_DX11(DOWN_RES_BUFFER)->m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
			m_pDeviceContext->PSSetShaderResources( 0, 1, RenderTargetViews );
			m_pDeviceContext->PSSetSamplers( 0, 1, SamplerStates );

			const auto & varBindings = m_pSimpleAdditiveShader->GetShaderVarBindings();
			ID3D11Buffer *const pConstantBuffer = GetConstantBuffer( varBindings.m_ConstantBufferSizeBytes );

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pDeviceContext->Map( pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			blk::error_check( SUCCEEDED(hr), "kbRenderer_DX11::RenderLightShafts() - Failed to map matrix buffer" );

			SetShaderMat4( "mvpMatrix", mvpMatrix, mappedResource.pData, varBindings );

			m_pDeviceContext->Unmap( pConstantBuffer, 0 );
			m_pDeviceContext->VSSetConstantBuffers( 0, 1, &pConstantBuffer );
			m_pDeviceContext->PSSetConstantBuffers( 0, 1, &pConstantBuffer );

			m_pDeviceContext->Draw( 6, 0 );
		}
		ID3D11ShaderResourceView *const nullArray[] = { nullptr };
		m_pDeviceContext->PSSetShaderResources( 0, 1, nullArray );
		m_RenderState.SetBlendState();
	}

	D3D11_VIEWPORT viewport;
	viewport.Width = (float) Back_Buffer_Width;
	viewport.Height = (float) Back_Buffer_Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	m_pDeviceContext->RSSetViewports( 1, &viewport );
	m_pDeviceContext->OMSetRenderTargets( 1, &GetAccumBuffer( m_iAccumBuffer )->m_pRenderTargetView, nullptr );
}