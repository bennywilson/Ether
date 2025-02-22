/// sw_defs.cpp
///
/// 2025 blk 1.0

#include "blk_core.h"
#include "Renderer_Sw.h"
#include "kbGameEntityHeader.h"
#include "model_component.h"
#include "sw_defs.h"

void TrianglePipeline::set_view_proj(const Mat4& view, const Mat4& proj) {
	m_view_mat = view;
	m_proj_mat = proj;
	m_view_proj = m_view_mat * m_proj_mat;
}

static int orient2d(const Vec2i& a, const Vec2i& b, const Vec2i& c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void TrianglePipeline::render(const unordered_set<const RenderComponent*>& comp, vector<u32>& color, vector<f32>& depth, const Vec2i frame_dim) {
	for (auto render_comp : comp) {
		if (render_comp->IsA(kbStaticModelComponent::GetType())) {
			kbStaticModelComponent* const skel_comp = (kbStaticModelComponent*)render_comp;
			const kbModel* const model = skel_comp->model();

			Mat4 world_mat;
			world_mat.make_scale(render_comp->owner_scale());
			world_mat *= render_comp->owner_rotation().to_mat4();
			world_mat[3] = render_comp->owner_position();

			Mat4 final_mat = world_mat * m_view_proj;
			const auto& meshes = model->GetMeshes();
			const auto& vertices = model->GetCPUVertices();
			const auto& indices = model->GetCPUIndices();

			// Shader params
			const auto& shader_params = render_comp->Materials()[0].shader_params();
			Vec4 shader_param_color(1.f, 1.f, 1.f, 1.f);
			const kbTexture* color_tex = nullptr;
			for (const auto& param : shader_params) {
				const kbString& param_name = param.param_name();
				if (param_name == "color") {
					shader_param_color = param.vector();
				} else if (param_name == "color_tex") {
					color_tex = param.texture();
				}
			}

			u32 tex_width = 0;
			u32 tex_height = 0;
			const u8* cpu_tex = ((kbTexture*)color_tex)->cpu_texture(tex_width, tex_height);

			for (size_t i = 0; i < indices.size(); i += 3) {
				//vertexLayout screen_verts[3];
				struct ScreenVert {
					Vec2i pos;
					f32 z;
					u32 idx;
				};
				ScreenVert v[3];

				for (size_t idx = 0; idx < 3; idx++) {
					const auto& v1 = vertices[indices[i + idx]];
					Vec4 vertex_pos = v1.position.extend(1.f);
					vertex_pos.x *= -1.f;
					vertex_pos.z *= -1.f;
					vertex_pos = vertex_pos.transform_point(final_mat, true);
					const u32 x = (u32)((vertex_pos.x * 0.5f + 0.5f) * frame_dim.x);
					const u32 y = (u32)((vertex_pos.y * -0.5f + 0.5f) * frame_dim.y);

					v[idx].idx = indices[i + idx];
					v[idx].pos.x = x;
					v[idx].pos.y = y;
					v[idx].z = vertex_pos.z;
				}
				/*const size_t screen_idx = (size_t)(x + y * m_frame_width) * 4;
				if (screen_idx >= tex_data.size()) {
					continue;
				}*/
				// Compute triangle bounding box
				i32 minX = min3(v[0].pos.x, v[1].pos.x, v[2].pos.x);
				i32 minY = min3(v[0].pos.y, v[1].pos.y, v[2].pos.y);
				i32 maxX = max3(v[0].pos.x, v[1].pos.x, v[2].pos.x);
				i32 maxY = max3(v[0].pos.y, v[1].pos.y, v[2].pos.y);

				// Clip against screen bounds
				minX = max(minX, 0);
				minY = max(minY, 0);
				maxX = min(maxX, (i32)frame_dim.x - 1);
				maxY = min(maxY, (i32)frame_dim.y - 1);

				// Rasterize
				Vec2i p;
				for (p.y = minY; p.y <= maxY; p.y++) {
					for (p.x = minX; p.x <= maxX; p.x++) {
						// Determine barycentric coordinates
						int w0 = orient2d(v[1].pos, v[2].pos, p);
						int w1 = orient2d(v[2].pos, v[0].pos, p);
						int w2 = orient2d(v[0].pos, v[1].pos, p);

						// If p is on or inside all edges, render pixel.
						if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
							const i32 sum = w0 + w1 + w2;
							const f32 bary0 = w0 / (f32)sum;
							const f32 bary1 = w1 / (f32)sum;
							const f32 bary2 = w2 / (f32)sum;

							const size_t screen_idx = (size_t)(p.x + p.y * frame_dim.x);
							const f32 z = v[0].z * bary0 + v[1].z * bary1 + v[2].z * bary2;
							if (z > depth[screen_idx]) {
								continue;
							}

							depth[screen_idx] = z;
							Vec3 normal = vertices[v[0].idx].GetNormal() * bary0;
							normal += vertices[v[1].idx].GetNormal() * bary1;
							normal += vertices[v[2].idx].GetNormal() * bary2;
							normal.x *= -1.f;
							normal.z *= -1.f;

							Vec2 uv = vertices[v[0].idx].uv * bary0;
							uv += vertices[v[1].idx].uv * bary1;
							uv += vertices[v[2].idx].uv * bary2;
							u32 x = (u32)(uv.x * tex_width);
							u32 y = (u32)(uv.y * tex_height);
							u32 tex_idx = 4 * (x + (y * tex_width));

							const Vec4 albedo = Vec4(cpu_tex[tex_idx + 0] / 255.f, cpu_tex[tex_idx + 1] / 255.f, cpu_tex[tex_idx + 2] / 255.f, 1.f) * shader_param_color;
							const f32 dot = clamp(normal.dot(Vec3(0.707f, 0.707f, 0.0)), 0.f, 1.0f) * 0.85f + 0.15f;
							const Vec4 sun_color = Vec4(0x75 / 255.f, 0x56 / 255.f, 0xd8 / 255.f, 1.f) * 1.7f;
							const Vec4 diffuse = sun_color * dot;
							const Vec4 final_color = (albedo * diffuse).saturate();
							u8 val = (u8)(255 * dot);
							color[screen_idx] =
								((u32)(final_color.x * 255)) |
								((u32)(final_color.y * 255) << 8) |
								((u32)(final_color.z * 255) << 16) |
								(0xff) << 24;

						}
					}
				}
			}
		}
	}
}

/// KuwaharaPipeline::render
void KuwaharaPipeline::render(
	const unordered_set<const RenderComponent*>& comp,
	vector<u32>& color,
	vector<f32>& depth,
	const Vec2i frame_dim) {

	for (u32 y = 0; y < frame_dim.y; y++) {
		for (u32 x = 0; x < frame_dim.x; x++) {
			const u32 idx = x + y * frame_dim.x;
			//color[idx] = color[idx] & 0x0f0f0fff;
		}
	}
}