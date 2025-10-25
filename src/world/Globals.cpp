#include "Globals.hpp"

std::string ROOM_TAGS[ROOM_TAG_COUNT] = { "SHELTER", "ANCIENTSHELTER", "GATE", "SWARMROOM", "PERF_HEAVY", "SCAVOUTPOST", "SCAVTRADER", "NOTRACKERS", "ARENA" };
std::string ROOM_TAG_NAMES[ROOM_TAG_COUNT] = { "Shelter", "Ancient Shelter", "Gate", "Swarm Room", "Performance Heavy", "Scavenger Outpost", "Scavenger Trader", "No Trackers", "Arena (MSC)" };


namespace EditorState {
	bool dropletOpen;

	// FloodForge
	OffscreenRoom* offscreenDen = nullptr;
	std::vector<Room*> rooms;
	std::vector<Connection*> connections;
	std::vector<std::string> subregions;
	int screenCount = 0;

	Vector2 placingRoomPosition;
	Vector2i placingRoomSize = { 48, 35 };
	bool placingRoom;

	Vector2 cameraOffset = Vector2(0.0, 0.0);
	double cameraScale = 32.0;
	double selectorScale = 1.0;

	int roomColours = 0;
	PositionType positionType = PositionType::CANON;
	bool visibleLayers[] = { true, true, true };
	bool visibleDevItems = false;

	Region region;

	int selectingState = 0;
	Room *roomPossibleSelect = nullptr;
	std::set<Room*> selectedRooms;

	std::vector<std::string> fails;

	bool denPopupLineageExtended = false;
	bool denPopupTagsExtended = false;

	Vector2 globalMouse;
	double lineSize = 0.0;


	bool showAnniversary = false;
}