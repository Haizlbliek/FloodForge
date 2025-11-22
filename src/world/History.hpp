#pragma once

#include <stack>
#include "Room.hpp"
#include "Connection.hpp"
#include "Den.hpp"
#include "Globals.hpp"

class Change {
	public:
		virtual void undo() = 0;
		virtual void redo() = 0;
		virtual void destroy() {};
};

class RoomAndConnectionChange : public Change {
	public:
		RoomAndConnectionChange(bool adding) {
			this->adding = adding;
		}

		virtual void destroy() override {
			if (!this->adding) return;

			for (Room *room : rooms) {
				delete room;
			}
			for (Connection *connection : connections) {
				delete connection;
			}
		}

		void addRoom(Room *room) {
			rooms.push_back(room);
		}

		void addConnection(Connection *connection) {
			connections.push_back(connection);
		}

		void add() {
			for (Room *room : rooms) {
				if (room->isOffscreen()) return;

				EditorState::rooms.push_back(room);
				// TODO: Add into right index
			}

			for (Connection * connection : connections) {
				// TODO: Add into right index
				EditorState::connections.push_back(connection);

				connection->roomA->connect(connection);
				connection->roomB->connect(connection);
			}
		}

		void remove() {
			for (Connection *connection : connections) {
				EditorState::connections.erase(std::remove(EditorState::connections.begin(), EditorState::connections.end(), connection), EditorState::connections.end());

				connection->roomA->disconnect(connection);
				connection->roomB->disconnect(connection);
			}

			for (Room *room : rooms) {
				if (room->isOffscreen()) return;

				EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), room), EditorState::rooms.end());
			}
		}

		virtual void undo() override {
			if (this->adding) {
				remove();
			} else {
				add();
			}
		}

		virtual void redo() override {
			if (this->adding) {
				add();
			} else {
				remove();
			}
		}

	private:
		bool adding;
		std::vector<Room *> rooms;
		std::vector<Connection *> connections;
};

class OverrideSubregionColorChange : public Change {
	public:
		enum class Type {
			Change,
			Add,
			Delete
		};

		OverrideSubregionColorChange(int index) {
			this->index = index;
			this->undoValue = EditorState::region.overrideSubregionColors[this->index];
			this->type = Type::Delete;
		}

		OverrideSubregionColorChange(int index, Color &to) {
			this->index = index;
			this->redoValue = to;
			this->type = Type::Add;
		}

		OverrideSubregionColorChange(int index, Color &from, Color &to) {
			this->index = index;
			this->redoValue = to;
			this->undoValue = from;
			this->type = Type::Change;
		}

		virtual void undo() override {
			if (this->type == Type::Change || this->type == Type::Delete) {
				EditorState::region.overrideSubregionColors[this->index] = this->undoValue;
			}
			else if (this->type == Type::Add) {
				EditorState::region.overrideSubregionColors.erase(this->index);
			}
		}

		virtual void redo() override {
			if (this->type == Type::Change || this->type == Type::Add) {
				EditorState::region.overrideSubregionColors[this->index] = this->redoValue;
			}
			else if (this->type == Type::Delete) {
				EditorState::region.overrideSubregionColors.erase(this->index);
			}
		}

	protected:
		Type type;
		int index;
		Color undoValue;
		Color redoValue;
};

class MoveToBackChange : public Change {
	public:
		MoveToBackChange(Room *room) {
			this->room = room;
			this->initialOffset = std::distance(EditorState::rooms.begin(), std::find(EditorState::rooms.begin(), EditorState::rooms.end(), room));
		}

		virtual void undo() {
			EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), this->room), EditorState::rooms.end());
			EditorState::rooms.insert(EditorState::rooms.begin() + initialOffset, this->room);
		}

		virtual void redo() {
			EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), this->room), EditorState::rooms.end());
			EditorState::rooms.insert(EditorState::rooms.begin(), this->room);
		}

	protected:
		Room *room;
		int initialOffset;
};

class MultipleRoomChange : public Change {
	protected:
		virtual void addRoom(Room *room) {
			rooms.push_back(room);
		}

		std::vector<Room *> rooms;
};

class CreatureDataChange : public Change {
	public:
		CreatureDataChange(DenCreature *creature, std::string type, int count, std::string tag, double data) {
			this->creature = creature;
			this->undoType = creature->type;
			this->undoCount = creature->count;
			this->undoTag = creature->tag;
			this->undoData = creature->data;
			this->redoType = type;
			this->redoCount = count;
			this->redoTag = tag;
			this->redoData = data;
		}

		virtual void undo() {
			creature->type = this->undoType;
			creature->count = this->undoCount;
			creature->tag = this->undoTag;
			creature->data = this->undoData;
		}

		virtual void redo() {
			creature->type = this->redoType;
			creature->count = this->redoCount;
			creature->tag = this->redoTag;
			creature->data = this->redoData;
		}

		DenCreature *creature;
		std::string undoType;
		std::string redoType;
		int undoCount;
		int redoCount;
		std::string undoTag;
		std::string redoTag;
		double undoData;
		double redoData;
};

class CreatureLineageChange : public Change {
	public:
		enum class Type {
			Add,
			Chance,
		};

		CreatureLineageChange(DenCreature *creature, double chance) {
			this->type = Type::Chance;
			this->creature = creature;
			this->undoChance = this->creature->lineageChance;
			this->redoChance = chance;
		}

		CreatureLineageChange(DenCreature *creature) {
			this->type = Type::Add;
			this->creature = creature;
			this->creatureAdd = new DenCreature("", 0, "", 0.0);
		}

		virtual void undo() override {
			if (this->type == Type::Chance) {
				this->creature->lineageChance = this->undoChance;
			}
			else if (this->type == Type::Add) {
				this->creature->lineageTo = nullptr;
			}
		}

		virtual void redo() override {
			if (this->type == Type::Chance) {
				this->creature->lineageChance = this->redoChance;
			}
			else if (this->type == Type::Add) {
				this->creature->lineageTo = this->creatureAdd;
			}
		}

	protected:
		Type type;
		DenCreature *creature;
		DenCreature *creatureAdd;
		double undoChance;
		double redoChance;
};

class LineageChange : public Change {
	public:
		enum class Type {
			Create,
			Delete,
		};

		LineageChange(Den *den) : lineage(new DenLineage("", 0, "", 0.0)) {
			this->den = den;
			this->index = -1;
			this->type = Type::Create;
		}

		LineageChange(Den *den, int index) : lineage(*std::next(den->creatures.begin(), index)) {
			this->den = den;
			this->index = index;
			this->type = Type::Delete;
		}

		virtual void undo() override {
			if (this->type == Type::Delete) {
				den->creatures.insert(std::next(den->creatures.begin(), this->index), this->lineage);
			}
			else if (this->type == Type::Create) {
				den->creatures.pop_back();
			}
		}

		virtual void redo() override {
			if (this->type == Type::Delete) {
				den->creatures.erase(std::next(den->creatures.begin(), this->index));
			}
			else if (this->type == Type::Create) {
				den->creatures.push_back(this->lineage);
			}
		}

	protected:
		Den *den;
		DenLineage *lineage;
		int index;
		Type type;
};

class CreatureDeleteChange : public Change {
	public:
		CreatureDeleteChange(DenCreature *creature, DenCreature *lastCreature) {
			this->creature = creature;
			this->lastCreature = lastCreature;
			if (this->lastCreature == nullptr) {
				this->previous = this->creature->lineageTo;
			} else {
				this->previous = this->lastCreature->lineageTo;
			}
		}

		virtual void undo() override {
			if (this->lastCreature == nullptr) {
				std::swap(this->creature->type, this->previous->type);
				std::swap(this->creature->tag, this->previous->tag);
				std::swap(this->creature->count, this->previous->count);
				std::swap(this->creature->data, this->previous->data);
				this->creature->lineageTo = this->previous;
			} else {
				this->lastCreature->lineageTo = this->previous;
			}
		}

		virtual void redo() override {
			if (this->lastCreature == nullptr) {
				std::swap(creature->type, this->previous->type);
				std::swap(creature->tag, this->previous->tag);
				std::swap(creature->count, this->previous->count);
				std::swap(creature->data, this->previous->data);
				this->creature->lineageTo = this->previous->lineageTo;
			} else {
				this->lastCreature->lineageTo = this->creature->lineageTo;
			}
		}

	protected:
		DenCreature *previous;
		DenCreature *lastCreature;
		DenCreature *creature;
};

class TagChange : public MultipleRoomChange {
	public:
		virtual void addRoom(Room *room, std::vector<std::string> redoValue) {
			MultipleRoomChange::addRoom(room);

			this->undoValues.push_back(room->tags);
			this->redoValues.push_back(redoValue);
		}

		void set(Room *room, std::vector<std::string> value) {
			room->tags = value;
		}

		virtual void undo() override {
			for (int i = 0; i < this->rooms.size(); i++) {
				set(this->rooms[i], this->undoValues[i]);
			}
		}

		virtual void redo() override {
			for (int i = 0; i < this->rooms.size(); i++) {
				set(this->rooms[i], this->redoValues[i]);
			}
		}


	protected:
		std::vector<std::vector<std::string>> undoValues;
		std::vector<std::vector<std::string>> redoValues;
};

class TimelineTypeChange : public MultipleRoomChange {
	public:
		virtual void addRoom(Room *room, TimelineType redoValue) {
			MultipleRoomChange::addRoom(room);

			this->undoValues.push_back(room->timelineType);
			this->redoValue = redoValue;
		}

		virtual void addConnection(Connection *connection, TimelineType redoValue) {
			this->connection = connection;
			this->redoValue = redoValue;
			this->undoValue = this->connection->timelineType;
		}

		virtual void addLineage(DenLineage *lineage, TimelineType redoValue) {
			this->lineage = lineage;
			this->redoValue = redoValue;
			this->undoValue = this->lineage->timelineType;
		}

		virtual void undo() override {
			if (this->connection != nullptr) {
				this->connection->timelineType = this->undoValue;
			}
			else if (this->lineage != nullptr) {
				this->lineage->timelineType = this->undoValue;
			}
			else {
				for (int i = 0; i < this->rooms.size(); i++) {
					this->rooms[i]->timelineType = this->undoValues[i];
				}
			}
		}

		virtual void redo() override {
			if (this->connection != nullptr) {
				this->connection->timelineType = this->redoValue;
			}
			else if (this->lineage != nullptr) {
				this->lineage->timelineType = this->redoValue;
			}
			else {
				for (int i = 0; i < this->rooms.size(); i++) {
					this->rooms[i]->timelineType = this->redoValue;
				}
			}
		}


	protected:
		Connection *connection = nullptr;
		DenLineage *lineage = nullptr;

		std::vector<TimelineType> undoValues;
		TimelineType undoValue;
		TimelineType redoValue;
};

class TimelineChange : public MultipleRoomChange {
	public:
		TimelineChange(bool add, std::string timeline) {
			this->add = add;
			this->timeline = timeline;
		}

		virtual void addRoom(Room *room) {
			MultipleRoomChange::addRoom(room);
		}

		virtual void addConnection(Connection *connection) {
			this->connection = connection;
		}

		virtual void addLineage(DenLineage *lineage) {
			this->lineage = lineage;
		}

		void erase() {
			if (this->connection != nullptr) {
				this->connection->timelines.erase(this->timeline);
			}
			else if (this->lineage != nullptr) {
				this->lineage->timelines.erase(this->timeline);
			}
			else {
				for (int i = 0; i < this->rooms.size(); i++) {
					this->rooms[i]->timelines.erase(this->timeline);
				}
			}
		}

		void insert() {
			if (this->connection != nullptr) {
				this->connection->timelines.insert(this->timeline);
			}
			else if (this->lineage != nullptr) {
				this->lineage->timelines.insert(this->timeline);
			}
			else {
				for (int i = 0; i < this->rooms.size(); i++) {
					this->rooms[i]->timelines.insert(this->timeline);
				}
			}
		}


		virtual void undo() override {
			if (this->add) {
				erase();
			} else {
				insert();
			}
		}

		virtual void redo() override {
			if (this->add) {
				insert();
			} else {
				erase();
			}
		}


	protected:
		bool add;
		std::string timeline;

		Connection *connection = nullptr;
		DenLineage *lineage = nullptr;
};

class AttractivenessChange : public MultipleRoomChange {
	public:
		AttractivenessChange(RoomAttractiveness attr, std::string creature) {
			this->attr = attr;
			this->creature = creature;

			std::unordered_map<std::string, RoomAttractiveness>::iterator index = EditorState::region.defaultAttractiveness.find(creature);
			bool has = index != EditorState::region.defaultAttractiveness.end();
			this->undoAttr = {
				has,
				has ? (*index).second : RoomAttractiveness::DEFAULT
			};
		}

		virtual void addRoom(Room *room) {
			MultipleRoomChange::addRoom(room);
			std::unordered_map<std::string, RoomAttractiveness>::iterator index = room->data.attractiveness.find(creature);
			bool has = index != room->data.attractiveness.end();
			this->undoAttrs.push_back({
				has,
				has ? (*index).second : RoomAttractiveness::DEFAULT
			});
		}

		void erase() {
			if (this->rooms.size() == 0) {
				EditorState::region.defaultAttractiveness.erase(this->creature);
			}
			else {
				for (Room *room : this->rooms) {
					room->data.attractiveness.erase(this->creature);
				}
			}
		}

		virtual void undo() override {
			if (this->rooms.size() == 0) {
				if (this->undoAttr.first) {
					EditorState::region.defaultAttractiveness[this->creature] = this->undoAttr.second;
				}
				else {
					EditorState::region.defaultAttractiveness.erase(this->creature);
				}
			}
			else {
				for (int i = 0; i < this->rooms.size(); i++) {
					if (this->undoAttrs[i].first) {
						this->rooms[i]->data.attractiveness[this->creature] = this->undoAttrs[i].second;
					}
					else {
						this->rooms[i]->data.attractiveness.erase(this->creature);
					}
				}
			}
		}

		virtual void redo() override {
			if (this->attr == RoomAttractiveness::DEFAULT) {
				erase();
			}
			else {
				if (this->rooms.size() == 0) {
					EditorState::region.defaultAttractiveness[this->creature] = this->attr;
				}
				else {
					for (Room *room : this->rooms) {
						room->data.attractiveness[this->creature] = this->attr;
					}
				}
			}
		}

	private:
		RoomAttractiveness attr;
		std::string creature;

		std::vector<std::pair<bool, RoomAttractiveness>> undoAttrs;
		std::pair<bool, RoomAttractiveness> undoAttr;
};

class MoveChange : public MultipleRoomChange {
	public:
		virtual void addRoom(Room *room, Vector2 devOffset, Vector2 canonOffset) {
			MultipleRoomChange::addRoom(room);
			devOffsets.push_back(devOffset);
			canonOffsets.push_back(canonOffset);
		}

		void merge(MoveChange *other) {
			for (int i = 0; i < other->rooms.size(); i++) {
				int k = -1;
				for (int j = 0; j < this->rooms.size(); j++) {
					if (this->rooms[j] == other->rooms[i]) {
						k = j;
						break;
					}
				}

				if (k == -1) {
					this->addRoom(other->rooms[i], other->devOffsets[i], other->canonOffsets[i]);
				} else {
					this->devOffsets[k] += other->devOffsets[i];
					this->canonOffsets[k] += other->canonOffsets[i];
				}
			}
		}

		void move(double multiplier) {
			for (int i = 0; i < this->rooms.size(); i++) {
				this->rooms[i]->devPosition += devOffsets[i] * multiplier;
				this->rooms[i]->canonPosition += canonOffsets[i] * multiplier;
			}
		}

		virtual void undo() override {
			move(-1.0);
		}

		virtual void redo() override {
			move(1.0);
		}

	protected:
		std::vector<Vector2> devOffsets;
		std::vector<Vector2> canonOffsets;
};

template<typename T>
class GeneralRoomChange : public MultipleRoomChange {
	public:
		enum class Type {
			Merge,
			Layer,
			Hidden,
			Subregion
		};

		GeneralRoomChange(Type type) {
			this->type = type;
		}

		virtual void addRoom(Room *room, T redoValue) {
			MultipleRoomChange::addRoom(room);

			if (this->type == Type::Merge) {
				this->undoValues.push_back(room->data.merge);
			}
			else if (this->type == Type::Layer) {
				this->undoValues.push_back(room->layer);
			}
			else if (this->type == Type::Hidden) {
				this->undoValues.push_back(room->data.hidden);
			}
			else if (this->type == Type::Subregion) {
				this->undoValues.push_back(room->subregion);
			}
			this->redoValues.push_back(redoValue);
		}

		void set(Room *room, T value) {
			if (this->type == Type::Merge) {
				room->data.merge = value;
			}
			else if (this->type == Type::Layer) {
				room->layer = value;
			}
			else if (this->type == Type::Hidden) {
				room->data.hidden = value;
			}
			else if (this->type == Type::Subregion) {
				room->subregion = value;
			}
		}

		virtual void undo() override {
			for (int i = 0; i < this->rooms.size(); i++) {
				set(this->rooms[i], this->undoValues[i]);
			}
		}

		virtual void redo() override {
			for (int i = 0; i < this->rooms.size(); i++) {
				set(this->rooms[i], this->redoValues[i]);
			}
		}

	protected:
		std::vector<T> undoValues;
		std::vector<T> redoValues;
		Type type;
};

class SubregionChange : public GeneralRoomChange<int> {
	public:
		enum class Type {
			Add,
			Delete,
			Rename
		};

		SubregionChange(std::string subregionName) : GeneralRoomChange(GeneralRoomChange::Type::Subregion) {
			this->type = Type::Add;
			this->subregionName = subregionName;
		}

		SubregionChange(int index, std::string subregionName) : GeneralRoomChange(GeneralRoomChange::Type::Subregion) {
			this->type = Type::Rename;
			this->subregionName = subregionName;
			this->subregionIndex = index;
			this->previousSubregionName = EditorState::subregions[this->subregionIndex];
		}

		SubregionChange(int index) : GeneralRoomChange(GeneralRoomChange::Type::Subregion) {
			this->type = Type::Delete;
			this->subregionIndex = index;
			this->subregionName = EditorState::subregions[this->subregionIndex];
		}

		virtual void undo() override {
			GeneralRoomChange::undo();

			if (this->type == Type::Add) {
				EditorState::subregions.pop_back();
			}
			else if (this->type == Type::Rename) {
				EditorState::subregions[this->subregionIndex] = this->previousSubregionName;
			}
			else if (this->type == Type::Delete) {
				EditorState::subregions.insert(EditorState::subregions.begin() + this->subregionIndex, this->subregionName);
			}
		}

		virtual void redo() override {
			GeneralRoomChange::redo();
			if (this->type == Type::Add) {
				EditorState::subregions.push_back(subregionName);
			}
			else if (this->type == Type::Rename) {
				EditorState::subregions[this->subregionIndex] = this->subregionName;
			}
			else if (this->type == Type::Delete) {
				EditorState::subregions.erase(EditorState::subregions.begin() + this->subregionIndex);
			}
		}

	protected:
		std::string previousSubregionName;
		std::string subregionName;
		int subregionIndex;
		Type type;
};

class History {
	public:
		void undo();
		void redo();
		void change(Change *change);
		void clear();

		Change *lastChange() {
			if (undos.empty()) return nullptr;

			return undos.top();
		}

	protected:
		std::stack<Change*> undos;
		std::stack<Change*> redos;
};