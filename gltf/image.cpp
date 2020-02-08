// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

void analyzeImages(cgltf_data* data, std::vector<ImageInfo>& images)
{
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		const cgltf_material& material = data->materials[i];

		if (material.has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

			if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
				images[pbr.base_color_texture.texture->image - data->images].srgb = true;
		}

		if (material.has_pbr_specular_glossiness)
		{
			const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

			if (pbr.diffuse_texture.texture && pbr.diffuse_texture.texture->image)
				images[pbr.diffuse_texture.texture->image - data->images].srgb = true;
		}

		if (material.emissive_texture.texture && material.emissive_texture.texture->image)
			images[material.emissive_texture.texture->image - data->images].srgb = true;

		if (material.normal_texture.texture && material.normal_texture.texture->image)
			images[material.normal_texture.texture->image - data->images].normal_map = true;
	}
}

std::string inferMimeType(const char* path)
{
	const char* ext = strrchr(path, '.');
	if (!ext)
		return "";

	std::string extl = ext + 1;
	for (size_t i = 0; i < extl.length(); ++i)
		extl[i] = char(tolower(extl[i]));

	if (extl == "jpg")
		return "image/jpeg";
	else
		return "image/" + extl;
}

bool checkBasis()
{
	const char* basisu_path = getenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

#ifdef _WIN32
	cmd += " 2>nul";
#else
	cmd += " 2>/dev/null";
#endif

	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe)
		return false;

	char buf[15];
	size_t read = fread(buf, 1, sizeof(buf), pipe);
	pclose(pipe);

	return read == sizeof(buf) && memcmp(buf, "Basis Universal", sizeof(buf)) == 0;
}

bool encodeBasis(const std::string& data, std::string& result, bool normal_map, bool srgb, int quality)
{
	TempFile temp_input(".raw");
	TempFile temp_output(".basis");

	if (!writeFile(temp_input.path.c_str(), data))
		return false;

	const char* basisu_path = getenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

	char ql[16];
	sprintf(ql, "%d", (quality * 255 + 50) / 100);

	cmd += " -q ";
	cmd += ql;

	cmd += " -mipmap";

	if (normal_map)
	{
		cmd += " -normal_map";
		// for optimal quality we should specify seperate_rg_to_color_alpha but this requires renderer awareness
	}
	else if (!srgb)
	{
		cmd += " -linear";
	}

	cmd += " -file ";
	cmd += temp_input.path;
	cmd += " -output_file ";
	cmd += temp_output.path;

#ifdef _WIN32
	cmd += " >nul";
#else
	cmd += " >/dev/null";
#endif

	int rc = system(cmd.c_str());

	return rc == 0 && readFile(temp_output.path.c_str(), result);
}
