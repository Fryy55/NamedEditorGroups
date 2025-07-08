#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/KeyframeGameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <NIDManager.hpp>
#include <NIDExtrasManager.hpp>

#include "utils.hpp"
#include "constants.hpp"
#include "globals.hpp"
#include "GradientLabel.hpp"

using namespace geode::prelude;

static auto objInArray = [](int id, const auto& container) {
	return ng::utils::getIndexOf(container, id) != -1;
};

struct NIDGameObject : geode::Modify<NIDGameObject, GameObject>
{
	CCNode* parentForZLayer(int zLayer, bool blending, int parentMode)
	{
		if (objInArray(this->m_objectID, ng::constants::OBJECTS_TO_MOVE))
			parentMode = 4;
		// this (pMode = 4) forces area triggers and stuff like that to be
		// in the special layer (CCNodeContainer for the respective zlayer)
		// since pMode for normal triggers is 4 when they get added,
		// so we can add children there :evil:

		return GameObject::parentForZLayer(zLayer, blending, parentMode);
	}
};

struct NIDKeyframeGameObject : geode::Modify<NIDKeyframeGameObject, KeyframeGameObject>
{
	void updateShadowObjects(GJBaseGameLayer* p0, EditorUI* p1)
	{
		KeyframeGameObject::updateShadowObjects(p0, p1);

		if (this->m_previewSprite)
		{
			// gd resets content size to 0 for some reason and it offsets everything
			// (even the trigger itself lmao)
			this->setContentSize({ 21.f, 21.f });
			this->m_previewSprite->CCNode::setPosition(10.5f, 10.5f);
			this->m_previewSprite->setZOrder(-1); // so that the name label is visible
		}
	}
};

struct NIDEffectGameObject : geode::Modify<NIDEffectGameObject, EffectGameObject>
{
	struct Fields
	{
		Ref<CCLabelBMFont> m_id_name_label = CCLabelBMFont::create("", "bigFont.fnt");
		bool m_has_id_name_label = false;
	};

	void customSetup()
	{
		EffectGameObject::customSetup();

		auto id = this->m_objectID;

		bool isTrigger = objInArray(id, ng::constants::TRIGGER_OBJECT_IDS_WITH_LABEL);
		bool isCollision = objInArray(id, ng::constants::COLLISION_OBJECT_IDS_WITH_LABEL);
		bool isCounter = objInArray(id, ng::constants::COUNTER_OBJECT_IDS_WITH_LABEL);
		bool isTimer = objInArray(id, ng::constants::TIMER_OBJECT_IDS_WITH_LABEL);

		if (!(isTrigger || isCollision || isCounter || isTimer))
			return;

		// lol why does the game not do this
		this->setCascadeOpacityEnabled(true);

		// 28.5f is content width of move trigger, which works well for all other triggers
		m_fields->m_id_name_label->limitLabelWidth(28.5f + 10.f, .6f, .1f);
		// can't add the label here
	}
};

struct NIDLevelEditorLayer : geode::Modify<NIDLevelEditorLayer, LevelEditorLayer>
{
	bool init(GJGameLevel* p0, bool p1)
	{
		NIDManager::updateMoreNumIDsSetting();

		return LevelEditorLayer::init(p0, p1);
	}

	static void updateObjectLabel(GameObject* object)
	{
		LevelEditorLayer::updateObjectLabel(object);

		auto id = object->m_objectID;

		bool isTrigger = objInArray(id, ng::constants::TRIGGER_OBJECT_IDS_WITH_LABEL);
		bool isCollision = objInArray(id, ng::constants::COLLISION_OBJECT_IDS_WITH_LABEL);
		bool isCounter = objInArray(id, ng::constants::COUNTER_OBJECT_IDS_WITH_LABEL);
		bool isTimer = objInArray(id, ng::constants::TIMER_OBJECT_IDS_WITH_LABEL);

		bool isNonNum = false;

		if (!(isTrigger || isCollision || isCounter || isTimer))
			return;

		auto effectGameObj = static_cast<NIDEffectGameObject*>(object);

		bool& hasIDNameLabel = effectGameObj->m_fields->m_has_id_name_label;
		CCLabelBMFont* idNameLabel = effectGameObj->m_fields->m_id_name_label;
		std::string idNameStr = "";
		CCPoint idLabelPos;

		auto nonNumTriggerUpdate = [&effectGameObj, &idLabelPos, &isNonNum](float y) {
			isNonNum = true;
			auto offsetX = effectGameObj->getContentWidth() / 2.f;
			idLabelPos = CCPoint{ offsetX, y };

			std::string idNumber = utils::numToString(effectGameObj->m_targetGroupID);

			// query selector because of checkpoints
			if (auto idLabel = static_cast<CCLabelBMFont*>(effectGameObj->querySelector("target-id-label")))
				idLabel->setString(idNumber.c_str());
			else
			{
				idLabel = CCLabelBMFont::create(
					idNumber.c_str(),
					"bigFont.fnt"
				);
				idLabel->setPosition(offsetX, y);
				idLabel->setScale(0.5f);
				idLabel->setZOrder(1);
				idLabel->setID("target-id-label");
				effectGameObj->addChild(idLabel);
			}
		};

		switch (id)
		{
			// Counter Label
			case 1615u:
				idLabelPos = CCPoint{ 21.75f, 7.75f };

				if (auto label = effectGameObj->getChildByID("counter-label"); !label)
					if (auto idLabel = effectGameObj->getChildByType<CCLabelBMFont*>(0))
						idLabel->setID("counter-label");
				break;

			// Area/AreaStop Triggers
			case 3006u: [[fallthrough]];
			case 3007u: [[fallthrough]];
			case 3008u: [[fallthrough]];
			case 3009u: [[fallthrough]];
			case 3010u: [[fallthrough]];
			case 3024u: [[fallthrough]];
			// Enter/EnterStop Triggers
			case 3017u: [[fallthrough]];
			case 3018u: [[fallthrough]];
			case 3019u: [[fallthrough]];
			case 3020u: [[fallthrough]];
			case 3021u: [[fallthrough]];
			case 3023u:
				nonNumTriggerUpdate(24.f);
				break;

			// EditArea Triggers
			case 3011u: [[fallthrough]];
			case 3012u: [[fallthrough]];
			case 3013u: [[fallthrough]];
			case 3014u: [[fallthrough]];
			case 3015u:
				nonNumTriggerUpdate(35.f);
				break;

			// End Trigger
			case 3600u: [[fallthrough]];
			// Event Trigger
			case 3604u: [[fallthrough]];
			// Reset Trigger
			case 3618u: [[fallthrough]];
			// Gravity Trigger
			case 2066u:
				nonNumTriggerUpdate(14.f);
				break;

			// LinkVisible Trigger
			case 3662u: [[fallthrough]];
			// Item Triggers
			case 3619u: [[fallthrough]];
			case 3620u: [[fallthrough]];
			case 3641u:
				nonNumTriggerUpdate(23.f);
				break;

			// Custom Orb
			case 1594u:
				nonNumTriggerUpdate(15.5f);
				break;

			// Keyframe
			case 3032u: [[fallthrough]];
			// Time/TimeEvent/TimeControl Triggers
			case 3614u: [[fallthrough]];
			case 3615u: [[fallthrough]];
			case 3617u:
				nonNumTriggerUpdate(11.5f);
				break;

			// UI Trigger
			case 3613u: [[fallthrough]];
			// StaticCamera Trigger
			case 1914u:
				nonNumTriggerUpdate(12.5f);
				break;

			// ReTargetAdvFollow Trigger
			case 3661u: [[fallthrough]];
			// SpawnParticle Trigger
			case 3608u:
				nonNumTriggerUpdate(22.f);
				break;

			// PlayerControl Trigger
			case 1932u:
				nonNumTriggerUpdate(21.f);
				break;

			// Teleport Trigger
			case 3022u:
				nonNumTriggerUpdate(9.f);
				break;

			// Teleport Portal
			case 2902u:
				nonNumTriggerUpdate(46.f);
				break;

			// Toggle Block
			case 3643u: [[fallthrough]];
			// Checkpoint
			case 2063u:
				nonNumTriggerUpdate(16.f);
				break;

			// Teleport Orb
			case 3027u:
				nonNumTriggerUpdate(18.5f);
				break;

			default:
				if (auto label = effectGameObj->getChildByID("target-id-label"))
					idLabelPos = label->getPosition();
				else if (auto idLabel = effectGameObj->getChildByType<CCLabelBMFont*>(0); idLabel)
				{
					idLabel->setID("target-id-label");
					idLabelPos = idLabel->getPosition();
				}
				break;
		}

		auto standardLabelLimit = [](CCLabelBMFont* label) { label->limitLabelWidth(35.f, 0.5f, 0.15f); };
		// a switch here *might* end up being faster than an if chain idk
		switch (id)
		{
			//* Basic triggers *named* ID adjustments
			// Pulse Trigger
			case 1006u: {
				auto pulseTrigger = static_cast<EffectGameObject*>(object);

				if (pulseTrigger->m_pulseTargetType == 1)
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
				else
					idNameStr = NIDManager::getNameForID<NID::COLOR>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
			}
			break;

			// Color Trigger
			case 899u:
				idNameStr = NIDManager::getNameForID<NID::COLOR>(
					effectGameObj->m_targetColor
				).unwrapOr("");
				break;

			// Stop Trigger
			case 1616u:
				idNameStr = effectGameObj->m_targetControlID ?
					"ControlID" // ControlID // TODO: use actual CIDs
					:
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("").c_str(); // GroupID
				break;

			//* Basic triggers *numeric* ID modifications (check for the "more-num-id" setting and limit num width maybe) + some of the above
			// Random Trigger
			case 1912u: [[fallthrough]];
			// ItemCompare Trigger
			case 3620u: [[fallthrough]];
			// InstantCollision Trigger
			case 3609u: [[fallthrough]];
			// Collision State
			case 3640u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if ((id == 3620u || id == 3609u) && NIDManager::getMoreNumIDsSetting()) // ItemCompare Trigger/InstantCollision Trigger
					idNumLabel->setString(
						fmt::format(
							"{}/{}",
							effectGameObj->m_targetGroupID,
							effectGameObj->m_centerGroupID
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"a|{}\nb|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = (id == 3640u) ?
					CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 15.f } // collision state
					:
					CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f }; // other triggers
			}
			break;

			// Follow Trigger
			case 1347u: [[fallthrough]];
			// AdvFollow Trigger
			case 3016u: [[fallthrough]];
			// ReTargetAdvFollow Trigger
			case 3661u: [[fallthrough]];
			// Move Trigger
			case 901u: {
				// if move trigger that doesn't have targets
				if (id == 901u && !(effectGameObj->m_useMoveTarget || effectGameObj->m_isDirectionFollowSnap360))
				{
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
					break;
				}

				std::string followIDNum = utils::numToString(effectGameObj->m_centerGroupID);
				std::string followID = NIDManager::getNameForID<NID::GROUP>(
					effectGameObj->m_centerGroupID
				).unwrapOr("Unnamed");
				if (effectGameObj->m_targetPlayer1)
				{
					followIDNum = "P1";
					followID = "Player 1";
				}
				else if (effectGameObj->m_targetPlayer2)
				{
					followIDNum = "P2";
					followID = "Player 2";
				}
				else if (effectGameObj->m_followCPP)
				{
					followIDNum = "C";
					followID = "Center";
				}

				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(
						fmt::format(
							"{}{}>{}",
							effectGameObj->m_targetGroupID,
							id == 901u ? // Move Trigger
								fmt::format(
									"c{}",
									effectGameObj->m_targetModCenterID
								).c_str()
								:
								"",
							std::move(followIDNum)
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"t|{}\n{}f|{}",
					effectGameObj->m_targetControlID ?
						"ControlID" // ControlID // TODO: use actual CIDs
						:
						NIDManager::getNameForID<NID::GROUP>(
							effectGameObj->m_targetGroupID
						).unwrapOr("Unnamed").c_str(), // GroupID
					id == 901u ? // Move Trigger
						fmt::format(
							"c|{}\n",
							NIDManager::getNameForID<NID::GROUP>(
								effectGameObj->m_targetModCenterID
							).unwrapOr("Unnamed")
						).c_str()
						:
						"",
					std::move(followID)
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// Rotate Trigger
			case 1346u: {
				std::string idNumStr = fmt::format(
					"{}c{}",
					effectGameObj->m_targetGroupID,
					effectGameObj->m_centerGroupID
				);
				idNameStr = fmt::format(
					"t|{}\nc|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed")
				);

				if (effectGameObj->m_useMoveTarget || effectGameObj->m_isDirectionFollowSnap360)
				{
					std::string followIDNum = utils::numToString(effectGameObj->m_rotationTargetID);
					std::string followID = NIDManager::getNameForID<NID::GROUP>(
							effectGameObj->m_rotationTargetID
					).unwrapOr("Unnamed");

					if (effectGameObj->m_targetPlayer1)
					{
						followIDNum = "P1";
						followID = "Player 1";
					}
					else if (effectGameObj->m_targetPlayer2)
					{
						followIDNum = "P2";
						followID = "Player 2";
					}

					idNumStr.append(
						fmt::format(
							">{}",
							std::move(followIDNum)
						)
					);
					idNameStr.append(
						fmt::format(
							"\nf|{}",
							std::move(followID)
						)
					);
				}

				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(idNumStr.c_str());
				standardLabelLimit(idNumLabel);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// Scale Trigger
			case 2067: {
				if (!effectGameObj->m_centerGroupID)
				{
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
					break;
				}

				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(
						fmt::format(
							"{}c{}",
							effectGameObj->m_targetGroupID,
							effectGameObj->m_centerGroupID
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"t|{}\nc|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// Touch Trigger
			case 1595u: {
				std::string playerMode;

				switch (effectGameObj->m_touchPlayerMode)
				{
					case TouchTriggerControl::Both:
						playerMode = "All";
						break;

					case TouchTriggerControl::Player1:
						playerMode = "P1";
						break;

					case TouchTriggerControl::Player2:
						playerMode = "P2";
						break;

					default:
						throw "Unknown player mode";
				}

				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(
						fmt::format(
							"{}:{}",
							std::move(playerMode),
							effectGameObj->m_targetGroupID
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = NIDManager::getNameForID<NID::GROUP>(
					effectGameObj->m_targetGroupID
				).unwrapOr("");
			}
			break;

			// KeyframeAnim Trigger
			case 3033u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				idNumLabel->setPosition(object->getContentWidth() / 2.f, 16.f); // rob's positioning of this sucks :thumbsdown:

				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(
						fmt::format(
							"{}|{}",
							effectGameObj->m_animationID,
							effectGameObj->m_targetGroupID
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"a|{}\nt|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_animationID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// Count Trigger
			case 1611u: [[fallthrough]];
			// InstantCount Trigger
			case 1811u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				if (NIDManager::getMoreNumIDsSetting())
					idNumLabel->setString(
						fmt::format(
							"{}|{}",
							effectGameObj->m_itemID,
							effectGameObj->m_targetGroupID
						).c_str()
					);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"i|{}\nt|{}",
					NIDManager::getNameForID<NID::COUNTER>(
						effectGameObj->m_itemID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			//* Non-num triggers setups
			// Counter Label
			case 1615u: {
				auto labelNode = static_cast<LabelGameObject*>(object);

				switch (labelNode->m_shownSpecial)
				{
					case -1:
						idNameStr = "MainTime";
						break;

					case -2:
						idNameStr = "Points";
						break;

					case -3:
						idNameStr = "Attempts";
						break;

					case 0:
						if (labelNode->m_isTimeCounter)
							idNameStr = NIDManager::getNameForID<NID::TIMER>(
								effectGameObj->m_itemID
							).unwrapOr("");
						else
							idNameStr = NIDManager::getNameForID<NID::COUNTER>(
								effectGameObj->m_itemID
							).unwrapOr("");
						break;

					default:
						throw "Unknown special mode";
				}
			}
			break;

			// Area Triggers
			case 3006u: [[fallthrough]];
			case 3007u: [[fallthrough]];
			case 3008u: [[fallthrough]];
			case 3009u: [[fallthrough]];
			case 3010u:
				if (auto effectID = static_cast<EnterEffectObject*>(object)->m_effectID)
				{
					auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
					idNumLabel->setString(fmt::format(
						"{}|{}",
						effectGameObj->m_targetGroupID,
						effectID
					).c_str());
					standardLabelLimit(idNumLabel);

					idNameStr = fmt::format(
						"g|{}\ne|{}",
						NIDManager::getNameForID<NID::GROUP>(
							effectGameObj->m_targetGroupID
						).unwrapOr("Unnamed"),
						NIDManager::getNameForID<NID::EFFECT>(
							effectID
						).unwrapOr("Unnamed")
					).c_str();

					idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
				}
				break;

			// AreaStop Trigger
			case 3024u:
				idNameStr = NIDManager::getNameForID<NID::EFFECT>(
					effectGameObj->m_targetGroupID
				).unwrapOr("");
				break;

			// Enter/EnterStop Triggers
			case 3017u: [[fallthrough]];
			case 3018u: [[fallthrough]];
			case 3019u: [[fallthrough]];
			case 3020u: [[fallthrough]];
			case 3021u: [[fallthrough]];
			case 3023u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				if (auto effectID = static_cast<EnterEffectObject*>(object)->m_effectID)
				{
					idNameStr = NIDManager::getNameForID<NID::EFFECT>(
						effectID
					).unwrapOr("");
					idNumLabel->setString(
						utils::numToString(effectID).c_str()
					);
				}
				else
					idNumLabel->setString("");
			}
			break;

			// EditArea Triggers
			case 3011u: [[fallthrough]];
			case 3012u: [[fallthrough]];
			case 3013u: [[fallthrough]];
			case 3014u: [[fallthrough]];
			case 3015u:
				if (static_cast<EnterEffectObject*>(object)->m_useEffectID)
					idNameStr = NIDManager::getNameForID<NID::EFFECT>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
				else
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
				break;

			// End Trigger
			case 3600u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				if (!effectGameObj->m_centerGroupID) // if doesn't have a targetpos
				{
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");	
					standardLabelLimit(idNumLabel);
					break;
				}

				idNumLabel->setString(
					fmt::format(
						"{}|{}",
						effectGameObj->m_targetGroupID,
						effectGameObj->m_centerGroupID
					).c_str()
				);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"s|{}\nt|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}

			break;

			// Gravity Trigger
			case 2066u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				if (effectGameObj->m_targetPlayer1)
					idNumLabel->setString("P1");
				else if (effectGameObj->m_targetPlayer2)
					idNumLabel->setString("P2");
				else if (effectGameObj->m_followCPP)
					idNumLabel->setString("PT");
				else
					idNumLabel->setString("All");
			}
			break;

			// Gradient Trigger
			case 2903u: {
				auto gradientTrigger = static_cast<GradientTriggerObject*>(object);
				constexpr CCSize labelsCS = { 40.f, 40.f }; // compensate for scale

				auto labels = static_cast<CCNodeRGBA*>(object->getChildByID("labels"_spr));
				GradientLabel* tluLabel;
				GradientLabel* trrLabel;
				GradientLabel* brdLabel;
				GradientLabel* bllLabel;

				if (!hasIDNameLabel)
				{
					hasIDNameLabel = true;

					labels = CCNodeRGBA::create();
					tluLabel = GradientLabel::create();
					trrLabel = GradientLabel::create();
					brdLabel = GradientLabel::create();
					bllLabel = GradientLabel::create();

					labels->setID("labels"_spr);
					tluLabel->setID("tl-u-label"_spr);
					trrLabel->setID("tr-r-label"_spr);
					brdLabel->setID("br-d-label"_spr);
					bllLabel->setID("bl-l-label"_spr);

					labels->addChild(tluLabel);
					labels->addChild(trrLabel);
					labels->addChild(brdLabel);
					labels->addChild(bllLabel);

					labels->setPosition(13.f, 2.f);
					object->addChild(labels);

					labels->setScale(0.5f);
					labels->setContentSize(labelsCS);
					labels->setCascadeOpacityEnabled(true);
				}
				else
				{
					tluLabel = static_cast<GradientLabel*>(labels->getChildByID("tl-u-label"_spr));
					trrLabel = static_cast<GradientLabel*>(labels->getChildByID("tr-r-label"_spr));
					brdLabel = static_cast<GradientLabel*>(labels->getChildByID("br-d-label"_spr));
					bllLabel = static_cast<GradientLabel*>(labels->getChildByID("bl-l-label"_spr));
				}

				if (gradientTrigger->m_vertexMode)
				{
					tluLabel->setup(GradientLabel::TL, gradientTrigger->m_leftTopLeftID);
					tluLabel->setPosition(0.f, labelsCS.height);

					trrLabel->setup(GradientLabel::TR, gradientTrigger->m_rightTopRightID);
					trrLabel->setPosition(labelsCS.width, labelsCS.height);

					brdLabel->setup(GradientLabel::BR, gradientTrigger->m_downBottomRightID);
					brdLabel->setPosition(labelsCS.width, 0.f);

					bllLabel->setup(GradientLabel::BL, gradientTrigger->m_upBottomLeftID);
					bllLabel->setPosition(0.f, 0.f);
				}
				else
				{
					tluLabel->setup(GradientLabel::U, gradientTrigger->m_upBottomLeftID);
					tluLabel->setPosition(labelsCS.width / 2.f, labelsCS.height);

					trrLabel->setup(GradientLabel::R, gradientTrigger->m_rightTopRightID);
					trrLabel->setPosition(labelsCS.width, labelsCS.height / 2.f);

					brdLabel->setup(GradientLabel::D, gradientTrigger->m_downBottomRightID);
					brdLabel->setPosition(labelsCS.width / 2.f, 0.f);

					bllLabel->setup(GradientLabel::L, gradientTrigger->m_leftTopLeftID);
					bllLabel->setPosition(0.f, labelsCS.height / 2.f);
				}
			}
			break;

			// ItemEdit Trigger
			case 3619u:
				switch (static_cast<ItemTriggerGameObject*>(object)->m_targetItemMode)
				{
					case 1:
						idNameStr = NIDManager::getNameForID<NID::COUNTER>(
							effectGameObj->m_targetGroupID
						).unwrapOr("");
						break;

					case 2:
						idNameStr = NIDManager::getNameForID<NID::TIMER>(
							effectGameObj->m_targetGroupID
						).unwrapOr("");
						break;

					case 3:
						static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"))->setString("P");
						idNameStr = "Points";
						break;

					default:
						throw "Unknown item mode";
				}
				break;

			// PersistentItem Trigger
			case 3641u:
				if (static_cast<ItemTriggerGameObject*>(object)->m_timer)
					idNameStr = NIDManager::getNameForID<NID::TIMER>(
						effectGameObj->m_itemID
					).unwrapOr("");
				else // counter
					idNameStr = NIDManager::getNameForID<NID::COUNTER>(
						effectGameObj->m_itemID
					).unwrapOr("");

				static_cast<CCLabelBMFont*>(
					object->getChildByID("target-id-label")
				)->setString(utils::numToString(effectGameObj->m_itemID).c_str());
				break;

			// Keyframe
			case 3032u: {
				auto keyframe = static_cast<KeyframeGameObject*>(object);
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				// only show stuff when we actually have a spawn group to keep keyframes clean
				if (!keyframe->m_centerGroupID)
					// remove the num id if the spawn group is 0
					idNumLabel->setString("");
				else
					idNumLabel->setString(utils::numToString(keyframe->m_centerGroupID).c_str());

				if (keyframe->m_previewArt)
				{
					idNumLabel->setOpacity(120);
					idNameLabel->setOpacity(120);
				}
				else
				{
					idNumLabel->setOpacity(255);
					idNameLabel->setOpacity(255);
				}

				idNameStr = NIDManager::getNameForID<NID::GROUP>(
					keyframe->m_centerGroupID
				).unwrapOr("");
			}
			break;

			// Time Trigger
			case 3614u: [[fallthrough]];
			// TimeEvent Trigger
			case 3615u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				idNumLabel->setString(
					fmt::format(
						"{}|{}",
						effectGameObj->m_itemID,
						effectGameObj->m_targetGroupID
					).c_str()
				);
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"t|{}\ng|{}",
					NIDManager::getNameForID<NID::TIMER>(
						effectGameObj->m_itemID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// TimeControl Trigger
			case 3617u:
				static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"))->setString(
					utils::numToString(effectGameObj->m_itemID).c_str()
				);
				idNameStr = NIDManager::getNameForID<NID::TIMER>(
					effectGameObj->m_itemID
				).unwrapOr("");
				break;

			// UI Trigger
			case 3613u: [[fallthrough]];
			// SpawnParticle Trigger
			case 3608u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));
				idNumLabel->setString(fmt::format(
					"{}|{}",
					effectGameObj->m_targetGroupID,
					effectGameObj->m_centerGroupID
				).c_str());
				standardLabelLimit(idNumLabel);

				idNameStr = fmt::format(
					"{}|{}\nt|{}",
					id == 3608u ?
						"p" // SpawnParticle Trigger
						:
						"g", // UI Trigger
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed")
				).c_str();

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 6.f };
			}
			break;

			// StaticCamera Trigger
			case 1914u:
				static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"))->setString(
					utils::numToString(effectGameObj->m_centerGroupID).c_str()
				);
				idNameStr = NIDManager::getNameForID<NID::GROUP>(
					effectGameObj->m_centerGroupID
				).unwrapOr("");
				break;

			// PlayerControl Trigger
			case 1932u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				switch (effectGameObj->m_targetPlayer1 * 2 + effectGameObj->m_targetPlayer2)
				{
					case 0: // x/x
						idNumLabel->setString("-");
						break;
					case 1: // x/v
						idNumLabel->setString("P2");
						break;
					case 2: // v/x
						idNumLabel->setString("P1");
						break;
					case 3: // v/v
						idNumLabel->setString("All");
						break;

					default:
						throw "Unknown target mode";
				}
			}
			break;

			// Teleport Portal
			case 2902u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->getChildByID("target-id-label"));

				idLabelPos.x += 13.f;
				idNumLabel->setPositionX(idLabelPos.x);
				idNumLabel->setOpacity(120);
				idNameLabel->setOpacity(120);

				idNameStr = NIDManager::getNameForID<NID::GROUP>(
					effectGameObj->m_targetGroupID
				).unwrapOr("");
			}
			break;

			// Checkpoint
			case 2063u: {
				auto idNumLabel = static_cast<CCLabelBMFont*>(object->querySelector("target-id-label"));

				// so basically for some reason checkpoint throws on GameObject::setOpacity
				// if you add non CCSprite children to it directly, so here's a super
				// silly workaround idk
				if (auto labels = static_cast<CCSprite*>(object->getChildByID("labels")); !labels)
				{
					labels = CCSprite::create();
					labels->setCascadeOpacityEnabled(true);
					labels->setID("labels");

					idNumLabel->removeFromParent();
					labels->addChild(idNumLabel);
					labels->addChild(idNameLabel);
					idNameLabel->setID("id-name-label"_spr);

					hasIDNameLabel = true; // setting this because we're adding labels here
					object->addChild(labels);
				}

				auto checkpoint = static_cast<CheckpointGameObject*>(object);
				idNumLabel->setString(
					fmt::format(
						"{}/{}/{}",
						effectGameObj->m_centerGroupID,
						effectGameObj->m_targetGroupID,
						checkpoint->m_respawnID
					).c_str()
				);
				idNumLabel->limitLabelWidth(55.f, 0.5f, 0.15f);
				idNameStr = fmt::format(
					"t|{}\ns|{}\nr|{}",
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_centerGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("Unnamed"),
					NIDManager::getNameForID<NID::GROUP>(
						checkpoint->m_respawnID
					).unwrapOr("Unnamed")
				);

				idLabelPos = CCPoint{ idLabelPos.x + .75f, idLabelPos.y - 10.f };
			}
			break;

			default:
				if (isTrigger)
					idNameStr = NIDManager::getNameForID<NID::GROUP>(
						effectGameObj->m_targetGroupID
					).unwrapOr("");
				else if (isCollision)
					idNameStr = NIDManager::getNameForID<NID::COLLISION>(
						effectGameObj->m_itemID
					).unwrapOr("");
				else if (isCounter)
					idNameStr = NIDManager::getNameForID<NID::COUNTER>(
						effectGameObj->m_itemID
					).unwrapOr("");
				else if (isTimer)
					idNameStr = NIDManager::getNameForID<NID::TIMER>(
						effectGameObj->m_itemID
					).unwrapOr("");
	}

		if (ng::globals::g_isEditorIDAPILoaded)
		{
			NID nid;
			short id;

			if (isTrigger)
				nid = NID::GROUP;
			else if (isCollision)
				nid = NID::COLLISION;
			else if (isCounter)
				nid = NID::COUNTER;
			else if (isTimer)
				nid = NID::TIMER;

			if (isTrigger)
			{
				idNameLabel->setVisible(
					NIDExtrasManager::getIsNamedIDPreviewed(nid, effectGameObj->m_targetGroupID).unwrapOr(true) &&
					NIDExtrasManager::getIsNamedIDPreviewed(nid, effectGameObj->m_centerGroupID).unwrapOr(true)
				);
			}
			else
			{
				idNameLabel->setVisible(
					NIDExtrasManager::getIsNamedIDPreviewed(nid, effectGameObj->m_itemID).unwrapOr(true)
				);
			}
		}

		idNameLabel->setString(idNameStr.c_str());
		// 28.5f is content width of move trigger, which works well for all other triggers
		idNameLabel->limitLabelWidth(28.5f + 10.f, .5f, .1f);
		idNameLabel->setPosition(idLabelPos.x, idLabelPos.y - 9.f);

		if (!hasIDNameLabel)
		{
			hasIDNameLabel = true;

			idNameLabel->setID("id-name-label"_spr);
			object->addChild(idNameLabel);
		}

		if (isNonNum && !NIDManager::getMoreNumIDsSetting())
			static_cast<CCLabelBMFont*>(object->querySelector("target-id-label"))->setString("");
	}
};
