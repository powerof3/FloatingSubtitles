#pragma once

class RayCollector : public RE::hkpClosestRayHitCollector
{
public:
	RayCollector() {}
	RayCollector(RE::Actor* a_actor, RE::COL_LAYER a_layer);

	void AddRayHit(const RE::hkpCdBody& a_body, const RE::hkpShapeRayCastCollectorOutput& a_hitInfo) override;  // 00
	~RayCollector() override = default;                                                                         // 01

private:
	// members
	RE::Actor* actor;
};

class RayCaster
{
public:
	struct StartPoint
	{
		void Init();

		RE::NiPoint3 camera;
		RE::NiPoint3 debug;
	};

	enum class Result
	{
		kOffscreen = 0,
		kObscured,
		kVisible
	};

	RayCaster() = default;
	RayCaster(RE::Actor* a_target);

	Result GetResult(bool a_debugRay);

private:
	void DebugRay(const RE::bhkPickData& a_pickData, const RE::NiPoint3& a_targetPos, ImU32 color) const;

	// members
	StartPoint                  startPoint;
	std::array<RE::NiPoint3, 4> targetPoints;
	std::array<ImU32, 4>        debugColors{ 0xFF2626FF, 0xFF26FFD3, 0xFF7CFF26, 0xFFFF7C2 };
	RE::Actor*                  actor;
};
