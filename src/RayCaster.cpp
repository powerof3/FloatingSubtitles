#include "RayCaster.h"

#include "ImGui/Util.h"

void StartPoint::Init()
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
	case RE::COL_LAYER::kUnidentified:
	case RE::COL_LAYER::kTransparent:
	case RE::COL_LAYER::kClutter:
	case RE::COL_LAYER::kProps:
	case RE::COL_LAYER::kTrigger:
	case RE::COL_LAYER::kNonCollidable:
	case RE::COL_LAYER::kCloudTrap:
	case RE::COL_LAYER::kActorZone:
	case RE::COL_LAYER::kProjectileZone:
	case RE::COL_LAYER::kGasTrap:
	case RE::COL_LAYER::kTransparentSmall:
	case RE::COL_LAYER::kInvisibleWall:
	case RE::COL_LAYER::kSpellExplosion:
		return;
	case RE::COL_LAYER::kBiped:
	case RE::COL_LAYER::kBipedNoCC:
	case RE::COL_LAYER::kDeadBip:
	case RE::COL_LAYER::kCharController:
		{
			auto owner = RE::TESHavokUtilities::FindCollidableRef(*rootCollidable);
			if (owner && owner != actor) {
				return;
			}
			hkpClosestRayHitCollector::AddRayHit(a_body, a_hitInfo);
		}
		break;
	default:
		hkpClosestRayHitCollector::AddRayHit(a_body, a_hitInfo);
		break;
	}
}

RayCaster::RayCaster(RE::Actor* a_target) :
	actor(a_target)
{
	startPoint.Init();
}

bool RayCaster::CanRayCastToTarget(bool a_debugRay)
{
	if (auto root = actor->Get3D()) {
		if (!RE::Main::WorldRootCamera()->PointInFrustum(root->worldBound.center, root->worldBound.radius * 2.0f)) {
			return false;
		}
	}

	targetPoints[0] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kHead);
	targetPoints[1] = actor->CalculateLOSLocation(RE::ACTOR_LOS_LOCATION::kEye);
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

		auto node = RE::TES::GetSingleton()->Pick(pickData);
		result = node && node->GetUserData() == actor;

		if (a_debugRay) {
			DebugRay(pickData, targetPoints[i], debugColors[i]);
		}
	}

	return result;
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
