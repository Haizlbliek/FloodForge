#pragma once

#include <vector>

#include "../../math/Vector.hpp"

class Object;

class Node {
	public:
		Node(Vector2 pos, Node *parent, Object *object);

		Vector2 pos;
		Node *parent;
		Object *object;
	
		Vector2 position() const;
};

class Object {
	public:
		Object();
		~Object();

		void addNode(Vector2 pos, Node *parent = nullptr);

		std::vector<Node *> nodes;
	
		virtual void draw(Vector2 offset) const;
};

class TerrainHandleObject : public Object {
	public:
		TerrainHandleObject();
	
		void draw(Vector2 offset) const override;

		const Vector2 Left() const;
		const Vector2 Middle() const;
		const Vector2 Right() const;
};

class MudPitObject : public Object {
	public:
		MudPitObject();

		void draw(Vector2 offset) const override;
};

class AirPocketObject : public Object {
	public:
		AirPocketObject();

		void draw(Vector2 offset) const override;
};