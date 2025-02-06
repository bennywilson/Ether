#pragma once

#include "renderer.h"

///	Renderer_Sw
class Renderer_Sw : public Renderer {
public:
	~Renderer_Sw();

	virtual void shut_down() override;

	virtual void render() override;

protected:
	virtual void initialize(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;
};
