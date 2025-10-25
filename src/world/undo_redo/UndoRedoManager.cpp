#include "UndoRedoManager.hpp"
#include "../../Logger.hpp"

namespace UndoRedo {

	UndoRedoManager manager;
	constexpr size_t MAX_HISTORY_STACK = 5;
	bool debugMode = true;

	//Push a new action to the undo stack, clearing the redo stack in the process
	void UndoRedoManager::pushAction(std::shared_ptr<IAction> action) {
		// Clear redo stack when pushing a new action
		if (!redoStack.empty()) {
			clearRedoStack();
			if (debugMode) {
				   Logger::info(
					   std::string("[UndoRedo] Redo stack cleared (") + std::to_string(redoStack.size()) + " elements removed)"
				   );
			}
		}
		
		undoDeque.push_front(action);

		if (debugMode) {
			   Logger::info(
				   std::string("[UndoRedo] Action pushed - Type: '") + action->toString() +
				   "' | Description: '" + action->getDescription() +
				   "' | Undo stack size: " + std::to_string(undoDeque.size())
			   );
		}

		if (undoDeque.size() > MAX_HISTORY_STACK) {
			undoDeque.pop_back();
			if (debugMode) {
				   Logger::info("[UndoRedo] Undo stack limit exceeded, oldest action removed");
			}
		}
	}

	bool UndoRedoManager::undo() {
		if (undoDeque.empty()) {
			if (debugMode) {
				   Logger::warn("[UndoRedo] No actions to undo");
			}
			return false;
		}

		auto action = undoDeque.front();
		undoDeque.pop_front();

		action->undo();
		redoStack.push(action);
		
		if (debugMode) {
			   if (undoDeque.empty()) {
				   Logger::info(
					   std::string("[UndoRedo] Undo executed (last action) - Type: '") + action->toString() +
					   "' | Description: '" + action->getDescription() + "'"
				   );
			   } else {
				   Logger::info(
					   std::string("[UndoRedo] Undo executed - Type: '") + action->toString() +
					   "' | Description: '" + action->getDescription() +
					   "' | Undo stack: " + std::to_string(undoDeque.size()) +
					   " | Redo stack: " + std::to_string(redoStack.size())
				   );
			   }
		}
		
		return true;
	}

	bool UndoRedoManager::redo() {
		if (redoStack.empty()) {
			if (debugMode) {
				   Logger::warn("[UndoRedo] No actions to redo");
			}
			return false;
		}
		
		auto action = redoStack.top();
		redoStack.pop();
		
		action->redo();
		undoDeque.push_front(action);
		
		if (debugMode) {
			   if (redoStack.empty()) {
				   Logger::info(
					   std::string("[UndoRedo] Redo executed (last action) - Type: '") + action->toString() +
					   "' | Description: '" + action->getDescription() + "'"
				   );
			   } else {
				   Logger::info(
					   std::string("[UndoRedo] Redo executed - Type: '") + action->toString() +
					   "' | Description: '" + action->getDescription() +
					   "' | Undo stack: " + std::to_string(undoDeque.size()) +
					   " | Redo stack: " + std::to_string(redoStack.size())
				   );
			   }
		}
		
		return true;
	}

	void UndoRedoManager::clear() {
		undoDeque = {};
		redoStack = {};
		
		if (debugMode) {
			   Logger::info("[UndoRedo] All stacks cleared");
		}
	}

	void UndoRedoManager::clearRedoStack() {
		redoStack = {};
	}

	std::string UndoRedoManager::getLastActionDescription() const {
		if (undoDeque.empty()) {
			return "No actions";
		}
		return undoDeque.front()->getDescription();
	}

	void init() {
		manager.clear();
		if (debugMode) {
			   Logger::info("[UndoRedo] System initialized");
		}
	}

	void cleanup() {
		manager.clear();
		if (debugMode) {
			   Logger::info("[UndoRedo] System cleaned up");
		}
	}

	bool undo() {
		return manager.undo();
	}

	bool redo() {
		return manager.redo();
	}

	size_t getUndoStackSize() {
		return manager.getUndoStackSize();
	}

	size_t getRedoStackSize() {
		return manager.getRedoStackSize();
	}

	std::string getLastAction() {
		return manager.getLastActionDescription();
	}

}
