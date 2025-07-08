#pragma once

#include <Geode/cocos/label_nodes/CCLabelBMFont.h>

class GradientLabel final : public cocos2d::CCNodeRGBA
{
public:
	enum Position : std::uint8_t
	{
		TL,
		U,
		TR,
		R,
		BR,
		D,
		BL,
		L
	};

	static GradientLabel* create();

	void setup(Position pos, int id);

private:
	virtual bool init() override;

	// Fields
	cocos2d::CCLabelBMFont* m_idNumLabel;
	cocos2d::CCLabelBMFont* m_idNameLabel;
};
