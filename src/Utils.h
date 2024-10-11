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
		mat.entry[0].pt[0] = a;
		mat.entry[0].pt[1] = b;
		mat.entry[0].pt[2] = c;
		mat.entry[1].pt[0] = d;
		mat.entry[1].pt[1] = e;
		mat.entry[1].pt[2] = f;
		mat.entry[2].pt[0] = g;
		mat.entry[2].pt[1] = h;
		mat.entry[2].pt[2] = i;
	}
	//Sarrus rule
	inline float Determinant(RE::NiMatrix3 mat)
	{
		return mat.entry[0].pt[0] * mat.entry[1].pt[1] * mat.entry[2].pt[2] + mat.entry[0].pt[1] * mat.entry[1].pt[2] * mat.entry[2].pt[0] + mat.entry[0].pt[2] * mat.entry[1].pt[0] * mat.entry[2].pt[1] - mat.entry[0].pt[2] * mat.entry[1].pt[1] * mat.entry[2].pt[0] - mat.entry[0].pt[1] * mat.entry[1].pt[0] * mat.entry[2].pt[2] - mat.entry[0].pt[0] * mat.entry[1].pt[2] * mat.entry[2].pt[1];
	}

	RE::NiMatrix3 Inverse(RE::NiMatrix3);

	inline RE::NiPoint3 ToDirectionVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat.entry[2].pt[0], mat.entry[2].pt[1], mat.entry[2].pt[2]);
	}

	inline RE::NiPoint3 ToUpVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat.entry[1].pt[0], mat.entry[1].pt[1], mat.entry[1].pt[2]);
	}

	inline RE::NiPoint3 ToRightVector(RE::NiMatrix3 mat)
	{
		return RE::NiPoint3(mat.entry[0].pt[0], mat.entry[0].pt[1], mat.entry[0].pt[2]);
	}

	inline float easeInOutQuad(float t)
	{
		return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
	}
}
