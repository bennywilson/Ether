//==============================================================================
// kbLightRendering.cpp
//
//
// 2017 kbEngine 2.0
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
	START_SCOPED_TIMER( RENDER_LIGHTING );

	std::map< const kbLightComponent *, kbRenderLight * >::iterator iter;
	for ( iter = m_pCurrentRenderWindow->m_RenderLightMap.begin(); iter != m_pCurrentRenderWindow->m_RenderLightMap.end(); iter++ ) {
		RenderLight( iter->second );
		ID3D11ShaderResourceView * const nullRTViews[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		m_pImmediateContext->PSSetShaderResources( 0, 8, nullRTViews );
	}

	ID3D11ShaderResourceView * const nullRTViews[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pImmediateContext->PSSetShaderResources( 0, 8, nullRTViews );
	m_pImmediateContext->OMSetBlendState( nullptr, nullptr, 0xffffffff );
}

/**
 *	kbRenderer_DX11::RenderLight
 */
kbVec3 frozenCameraPosition;

void kbRenderer_DX11::RenderLight( const kbRenderLight *const pLight ) {

	// Matrices that are scaled to 0 will produce a 0 depth in the projection shader and not shadow the pixel
	kbMat4 splitMatrices[4] = { kbMat4::identity, kbMat4::identity, kbMat4::identity, kbMat4::identity };
	splitMatrices[0].MakeScale( kbVec3::zero );
	splitMatrices[1].MakeScale( kbVec3::zero );
	splitMatrices[2].MakeScale( kbVec3::zero );
	splitMatrices[3].MakeScale( kbVec3::zero );

	if ( pLight->m_bCastsShadow ) {
		m_RenderState.SetBlendState();
		RenderShadow( pLight, splitMatrices );
	}

	START_SCOPED_TIMER( RENDER_LIGHT );

	// Render Light
	m_RenderState.SetBlendState( false,
								 false,
								 true,
								 kbRenderState::BF_One,
								 kbRenderState::BF_One,
								 kbRenderState::BO_Add,
								 kbRenderState::BF_One,
								 kbRenderState::BF_Zero,
								 kbRenderState::BO_Add,
							     kbRenderState::CW_All );


	m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, NULL );

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;
	m_pImmediateContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pImmediateContext->RSSetState( m_pRasterizerState );

	ID3D11ShaderResourceView *const  RenderTargetViews[] = {	m_RenderTargets[COLOR_BUFFER].m_pShaderResourceView, 
																m_RenderTargets[NORMAL_BUFFER].m_pShaderResourceView, 
																m_RenderTargets[DEPTH_BUFFER].m_pShaderResourceView,
																m_RenderTargets[SHADOW_BUFFER].m_pShaderResourceView };

	ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState, m_pNormalMapSamplerState, m_pShadowMapSamplerState, m_pShadowMapSamplerState };
	m_pImmediateContext->PSSetShaderResources( 0, 4, RenderTargetViews );
	m_pImmediateContext->PSSetSamplers( 0, 4, SamplerStates );

	kbShader * pShader = nullptr;
	const kbLightComponent *const pLightComponent = pLight->m_pLightComponent;
	
	if ( pLightComponent->IsA( kbDirectionalLightComponent::GetType() ) ) {
		pShader = m_pDirectionalLightShader;
	} else if ( pLightComponent->IsA( kbCylindricalLightComponent::GetType() ) ) {
		pShader = m_pCylindricalLightShader;
	} else {
		pShader = m_pPointLightShader;
	}

	m_pImmediateContext->IASetInputLayout( (ID3D11InputLayout*)pShader->GetVertexLayout() );
	m_pImmediateContext->VSSetShader( (ID3D11VertexShader *)pShader->GetVertexShader(), NULL, 0 );
	m_pImmediateContext->PSSetShader( (ID3D11PixelShader *)pShader->GetPixelShader(), NULL, 0 );

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HRESULT hr = m_pImmediateContext->Map( m_pLightShaderConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	if ( FAILED( hr ) ) {
		kbError( "Failed to map matrix buffer" );
	}

	LightShaderConstants sourceBuffer;
	sourceBuffer.lightDirection = -pLight->m_Orientation.ToMat4()[2].ToVec3();
	sourceBuffer.lightDirection.w = pLight->m_Length;
	sourceBuffer.lightColor = pLight->m_Color;
	sourceBuffer.inverseViewProjection = m_pCurrentRenderWindow->m_InverseViewProjectionMatrix;
	sourceBuffer.cameraPosition = frozenCameraPosition;

	for ( int i = 0; i < 4; i++ ) {
		sourceBuffer.lightMatrix[i] = splitMatrices[i];
		sourceBuffer.splitDistances[i] = pLight->m_CascadedShadowSplits[i];
	}

	if ( m_bRenderToHMD ) {
		sourceBuffer.mvpMatrix.MakeScale( kbVec3( 0.5f, 1.0f, 1.0f ) );
	} else {
		sourceBuffer.mvpMatrix.MakeIdentity();
	}
	sourceBuffer.lightPosition.Set( pLight->m_Position.x, pLight->m_Position.y, pLight->m_Position.z, pLight->m_Radius );

	LightShaderConstants * dataPtr = ( LightShaderConstants * ) mappedResource.pData;
	memcpy( dataPtr, &sourceBuffer, sizeof( LightShaderConstants ) );

	m_pImmediateContext->Unmap( m_pLightShaderConstantsBuffer, 0 );
	m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pLightShaderConstantsBuffer );
	m_pImmediateContext->PSSetConstantBuffers( 0, 1, &m_pLightShaderConstantsBuffer );

	m_pImmediateContext->Draw( 6, 0 );

	ID3D11ShaderResourceView * nullArray[] = { NULL };

	m_pImmediateContext->PSSetShaderResources( 0, 1, nullArray );

	PLACE_GPU_TIME_STAMP( "Light Rendering" );
}

/**
 *	kbRenderer_DX11::RenderShadow
 */
void kbRenderer_DX11::RenderShadow( const kbRenderLight *const pLight, kbMat4 splitMatrices[] ) {

	if ( g_ShowShadows.GetBool() == false ) {
		return;
	}

	START_SCOPED_TIMER( RENDER_SHADOW_DEPTH );

	if ( m_RenderTargets[SHADOW_BUFFER].m_bIsDirty ) {
		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_pImmediateContext->ClearRenderTargetView( m_RenderTargets[SHADOW_BUFFER].m_pRenderTargetView, color );
		m_pImmediateContext->ClearDepthStencilView( m_RenderTargets[SHADOW_BUFFER_DEPTH].m_pDepthStencilView , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );
		m_RenderTargets[SHADOW_BUFFER].m_bIsDirty = false;
	}

	if ( m_bRenderToHMD ) {
		const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_pImmediateContext->ClearRenderTargetView( m_RenderTargets[SHADOW_BUFFER].m_pRenderTargetView, color );
		return;
	}

	const float shadowBufferSize = (float) m_RenderTargets[SHADOW_BUFFER].m_Width;
	const float halfShadowBufferSize = shadowBufferSize * 0.5f;

	kbMat4 textureMatrix;
	textureMatrix.MakeIdentity();
	textureMatrix[0].x = 0.5f;
	textureMatrix[1].y = -0.5f;
	textureMatrix[3].x = 0.5f + ( 0.5f / shadowBufferSize );
	textureMatrix[3].y = 0.5f + ( 0.5f / shadowBufferSize );

	// Render Shadow map
	m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[SHADOW_BUFFER].m_pRenderTargetView, m_RenderTargets[SHADOW_BUFFER_DEPTH].m_pDepthStencilView );

	const kbMat4 oldViewMatrix = m_pCurrentRenderWindow->m_ViewMatrix;
	const kbMat4 oldProjectionMatrix = m_pCurrentRenderWindow->m_ProjectionMatrix;
	const kbMat4 oldViewProjMatrix = m_pCurrentRenderWindow->m_ViewProjectionMatrix;

	kbPlane frustumPlanes[6];
	kbVec3 upperLeft, upperRight, lowerRight, lowerLeft, dummyPoint;

	static kbMat4 frozenMatrix = m_pCurrentRenderWindow->m_ViewProjectionMatrix;
	static kbVec3 camDir = m_pCurrentRenderWindow->m_CameraRotation.ToMat4()[2].ToVec3();

	// Toggle between normal cam and debug cam
	static bool bKeyDown = false;
	static bool debuggingShadowBounds = false;
	if ( g_DebugShadowBounds.GetBool() != debuggingShadowBounds ) {
		debuggingShadowBounds = g_DebugShadowBounds.GetBool();
		if ( debuggingShadowBounds ) {
			debuggingShadowBounds = true;
			frozenMatrix = m_pCurrentRenderWindow->m_ViewProjectionMatrix;
			frozenCameraPosition = m_pCurrentRenderWindow->m_CameraPosition;
			camDir = m_pCurrentRenderWindow->m_CameraRotation.ToMat4()[2].ToVec3();
		}
	}

	if ( debuggingShadowBounds == false ) {
		frozenMatrix = m_pCurrentRenderWindow->m_ViewProjectionMatrix;
		frozenCameraPosition = m_pCurrentRenderWindow->m_CameraPosition;
		camDir = m_pCurrentRenderWindow->m_CameraRotation.ToMat4()[2].ToVec3();
	}

	frozenMatrix.GetLeftClipPlane( frustumPlanes[0] );
	frozenMatrix.GetTopClipPlane( frustumPlanes[1] );
	frozenMatrix.GetRightClipPlane( frustumPlanes[2]);
	frozenMatrix.GetBottomClipPlane( frustumPlanes[3] );
	frozenMatrix.GetNearClipPlane( frustumPlanes[4] );
	frozenMatrix.GetFarClipPlane( frustumPlanes[5] );

	frustumPlanes[1].PlanesIntersect( dummyPoint, upperLeft, frustumPlanes[0] );
	frustumPlanes[2].PlanesIntersect( dummyPoint, upperRight, frustumPlanes[1] );
	frustumPlanes[3].PlanesIntersect( dummyPoint, lowerRight, frustumPlanes[2] );
	frustumPlanes[0].PlanesIntersect( dummyPoint, lowerLeft, frustumPlanes[3] );

	const float DistToFarCorner = kbRenderer_DX11::Far_Plane / ( camDir.Dot( upperLeft ) );
	const float NearCornerDist = ( ( kbRenderer_DX11::Near_Plane ) * DistToFarCorner ) / kbRenderer_DX11::Far_Plane;

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

		m_DebugLines.push_back( lines[0] );
		m_DebugLines.push_back( lines[1] );
		m_DebugLines.push_back( lines[2] );
		m_DebugLines.push_back( lines[3] );
		m_DebugLines.push_back( lines[4] );
		m_DebugLines.push_back( lines[5] );
		m_DebugLines.push_back( lines[6] );
		m_DebugLines.push_back( lines[7] );
	}

	for ( int i = 0; i < 4 && pLight->m_CascadedShadowSplits[i] < FLT_MAX; i++ ) {
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = ( i % 2 ) * halfShadowBufferSize;
		viewport.TopLeftY = ( i / 2 ) * halfShadowBufferSize;
		viewport.Width = halfShadowBufferSize;
		viewport.Height = halfShadowBufferSize;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_pImmediateContext->RSSetViewports( 1, &viewport );

		// Calculate shadow map bounds
		const float prevDist = ( i == 0 ) ? ( 0.0f ) : ( pLight->m_CascadedShadowSplits[i] - 1 );
		const kbVec3 lookAtPoint = frozenCameraPosition + camDir * ( prevDist + ( pLight->m_CascadedShadowSplits[i] - prevDist ) * 0.5f );
		const float halfFOV = kbToRadians ( 75.0f ) * 0.5f;
		const float distToCorner = pLight->m_CascadedShadowSplits[i] / ( cos( halfFOV ) );
		kbVec3 cornerVert = frozenCameraPosition + distToCorner * upperLeft;
		const float boundsLength = ( lookAtPoint - cornerVert ).Length();

		// Debug Drawing -
		if ( debuggingShadowBounds ) {
			vertexLayout start, end;
			start.SetColor( kbVec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
			end.SetColor( kbVec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
	
			start.position = frozenCameraPosition + distToCorner * upperLeft;
			end.position = frozenCameraPosition + distToCorner * upperRight;
			m_DebugLines.push_back( start );
			m_DebugLines.push_back( end );
		
			start.position = frozenCameraPosition + distToCorner * upperRight;
			end.position = frozenCameraPosition + distToCorner * lowerRight;
			m_DebugLines.push_back( start );
			m_DebugLines.push_back( end );
	
			start.position = frozenCameraPosition + distToCorner * lowerRight;
			end.position = frozenCameraPosition + distToCorner * lowerLeft;
			m_DebugLines.push_back( start );
			m_DebugLines.push_back( end );
	
			start.position = frozenCameraPosition + distToCorner * lowerLeft;
			end.position = frozenCameraPosition + distToCorner * upperLeft;
			m_DebugLines.push_back( start );
			m_DebugLines.push_back( end );
		}
		// --------------------------------------------------
		kbVec3 lightDir = -pLight->m_Orientation.ToMat4()[2].ToVec3();

		kbMat4 lightViewMatrix;
		lightViewMatrix.LookAt( lookAtPoint + lightDir * boundsLength * 10.0f, lookAtPoint, kbVec3( 0.0f, 1.0f, 0.0f ) );

		kbMat4 lightProjMatrix;
		lightProjMatrix.OrthoLH( boundsLength * 2.0f, boundsLength * 2.0f, 10.0f, boundsLength * 40.0f );

		const kbMat4 lightViewProjMatrix = lightViewMatrix * lightProjMatrix;
		const float texelSize = 2.0f / (shadowBufferSize * 0.5f);
		kbVec4 projCenter( 0.0f, 0.0f, 0.0f, 1.0f );
		projCenter = projCenter.TransformPoint( lightViewProjMatrix, true);

		const float fracX = fmod( projCenter.x, texelSize );
		const float fracY = fmod( projCenter.y, texelSize );
        
		kbMat4 offset;
		offset.MakeIdentity();
		offset[3][0] = -fracX;
		offset[3][1] = -fracY;

		splitMatrices[i] = lightViewProjMatrix * offset * textureMatrix;

		m_pCurrentRenderWindow->m_ViewMatrix = lightViewMatrix;
		m_pCurrentRenderWindow->m_ProjectionMatrix = lightProjMatrix;
		m_pCurrentRenderWindow->m_ViewProjectionMatrix = lightViewProjMatrix * offset;

		std::map< const kbComponent *, kbRenderObject * >::iterator iter;
		for ( iter = m_pCurrentRenderWindow->m_RenderObjectMap.begin(); iter != m_pCurrentRenderWindow->m_RenderObjectMap.end(); iter++ ) {
			if ( iter->second->m_RenderPass == RP_Lighting && iter->second->m_bCastsShadow ) {
				RenderModel( iter->second, RP_Lighting, true );
			}
		}
		m_RenderTargets[SHADOW_BUFFER].m_bIsDirty = true;
	}

	D3D11_VIEWPORT viewport;
	if ( m_bRenderToHMD ) {

		viewport.TopLeftX = ( float )m_EyeRenderViewport[m_HMDPass].Pos.x;
		viewport.TopLeftY = ( float )m_EyeRenderViewport[m_HMDPass].Pos.y;
		viewport.Width = ( float )m_EyeRenderViewport[m_HMDPass].Size.w;
		viewport.Height = ( float )m_EyeRenderViewport[m_HMDPass].Size.h;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;
	} else {
		viewport.Width = (float)Back_Buffer_Width;
		viewport.Height = (float)Back_Buffer_Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
	}
	m_pImmediateContext->RSSetViewports( 1, &viewport );

	m_pCurrentRenderWindow->m_ViewMatrix = oldViewMatrix;
	m_pCurrentRenderWindow->m_ProjectionMatrix = oldProjectionMatrix;
	m_pCurrentRenderWindow->m_ViewProjectionMatrix = oldViewProjMatrix;

	PLACE_GPU_TIME_STAMP( "Shadow Depth" );
}

/**
 *	kbRenderer_DX11::RenderLightShaft
 */
void kbRenderer_DX11::RenderLightShafts() {

	if ( m_bRenderToHMD ) {
		return;	
	}

	if ( g_ShowLightShafts.GetBool() == false ) {
		return;
	}

	START_SCOPED_TIMER( RENDER_LIGHTSHAFTS );

	const unsigned int stride = sizeof( vertexLayout );
	const unsigned int offset = 0;
	m_pImmediateContext->IASetVertexBuffers( 0, 1, &m_pUnitQuad, &stride, &offset );
	m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_pImmediateContext->RSSetState( m_pRasterizerState );

	for ( int i = 0; i < m_LightShafts_RenderThread.size(); i++ ) {

		const kbLightShafts & CurLightShafts = m_LightShafts_RenderThread[i];
		const float HeightMultiplier = (float)kbRenderer_DX11::Back_Buffer_Width / (float)kbRenderer_DX11::Back_Buffer_Height;
		const float HalfBaseHeight = CurLightShafts.m_Height * HeightMultiplier * 0.5f;
		const float HalfIterationHeight = CurLightShafts.m_IterationHeight * HeightMultiplier;

		const kbVec3 worldVecToShaft = m_pCurrentRenderWindow->m_CameraPosition + CurLightShafts.m_Rotation.ToMat4()[2].ToVec3() * -3000.0f;
		kbVec4 shaftScreenPos = kbVec4( worldVecToShaft ).TransformPoint( m_pCurrentRenderWindow->m_ViewProjectionMatrix, false );
		if ( shaftScreenPos.w < 0.0f ) {
			continue;
		}
		shaftScreenPos /= shaftScreenPos.w;

		// Generate Flare Mask
		{
			kbMat4 mvpMatrix;
			mvpMatrix.MakeIdentity();
			mvpMatrix.MakeScale( kbVec3( CurLightShafts.m_Width * 0.5f, HalfBaseHeight, 1.0f ) );
			mvpMatrix[3] = kbVec3((shaftScreenPos.x * 0.5f) + 0.5f, (shaftScreenPos.y * -0.5f) + 0.5f, 0.0f );
			mvpMatrix[3].x -= CurLightShafts.m_Width * 0.25f;
			mvpMatrix[3].y -= HalfBaseHeight * 0.5f;

			m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[SCRATCH_BUFFER].m_pRenderTargetView, NULL );
		
			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = ( float )m_RenderTargets[SCRATCH_BUFFER].m_Width;
			viewport.Height = ( float )m_RenderTargets[SCRATCH_BUFFER].m_Height;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
			m_pImmediateContext->RSSetViewports( 1, &viewport );

			ID3D11ShaderResourceView *const  RenderTargetViews[] = { (ID3D11ShaderResourceView*)CurLightShafts.m_pTexture->GetGPUTexture(), m_RenderTargets[DEPTH_BUFFER].m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState, m_pShadowMapSamplerState };
			m_pImmediateContext->PSSetShaderResources( 0, 2, RenderTargetViews );
			m_pImmediateContext->PSSetSamplers( 0, 2, SamplerStates );

			m_pImmediateContext->IASetInputLayout( (ID3D11InputLayout*)m_pLightShaftsShader->GetVertexLayout() );
			m_pImmediateContext->VSSetShader( (ID3D11VertexShader *)m_pLightShaftsShader->GetVertexShader(), NULL, 0 );
			m_pImmediateContext->PSSetShader( (ID3D11PixelShader *)m_pLightShaftsShader->GetPixelShader(), NULL, 0 );

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pImmediateContext->Map( m_pLightShaftsShaderConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			if ( FAILED( hr ) ) {
				kbError( "Failed to map matrix buffer" );
			}

			LightShaftsConstants sourceBuffer;
			sourceBuffer.mvpMatrix = mvpMatrix;
			sourceBuffer.color[0] = CurLightShafts.m_Color.r;
			sourceBuffer.color[1] = CurLightShafts.m_Color.g;
			sourceBuffer.color[2] = CurLightShafts.m_Color.b;
			sourceBuffer.color[3] = CurLightShafts.m_Color.a;

			LightShaftsConstants * dataPtr = ( LightShaftsConstants * ) mappedResource.pData;
			memcpy( dataPtr, &sourceBuffer, sizeof( LightShaftsConstants ) );

			m_pImmediateContext->Unmap( m_pLightShaftsShaderConstantsBuffer, 0 );
			m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pLightShaftsShaderConstantsBuffer );
			m_pImmediateContext->PSSetConstantBuffers( 0, 1, &m_pLightShaftsShaderConstantsBuffer );

			m_pImmediateContext->Draw( 6, 0 );
		}

		// Flare out here
		{
			const float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			m_pImmediateContext->ClearRenderTargetView( m_RenderTargets[DOWN_RES_BUFFER].m_pRenderTargetView, color );
			m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[DOWN_RES_BUFFER].m_pRenderTargetView, NULL );

			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = ( float )m_RenderTargets[DOWN_RES_BUFFER].m_Width;
			viewport.Height = ( float )m_RenderTargets[DOWN_RES_BUFFER].m_Height;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1.0f;
			m_pImmediateContext->RSSetViewports( 1, &viewport );	

			m_RenderState.SetBlendState( false,
										 false,
										 true,
										 kbRenderState::BF_One,
										 kbRenderState::BF_One,
										 kbRenderState::BO_Add,
										 kbRenderState::BF_One,
										 kbRenderState::BF_Zero,
										 kbRenderState::BO_Add,
										 kbRenderState::CW_All );


			ID3D11ShaderResourceView *const  RenderTargetViews[] = { m_RenderTargets[SCRATCH_BUFFER].m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
			m_pImmediateContext->PSSetShaderResources( 0, 1, RenderTargetViews );
			m_pImmediateContext->PSSetSamplers( 0, 1, SamplerStates );

			m_pImmediateContext->IASetInputLayout( (ID3D11InputLayout*)m_pSimpleAdditiveShader->GetVertexLayout() );
			m_pImmediateContext->VSSetShader( (ID3D11VertexShader *)this->m_pSimpleAdditiveShader->GetVertexShader(), NULL, 0 );
			m_pImmediateContext->PSSetShader( (ID3D11PixelShader *)m_pSimpleAdditiveShader->GetPixelShader(), NULL, 0 );

			kbMat4 mvpMatrix;
			mvpMatrix.MakeIdentity();
			mvpMatrix.MakeScale( kbVec3( CurLightShafts.m_Width * 0.5f, HalfBaseHeight, 1.0f ) );
			mvpMatrix[3] = kbVec3(shaftScreenPos.x, shaftScreenPos.y, 0.0f );

			for ( int itr = 0; itr < CurLightShafts.m_NumIterations; itr++ ) {

				kbVec3 curScale( CurLightShafts.m_IterationWidth * (float)itr, HalfIterationHeight * (float)itr, 1.0f );
				mvpMatrix[0].x += curScale.x;
				mvpMatrix[1].y += curScale.y;

				D3D11_MAPPED_SUBRESOURCE mappedResource;
				HRESULT hr = m_pImmediateContext->Map( m_pDefaultShaderConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
				if ( FAILED( hr ) ) {
					kbError( "Failed to map matrix buffer" );
				}

				ShaderConstantMatrices sourceBuffer;
				sourceBuffer.mvpMatrix = mvpMatrix;

				ShaderConstantMatrices * dataPtr = ( ShaderConstantMatrices * ) mappedResource.pData;
				memcpy( dataPtr, &sourceBuffer, sizeof( ShaderConstantMatrices ) );

				m_pImmediateContext->Unmap( m_pDefaultShaderConstantsBuffer, 0 );
				m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pDefaultShaderConstantsBuffer );
				m_pImmediateContext->PSSetConstantBuffers( 0, 1, &m_pDefaultShaderConstantsBuffer );

				m_pImmediateContext->Draw( 6, 0 );
			}
		}

		// Final Render To Screen
		{
			const kbMat4 mvpMatrix = kbMat4::identity;

			D3D11_VIEWPORT viewport;
			if ( m_bRenderToHMD ) {

				viewport.TopLeftX = ( float )m_EyeRenderViewport[m_HMDPass].Pos.x;
				viewport.TopLeftY = ( float )m_EyeRenderViewport[m_HMDPass].Pos.y;
				viewport.Width = ( float )m_EyeRenderViewport[m_HMDPass].Size.w;
				viewport.Height = ( float )m_EyeRenderViewport[m_HMDPass].Size.h;
				viewport.MinDepth = 0;
				viewport.MaxDepth = 1.0f;
			} else {
				viewport.Width = (float)Back_Buffer_Width;
				viewport.Height = (float)Back_Buffer_Height;
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;
				viewport.TopLeftX = 0;
				viewport.TopLeftY = 0;
			}
			m_pImmediateContext->RSSetViewports( 1, &viewport );
			m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, NULL );

			ID3D11ShaderResourceView *const  RenderTargetViews[] = { m_RenderTargets[DOWN_RES_BUFFER].m_pShaderResourceView };
			ID3D11SamplerState *const  SamplerStates[] = { m_pBasicSamplerState };
			m_pImmediateContext->PSSetShaderResources( 0, 1, RenderTargetViews );
			m_pImmediateContext->PSSetSamplers( 0, 1, SamplerStates );

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pImmediateContext->Map( m_pDefaultShaderConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
			if ( FAILED( hr ) ) {
				kbError( "Failed to map matrix buffer" );
			}

			ShaderConstantMatrices sourceBuffer;
			sourceBuffer.mvpMatrix = mvpMatrix;

			ShaderConstantMatrices * dataPtr = ( ShaderConstantMatrices * ) mappedResource.pData;
			memcpy( dataPtr, &sourceBuffer, sizeof( ShaderConstantMatrices ) );

			m_pImmediateContext->Unmap( m_pDefaultShaderConstantsBuffer, 0 );
			m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pDefaultShaderConstantsBuffer );
			m_pImmediateContext->PSSetConstantBuffers( 0, 1, &m_pDefaultShaderConstantsBuffer );

			m_pImmediateContext->Draw( 6, 0 );
		}
		ID3D11ShaderResourceView *const nullArray[] = { NULL };
		m_pImmediateContext->PSSetShaderResources( 0, 1, nullArray );
		m_pImmediateContext->OMSetBlendState( NULL, NULL, 0xffffffff );
	}

	D3D11_VIEWPORT viewport;
	if ( m_bRenderToHMD ) {

		viewport.TopLeftX = ( float )m_EyeRenderViewport[m_HMDPass].Pos.x;
		viewport.TopLeftY = ( float )m_EyeRenderViewport[m_HMDPass].Pos.y;
		viewport.Width = ( float )m_EyeRenderViewport[m_HMDPass].Size.w;
		viewport.Height = ( float )m_EyeRenderViewport[m_HMDPass].Size.h;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;
	} else {
		viewport.Width = (float)Back_Buffer_Width;
		viewport.Height = (float)Back_Buffer_Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
	}
	m_pImmediateContext->RSSetViewports( 1, &viewport );

	m_pImmediateContext->OMSetRenderTargets( 1, &m_RenderTargets[ACCUMULATION_BUFFER].m_pRenderTargetView, NULL );
}