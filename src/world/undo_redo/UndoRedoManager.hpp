#pragma once

#include <stack>
#include <deque>
#include <string>
#include <memory>
#include "IAction.hpp"

namespace UndoRedo {

	/**
	 * Manages the undo/redo stacks for all actions
	 */
	class UndoRedoManager {
	private:
		std::deque<std::shared_ptr<IAction>> undoDeque;
		std::stack<std::shared_ptr<IAction>> redoStack;
		
	public:

		void pushAction(std::shared_ptr<IAction> action);	
		bool undo();
		bool redo();
		void clear();
		void clearRedoStack();
		int getUndoStackSize() const { return undoDeque.size(); }
		int getRedoStackSize() const { return redoStack.size(); }
		std::string getLastActionDescription() const;
	};

	extern UndoRedoManager manager;
	extern bool debugMode;
	void init();
	void cleanup();
	bool undo();
	bool redo();
	size_t getUndoStackSize();
	size_t getRedoStackSize();
	std::string getLastAction();
}
