#define TINYOBJLOADER_IMPLEMENTATION

#include "model.h"

#include "utils/error_handler.h"

#include <DirectXMath.h>
#include <iostream>
#include <linalg.h>
#include <random>
#include <set>


using namespace linalg::aliases;
using namespace cg::world;

cg::world::model::model() {}

cg::world::model::~model() {}

void cg::world::model::load_obj(const std::filesystem::path& model_path)
{
	std::string error_message, warning_message;
	std::filesystem::path dir = model_path;
	dir.remove_filename();
	const bool status = LoadObj(&attrib, &shapes, &materials,
								&warning_message, &error_message,
								model_path.generic_string().c_str(),
								dir.generic_string().c_str(), true);

	if (warning_message.size()) {
		std::cerr << "Warning: " << warning_message << std::endl;
	}

	if (!status) {
		THROW_ERROR(error_message);
	}

	const size_t num_vertices = attrib.vertices.size() / 3;
	std::vector<vertex> vertices(num_vertices);
	for (size_t i = 0; i != num_vertices; ++i)
	{
		vertex& current_vertex = vertices[i];
		current_vertex.position = DirectX::XMFLOAT3(&attrib.vertices.at(3 * i));
	}

	for (auto& [name, mesh, lines, points]: shapes) {
		std::vector<vertex> vertex_accumulator;
		std::map<unsigned int, unsigned int> index_map{};
		for (size_t i = 0; i != mesh.indices.size(); ++i) {
			unsigned int index = mesh.indices[i].vertex_index;
			if (index_map.count(index) == 0) {
				const unsigned int local_index = static_cast<unsigned int>(vertex_accumulator.size());
				vertex_accumulator.push_back(vertices[index]);

				vertex_accumulator.back().diffuse = DirectX::XMFLOAT3(materials[mesh.material_ids[i / 3]].diffuse);
				vertex_accumulator.back().ambient = DirectX::XMFLOAT3(materials[mesh.material_ids[i / 3]].ambient);
				vertex_accumulator.back().specular = DirectX::XMFLOAT3(materials[mesh.material_ids[i / 3]].specular);
				vertex_accumulator.back().emissive = DirectX::XMFLOAT3(materials[mesh.material_ids[i / 3]].emission);

				vertex_accumulator.back().shininess = materials[mesh.material_ids[i / 3]].shininess;

				index_map[index] = local_index;
			}
		}

		auto index_buffer = std::make_shared<resource<unsigned int>>(mesh.indices.size());
		for (size_t i = 0; i != mesh.indices.size(); ++i) {
			index_buffer->item(i) = index_map[mesh.indices[mesh.indices.size() - i - 1].vertex_index];
		}

		auto vertex_buffer = std::make_shared<resource<vertex>>(vertex_accumulator.size());
		for (size_t i = 0; i != vertex_accumulator.size(); ++i) {
			vertex_buffer->item(i) = vertex_accumulator[i];
		}

		vertex_buffers.emplace_back(vertex_buffer);
		index_buffers.emplace_back(index_buffer);
	}
}


const std::vector<std::shared_ptr<cg::resource<cg::vertex>>>&
cg::world::model::get_vertex_buffers() const
{
	return vertex_buffers;
}


const std::vector<std::shared_ptr<cg::resource<unsigned int>>>&
cg::world::model::get_index_buffers() const
{
	return index_buffers;
}

std::vector<std::filesystem::path>
cg::world::model::get_per_shape_texture_files() const
{
	THROW_ERROR("Not implemented yet");
}


const DirectX::XMMATRIX cg::world::model::get_world_matrix() const
{
	return DirectX::XMMatrixRotationY(0);
}