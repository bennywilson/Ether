/// sw_defs.h
///
/// 2025 blk 1.0

#pragma once

using namespace std;

/// RenderPipeline_Sw
class RenderPipeline_Sw : public RenderPipeline {
public:
	virtual void render(const unordered_set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& m_frame_dim) = 0;

protected:
	~RenderPipeline_Sw() {}

	virtual void release() {}
};

/// TrianglePipeline
class TrianglePipeline : public RenderPipeline_Sw {
public:
	~TrianglePipeline() {}

	virtual void render(const unordered_set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& m_frame_dim) override;

	void set_view_proj(const Mat4& view, const Mat4& proj);

protected:

private:
	Mat4 m_view_mat;
	Mat4 m_proj_mat;
	Mat4 m_view_proj;

	Vec2i m_screen_dim;
};

/// PostProcess
class PostProcessPipeline : public RenderPipeline_Sw {
public:
	~PostProcessPipeline() {}

	virtual void render(const unordered_set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& m_frame_dim) override {}

protected:

private:
	Mat4 m_view_mat;
	Mat4 m_proj_mat;
	Mat4 m_view_proj;
};

/// KuwaharaPipeline
class KuwaharaPipeline : public RenderPipeline_Sw {
public:
	~KuwaharaPipeline() {}

	virtual void render(const unordered_set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& frame_dim) override;

private:
};

/// OutlinePipeline
class OutlinePipeline : public RenderPipeline_Sw {
public:
	~OutlinePipeline() {}

	virtual void render(const unordered_set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& frame_dim) override;
};