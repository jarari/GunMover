#pragma once

namespace Utils
{
	extern REL::Relocation<float*> ptr_deltaTime;
	RE::TESForm* GetFormFromMod(std::string, uint32_t);
	std::string SplitString(const std::string, const std::string, std::string&);
	RE::NiMatrix3 GetRotationMatrix33(float, float, float);
	bool GetTimesForClipFromEventOnActor(RE::NiPointer<RE::Actor>&, RE::BSFixedString const&, float&, float&);
	inline RE::NiNode* CreateBone(const char* name)
	{
		RE::NiNode* newbone = new RE::NiNode(0);
		newbone->name = name;
		return newbone;
	}
	inline void SetMatrix33(float a, float b, float c, float d, float e, float f, float g, float h, float i, RE::NiMatrix3& mat)
	{
		mat[0][0] = a;
		mat[0][1] = b;
		mat[0][2] = c;
		mat[1][0] = d;
		mat[1][1] = e;
		mat[1][2] = f;
		mat[2][0] = g;
		mat[2][1] = h;
		mat[2][2] = i;
	}
	//Sarrus rule
	inline float Determinant(RE::NiMatrix3 mat)
	{
		return mat[0][0] * mat[1][1] * mat[2][2] + mat[0][1] * mat[1][2] * mat[2][0] + mat[0][2] * mat[1][0] * mat[2][1] - mat[0][2] * mat[1][1] * mat[2][0] - mat[0][1] * mat[1][0] * mat[2][2] - mat[0][0] * mat[1][2] * mat[2][1];
	}

	RE::NiMatrix3 Inverse(RE::NiMatrix3);

	inline RE::NiPoint3 ToDirectionVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat[2][0], mat[2][1], mat[2][2]);
	}

	inline RE::NiPoint3 ToUpVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat[1][0], mat[1][1], mat[1][2]);
	}

	inline RE::NiPoint3 ToRightVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat[0][0], mat[0][1], mat[0][2]);
	}

	inline float easeInOutQuad(float t)
	{
		return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
	}
}
