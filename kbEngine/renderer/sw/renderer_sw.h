#pragma once

#include <wrl/client.h>
#include "renderer.h"

struct SDL_Window;
struct SDL_Renderer;

///	Renderer_Sw
class Renderer_Sw : public Renderer {
public:
	~Renderer_Sw();

	virtual void render() override;

protected:
	virtual void initialize_internal(HWND hwnd, const uint32_t frameWidth, const uint32_t frameHeight) override;
	virtual void shut_down_internal() override;

	virtual RenderPipeline* create_pipeline(const std::wstring& path) override;
	virtual RenderBuffer* create_render_buffer_internal();

private:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
};
