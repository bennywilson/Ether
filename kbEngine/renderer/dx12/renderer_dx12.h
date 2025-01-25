//==============================================================================
// renderer_dx12.h
//
// Renderer implementation using DX12 API
//
// 2025 kbEngine 2.0
//==============================================================================
#ifndef _KBRENDERER_DX11_H_
#define _KBRENDERER_DX11_H_

#include <D3D11.h>
#include <DirectXMath.h>
#include "kbVector.h"

#include "kbRenderer.h"

using namespace DirectX;


///
///	renderer_dx12
///
class renderer_dx12 : public kbRenderer {

public:
	renderer_dx12();
	~renderer_dx12();
};

#endif