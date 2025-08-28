#pragma once

class Connection;

#include "../gl.h"

#include "../Utils.hpp"
#include "../math/Vector.hpp"
#include "../Settings.hpp"

#include "TimelineType.hpp"
#include "Room.hpp"

class Connection {
	public:
		Connection(Room *roomA, unsigned int connectionA, Room *roomB, unsigned int connectionB);

		void draw(Vector2 mousePosition, double lineSize);

		bool hovered(Vector2 mouse, double lineSize);

		bool collides(Vector2 vector);

		bool allowsTimeline(std::string timeline);

		Room *roomA;
		Room *roomB;

		unsigned int connectionA;
		unsigned int connectionB;

		std::set<std::string> timelines;
		ConnectionTimelineType timelineType;

	private:
		int segments;
		double directionStrength;
};