#pragma once

#include "../../popup/Popups.hpp"
#include "../Connection.hpp"

class ConditionalPopup : public Popup {
	public:
		ConditionalPopup(Connection *connection);

		ConditionalPopup(std::set<Room*> rooms);

		ConditionalPopup(DenLineage *lineage);

		void draw();

		std::string PopupName() { return "ConditionalPopup"; }

	protected:
		ConditionalPopup();

		const TimelineType timelineType() const;
		void timelineType(TimelineType type);

		void drawButton(Rect rect, std::string text, TimelineType type);

		Connection *connection;
		std::set<Room*> rooms;
		DenLineage *lineage;

		double scroll;
};