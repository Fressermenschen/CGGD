#pragma once

#include "resource.h"

#include <functional>
#include <iostream>
#include <linalg.h>
#include <memory>


using namespace linalg::aliases;

namespace cg::renderer
{
	template<typename VB, typename RT>
	class rasterizer
	{
	public:
		rasterizer(){};
		~rasterizer(){};
		void set_render_target(
				std::shared_ptr<resource<RT>> in_render_target,
				std::shared_ptr<resource<float>> in_depth_buffer = nullptr);
		void clear_render_target(
				const float in_depth = FLT_MIN);

		void set_vertex_buffer(std::shared_ptr<resource<VB>> in_vertex_buffer);
		void set_index_buffer(std::shared_ptr<resource<unsigned int>> in_index_buffer);

		void set_viewport(size_t in_width, size_t in_height);

		void draw(size_t num_indices);

		std::function<VB(VB vertex_data)> vertex_shader;
		std::function<cg::color(const VB& vertex_data, const float b, const float z)> pixel_shader;

	protected:
		std::shared_ptr<cg::resource<VB>> vertex_buffer;
		std::shared_ptr<cg::resource<unsigned int>> index_buffer;
		std::shared_ptr<cg::resource<RT>> render_target;
		std::shared_ptr<cg::resource<float>> depth_buffer;

		size_t width = 3440;
		size_t height = 1440;

		float edge_function(float2 a, float2 b, float2 c);
		bool depth_test(float z, size_t x, size_t y);
	};

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_render_target(
			std::shared_ptr<resource<RT>> in_render_target,
			std::shared_ptr<resource<float>> in_depth_buffer)
	{
		render_target = in_render_target;
		depth_buffer = in_depth_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::clear_render_target(
			const float in_depth)
	{
		if (render_target) {
			for (size_t y = 0; y != height; ++y) {
				for (size_t x = 0; x != width; ++x) {
					render_target->item(x, y) = unsigned_color::from_float3({float(x) / width, float(y) / height, 1});
				}
			}
		}
		if (depth_buffer) {
			for (size_t y = 0; y != height; ++y) {
				for (size_t x = 0; x != width; ++x) {
					depth_buffer->item(x, y) = in_depth;
				}
			}
		}
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_vertex_buffer(
			std::shared_ptr<resource<VB>> in_vertex_buffer)
	{
		vertex_buffer = in_vertex_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_index_buffer(
			std::shared_ptr<resource<unsigned int>> in_index_buffer)
	{
		index_buffer = in_index_buffer;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::set_viewport(size_t in_width, size_t in_height)
	{
		width = in_width;
		height = in_height;
	}

	template<typename VB, typename RT>
	inline void rasterizer<VB, RT>::draw(size_t num_indices)
	{
		for (size_t face_idx = 0; face_idx != num_indices / 3; ++face_idx) {

			std::array<vertex, 3> face{};
			for (size_t i = 0; i != 3; ++i) {
				face[i] = vertex_buffer->item(index_buffer->item(3 * face_idx + i));
			}

			std::array<float3, 3> vertices;
			for (size_t i = 0; i != 3; ++i) {
				face[i] = vertex_shader(face[i]);
				vertices[i] = float3(&face[i].position.x);
			}

			float ymin = std::min_element(vertices.begin(), vertices.end(), [](float3 a, float3 b) { return a.y < b.y; })->y;
			float ymax = std::max_element(vertices.begin(), vertices.end(), [](float3 a, float3 b) { return a.y < b.y; })->y;

			int yfrom = (std::clamp(static_cast<int>(std::ceil(ymin)), 0, static_cast<int>(height - 1)));
			int yto = (std::clamp(static_cast<int>(std::ceil(ymax)), 0, static_cast<int>(height - 1)));

			float xmin = std::min_element(vertices.begin(), vertices.end(), [](float3 a, float3 b) { return a.x < b.x; })->x;
			float xmax = std::max_element(vertices.begin(), vertices.end(), [](float3 a, float3 b) { return a.x < b.x; })->x;

			int xfrom = (std::clamp(static_cast<int>(std::ceil(xmin)), 0, static_cast<int>(width - 1)));
			int xto = (std::clamp(static_cast<int>(std::ceil(xmax)), 0, static_cast<int>(width - 1)));

			float area_twice = (cross(vertices[1] - vertices[0], vertices[2] - vertices[0])).z;

			for (int y = yfrom; y < yto; ++y) {
				for (int x = xfrom; x < xto; ++x) {

					float xf = static_cast<float>(x);
					float yf = static_cast<float>(y);
					float3 current_point{static_cast<float>(xf), static_cast<float>(yf), 0.0f};

					float u = abs(cross(vertices[1] - current_point, vertices[2] - current_point).z) / area_twice;
					float v = abs(cross(vertices[0] - current_point, vertices[2] - current_point).z) / area_twice;
					float w = abs(cross(vertices[0] - current_point, vertices[1] - current_point).z) / area_twice;

					auto is_inside_triangle = [](float3 bc) { return abs(bc[0] + bc[1] + bc[2] - 1) < 0.00001; };

					vertex pixel_data{};
					if (is_inside_triangle({u, v, w})) {
						pixel_data = face[0] * u + face[1] * v + face[2] * w;
					}
					else if (is_inside_triangle({-u, -v, -w})) {
						pixel_data = face[0] * -u + face[1] * -v + face[2] * -w;
					}
					else {
						continue;
					}

					if (depth_test(pixel_data.position.z, x, y)) {
						float& depth = depth_buffer->item(x, y);
						depth = pixel_data.position.z;

						color pixel_value = pixel_shader(pixel_data, u * u + v * v + w * w, depth);
						render_target->item(x, y) = unsigned_color::from_color(pixel_value);
					}
				}
			}
		}
	}

	template<typename VB, typename RT>
	inline float
	rasterizer<VB, RT>::edge_function(float2 a, float2 b, float2 c)
	{
		THROW_ERROR("Not implemented yet");
	}

	template<typename VB, typename RT>
	inline bool rasterizer<VB, RT>::depth_test(float z, size_t x, size_t y)
	{
		return z < depth_buffer->item(x, y);
	}

}// namespace cg::renderer