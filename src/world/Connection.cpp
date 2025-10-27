#include "Connection.hpp"
#include "ConditionalTimelineTextures.hpp"

Connection::Connection(Room *roomA, unsigned int connectionA, Room *roomB, unsigned int connectionB) : roomA(roomA), roomB(roomB), connectionA(connectionA), connectionB(connectionB) {
	segments = 10;
	directionStrength = 10.0;
	timelineType = TimelineType::ALL;
}

void drawTexturedRect(GLuint texture, Rect rect) {
	Draw::color(1.0, 1.0, 1.0);
	glEnable(GL_BLEND);
	Draw::useTexture(texture);
	Draw::begin(Draw::QUADS);

	int w, h;
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindTexture(GL_TEXTURE_2D, 0);

	float ratio = (float(w) / float(h) + 1.0) * 0.5;
	float uvx = 1.0 / ratio;
	float uvy = ratio;
	if (uvx < 1.0) {
		uvy /= uvx;
		uvx = 1.0;
	}
	if (uvy < 1.0) {
		uvx /= uvy;
		uvy = 1.0;
	}
	uvx *= 0.5;
	uvy *= 0.5;
	Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(rect.x0, rect.y0);
	Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(rect.x1, rect.y0);
	Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(rect.x1, rect.y1);
	Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(rect.x0, rect.y1);
	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);
}

void Connection::draw(Vector2 mousePosition, double lineSize) {
	if (hovered(mousePosition, lineSize)) {
		Draw::color(RoomHelpers::RoomConnectionHover);
	} else {
		Draw::color(RoomHelpers::RoomConnection);
	}
	if (Settings::getSetting<double>(Settings::Setting::ConnectionOpacity) < 0.999f) {
		Draw::alpha(Settings::getSetting<double>(Settings::Setting::ConnectionOpacity));
		glEnable(GL_BLEND);
	}

	Vector2 pointA = roomA->getRoomEntranceOffsetPosition(connectionA);
	Vector2 pointB = roomB->getRoomEntranceOffsetPosition(connectionB);

	segments = std::clamp((int) (pointA.distanceTo(pointB) / 2.0), 4, 100);
	directionStrength = pointA.distanceTo(pointB);
	if (directionStrength > 300.0) directionStrength = (directionStrength - 300.0) * 0.5 + 300.0;

	Vector2 center;

	if (Settings::getSetting<int>(Settings::Setting::ConnectionType) == 0) {
		drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 16.0 / lineSize);
		center = Vector2((pointA.x + pointB.x) * 0.5, (pointA.y + pointB.y) * 0.5);
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

			drawLine(lastPoint.x, lastPoint.y, point.x, point.y, 16.0 / lineSize, LINE_NONE);

			lastPoint = point;
		}

		center = bezierCubic(0.5, pointA, pointA + directionA, pointB + directionB, pointB);
	}

	glDisable(GL_BLEND);

	if (timelines.size() == 0 || timelineType == TimelineType::ALL) return;

	if (timelineType == TimelineType::EXCEPT) {
		Draw::color(1.0, 0.0, 0.0);
		double xSize = 2.25 / lineSize;
		drawLine(center.x - xSize, center.y - xSize, center.x + xSize, center.y + xSize, 16.0 / lineSize);
		drawLine(center.x + xSize, center.y - xSize, center.x - xSize, center.y + xSize, 16.0 / lineSize);
	}

	double size = 2.0 / lineSize;
	int count = this->timelines.size();
	int width = std::max((int) std::round(std::log2(count)), 1);
	int height = std::max((int) std::ceil(((double) count) / width), 1);

	int i = 0;
	auto iterator = this->timelines.begin();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (i >= this->timelines.size()) break;

			double ox = (width * -0.5 + x + 0.5) * size * 2.2;
			double oy = (height * -0.5 + y + 0.5) * size * 2.2;
			GLuint texture = ConditionalTimelineTextures::getTexture(*iterator);
			Rect rect = Rect::fromSize(center.x - size - ox, center.y - size - oy, size * 2.0, size * 2.0);
			drawTexturedRect(texture, rect);
			i++;
			iterator = std::next(iterator);
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

bool Connection::allowsTimeline(std::string timeline) {
	if (timelineType == TimelineType::ALL) return true;
	if (timelineType == TimelineType::ONLY) return timelines.find(timeline) != timelines.end();
	if (timelineType == TimelineType::EXCEPT) return timelines.find(timeline) == timelines.end();

	return false;
}