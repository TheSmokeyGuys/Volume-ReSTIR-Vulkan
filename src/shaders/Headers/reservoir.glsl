// these structs are not supposed to be seen by the cpu



Reservoir unpackResovirStruct(vec4 resovirInfo, vec4 resovirWeight) {
	Reservoir res;
	res.numStreamSamples = floatBitsToUint(resovirInfo.x);
	res.lightIndex = floatBitsToUint(resovirInfo.y);
	res.lightKind = floatBitsToInt(resovirInfo.z);
	res.sampleSeed = floatBitsToUint(resovirInfo.w);

	res.pHat = resovirWeight.x;
	res.sumWeights = resovirWeight.y;
	res.w = resovirWeight.z;

	return res;
}

void packResovirStruct(Reservoir res, out vec4 resovirInfo, out vec4 resovirWeight) {
	resovirInfo.x = uintBitsToFloat(res.numStreamSamples);
	resovirInfo.y = uintBitsToFloat(res.lightIndex);
	resovirInfo.z = intBitsToFloat(res.lightKind);
	resovirInfo.w = uintBitsToFloat(res.sampleSeed);

	resovirWeight.x = res.pHat;
	resovirWeight.y = res.sumWeights;
	resovirWeight.z = res.w;


}

void updateReservoir(inout Reservoir res, uint lightIdx, int lightKind, float weight, float pHat, float w, vec3 lightPos, inout uint seed, in uint sampleSeed) {
	res.sumWeights += weight;
	float replacePossibility = weight / res.sumWeights;
	if (rnd(seed) < replacePossibility) {
		res.lightIndex = lightIdx;
		res.lightKind = lightKind;
		res.pHat = pHat;
		res.w = w;
		res.sampleSeed = sampleSeed;
		res.lightPos = lightPos;
	}
}

void addSampleToReservoir(inout Reservoir res, uint lightIdx, int lightKind, float lightPdf, vec3 lightPos, in GeometryInfo gInfo, inout uint seed) {
	float pHat = evaluatePHat(lightIdx, lightKind, gInfo);
	float weight = pHat / lightPdf;
	res.numStreamSamples += 1;
	float w = (res.sumWeights + weight) / (res.numStreamSamples * pHat);
	updateReservoir(res, lightIdx, lightKind, weight, pHat, w, lightPos, seed, gInfo.sampleSeed);
}

void combineReservoirs(inout Reservoir self, Reservoir other, in GeometryInfo gInfo, in GeometryInfo otherGInfo, inout uint seed) {
	uint Z = self.numStreamSamples;

	self.numStreamSamples += other.numStreamSamples;
	float pHat = evaluatePHat(other.lightIndex, other.lightKind, gInfo);
	float weight = pHat * other.w * other.numStreamSamples;
	if (weight > 0.0f) {
		updateReservoir(
			self,
			other.lightIndex, other.lightKind, weight, pHat,
			other.w, other.lightPos, seed, other.sampleSeed
		);

	}

	pHat = evaluatePHat(self.lightIndex, self.lightKind, otherGInfo);
	if (pHat > 0.0f) {
		Z += other.numStreamSamples;
	}
	if (self.w > 0.0f) {
		self.w = self.sumWeights / (Z * self.pHat);
	}


}


void combineReservoirs(inout Reservoir self, Reservoir other, float pHat, inout uint seed) {
	self.numStreamSamples += other.numStreamSamples;

	float weight = pHat * other.w * other.numStreamSamples;
	if (weight > 0.0f) {
		updateReservoir(
			self,
			other.lightIndex, other.lightKind, weight, pHat,
			other.w, other.lightPos, seed, other.sampleSeed
		);
	}
	if (self.w > 0.0f) {
		self.w = self.sumWeights / (self.numStreamSamples * self.pHat);
	}

}


Reservoir newReservoir() {
	Reservoir result;
	result.sumWeights = 0.0f;
	result.w = 0.0f;
	result.numStreamSamples = 0;
	result.pHat = 0;

	return result;
}


//void updateReservoirAt(
//	inout Reservoir res, int i, float weight, vec3 position, vec4 normal, float emissionLum, uint lightIdx, int lightKind,
//	float pHat, float w,
//	float sumPHat,
//	inout uint seed
//) {
//	res.samples[i].sumWeights += weight;
//	float replacePossibility = weight / res.samples[i].sumWeights;
//	if (rnd(seed) < replacePossibility) {
//		res.samples[i].position_emissionLum = vec4(position, emissionLum);
//		res.samples[i].normal = normal;
//		res.samples[i].lightIndex = lightIdx;
//		res.samples[i].lightKind = lightKind;
//		res.samples[i].pHat = pHat;
//		res.samples[i].w = w;
//		res.samples[i].sumPHat += sumPHat;
//	}
//}
//
//void addSampleToReservoir(inout Reservoir res, vec3 position, vec4 normal, float emissionLum, uint lightIdx, int lightKind, float pHat, float sampleP, inout uint seed) {
//	float weight = pHat / sampleP;
//	res.numStreamSamples += 1;
//
//	for (int i = 0; i < RESERVOIR_SIZE; ++i) {
//		float w = (res.samples[i].sumWeights + weight) / (res.numStreamSamples * pHat);
//		updateReservoirAt(
//			res, i, weight, position, normal, emissionLum, lightIdx,lightKind, pHat, w,
//			pHat,
//			seed
//		);
//	}
//}
//
//void combineReservoirs(inout Reservoir self, Reservoir other, float pHat[RESERVOIR_SIZE], inout uint seed) {
//	self.numStreamSamples += other.numStreamSamples;
//
//	for (int i = 0; i < RESERVOIR_SIZE; ++i) {
//		float weight = pHat[i] * other.samples[i].w * other.numStreamSamples;
//		if (weight > 0.0f) {
//			updateReservoirAt(
//				self, i, weight,
//				other.samples[i].position_emissionLum.xyz, other.samples[i].normal, other.samples[i].position_emissionLum.w,
//				other.samples[i].lightIndex, other.samples[i].lightKind ,pHat[i],
//				other.samples[i].sumPHat,
//				other.samples[i].w, seed
//			);
//		}
//		if (self.samples[i].w > 0.0f) {
//			self.samples[i].w = self.samples[i].sumWeights / (self.numStreamSamples * self.samples[i].pHat);
//		}
//	}
//}
//
//Reservoir newReservoir() {
//	Reservoir result;
//	for (int i = 0; i < RESERVOIR_SIZE; ++i) {
//		result.samples[i].sumWeights = 0.0f;
//		result.samples[i].sumPHat = 0.0f;
//	}
//	result.numStreamSamples = 0;
//	return result;
//}
