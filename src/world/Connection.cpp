#include "Connection.hpp"

Connection::Connection(Room *roomA, unsigned int connectionA, Room *roomB, unsigned int connectionB) : roomA(roomA), roomB(roomB), connectionA(connectionA), connectionB(connectionB) {
	segments = 10;
	directionStrength = 10.0;
}

void Connection::draw(Vector2 mousePosition, double lineSize) {
	if (hovered(mousePosition, lineSize)) {
		Draw::color(RoomHelpers::RoomConnectionHover);
	} else {
		Draw::color(RoomHelpers::RoomConnection);
	}

	Vector2 pointA = roomA->getRoomEntranceOffsetPosition(connectionA);
	Vector2 pointB = roomB->getRoomEntranceOffsetPosition(connectionB);

	segments = std::clamp((int) (pointA.distanceTo(pointB) / 2.0), 4, 100);
	directionStrength = pointA.distanceTo(pointB);
	if (directionStrength > 300.0) directionStrength = (directionStrength - 300.0) * 0.5 + 300.0;

	if (Settings::getSetting<int>(Settings::Setting::ConnectionType) == 0) {
		drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 16.0 / lineSize);
	} else {
		Vector2 directionA = roomA->getRoomEntranceDirectionVector(connectionA);
		Vector2 directionB = roomB->getRoomEntranceDirectionVector(connectionB);

		if (directionA.x == -directionB.x || directionA.y == -directionB.y) {
			directionStrength *= 0.3333;
		} else {
			directionStrength *= 0.6666;
		}

		directionA *= directionStrength;
		directionB *= directionStrength;

		Vector2 lastPoint = bezierCubic(0.0, pointA, pointA + directionA, pointB + directionB, pointB);
		for (double t = 1.0 / segments; t <= 1.01; t += 1.0 / segments) {
			Vector2 point = bezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);

			drawLine(lastPoint.x, lastPoint.y, point.x, point.y, 16.0 / lineSize);

			lastPoint = point;
		}
	}
}

bool Connection::hovered(Vector2 mouse, double lineSize) {
	Vector2 pointA = roomA->getRoomEntranceOffsetPosition(connectionA);
	Vector2 pointB = roomB->getRoomEntranceOffsetPosition(connectionB);

	if (Settings::getSetting<int>(Settings::Setting::ConnectionType) == 0) {
		return lineDistance(mouse, pointA, pointB) < 1.0 / lineSize;
	} else {
		Vector2 directionA = roomA->getRoomEntranceDirectionVector(connectionA) * directionStrength;
		Vector2 directionB = roomB->getRoomEntranceDirectionVector(connectionB) * directionStrength;

		Vector2 lastPoint = bezierCubic(0.0, pointA, pointA + directionA, pointB + directionB, pointB);
		for (double t = 1.0 / segments; t <= 1.01; t += 1.0 / segments) {
			Vector2 point = bezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);

			if (lineDistance(mouse, lastPoint, point) < 1.0 / lineSize) return true;

			lastPoint = point;
		}

		return false;
	}
}

bool Connection::collides(Vector2 vector) {
	Vector2 pointA = roomA->getRoomEntranceOffsetPosition(connectionA);
	Vector2 pointB = roomB->getRoomEntranceOffsetPosition(connectionB);

	double length = pointA.distanceTo(pointB);
	double d1 = pointA.distanceTo(vector);
	double d2 = pointB.distanceTo(vector);

	double buffer = 0.001;

	if (d1 + d2 >= length - buffer && d1 + d2 <= length + buffer) {
		return true;
	}

	return false;
}
