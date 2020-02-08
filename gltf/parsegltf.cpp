// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <string.h>

static const char* getError(cgltf_result result, cgltf_data* data)
{
	switch (result)
	{
	case cgltf_result_file_not_found:
		return data ? "resource not found" : "file not found";

	case cgltf_result_io_error:
		return "I/O error";

	case cgltf_result_invalid_json:
		return "invalid JSON";

	case cgltf_result_invalid_gltf:
		return "invalid GLTF";

	case cgltf_result_out_of_memory:
		return "out of memory";

	case cgltf_result_legacy_gltf:
		return "legacy GLTF";

	case cgltf_result_data_too_short:
		return data ? "buffer too short" : "not a GLTF file";

	case cgltf_result_unknown_format:
		return data ? "unknown resource format" : "not a GLTF file";

	default:
		return "unknown error";
	}
}

static void readAccessor(std::vector<float>& data, const cgltf_accessor* accessor)
{
	assert(accessor->type == cgltf_type_scalar);

	data.resize(accessor->count);
	cgltf_accessor_unpack_floats(accessor, &data[0], data.size());
}

static void readAccessor(std::vector<Attr>& data, const cgltf_accessor* accessor)
{
	size_t components = cgltf_num_components(accessor->type);

	std::vector<float> temp(accessor->count * components);
	cgltf_accessor_unpack_floats(accessor, &temp[0], temp.size());

	data.resize(accessor->count);

	for (size_t i = 0; i < accessor->count; ++i)
	{
		for (size_t k = 0; k < components && k < 4; ++k)
			data[i].f[k] = temp[i * components + k];
	}
}

static void parseMeshesGltf(cgltf_data* data, std::vector<Mesh>& meshes)
{
	for (size_t ni = 0; ni < data->nodes_count; ++ni)
	{
		cgltf_node& node = data->nodes[ni];

		if (!node.mesh)
			continue;

		const cgltf_mesh& mesh = *node.mesh;
		int mesh_id = int(&mesh - data->meshes);

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];

			if (primitive.type != cgltf_primitive_type_triangles && primitive.type != cgltf_primitive_type_points)
			{
				fprintf(stderr, "Warning: ignoring primitive %d of mesh %d because type %d is not supported\n", int(pi), mesh_id, primitive.type);
				continue;
			}

			if (primitive.type == cgltf_primitive_type_points && primitive.indices)
			{
				fprintf(stderr, "Warning: ignoring primitive %d of mesh %d because indexed points are not supported\n", int(pi), mesh_id);
				continue;
			}

			Mesh result = {};

			result.node = &node;

			result.material = primitive.material;
			result.skin = node.skin;

			result.type = primitive.type;

			if (primitive.indices)
			{
				result.indices.resize(primitive.indices->count);
				for (size_t i = 0; i < primitive.indices->count; ++i)
					result.indices[i] = unsigned(cgltf_accessor_read_index(primitive.indices, i));
			}
			else if (primitive.type != cgltf_primitive_type_points)
			{
				size_t count = primitive.attributes ? primitive.attributes[0].data->count : 0;

				// note, while we could generate a good index buffer, reindexMesh will take care of this
				result.indices.resize(count);
				for (size_t i = 0; i < count; ++i)
					result.indices[i] = unsigned(i);
			}

			for (size_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const cgltf_attribute& attr = primitive.attributes[ai];

				if (attr.type == cgltf_attribute_type_invalid)
				{
					fprintf(stderr, "Warning: ignoring unknown attribute %s in primitive %d of mesh %d\n", attr.name, int(pi), mesh_id);
					continue;
				}

				Stream s = {attr.type, attr.index};
				readAccessor(s.data, attr.data);

				if (attr.type == cgltf_attribute_type_color && attr.data->type == cgltf_type_vec3)
				{
					for (size_t i = 0; i < s.data.size(); ++i)
						s.data[i].f[3] = 1.0f;
				}

				result.streams.push_back(s);
			}

			for (size_t ti = 0; ti < primitive.targets_count; ++ti)
			{
				const cgltf_morph_target& target = primitive.targets[ti];

				for (size_t ai = 0; ai < target.attributes_count; ++ai)
				{
					const cgltf_attribute& attr = target.attributes[ai];

					if (attr.type == cgltf_attribute_type_invalid)
					{
						fprintf(stderr, "Warning: ignoring unknown attribute %s in morph target %d of primitive %d of mesh %d\n", attr.name, int(ti), int(pi), mesh_id);
						continue;
					}

					Stream s = {attr.type, attr.index, int(ti + 1)};
					readAccessor(s.data, attr.data);

					result.streams.push_back(s);
				}
			}

			result.targets = primitive.targets_count;
			result.target_weights.assign(mesh.weights, mesh.weights + mesh.weights_count);
			result.target_names.assign(mesh.target_names, mesh.target_names + mesh.target_names_count);

			meshes.push_back(result);
		}
	}
}

static void parseAnimationsGltf(cgltf_data* data, std::vector<Animation>& animations)
{
	for (size_t i = 0; i < data->animations_count; ++i)
	{
		const cgltf_animation& animation = data->animations[i];

		Animation result = {};
		result.name = animation.name;

		for (size_t j = 0; j < animation.channels_count; ++j)
		{
			const cgltf_animation_channel& channel = animation.channels[j];

			if (!channel.target_node)
			{
				fprintf(stderr, "Warning: ignoring channel %d of animation %d because it has no target node\n", int(j), int(i));
				continue;
			}

			Track track = {};
			track.node = channel.target_node;
			track.path = channel.target_path;

			track.components = (channel.target_path == cgltf_animation_path_type_weights) ? track.node->mesh->primitives[0].targets_count : 1;

			track.interpolation = channel.sampler->interpolation;

			readAccessor(track.time, channel.sampler->input);
			readAccessor(track.data, channel.sampler->output);

			result.tracks.push_back(track);
		}

		if (result.tracks.empty())
		{
			fprintf(stderr, "Warning: ignoring animation %d because it has no valid tracks\n", int(i));
			continue;
		}

		animations.push_back(result);
	}
}

static bool requiresExtension(cgltf_data* data, const char* name)
{
	for (size_t i = 0; i < data->extensions_required_count; ++i)
		if (strcmp(data->extensions_required[i], name) == 0)
			return true;

	return false;
}

static bool needsDummyBuffers(cgltf_data* data)
{
	for (size_t i = 0; i < data->accessors_count; ++i)
	{
		cgltf_accessor* accessor = &data->accessors[i];

		if (accessor->buffer_view && accessor->buffer_view->buffer->data == NULL)
			return true;

		if (accessor->is_sparse)
		{
			cgltf_accessor_sparse* sparse = &accessor->sparse;

			if (sparse->indices_buffer_view->buffer->data == NULL)
				return true;
			if (sparse->values_buffer_view->buffer->data == NULL)
				return true;
		}
	}

	return false;
}

cgltf_data* parseGltf(const char* path, std::vector<Mesh>& meshes, std::vector<Animation>& animations, const char** error)
{
	cgltf_data* data = 0;

	cgltf_options options = {};
	cgltf_result result = cgltf_parse_file(&options, path, &data);
	result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, data, path) : result;
	result = (result == cgltf_result_success) ? cgltf_validate(data) : result;

	*error = NULL;

	if (result != cgltf_result_success)
		*error = getError(result, data);
	else if (requiresExtension(data, "KHR_draco_mesh_compression"))
		*error = "file requires Draco mesh compression support";
	else if (requiresExtension(data, "MESHOPT_compression"))
		*error = "file has already been compressed using gltfpack";
	else if (needsDummyBuffers(data))
		*error = "buffer has no data";

	if (*error)
	{
		cgltf_free(data);
		return 0;
	}

	parseMeshesGltf(data, meshes);
	parseAnimationsGltf(data, animations);

	return data;
}
