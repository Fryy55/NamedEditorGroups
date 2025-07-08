#include "GradientLabel.hpp"

#include <NIDManager.hpp>

using namespace geode::prelude;

GradientLabel* GradientLabel::create()
{
	auto ret = new GradientLabel;

	if (ret->init())
	{
		ret->autorelease();
		return ret;
	}

	delete ret;
	return nullptr;
}

void GradientLabel::setup(GradientLabel::Position pos, int id)
{
	if (!id) // if no group is set there's no need to display a nonexisting bound
	{
		m_idNumLabel->setString("");
		m_idNameLabel->setString("");

		return;
	}

	m_idNumLabel->setString(NIDManager::getMoreNumIDsSetting() ? utils::numToString(id).c_str() : "");
	m_idNameLabel->setString(
		NIDManager::getNameForID<NID::GROUP>(
			id
		).unwrapOr("").c_str()
	);
	m_idNumLabel->limitLabelWidth(35.f, 1.f, 0.1f);
	m_idNameLabel->limitLabelWidth(45.f, 1.f, 0.1f);
	this->setContentSize(m_idNumLabel->getScaledContentSize());

	m_idNumLabel->setAnchorPoint({ 0.f, 0.f });
	m_idNameLabel->setPosition(0.f, this->getContentHeight() / 2.f);

	switch (pos)
	{
		case TL: [[fallthrough]];
		case L: [[fallthrough]];
		case BL:
			m_idNameLabel->setAnchorPoint({ 1.f, 0.5f });
			break;

		case TR: [[fallthrough]];
		case R: [[fallthrough]];
		case BR:
			m_idNameLabel->setAnchorPoint({ 0.f, 0.5f });
			m_idNameLabel->setPositionX(this->getContentWidth());
			break;

		case U:
			m_idNameLabel->setAnchorPoint({ NIDManager::getMoreNumIDsSetting() ? 0.f : 0.5f, 0.5f });	
			m_idNameLabel->setPositionX(this->getContentWidth());
			break;

		case D:
			m_idNameLabel->setAnchorPoint({ 0.5f, 1.f });
			m_idNameLabel->setPosition(this->getContentWidth() / 2.f, 5.f);
			break;

		default:
			throw "Unknown position";
	}
}

bool GradientLabel::init()
{
	if (!CCNodeRGBA::init())
		return false;

	m_idNumLabel = CCLabelBMFont::create("", "bigFont.fnt");
	m_idNumLabel->setID("num-id-label");
	this->addChild(m_idNumLabel);
	m_idNameLabel = CCLabelBMFont::create("", "bigFont.fnt");
	m_idNameLabel->setID("named-id-label");
	this->addChild(m_idNameLabel);

	this->setCascadeOpacityEnabled(true);
	this->setAnchorPoint({ 0.5f, 0.5f });

	return true;
}
