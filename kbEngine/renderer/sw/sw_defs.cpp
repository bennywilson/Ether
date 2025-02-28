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

void TrianglePipeline::render(const set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& frame_dim) {
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
				const std::string param_name = param.param_name().c_str();
				if (param_name == "color") {
					shader_param_color = param.vector();
				} else if (param_name == "color_tex" || param_name == "shaderTexture") {
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

							const size_t depth_idx = (size_t)(p.x + p.y * frame_dim.x);
							const f32 z = v[0].z * bary0 + v[1].z * bary1 + v[2].z * bary2;
							if (z > depth[depth_idx]) {
								continue;
							}
							depth[depth_idx] = z;

							Vec3 normal = vertices[v[0].idx].GetNormal() * bary0;
							normal += vertices[v[1].idx].GetNormal() * bary1;
							normal += vertices[v[2].idx].GetNormal() * bary2;
							normal.x *= -1.f;
							normal.z *= -1.f;

							Vec2 uv = vertices[v[0].idx].uv * bary0;
							uv += vertices[v[1].idx].uv * bary1;
							uv += vertices[v[2].idx].uv * bary2;
							u32 x = clamp((u32)(uv.x * tex_width), (u32)0, tex_width - 1);
							u32 y = clamp((u32)(uv.y * tex_height), (u32)0, tex_height - 1);
							u32 tex_idx = 4 * (x + (y * tex_width));

							const Vec4 albedo = Vec4(cpu_tex[tex_idx + 0] / 255.f, cpu_tex[tex_idx + 1] / 255.f, cpu_tex[tex_idx + 2] / 255.f, 1.f) * shader_param_color;
							const f32 dot = clamp(normal.dot(Vec3(0.707f, 0.707f, 0.0)), 0.f, 1.0f) * 0.85f + 0.15f;
							const Vec4 sun_color = Vec4(0x75 / 255.f, 0x56 / 255.f, 0xd8 / 255.f, 1.f) * 1.7f;
							const Vec4 diffuse = sun_color * dot;
							const Vec4 final_color = albedo;//(albedo* diffuse).saturate();
							u8 val = (u8)(255 * dot);

							const size_t color_idx = depth_idx * 4;
							color[color_idx + 0] = (u8)(final_color.x * 255);
							color[color_idx + 1] = (u8)(final_color.y * 255);
							color[color_idx + 2] = (u8)(final_color.z * 255);
							color[color_idx + 3] = (u8)(final_color.w * 255);
						}
					}
				}
			}
		}
	}
}

/// KuwaharaPipeline::render
void KuwaharaPipeline::render(
	const set<const RenderComponent*>& comp,
	vector<u8>& color,
	vector<f32>& depth,
	const Vec2i& frame_dim) {

	const i32 half_filter_size = 3;
	const i32 frame_width = frame_dim.x;
	const i32 frame_height = frame_dim.y;

	struct MeanAndVariance_t {
		Vec3 mean = Vec3::zero;
		f32 variance = 0.f;
	};
	vector<MeanAndVariance_t> mean_variance;
	mean_variance.resize((size_t)frame_width * frame_height);

	// Find mean and variance of each pixel
	for (i32 y = 0; y < frame_height; y++) {
		for (i32 x = 0; x < frame_width; x++) {
			i32 num_samples = 0;
			Vec3 total_rgb = Vec3::zero;

			// Iterate over the filter and calculate mean
			for (i32 filterY = y - half_filter_size; filterY <= y + half_filter_size; filterY++) {
				if (filterY < 0 || filterY >= frame_height) {
					continue;
				}

				for (i32 filterX = x - half_filter_size; filterX <= x + half_filter_size; filterX++) {
					if (filterX < 0 || filterX >= frame_width) {
						continue;
					}

					num_samples++;

					const size_t index = (size_t)((filterY * frame_width) + filterX) * 4;
					total_rgb.x += color[index + 0];
					total_rgb.y += color[index + 1];
					total_rgb.z += color[index + 2];
				}
			}

			const i32 dst_idx = (y * frame_width) + x;
			mean_variance[dst_idx].mean[0] = (f32)total_rgb.x / (f32)num_samples;
			mean_variance[dst_idx].mean[1] = (f32)total_rgb.y / (f32)num_samples;
			mean_variance[dst_idx].mean[2] = (f32)total_rgb.z / (f32)num_samples;

			// Iterate over the filter and calculate variance
			Vec3 variance = Vec3::zero;

			for (i32 filterY = y - half_filter_size; filterY <= y + half_filter_size; filterY++) {
				if (filterY < 0 || filterY >= frame_height) {
					continue;
				}

				for (i32 filterX = x - half_filter_size; filterX <= x + half_filter_size; filterX++) {
					if (filterX < 0 || filterX >= frame_width) {
						continue;
					}

					const size_t src_idx = (size_t)((filterY * frame_width) + filterX) * 4;
					for (i32 i = 0; i < 3; i++) {
						const f32 value = (f32)color[src_idx + i];
						variance[i] += (value - mean_variance[dst_idx].mean[i]) * (value - mean_variance[dst_idx].mean[i]);
					}
				}
			}

			mean_variance[dst_idx].variance = variance.dot(variance);
		}
	}

	// For each pixel, find the neighbor with the lowest variance, and use its mean as the output color
	for (i32 y = 0; y < frame_height; y++) {
		for (i32 x = 0; x < frame_width; x++) {
			Vec3 rgb = Vec3::zero;
			f32 lowest_variance = FLT_MAX;

			for (i32 filterY = y - half_filter_size; filterY <= y + half_filter_size; filterY++) {
				if (filterY < 0 || filterY >= frame_height) {
					continue;
				}

				for (i32 filterX = x - half_filter_size; filterX <= x + half_filter_size; filterX++) {
					if (filterX < 0 || filterX >= frame_width) {
						continue;
					}

					const size_t src_idx = (size_t)((filterY * frame_width) + filterX);
					const f32 variance = mean_variance[src_idx].variance;
					if (variance < lowest_variance) {
						lowest_variance = variance;
						rgb.x = mean_variance[src_idx].mean.x;
						rgb.y = mean_variance[src_idx].mean.y;
						rgb.z = mean_variance[src_idx].mean.z;
					}
				}
			}

			const size_t dest_idx = (size_t)(((y * frame_width) + x) * 4);
			color[dest_idx + 0] = (u8)clamp(rgb.x, 0.f, 255.f);
			color[dest_idx + 1] = (u8)clamp(rgb.y, 0.f, 255.f);
			color[dest_idx + 2] = (u8)clamp(rgb.z, 0.f, 255.f);
		}
	}
}

void OutlinePipeline::render(const set<const RenderComponent*>& comp, vector<u8>& color, vector<f32>& depth, const Vec2i& frame_dim) {
	const i32 half_filter_size = 4;
	const i32 frame_width = frame_dim.x;
	const i32 frame_height = frame_dim.y;

	for (i32 y = 0; y < frame_height; y++) {
		for (i32 x = 0; x < frame_width; x++) {
			const i32 min_y = max(0, y - half_filter_size);
			const i32 max_y = min(frame_height - 1, y + half_filter_size);
			const i32 depth_idx = x + y * frame_width;
			const f32 dst_z = depth[depth_idx];
			f32 max_z_diff = 0.f;
			for (i32 filter_y = min_y; filter_y <= max_y; filter_y++) {
				const i32 min_x = max(0, x - half_filter_size);
				const i32 max_x = min(frame_width - 1, x + half_filter_size);

				for (i32 filter_x = min_x; filter_x <= max_x; filter_x++) {
					const i32 src_idx = filter_x + filter_y * frame_width;
					const f32 src_z = depth[src_idx];
					const f32 cur_diff = abs(src_z - dst_z);
					max_z_diff = max(max_z_diff, cur_diff);
				}
			}

			if (max_z_diff > 155554.f) {
				const size_t dst_idx = (size_t)depth_idx * 4;
				color[dst_idx + 0] = 0x26 * 2;
				color[dst_idx + 1] = 0x23 * 2;
				color[dst_idx + 2] = 0x6b * 2;
			}
		}
	}
}
