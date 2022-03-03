#pragma once

#include "utils/error_handler.h"

#include "DirectXMath.h"
#include <algorithm>
#include <linalg.h>
#include <vector>


using namespace linalg::aliases;

namespace cg
{
	template<typename T>
	class resource
	{
	public:
		resource(size_t size);
		resource(size_t x_size, size_t y_size);
		~resource();

		const T* get_data();
		T& item(size_t item);
		T& item(size_t x, size_t y);

		size_t get_size_in_bytes() const;
		size_t get_number_of_elements() const;
		size_t get_stride() const;

	private:
		std::vector<T> data;
		size_t stride{0};
	};
	template<typename T>
	inline resource<T>::resource(size_t size) : data(size)
	{
		//THROW_ERROR("Not implemented yet");
	}
	template<typename T>
	inline resource<T>::resource(size_t x_size, size_t y_size) : data(x_size * y_size), stride(x_size)
	{
		//THROW_ERROR("Not implemented yet");
	}
	template<typename T>
	inline resource<T>::~resource()
	{
	}
	template<typename T>
	inline const T* resource<T>::get_data()
	{
		return data.data();
	}
	template<typename T>
	inline T& resource<T>::item(size_t item)
	{
		return data.at(item);
	}
	template<typename T>
	inline T& resource<T>::item(size_t x, size_t y)
	{
		return data.at(y * stride + x);
	}
	template<typename T>
	inline size_t resource<T>::get_size_in_bytes() const
	{
		return get_number_of_elements() * sizeof(T);
	}
	template<typename T>
	inline size_t resource<T>::get_number_of_elements() const
	{
		return data.size();
	}

	template<typename T>
	inline size_t resource<T>::get_stride() const
	{
		return stride;
	}

	struct color
	{
		static color from_float3(const float3& in)
		{
			return {in.x, in.y, in.z};
		};

		static color from_XMFLOAT3(const DirectX::XMFLOAT3& in)
		{
			return {in.x, in.y, in.z};
		}

		float3 to_float3() const
		{
			return {r, g, b};
		}
		float r;
		float g;
		float b;
	};

	struct unsigned_color
	{
		static unsigned_color from_color(const color& color)
		{
			return {
					static_cast<unsigned char>(255.0f * std::clamp(color.r, 0.0f, 1.0f)),
					static_cast<unsigned char>(255.0f * std::clamp(color.g, 0.0f, 1.0f)),
					static_cast<unsigned char>(255.0f * std::clamp(color.b, 0.0f, 1.0f)),
			};
		};
		static unsigned_color from_float3(const float3& color)
		{
			return {
					static_cast<unsigned char>(255.0f * std::clamp(color.x, 0.0f, 1.0f)),
					static_cast<unsigned char>(255.0f * std::clamp(color.y, 0.0f, 1.0f)),
					static_cast<unsigned char>(255.0f * std::clamp(color.z, 0.0f, 1.0f)),
			};
		}
		static unsigned_color from_xmvector(const DirectX::FXMVECTOR color)
		{
			DirectX::XMFLOAT3 temp;
			DirectX::XMStoreFloat3(&temp, color);
			return from_color(color::from_XMFLOAT3(temp));
		}
		float3 to_float3() const
		{
			return {
					static_cast<float>(r) / 255.0f,
					static_cast<float>(g) / 255.0f,
					static_cast<float>(b) / 255.0f};
		};
		DirectX::XMVECTOR to_xmvector() const
		{
			DirectX::XMFLOAT3 f(&this->to_float3()[0]);
			return DirectX::XMLoadFloat3(&f);
		}
		unsigned char r;
		unsigned char g;
		unsigned char b;
	};

	struct d3d_vertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 emissive;
		DirectX::XMFLOAT3 bary;
	};

	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;

		DirectX::XMFLOAT3 ambient;
		DirectX::XMFLOAT3 diffuse;
		DirectX::XMFLOAT3 specular;

		DirectX::XMFLOAT3 emissive;

		float shininess;

		DirectX::XMFLOAT2 uv;

		vertex operator+(const vertex& other) const
		{
			using namespace DirectX;

			vertex result;
			XMVECTOR converter = XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&other.position));
			XMStoreFloat3(&result.position, converter);

			converter = XMVectorAdd(XMLoadFloat3(&normal), XMLoadFloat3(&other.normal));
			XMStoreFloat3(&result.normal, converter);

			converter = XMVectorAdd(XMLoadFloat3(&ambient), XMLoadFloat3(&other.ambient));
			XMStoreFloat3(&result.ambient, converter);

			converter = XMVectorAdd(XMLoadFloat3(&diffuse), XMLoadFloat3(&other.diffuse));
			XMStoreFloat3(&result.diffuse, converter);

			converter = XMVectorAdd(XMLoadFloat3(&specular), XMLoadFloat3(&other.specular));
			XMStoreFloat3(&result.specular, converter);

			converter = XMVectorAdd(XMLoadFloat3(&emissive), XMLoadFloat3(&other.emissive));
			XMStoreFloat3(&result.emissive, converter);

			result.shininess = shininess + other.shininess;

			converter = XMVectorAdd(XMLoadFloat2(&uv), XMLoadFloat2(&other.uv));
			XMStoreFloat2(&result.uv, converter);

			return result;
		}

		vertex operator*(const float value) const
		{
			using namespace DirectX;

			vertex result;
			XMVECTOR converter = XMVectorScale(XMLoadFloat3(&position), value);
			XMStoreFloat3(&result.position, converter);

			converter = XMVectorScale(XMLoadFloat3(&normal), value);
			XMStoreFloat3(&result.normal, converter);

			converter = XMVectorScale(XMLoadFloat3(&ambient), value);
			XMStoreFloat3(&result.ambient, converter);

			converter = XMVectorScale(XMLoadFloat3(&diffuse), value);
			XMStoreFloat3(&result.diffuse, converter);

			converter = XMVectorScale(XMLoadFloat3(&specular), value);
			XMStoreFloat3(&result.specular, converter);

			converter = XMVectorScale(XMLoadFloat3(&emissive), value);
			XMStoreFloat3(&result.emissive, converter);

			result.shininess = shininess * value;

			converter = XMVectorScale(XMLoadFloat2(&uv), value);
			XMStoreFloat2(&result.uv, converter);

			return result;
		}
	};

}// namespace cg