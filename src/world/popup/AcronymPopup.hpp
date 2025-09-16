#pragma once

#include "../../gl.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../../Window.hpp"
#include "../../Theme.hpp"

#include "../Globals.hpp"
#include "../Room.hpp"
#include "../../popup/Popups.hpp"

class AcronymPopup : public Popup {
	public:
		AcronymPopup();

		void draw();

		void close();

		virtual std::string banLetters() { return "_/\\"; };

		virtual void submit(std::string acronym);

	protected:
		std::string setTo;
		bool needsSet = false;
		bool canClose;
};