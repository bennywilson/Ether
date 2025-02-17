/// renderer_sw.cpp
///
/// 2025 blk 1.0

#include "blk_core.h"
#include "renderer_sw.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"

///	Renderer_Sw::Renderer_Sw
Renderer_Sw::~Renderer_Sw() {

}

///	Renderer_Sw::initialize_internal
void Renderer_Sw::initialize_internal(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {

	blk::error_check(SDL_Init(SDL_INIT_VIDEO),
		"Renderer_Sw::initialize_internal() - failed to init SDL");

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Blk 1.0 - Software Rasterizer");
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, frame_width);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, frame_height);
	// For window flags you should use separate window creation properties,
	// but for easier migration from SDL2 you can use the following:*/
	SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, hwnd);

////	SDL_PropertiesID id;

	window = SDL_CreateWindowWithProperties(props);

	renderer = SDL_CreateRenderer(window, "Blk 1.0");
	//SDL_SetWindowTitle(sdlWnd, "SDL Window - Set by SDL");
	//SDL_Surface* s = SDL_GetWindowSurface(sdlWnd);
	//SDL_FillRect(s, &s->clip_rect, 0xffff00ff);
	//SDL_UpdateWindowSurface(sdlWnd);
//SDL_GetWMInfo();

	//blk::error_check(SDL_CreateWindowAndRenderer("examples/renderer/lines", 640, 480, SDL_WindowID, &window, &renderer),
	//	"Renderer_Sw::initialize_internal() - failed to init SDL");


	/*SDL_Texture* windowTexture = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

	unsigned int* lockedPixels = nullptr;
	std::vector<int> pixels(screenHeight * screenWidth * 4, 0);
	int pitch = 0;

	int start = (y * screenWidth + x) * 4;
	pixels[start + 0] = B;
	pixels[start + 1] = G;
	pixels[start + 2] = R;
	pixels[start + 3] = A;

	SDL_UpdateTexture(windowTexture, nullptr, pixels.data(), screenWidth * 4);*/
}
/// Renderer_Dx12::shut_down_internal
void Renderer_Sw::shut_down_internal() {

}

///	Renderer_Sw::render
void Renderer_Sw::render() {
	SDL_Surface* s = SDL_GetWindowSurface(window);

	SDL_FRect new_rect = {};
	new_rect.x = 0;
	new_rect.y = 0;
	new_rect.w = 1050;
	new_rect.h = 1500;

	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderFillRect(renderer, &new_rect);


	SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
}

///	Renderer_Sw::create_render_buffer_internal
RenderBuffer* Renderer_Sw::create_render_buffer_internal() {
	return nullptr;
}

///	Renderer_Sw::create_pipeline
RenderPipeline* Renderer_Sw::create_pipeline(const std::wstring& path) {
	return nullptr;
}