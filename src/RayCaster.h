#pragma once

struct StartPoint
{
	void Init();

	RE::NiPoint3 camera;
	RE::NiPoint3 debug;
};

class RayCollector : public RE::hkpClosestRayHitCollector
{
public:
	RayCollector(){}
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
	RayCaster() = default;
	RayCaster(RE::Actor* a_target);

	bool CanRayCastToTarget(bool a_debugRay);

private:
	void DebugRay(const RE::bhkPickData& a_pickData, const RE::NiPoint3& a_targetPos, ImU32 color) const;

	// members
	StartPoint                  startPoint;
	std::array<RE::NiPoint3, 4> targetPoints;
	std::array<ImU32, 4>        debugColors{ 0xFF9226FF, 0xFFFF2626, 0xFF92FF26, 0xFF26FFFF };
	RE::Actor*                  actor;
};
