#pragma once

#include "ui/Window.h"
#include "core/Common.h"

namespace frontend {

class DisconnectWindow: public ui::Window {
public:
	DisconnectWindow(Window* parent) :
			ui::Window(parent) {
		core_assert(loadResourceFile("ui/window/disconnect.tb.txt"));
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == tb::TBIDC("ok")) {
			Close();
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};

}
