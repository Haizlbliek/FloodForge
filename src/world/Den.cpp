#include "Den.hpp"

DenCreature::DenCreature(std::string type, int count, std::string tag, double data) {
	this->type = type;
	this->count = count;
	this->tag = tag;
	this->data = data;
	this->lineageTo = nullptr;
	this->lineageChance = 0.0;
}

DenLineage::DenLineage(std::string type, int count, std::string tag, double data) : DenCreature(type, count, tag, data) {
	timelineType = ConnectionTimelineType::ALL;
}

bool DenLineage::timelinesMatch(const DenLineage *other) const {
	if (timelineType != other->timelineType) return false;
	if (timelineType == ConnectionTimelineType::ALL) return true;

	if (timelines.size() != other->timelines.size()) return false;

	for (std::string timeline : timelines) {
		if (other->timelines.count(timeline) != 1) return false;
	}

	return true;
}

Den::Den() {}