#pragma once

class Room;

#include "../gl.h"

#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <stack>
#include <set>
#include <algorithm>

#include "../Texture.hpp"
#include "../Theme.hpp"
#include "../Settings.hpp"
#include "../math/Vector.hpp"
#include "../math/Matrix4.hpp"
#include "../font/Fonts.hpp"
#include "../Logger.hpp"

#include "TimelineType.hpp"
#include "Connection.hpp"
#include "ExtraRoomData.hpp"
#include "Shaders.hpp"
#include "Den.hpp"

#define CONNECTION_TYPE_NONE 0
#define CONNECTION_TYPE_EXIT 1
#define CONNECTION_TYPE_DEN  2
#define CONNECTION_TYPE_MOLE 3
#define CONNECTION_TYPE_SCAV 4

enum class ShortcutType {
	ROOM,
	DEN
};

struct Vertex {
	Vertex(float x, float y, Colour col) {
		this->x = x;
		this->y = y;
		this->r = col.r;
		this->g = col.g;
		this->b = col.b;
		this->a = col.a;
	}

	Vertex(float x, float y, float r, float g, float b, float a = 1.0f) {
		this->x = x;
		this->y = y;
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	float x, y;
	float r, g, b, a;
};

namespace RoomHelpers {
	extern Colour RoomAir;
	extern Colour RoomSolid;
	extern Colour RoomPole;
	extern Colour RoomPlatform;
	extern Colour RoomShortcutEnterance;
	extern Colour RoomShortcutDot;
	extern Colour RoomShortcutRoom;
	extern Colour RoomShortcutDen;
	extern Colour RoomConnection;
	extern Colour RoomConnectionHover;
	
	void loadColours();

	void drawTexture(GLuint texture, double rectX, double rectY, double scale);
};

class Room {
	public:
		virtual bool isOffscreen() { return false; }

		Room(std::filesystem::path path, std::string name);

		virtual ~Room();

		virtual void drawBlack(Vector2 mousePosition, int positionType);
		virtual void draw(Vector2 mousePosition, int positionType);

		void drawDen(Den &den, double x, double y, bool hovered);

		bool inside(Vector2 otherPosition);

		bool intersects(Vector2 corner0, Vector2 corner1);

		int getTile(int x, int y) const;

		bool tileIsShortcut(int x, int y) const;

		const std::vector<Vector2> ShortcutEntranceOffsetPositions() const;

		int getShortcutEntranceId(const Vector2i &searchPosition) const;

		const Vector2 getRoomEntranceOffsetPosition(unsigned int connectionId) const;

		int getRoomEntranceId(const Vector2i &searchPosition) const;

		const Vector2i getRoomEntrancePosition(unsigned int connectionId) const;

		Vector2 getRoomEntranceDirectionVector(unsigned int connectionId) const;

		Direction getRoomEntranceDirection(unsigned int connectionId) const;
		
		bool canConnect(unsigned int connectionId);

		void connect(Connection *connection);

		void disconnect(Connection *connection);

		int RoomEntranceCount() const;

		const std::vector<std::pair<Vector2i, ShortcutType>> ShortcutConnections() const;

		const std::vector<Vector2i> RoomEntrances() const;

		const int DenId(Vector2i coord) const;

		bool CreatureDenExists(int id);

		bool CreatureDen01Exists(int id);

		Den &CreatureDen(int id);

		Den &CreatureDen01(int id);

		const int DenCount() const;

		const std::vector<Vector2i> DenEntrances() const;

		bool InBounds(int x, int y) const;

		const int Width() const;
		const int Height() const;

		void SetTag(const std::string newTag);
		void ToggleTag(const std::string newTag);
		const std::vector<std::string> Tags() const;

		int cameras;
		std::string roomName = "";
		int water = 0;
		bool valid;
		int width;
		int height;
		int *geometry;

		Vector2 canonPosition;
		Vector2 devPosition;
		int layer = 0;
		int subregion = 0;
		std::vector<Connection*> connections;
		ExtraRoomData data;

		int hoveredDen = -1;

		std::set<std::string> timelines;
		RoomTimelineType timelineType;

		std::vector<Den> dens;

		void moveBoth();

		Vector2 &currentPosition();

		std::filesystem::path path;
		void regeneateGeometry();

	protected:
		Room() {}

		const Vector2 staticCurrentPosition() const;

		std::vector<uint8_t> parseStringToUint8Vector(const std::string& input);

		void ensureConnections();

		void loadGeometry();
		
		void checkImages();

		void generateVBO();
		void addQuad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		size_t cur_index;
		GLuint vbo[2]; // first: vertices, second: indices
		GLuint vao;

		std::vector<std::string> tags;

		std::vector<Vector2i> roomEntrances;
		std::vector<Vector2i> denEntrances;
		std::vector<std::pair<Vector2i, ShortcutType>> shortcutEntrances;
};