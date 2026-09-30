#pragma once

#include "KamataEngine.h"
#include <cmath>

static KamataEngine::Vector3 operator+(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

static KamataEngine::Vector3 operator-(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

static KamataEngine::Vector3 operator*(const KamataEngine::Vector3& v, float scalar) {
	return {v.x * scalar, v.y * scalar, v.z * scalar};
}

static KamataEngine::Vector3 operator*(float scalar, const KamataEngine::Vector3& v) {
	return v * scalar;
}

static KamataEngine::Vector3& operator+=(KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	return v1;
}

static KamataEngine::Vector3& operator-=(KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	return v1;
}

static KamataEngine::Vector3 Transform(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix) {
	KamataEngine::Vector3 result;

	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];

	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}

	return result;
}

static KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2) {
	KamataEngine::Matrix4x4 result{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j]; // 🌟通常の数学・列優先の掛け算
			}
		}
	}
	return result;
}

static KamataEngine::Matrix4x4 MakeScale(const KamataEngine::Vector3& scale) {
	KamataEngine::Matrix4x4 result{};
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;
	return result;
}

static KamataEngine::Matrix4x4 MakeRotateX(float angle) {
	float sin = std::sin(angle);
	float cos = std::cos(angle);
	KamataEngine::Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = cos;
	result.m[1][2] = -sin;
	result.m[2][1] = sin;
	result.m[2][2] = cos;
	result.m[3][3] = 1.0f;
	return result;
}

static KamataEngine::Matrix4x4 MakeRotateY(float angle) {
	float sin = std::sin(angle);
	float cos = std::cos(angle);
	KamataEngine::Matrix4x4 result{};
	result.m[0][0] = cos;
	result.m[0][2] = sin;
	result.m[1][1] = 1.0f;
	result.m[2][0] = -sin;
	result.m[2][2] = cos;
	result.m[3][3] = 1.0f;
	return result;
}

static KamataEngine::Matrix4x4 MakeRotateZ(float angle) {
	float sin = std::sin(angle);
	float cos = std::cos(angle);
	KamataEngine::Matrix4x4 result{};
	result.m[0][0] = cos;
	result.m[0][1] = sin;
	result.m[1][0] = -sin;
	result.m[1][1] = cos;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

static KamataEngine::Matrix4x4 MakeTranslate(const KamataEngine::Vector3& translate) {
	KamataEngine::Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;
	return result;
}

static KamataEngine::Matrix4x4 MakeAffineMatrix(
    const KamataEngine::Vector3& scale,
    const KamataEngine::Vector3& rotate,
    const KamataEngine::Vector3& translate) {
	KamataEngine::Matrix4x4 scaleMatrix = MakeScale(scale);
	KamataEngine::Matrix4x4 rotateXMatrix = MakeRotateX(rotate.x);
	KamataEngine::Matrix4x4 rotateYMatrix = MakeRotateY(rotate.y);
	KamataEngine::Matrix4x4 rotateZMatrix = MakeRotateZ(rotate.z);
	KamataEngine::Matrix4x4 translateMatrix = MakeTranslate(translate);
	KamataEngine::Matrix4x4 rotateMatrix = Multiply(Multiply(rotateXMatrix, rotateYMatrix), rotateZMatrix);
	return Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
}
