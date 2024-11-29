#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../gl.h"

#include "../Utils.hpp"
#include "../math/Vector.hpp"

#include "Room.hpp"

class Connection {
	public:
		Connection(Room *roomA, unsigned int connectionA, Room *roomB, unsigned int connectionB)
		: roomA(roomA),
		  roomB(roomB),
		  connectionA(connectionA),
		  connectionB(connectionB) {
		}

		void draw(Vector2 mousePosition, double lineSize) {
			Vector2 pointA = roomA->getConnection(connectionA);
			Vector2 pointB = roomB->getConnection(connectionB);

			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

			if (distance(mousePosition) < 1.0 / lineSize) {
				glColor3f(0.0f, 1.0f, 1.0f);
				drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 32.0 / lineSize);
			}

			glColor3f(1.0f, 1.0f, 0.0f);
			drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 16.0 / lineSize);

			glDisable(GL_LINE_SMOOTH);
		}

		bool collides(Vector2 vector) {
			Vector2 pointA = roomA->getConnection(connectionA);
			Vector2 pointB = roomB->getConnection(connectionB);

			double length = pointA.distanceTo(pointB);
			double d1 = pointA.distanceTo(vector);
			double d2 = pointB.distanceTo(vector);

			double buffer = 0.001;

			if (d1 + d2 >= length - buffer && d1 + d2 <= length + buffer) {
				return true;
			}

			return false;
		}

		double distance(Vector2 vector) {
			Vector2 pointA = roomA->getConnection(connectionA);
			Vector2 pointB = roomB->getConnection(connectionB);

			Vector2 AB = pointB - pointA;
			Vector2 AP = vector - pointA;
			double lengthSqrAB = AB.x * AB.x + AB.y * AB.y;
			double t = (AP.x * AB.x + AP.y * AB.y) / lengthSqrAB;

			if (t < 0.0) t = 0.0;
			if (t > 1.0) t = 1.0;

			Vector2 closestPoint = pointA + t * AB;

			return closestPoint.distanceTo(vector);
		}

		void RoomA(Room *roomA) { this->roomA = roomA; }

		void RoomB(Room *roomB) { this->roomB = roomB; }

		void ConnectionA(unsigned int connectionA) { this->connectionA = connectionA; }

		void ConnectionB(unsigned int connectionB) { this->connectionB = connectionB; }

		Room *RoomA() { return roomA; }

		Room *RoomB() { return roomB; }

		unsigned int ConnectionA() { return connectionA; }

		unsigned int ConnectionB() { return connectionB; }

	private:
		Room *roomA;
		Room *roomB;

		unsigned int connectionA;
		unsigned int connectionB;
};

#endif