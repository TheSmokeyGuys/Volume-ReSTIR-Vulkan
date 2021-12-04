#include "GeneralUtils.h"
#include <queue>

extern bool GeneratePointLight;
extern bool GenerateWhiteLight;
extern uint32_t numPointLightGenerates;

std::vector<shader::pointLight> collectPointLights(const nvh::GltfScene& scene) {
	std::vector<shader::pointLight> result;
	result.reserve(scene.m_lights.size());
	for (const nvh::GltfLight& light : scene.m_lights) {
		shader::pointLight& addedLight = result.emplace_back(shader::pointLight{
			 light.worldMatrix.col(3),
			 nvmath::vec4(light.light.color[0], light.light.color[1], light.light.color[2], 0.0f)
			});
		addedLight.emission_luminance.w = shader::luminance(
			addedLight.emission_luminance.x, addedLight.emission_luminance.y, addedLight.emission_luminance.z
		);
	}
	return result;
}

std::vector<shader::pointLight> generatePointLights(
	nvmath::vec3 min, nvmath::vec3 max
) {
	std::uniform_real_distribution<float> distR = std::uniform_real_distribution<float>(0.0f, 1.0f);
	std::uniform_real_distribution<float> distG = std::uniform_real_distribution<float>(0.0f, 1.0f);
	std::uniform_real_distribution<float> distB = std::uniform_real_distribution<float>(0.0f, 1.0f);
	std::uniform_real_distribution<float> distX(min.x, max.x);
	std::uniform_real_distribution<float> distY(min.y, max.y);
	std::uniform_real_distribution<float> distZ(min.z, max.z);
	std::default_random_engine rand;

	std::vector<shader::pointLight> result(numPointLightGenerates);
	for (shader::pointLight& light : result) {
		light.pos = nvmath::vec4(distX(rand), distY(rand), distZ(rand), 1.0f);
		if (GenerateWhiteLight) {
			light.emission_luminance = nvmath::vec4(1.0f, 1.0f, 1.0f, 0.0f);

		}
		else {
			light.emission_luminance = nvmath::vec4(distR(rand), distG(rand), distB(rand), 0.0f);
		}
		light.emission_luminance.w = shader::luminance(
			light.emission_luminance.x, light.emission_luminance.y, light.emission_luminance.z
		);
	}
	return result;
}

std::vector<shader::triangleLight> collectTriangleLights(const nvh::GltfScene& scene) {
	std::vector<shader::triangleLight> result;
	for (const nvh::GltfNode& node : scene.m_nodes) {
		const nvh::GltfPrimMesh& mesh = scene.m_primMeshes[node.primMesh];
		const nvh::GltfMaterial& material = scene.m_materials[mesh.materialIndex];
		if (material.emissiveFactor.sq_norm() > 1e-6) {
			const uint32_t* indices = scene.m_indices.data() + mesh.firstIndex;
			const nvmath::vec3* pos = scene.m_positions.data() + mesh.vertexOffset;
			for (uint32_t i = 0; i < mesh.indexCount; i += 3, indices += 3) {
				// triangle
				vec4 p1 = node.worldMatrix * nvmath::vec4(pos[indices[0]], 1.0f);
				vec4 p2 = node.worldMatrix * nvmath::vec4(pos[indices[1]], 1.0f);
				vec4 p3 = node.worldMatrix * nvmath::vec4(pos[indices[2]], 1.0f);
				vec3 p1_vec3(p1.x, p1.y, p1.z), p2_vec3(p2.x, p2.y, p2.z), p3_vec3(p3.x, p3.y, p3.z);

				vec3 normal = nvmath::cross(p2_vec3 - p1_vec3, p3_vec3 - p1_vec3);
				float area = normal.norm();
				normal /= area;
				area *= 0.5f;

				float emissionLuminance = shader::luminance(
					material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z
				);

				shader::triangleLight tmpTriLight{ p1, p2, p3, vec4(material.emissiveFactor, 0.0), area };
				result.push_back(shader::triangleLight{
					p1, p2, p3,
					 nvmath::vec4(material.emissiveFactor, emissionLuminance),
					 nvmath::vec4(normal, area)
					});
			}
		}
	}
	return result;
}

[[nodiscard]] std::vector<shader::aliasTableCell> createAliasTable(std::vector<float>& pdf) {
	std::queue<int> biggerQueue;
	std::queue<int> smallerQueue;
	std::vector<float> lightPdf;

	float powerSum = 0.f;
	uint32_t lightNum = 0;
	lightNum = static_cast<uint32_t>(pdf.size());
	for (float p : pdf) {
		powerSum += p;
		lightPdf.push_back(p);
	}

	std::vector<shader::aliasTableCell> aliasTable(lightNum, shader::aliasTableCell{ -1,0.f, 0.f });

	for (uint32_t i = 0; i < lightNum; ++i) {
		aliasTable[i].pdf = lightPdf[i] / powerSum;
		// aliasTable[i].pdf = 1.f / lightNum;
		lightPdf[i] = float(lightPdf.size()) * lightPdf[i] / powerSum;
		if (lightPdf[i] >= 1.f) {
			biggerQueue.push(i);
		}
		else {
			smallerQueue.push(i);
		}
	}

	// Construct Alias Table
	while (!biggerQueue.empty() && !smallerQueue.empty()) {
		int g = biggerQueue.front();
		biggerQueue.pop();
		int l = smallerQueue.front();
		smallerQueue.pop();

		aliasTable[l].prob = lightPdf[l];
		aliasTable[l].alias = g;
		lightPdf[g] = (lightPdf[g] + lightPdf[l]) - 1.f;

		if (lightPdf[g] < 1.f) {
			smallerQueue.push(g);
		}
		else {
			biggerQueue.push(g);
		}
	}

	while (!biggerQueue.empty()) {
		int g = biggerQueue.front();
		biggerQueue.pop();
		aliasTable[g].prob = 1.f;
		aliasTable[g].alias = g;
	}

	while (!smallerQueue.empty()) {
		int l = smallerQueue.front();
		smallerQueue.pop();
		aliasTable[l].prob = 1.f;
		aliasTable[l].alias = l;

	}

	for (shader::aliasTableCell& col : aliasTable) {
		col.aliasPdf = aliasTable[col.alias].pdf;
	}

	return aliasTable;
}

