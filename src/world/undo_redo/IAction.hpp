#pragma once

#include <string>

namespace UndoRedo {

	/**
	 * Abstract action interface
	 * All undoable actions must implement this interface
	 */
	class IAction {
	public:
		virtual ~IAction() = default;
		
		virtual void undo() = 0;
		virtual void redo() = 0;

		// TO DO: IMplement a merge option for consecutive similar actions, optimization purposes

		virtual std::string getDescription() const = 0;
		virtual std::string toString() const = 0;
	};

}
