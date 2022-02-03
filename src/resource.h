#pragma once

#include "utils/error_handler.h"

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
		//size_t item_size = sizeof(T);
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
		//THROW_ERROR("Not implemented yet");
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
		float3 to_float3() const
		{
			return {
					static_cast<float>(r) / 255.0f,
					static_cast<float>(g) / 255.0f,
					static_cast<float>(b) / 255.0f};
		};
		unsigned char r;
		unsigned char g;
		unsigned char b;
	};


	struct vertex
	{
		float x;
		float y;
		float z;

		float nx;
		float ny;
		float nz;

		float ambient_r;
		float ambient_g;
		float ambient_b;

		float diffuse_r;
		float diffuse_g;
		float diffuse_b;

		float emissive_r;
		float emissive_g;
		float emissive_b;

		float u;
		float v;

		vertex operator+(const vertex& other) const
		{
			vertex result{};
			result.x = this->x + other.x;
			result.y = this->y + other.y;
			result.z = this->z + other.z;
			result.nx = this->nx + other.nx;
			result.ny = this->ny + other.ny;
			result.nz = this->nz + other.nz;

			return result;
		}

		vertex operator*(const float value) const
		{
			vertex result;
			result.x = this->x * value;
			result.y = this->y * value;
			result.z = this->z * value;
			result.nx = this->nx * value;
			result.ny = this->ny * value;
			result.nz = this->nz * value;

			return result;
		}
	};

}// namespace cg