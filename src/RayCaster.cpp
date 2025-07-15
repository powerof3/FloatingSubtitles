#include "RayCaster.h"

#include "ImGui/Util.h"

RayCollector::RayCollector(RE::Actor* a_actor, RE::COL_LAYER a_layer) :
	actor(a_actor)
{
	RE::CFilter actorFilter;
	a_actor->GetCollisionFilterInfo(actorFilter);
	RE::bhkCollisionFilter::GetSingleton()->layerBitfields[std::to_underlying(a_layer)] = (1LL << (actorFilter.filter & RE::CFilter::Flags::kLayerMask)) | 0x22757;
}

void RayCollector::AddRayHit(const RE::hkpCdBody& a_body, const RE::hkpShapeRayCastCollectorOutput& a_hitInfo)
{
	const auto* body = std::addressof(a_body);
	for (const auto* parent = body->parent; parent; parent = parent->parent) {
		body = parent;
	}

	if (!body) {
		return;
	}

	const auto rootCollidable = static_cast<const RE::hkpCollidable*>(body);
	auto       rootColFilter = rootCollidable->broadPhaseHandle.collisionFilterInfo;

	switch (rootColFilter.GetCollisionLayer()) {
	case RE::COL_LAYER::kStatic:
	case RE::COL_LAYER::kTerrain:
	case RE::COL_LAYER::kGround:
		hkpClosestRayHitCollector::AddRayHit(a_body, a_hitInfo);
		break;
	case RE::COL_LAYER::kBiped:
	case RE::COL_LAYER::kBipedNoCC:
	case RE::COL_LAYER::kDeadBip:
	case RE::COL_LAYER::kCharController:
		{
			if (const auto owner = RE::TESHavokUtilities::FindCollidableRef(*rootCollidable); owner && owner == actor) {
				hkpClosestRayHitCollector::AddRayHit(a_body, a_hitInfo);
			}
		}
		break;
	default:
		break;
	}
}

void RayCaster::StartPoint::Init()
{
	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return;
	}

	debug = player->GetPosition();
	debug.z += player->eyeHeight;

	if (auto worldRoot = RE::Main::WorldRootNode(); !worldRoot->children.empty()) {
		camera = worldRoot->children.front()->local.translate;
	} else if (auto pcCamera = RE::PlayerCamera::GetSingleton(); pcCamera && pcCamera->cameraRoot) {
		camera = pcCamera->cameraRoot->world.translate;
	} else {
		camera = debug;
	}
}

RayCaster::RayCaster(RE::Actor* a_target) :
	actor(a_target)
{
	startPoint.Init();
}

RayCaster::Result RayCaster::GetResult(bool a_debugRay)
{
	if (auto root = actor->Get3D()) {
		if (!RE::Main::WorldRootCamera()->PointInFrustum(root->worldBound.center, root->worldBound.radius)) {
			return Result::kOffScreen;
		}
	}

	auto cell = actor->parentCell;
	auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;

	if (!bhkWorld) {
		return Result::kOffScreen;  // can't raycast so might as well return true
	}

	targetPoints[0] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kEye);
	targetPoints[1] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kHead);
	targetPoints[2] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kTorso);
	targetPoints[3] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kFeet);

	RE::bhkPickData pickData{};

	const auto havokWorldScale = RE::bhkWorld::GetWorldScale();
	pickData.rayInput.from = startPoint.camera * havokWorldScale;
	pickData.rayInput.enableShapeCollectionFilter = false;
	pickData.rayInput.filterInfo.SetCollisionLayer(RE::COL_LAYER::kLOS);

	RayCollector collector(actor, RE::COL_LAYER::kLOS);
	pickData.rayHitCollectorA8 = &collector;

	bool result = false;

	for (std::uint32_t i = 0; i < targetPoints.size(); ++i) {
		if (result) {
			break;
		}

		collector.Reset();
		pickData.rayInput.to = targetPoints[i] * havokWorldScale;

		if (bhkWorld->PickObject(pickData)) {
			if (auto* collidable = pickData.rayOutput.rootCollidable) {
				result = RE::TESHavokUtilities::FindCollidableRef(*collidable) == actor;
			}
		}

		if (a_debugRay) {
			DebugRay(pickData, targetPoints[i], debugColors[i]);
		}
	}

	return result ? Result::kVisible : Result::kObscured;
}

void RayCaster::DebugRay(const RE::bhkPickData& a_pickData, const RE::NiPoint3& a_targetPos, ImU32 color) const
{
	auto hitPos = (a_targetPos - startPoint.debug) * a_pickData.rayOutput.hitFraction + startPoint.debug;

	ImGui::DrawLine(startPoint.debug, hitPos, a_pickData.rayOutput.HasHit() ? color : IM_COL32_BLACK);

	if (auto* collidable = a_pickData.rayOutput.rootCollidable) {
		auto node = RE::TESHavokUtilities::FindCollidableObject(*collidable);
		auto userData = node ? node->GetUserData() : nullptr;

		std::string nodeName = node ? node->name.c_str() : "";
		if (node && nodeName.empty()) {
			nodeName = node->GetRTTI()->GetName();
		}

		auto text = std::format("{} : {} [{}]",
			userData ? userData->GetDisplayFullName() : "",
			nodeName,
			collidable->broadPhaseHandle.collisionFilterInfo.GetCollisionLayer());

		ImGui::DrawTextAtPoint(hitPos, text.c_str(), color);
	}
}
