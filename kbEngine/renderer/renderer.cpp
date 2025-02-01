/// renderer.cpp
///
/// 2025 kbEngine 2.0

#include "kbCore.h"
#include "renderer.h"

Renderer* g_renderer = nullptr;

/// Renderer::Renderer()
Renderer::Renderer() {
	g_renderer = this;
}

/// Renderer::~Renderer()
Renderer::~Renderer() {
	shut_down();
}

/// Renderer::initialize()
void Renderer::initialize(HWND hwnd, const uint32_t frame_width, const uint32_t frame_height) {
	m_frame_width = frame_width;
	m_frame_height = frame_height;
}