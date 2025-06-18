#pragma once

#include <functional>

#include "EditNamedIDPopup.hpp"

#include <NIDEnum.hpp>

// NID::GROUP is just a placeholder
class AddNamedIDPopup : public EditNamedIDPopup<NID::GROUP>
{
public:
	static AddNamedIDPopup* create(NID, std::function<void(const std::string&, short)>&&);

protected:
	bool setup(short, std::function<void(short)>&&, std::function<void()>&&) override;
	void keyDown(cocos2d::enumKeyCodes) override;

	void onSaveButton(cocos2d::CCObject*) override;

private:
	NID m_nid;
	std::function<void(const std::string&, short)> m_saved_callback;
};
