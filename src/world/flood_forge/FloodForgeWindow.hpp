#pragma once

#include "../../math/Vector.hpp"

#include "../../popup/Popups.hpp"
#include "../Connection.hpp"
#include "../Room.hpp"

namespace FloodForgeWindow {
	enum class ConnectionState {
		None,
		NoConnection,
		Connection
	};

	void updateCamera();

	void updateOriginalControls();

	void updateFloodForgeControls();

	void updateMain();

	void Draw();

	extern Vector2 worldMouse;


	extern bool cameraPanning;
	extern bool cameraPanningBlocked;
	extern Vector2 cameraPanStartMouse;
	extern Vector2 cameraPanStart;
	extern Vector2 cameraPanTo;
	extern double cameraScaleTo;
	
	extern Room *holdingRoom;
	extern Popup *holdingPopup;
	extern Vector2 holdingStart;
	extern int holdingType;
	
	extern Vector2 selectionStart;
	extern Vector2 selectionEnd;
	extern int roomSnap;
	
	extern Vector2 *connectionStart;
	extern Vector2 *connectionEnd;
	extern Connection *currentConnection;
	extern bool currentConnectionValid;
	extern std::string connectionError;
	extern ConnectionState connectionState;
}