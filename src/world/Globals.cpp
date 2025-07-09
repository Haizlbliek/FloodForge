#include "Globals.hpp"

std::string ROOM_TAGS[9] = { "SHELTER", "ANCIENTSHELTER", "GATE", "SWARMROOM", "PERF_HEAVY", "SCAVOUTPOST", "SCAVTRADER", "NOTRACKERS", "ARENA" };
std::string ROOM_TAG_NAMES[9] = { "Shelter", "Ancient Shelter", "Gate", "Swarm Room", "Performance Heavy", "Scavenger Outpost", "Scavenger Trader", "No Trackers", "Arena (MSC)" };


namespace EditorState {
	Mouse *mouse = nullptr;
	Window *window = nullptr;

	OffscreenRoom* offscreenDen = nullptr;
	std::vector<Room*> rooms;
	std::vector<Connection*> connections;
	std::vector<std::string> subregions;
	
	Vector2 cameraOffset = Vector2(0.0, 0.0);
	double cameraScale = 32.0;
	double selectorScale = 1.0;

	int roomColours = 0;
	bool visibleLayers[] = { true, true, true };
	bool visibleDevItems = false;
	
	Region region;

	int selectingState = 0;
	Room *roomPossibleSelect = nullptr;
	std::set<Room*> selectedRooms;

	std::vector<std::string> fails;
}