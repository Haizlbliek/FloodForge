#pragma once

#include <map>
#include <string>
#include "IAction.hpp"
#include "../../math/Vector.hpp"
class Room; // Forward declaration

namespace UndoRedo {

	struct RoomPositionData {
		Vector2 canonPosition;
		Vector2 devPosition;
		
		RoomPositionData() : canonPosition(0, 0), devPosition(0, 0) {}
		RoomPositionData(Vector2 canon, Vector2 dev) 
			: canonPosition(canon), devPosition(dev) {}
	};
	enum class RoomPositionActionType {
		MoveRooms,
	};


	class RoomPositionAction : public IAction {
	private:
		std::map<Room*, RoomPositionData> beforeState;
		std::map<Room*, RoomPositionData> afterState;
		RoomPositionActionType actionType;

	public:
		RoomPositionAction(
			const std::map<Room*, RoomPositionData>& before,
			const std::map<Room*, RoomPositionData>& after,
			RoomPositionActionType type
		);

		void undo() override;
		void redo() override;
		std::string getDescription() const override;
		std::string toString() const override { return "RoomPosition"; }
	};

	void pushRoomPositionSnapshot(
		const std::map<Room*, RoomPositionData>& beforePositions,
		const std::map<Room*, RoomPositionData>& afterPositions,
		RoomPositionActionType type
	);

}
