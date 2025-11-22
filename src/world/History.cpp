#include "History.hpp"

void History::undo() {
	if (undos.empty()) return;

	undos.top()->undo();
	redos.push(undos.top());
	undos.pop();

	Logger::info("UNDO: ", undos.size(), " - ", redos.size());
}

void History::redo() {
	if (redos.empty()) return;

	redos.top()->redo();
	undos.push(redos.top());
	redos.pop();

	Logger::info("REDO: ", undos.size(), " - ", redos.size());
}

void History::change(Change *change) {
	change->redo();

	while (!redos.empty()) {
		redos.top()->destroy();
		delete redos.top();
		redos.pop();
	}

	undos.push(change);
	Logger::info("CHANGE: ", undos.size(), " - ", redos.size());
}

void History::clear() {
	while (!redos.empty()) {
		delete redos.top();
		redos.pop();
	}
	while (!undos.empty()) {
		delete undos.top();
		undos.pop();
	}
}