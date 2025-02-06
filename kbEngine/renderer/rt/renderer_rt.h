#pragma once

/// renderer_rt.h	
///
/// 2025 blk 1.0

#pragma once

#include "renderer.h"

///	Renderer_RT
class Renderer_Rt : public Renderer {
public:
	~Renderer_Rt();

	virtual void shut_down() override;

	virtual void render() override;

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;
};
