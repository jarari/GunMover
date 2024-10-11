#include "Utils.h"

REL::Relocation<float*> Utils::ptr_deltaTime{ REL::ID(1013228) };

RE::TESForm* Utils::GetFormFromMod(std::string modname, uint32_t formid)
{
	if (!modname.length() || !formid)
		return nullptr;
	return RE::TESDataHandler::GetSingleton()->LookupForm(formid, modname);
}

std::string Utils::SplitString(const std::string str, const std::string delimiter, std::string& remainder)
{
	std::string ret;
	size_t i = str.find(delimiter);
	if (i == std::string::npos) {
		ret = str;
		remainder = "";
		return ret;
	}

	ret = str.substr(0, i);
	remainder = str.substr(i + 1);
	return ret;
}

RE::NiMatrix3 Utils::GetRotationMatrix33(float a_pitch, float a_yaw, float a_roll)
{
	RE::NiMatrix3 m_roll;
	SetMatrix33(cos(a_roll), -sin(a_roll), 0,
		sin(a_roll), cos(a_roll), 0,
		0, 0, 1,
		m_roll);
	RE::NiMatrix3 m_yaw;
	SetMatrix33(1, 0, 0,
		0, cos(a_yaw), -sin(a_yaw),
		0, sin(a_yaw), cos(a_yaw),
		m_yaw);
	RE::NiMatrix3 m_pitch;
	SetMatrix33(cos(a_pitch), 0, sin(a_pitch),
		0, 1, 0,
		-sin(a_pitch), 0, cos(a_pitch),
		m_pitch);
	return m_roll * m_pitch * m_yaw;
}

bool Utils::GetTimesForClipFromEventOnActor(RE::NiPointer<RE::Actor>& a_actor, RE::BSFixedString const& a_evn, float& a_timeLeft, float& a_time)
{
	using func_t = decltype(Utils::GetTimesForClipFromEventOnActor);
	REL::Relocation<func_t> func{ REL::ID(238515) };
	return (*func)(a_actor, a_evn, a_timeLeft, a_time);
}

RE::NiMatrix3 Utils::Inverse(RE::NiMatrix3 mat)
{
	float det = Determinant(mat);
	if (det == 0) {
		RE::NiMatrix3 idmat;
		idmat.MakeIdentity();
		return idmat;
	}
	float a = mat.entry[0].pt[0];
	float b = mat.entry[0].pt[1];
	float c = mat.entry[0].pt[2];
	float d = mat.entry[1].pt[0];
	float e = mat.entry[1].pt[1];
	float f = mat.entry[1].pt[2];
	float g = mat.entry[2].pt[0];
	float h = mat.entry[2].pt[1];
	float i = mat.entry[2].pt[2];
	RE::NiMatrix3 invmat;
	SetMatrix33(e * i - f * h, -(b * i - c * h), b * f - c * e,
		-(d * i - f * g), a * i - c * g, -(a * f - c * d),
		d * h - e * g, -(a * h - b * g), a * e - b * d,
		invmat);
	return invmat * (1.0f / det);
}
