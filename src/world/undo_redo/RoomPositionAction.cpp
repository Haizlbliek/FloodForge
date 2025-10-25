#include "RoomPositionAction.hpp"
#include "UndoRedoManager.hpp"
#include "../Room.hpp"
#include "../../Logger.hpp"

namespace UndoRedo {
	RoomPositionAction::RoomPositionAction(
		const std::map<Room*, RoomPositionData>& before,
		const std::map<Room*, RoomPositionData>& after,
		RoomPositionActionType type
	) : beforeState(before), afterState(after), actionType(type) {}
	
	std::string RoomPositionAction::getDescription() const {
		switch (actionType) {
		case RoomPositionActionType::MoveRooms:
			return "Move Rooms";
		default:
			return "[Room Position Action]";
		}
	}

	void RoomPositionAction::undo() {
		for (const auto& pair : beforeState) {
			Room* room = pair.first;
			const RoomPositionData& data = pair.second;

			room->canonPosition = data.canonPosition;
			room->devPosition = data.devPosition;
		}
	}

	void RoomPositionAction::redo() {
		for (const auto& pair : afterState) {
			Room* room = pair.first;
			const RoomPositionData& data = pair.second;
			
			room->canonPosition = data.canonPosition;
			room->devPosition = data.devPosition;
		}
	}
	void pushRoomPositionSnapshot(
		const std::map<Room*, RoomPositionData>& beforePositions,
		const std::map<Room*, RoomPositionData>& afterPositions,
		RoomPositionActionType type
	) {
		auto action = std::make_shared<RoomPositionAction>(
			beforePositions, 
			afterPositions, 
			type
		);
		manager.pushAction(action);
	}

}
