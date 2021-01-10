#pragma once

#include "ui/TextWidget.hpp"

#include "input/InputListener.hpp"
#include "Delegate.hpp"

NS_UI

class Input
	: public ui::Text
	, public input::Listener
{

public:
	Input();

	ExecuteDelegate<void(std::string)> onConfirm;

	void setActive(bool bActive);
	void clear();

protected:
	ui32 desiredCharacterCount() const override;
	void prePushCharacter(uIndex charIndex, math::Vector2 &cursorPos, f32 fontHeight) override;
	void onPushedAllCharacters(math::Vector2 &cursorPos, f32 fontHeight) override;

private:
	bool mbIsActive;
	uIndex mCursorPos;
	std::string mFieldContent;

	void onInput(input::Event const& evt) override;
	void removeInputAtCursor();
	void removeInputAfterCursor();

};

NS_END