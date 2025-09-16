#include "Node.hpp"

#include "../../Draw.hpp"
#include "../../Utils.hpp"
#include "../../Logger.hpp"

Node::Node(Vector2 pos, Node *parent, Object *object) {
	this->pos = pos;
	this->parent = parent;
	this->object = object;
}

Vector2 Node::position() const {
	if (parent == nullptr) return pos;

	return pos + parent->position();
}

void Object::addNode(Vector2 pos, Node *parent) {
	nodes.push_back(new Node(pos, parent, this));
}

Object::Object() {
	addNode(Vector2(0.0, 0.0));
}

Object::~Object() {
	for (Node *node : nodes) {
		delete node;
	}
	nodes.clear();
}

void Object::draw(Vector2 offset) const {
}

TerrainHandleObject::TerrainHandleObject() : Object() {
	addNode(Vector2(-40.0, 0.0), nodes[0]);
	addNode(Vector2(40.0, 0.0), nodes[0]);
}

void TerrainHandleObject::draw(Vector2 offset) const {
	Draw::color(0.5);
	drawLine(
		offset.x + nodes[0]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0,
		offset.x + nodes[0]->pos.x / 20.0 + nodes[1]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0 + nodes[1]->pos.y / 20.0
	);
	drawLine(
		offset.x + nodes[0]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0,
		offset.x + nodes[0]->pos.x / 20.0 + nodes[2]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0 + nodes[2]->pos.y / 20.0
	);
}

const Vector2 TerrainHandleObject::Left() const {
	return nodes[0]->pos + nodes[1]->pos;
}

const Vector2 TerrainHandleObject::Middle() const {
	return nodes[0]->pos;
}

const Vector2 TerrainHandleObject::Right() const {
	return nodes[0]->pos + nodes[2]->pos;
}



MudPitObject::MudPitObject() : Object() {
	addNode(Vector2(200.0, 30.0), nodes[0]);
}

void MudPitObject::draw(Vector2 offset) const {
	Draw::color(0.478, 0.282, 0.196);
	strokeRect(Rect::fromSize(
		offset.x + nodes[0]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0,
		nodes[1]->pos.x / 20.0,
		nodes[1]->pos.y / 20.0
	));
}



AirPocketObject::AirPocketObject() : Object() {
	addNode(Vector2(100.0, 200.0), nodes[0]);
	addNode(Vector2(0.0, 80.0), nodes[0]);
}

void AirPocketObject::draw(Vector2 offset) const {
	Draw::color(0.0, 0.0, 1.0);
	strokeRect(Rect::fromSize(
		offset.x + nodes[0]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0,
		nodes[1]->pos.x / 20.0,
		nodes[1]->pos.y / 20.0
	));
	Draw::color(0.0, 1.0, 1.0);
	drawLine(
		offset.x + nodes[0]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0 + nodes[2]->pos.y / 20.0,
		offset.x + nodes[0]->pos.x / 20.0 + nodes[1]->pos.x / 20.0,
		offset.y + nodes[0]->pos.y / 20.0 + nodes[2]->pos.y / 20.0
	);
	nodes[2]->pos.x = 0.0;
}