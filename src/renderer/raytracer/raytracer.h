#pragma once

#include "resource.h"
#include "world/camera.h"

#include "DirectXCollision.h"
#include "DirectXMath.h"
#include "linalg.h"

#include <cmath>
#include <memory>
#include <set>

template<class T>
bool IsEqual(const T v1, const T v2, const T tolerance = static_cast<T>(0.001))
{
	return std::abs(v1 - v2) <= tolerance;
}

namespace DirectX
{
	inline XMVECTOR XMTriangleAreaTwice(FXMVECTOR a, FXMVECTOR b)
	{
		return XMVector3Length(XMVector3Cross(a, b));
	}

	inline XMVECTOR XMFindBarycentric(FXMVECTOR p, FXMVECTOR v0, FXMVECTOR v1, GXMVECTOR v2)
	{
		XMVECTOR v0v1 = XMVectorSubtract(v1, v0);
		XMVECTOR v0v2 = XMVectorSubtract(v2, v0);
		XMVECTOR area = XMTriangleAreaTwice(v0v1, v0v2);

		XMVECTOR pv0 = XMVectorSubtract(v0, p);
		XMVECTOR pv1 = XMVectorSubtract(v1, p);
		XMVECTOR pv2 = XMVectorSubtract(v2, p);

		XMVECTOR result = XMVectorSet(XMVectorGetX(XMVectorDivide(XMTriangleAreaTwice(pv1, pv2), area)),
									  XMVectorGetX(XMVectorDivide(XMTriangleAreaTwice(pv0, pv2), area)),
									  XMVectorGetX(XMVectorDivide(XMTriangleAreaTwice(pv0, pv1), area)),
									  0.0f);
		return result;
	}

	inline XMVECTOR XMVectorDotAbsolute(FXMVECTOR a, FXMVECTOR b)
	{
		return XMVectorClamp(XMVector3Dot(a, b), XMVectorZero(), XMVectorSplatOne());
	}
}// namespace DirectX

namespace cg::renderer
{
	struct ray
	{
		ray(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR dir) : position(pos),
															  direction(DirectX::XMVector3Normalize(dir))
		{
			//removing these brackets breakes next XMVECTOR
		}

		DirectX::XMVECTOR position;
		DirectX::XMVECTOR direction;
	};

	struct payload
	{
		float depth;
		vertex point;

		bool operator<(const payload& other) const
		{
			return depth < other.depth;
		}
	};

	struct light
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR specular;
		DirectX::XMVECTOR duffuse;
		DirectX::XMVECTOR ambient;
	};


	template<typename VB, typename RT>
	class raytracer
	{
	public:
		void set_render_target(std::shared_ptr<resource<RT>> in_render_target);

		void clear_render_target();

		void set_viewport(size_t in_width, size_t in_height);

		void set_camera(std::shared_ptr<world::camera> in_camera);

		void set_vertex_buffers(std::vector<std::shared_ptr<resource<VB>>> in_vertex_buffers);

		void set_index_buffers(std::vector<std::shared_ptr<resource<unsigned int>>> in_index_buffers);

		void build_acceleration_structure();

		void launch_ray_generation(size_t frame_id);

		bool trace_ray(const ray& ray, float max_t, float min_t, payload& payload, bool bIsShadowRay = false) const;

		DirectX::XMVECTOR hit_shader(const payload& p, const ray& camera_ray) const;

		DirectX::XMVECTOR miss_shader(const payload& p, const ray& camera_ray) const;

		bool trace_floor_grid(const ray& camera_ray, DirectX::XMVECTOR& output) const;

		static bool trace_main_axes(const ray& camera_ray, DirectX::XMVECTOR& output);

		bool trace_sky_sphere_grid(const ray& camera_ray, DirectX::XMVECTOR& output) const;

		static DirectX::XMFLOAT2 get_jitter(size_t frame_id);

	protected:
		std::shared_ptr<resource<RT>> render_target;
		std::shared_ptr<resource<RT>> history;
		std::vector<std::shared_ptr<resource<unsigned int>>> index_buffers;
		std::vector<std::shared_ptr<resource<VB>>> vertex_buffers;
		std::vector<DirectX::BoundingBox> acceleration_structures;

		std::shared_ptr<world::camera> camera;

		size_t width = 3440;
		size_t height = 1440;
	};


	template<typename VB, typename RT>
	void raytracer<VB, RT>::set_render_target(
			std::shared_ptr<resource<RT>> in_render_target)
	{
		render_target = in_render_target;
		history = std::make_shared<resource<RT>>(width, height);
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::clear_render_target()
	{
		if (render_target)
		{
			for (size_t y = 0; y != height; ++y)
			{
				for (size_t x = 0; x != width; ++x)
				{
					render_target->item(x, y) = unsigned_color::from_float3({static_cast<float>(x) / width,
																			 static_cast<float>(y) / height,
																			 1.0});
				}
			}
		}
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::set_index_buffers(std::vector<std::shared_ptr<resource<unsigned int>>> in_index_buffers)
	{
		index_buffers = in_index_buffers;
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::set_vertex_buffers(std::vector<std::shared_ptr<resource<VB>>> in_vertex_buffers)
	{
		vertex_buffers = in_vertex_buffers;
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::build_acceleration_structure()
	{
		using namespace DirectX;
		acceleration_structures.clear();
		acceleration_structures.reserve(vertex_buffers.size());

		for (std::shared_ptr<resource<VB>>& vb: vertex_buffers)
		{
			acceleration_structures.emplace_back();
			BoundingBox::CreateFromPoints(acceleration_structures.back(),
										  vb->get_number_of_elements(),
										  &vb->item(0).position,
										  sizeof(VB));
		}
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::set_viewport(size_t in_width, size_t in_height)
	{
		width = in_width;
		height = in_height;
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::set_camera(std::shared_ptr<world::camera> in_camera)
	{
		camera = in_camera;
	}

	template<typename VB, typename RT>
	void raytracer<VB, RT>::launch_ray_generation(size_t frame_id)
	{
		using namespace DirectX;

		const float h = static_cast<float>(height);
		const float w = static_cast<float>(width);
		const float minZ = camera->get_z_near();
		const float maxZ = camera->get_z_far();
		const XMVECTOR eye = camera->get_position();
		const XMMATRIX view = camera->get_view_matrix();
		XMMATRIX projection = camera->get_projection_matrix();

		XMFLOAT2 jitter = get_jitter(frame_id);
		jitter.x = (jitter.x * 2.0f - 1.0f) / w * 2;
		jitter.y = (jitter.y * 2.0f - 1.0f) / h * 2;
		projection.r[2] = XMVectorAdd(projection.r[2], XMLoadFloat2(&jitter));

		for (size_t y = 0; y != height; ++y)
		{
			for (size_t x = 0; x != width; ++x)
			{
				const float fx = static_cast<float>(x);
				const float fy = static_cast<float>(y);
				const XMVECTOR pixel = XMVectorSet(fx, fy, 1.0f, 0.0f);

				XMVECTOR pixelDir = XMVector3Normalize(XMVector3Unproject(pixel,
																			 0.0f, 0.0f, w, h,
																			 0.0f, 1.0f,
																			 projection,
																			 view,
																			 XMMatrixIdentity()));
				ray r(eye, pixelDir);

				payload p;
				if (trace_ray(r, maxZ, minZ, p))
				{
					const XMVECTOR output = hit_shader(p, r);
					render_target->item(x, y) = unsigned_color::from_xmvector(output);
				}
				else
				{
					const XMVECTOR output = miss_shader(p, r);
					if (XMVectorGetX(XMVector3Length(output)) > 0)
					{
						render_target->item(x, y) = unsigned_color::from_xmvector(output);
					}
				}

				XMVECTOR current_color = render_target->item(x, y).to_xmvector();
				const XMVECTOR history_color = history->item(x, y).to_xmvector();
				if (frame_id > 0)
				{
					constexpr float mix_factor = 0.75f;
					current_color = XMVectorLerp(current_color, history_color, mix_factor);
				}
				render_target->item(x, y) = unsigned_color::from_xmvector(current_color);
				history->item(x, y) = unsigned_color::from_xmvector(current_color);
			}
		}
	}

	template<typename VB, typename RT>
	bool raytracer<VB, RT>::trace_ray(
			const ray& ray, float max_t, float min_t, payload& outPayload, const bool bIsShadowRay) const
	{
		using namespace DirectX;
		std::set<payload> hits;

		for (size_t modelIdx = 0; modelIdx != index_buffers.size(); ++modelIdx)
		{
			if (float _; !acceleration_structures[modelIdx].Intersects(ray.position, ray.direction, _))
			{
				continue;
			}

			const size_t numIndices = index_buffers.at(modelIdx)->get_number_of_elements();
			const size_t numFaces = numIndices / 3;

			for (size_t faceIdx = 0; faceIdx != numFaces; ++faceIdx)
			{
				std::array<vertex, 3> face;
				std::array<XMVECTOR, 3> triangle;
				for (size_t i = 0; i != 3; ++i)
				{
					const unsigned index = index_buffers.at(modelIdx)->item(3 * faceIdx + i);
					face.at(i) = vertex_buffers.at(modelIdx)->item(index);
					triangle.at(i) = XMLoadFloat3(&face.at(i).position);
				}

				const XMVECTOR faceBasisX = XMVectorSubtract(triangle.at(1), triangle.at(0));
				const XMVECTOR faceBasisY = XMVectorSubtract(triangle.at(2), triangle.at(0));
				const XMVECTOR normal = XMVector3Normalize(XMVector3Cross(faceBasisY, faceBasisX));

				float t;
				if (TriangleTests::Intersects(ray.position, ray.direction,
											  triangle.at(0), triangle.at(1), triangle.at(2),
											  t))
				{
					if (t >= min_t && t <= max_t)
					{
						if (bIsShadowRay)
						{
							outPayload.depth = t;
							return true;
						}

						const XMVECTOR hitPoint = XMVectorAdd(ray.position, XMVectorScale(ray.direction, t));
						const XMVECTOR barycentric = XMFindBarycentric(hitPoint, triangle.at(0), triangle.at(1),
																	   triangle.at(2));

						assert(std::abs(XMVectorGetX(XMVectorSum(barycentric)) - 1.0f) < 0.001f);

						payload hit;
						hit.depth = t;
						hit.point = face.at(0) * XMVectorGetX(barycentric) + face.at(1) * XMVectorGetY(barycentric) + face.at(2) * XMVectorGetZ(barycentric);

						XMStoreFloat3(&hit.point.normal, normal);

						hits.insert(hit);
					}
				}
			}
		}

		if (!hits.empty())
		{
			outPayload = *hits.begin();
		}
		return !hits.empty();
	}

	template<typename VB, typename RT>
	DirectX::XMVECTOR raytracer<VB, RT>::hit_shader(const payload& p, const ray& camera_ray) const
	{

		using namespace DirectX;

		constexpr bool USE_BLINN_LIGHTING = false;
		constexpr bool USE_AMBIENT = true;
		constexpr bool USE_DIFFUSE = true;
		constexpr bool USE_SPECULAR = true;

		const std::vector<light> lights =
				{
						{XMVectorSet(0.0f, 1.925f, 0.0f, 1.0f),
						 XMVectorSet(0.25f, 0.25f, 0.25f, 1.0f),
						 XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f),
						 XMVectorSet(0.4f, 0.4f, 0.4f, 1.0f)}};

		XMVECTOR output = XMVectorZero();
		for (const light& l: lights)
		{
			const XMVECTOR address = XMLoadFloat3(&p.point.position);
			const XMVECTOR surfaceNormal = XMLoadFloat3(&p.point.normal);
			const XMVECTOR lightVector = XMVectorSubtract(l.position, address);
			const XMVECTOR lightDir = XMVector3Normalize(lightVector);
			const XMVECTOR incidentDir = XMVectorScale(lightDir, -1.0f);
			const XMVECTOR reflectedLightDir = XMVector3Reflect(incidentDir, surfaceNormal);
			const XMVECTOR cameraDir = XMVector3Normalize(XMVectorSubtract(camera_ray.position, address));
			XMVECTOR shininess = XMVectorReplicate(p.point.shininess);
			XMVECTOR shadow = XMVectorSplatOne();

			if (USE_AMBIENT)
			{
				const XMVECTOR materialAmbient = XMLoadFloat3(&p.point.ambient);
				const XMVECTOR ambientComponent = XMColorModulate(l.ambient, materialAmbient);
				output = XMVectorAdd(output, ambientComponent);
			}

			if (XMVectorGetX(XMVector3Dot(lightDir, surfaceNormal)) < 0.0f)
			{
				continue;
			}

			ray lightRay(address, lightDir);
			payload shadowPayload;
			const bool bIsShadow = trace_ray(lightRay, XMVectorGetX(XMVector3Length(lightVector)), 0.0001f,
											 shadowPayload, true);
			if (bIsShadow)
			{
				shadow = XMVectorReplicate(0.5f);
			}

			if (USE_DIFFUSE)
			{
				const XMVECTOR materialDiffuse = XMLoadFloat3(&p.point.diffuse);
				XMVECTOR diffuseComponent = XMVectorDotAbsolute(lightDir, surfaceNormal);
				diffuseComponent = XMColorModulate(diffuseComponent, l.duffuse);
				diffuseComponent = XMColorModulate(diffuseComponent, shadow);
				diffuseComponent = XMColorModulate(diffuseComponent, materialDiffuse);

				output = XMVectorAdd(output, diffuseComponent);
			}

			if (!bIsShadow && USE_SPECULAR)
			{
				const XMVECTOR materialSpecular = XMVectorSplatOne();
				XMVECTOR specularComponent;
				if (USE_BLINN_LIGHTING)
				{
					shininess = XMVectorScale(shininess, 0.25f);
					const XMVECTOR halfDir = XMVector3Normalize(XMVectorAdd(lightDir, cameraDir));
					specularComponent = XMVectorDotAbsolute(surfaceNormal, halfDir);
				}
				else
				{
					specularComponent = XMVectorDotAbsolute(reflectedLightDir, cameraDir);
				}
				specularComponent = XMVectorPow(specularComponent, shininess);
				specularComponent = XMColorModulate(specularComponent, materialSpecular);
				specularComponent = XMColorModulate(specularComponent, l.specular);
				output = XMVectorAdd(output, specularComponent);
			}
		}
		return output;
	}

	template<typename VB, typename RT>
	DirectX::XMVECTOR raytracer<VB, RT>::miss_shader(const payload& p, const ray& camera_ray) const
	{
		DirectX::XMVECTOR output = DirectX::XMVectorZero();
		
		return output;
	}

	template<typename VB, typename RT>
	DirectX::XMFLOAT2 raytracer<VB, RT>::get_jitter(size_t frame_id)
	{
		DirectX::XMFLOAT2 result(0.0f, 0.0f);
		constexpr int base_x = 2;
		int index = static_cast<int>(frame_id) + 1;
		float inv_base = 1.0f / base_x;
		float fraction = inv_base;
		while (index > 0)
		{
			result.x += (index % base_x) * fraction;
			index /= base_x;
			fraction *= inv_base;
		}
		constexpr int base_y = 3;
		index = static_cast<int>(frame_id) + 1;
		inv_base = 1.0f / base_y;
		fraction = inv_base;
		while (index > 0)
		{
			result.y += index % base_y * fraction;
			index /= base_y;
			fraction *= inv_base;
		}
		return result;
	}
}// namespace cg::renderer