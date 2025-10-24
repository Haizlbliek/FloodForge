#pragma once

#include <string>
#include <set>
#include <vector>

#include "TimelineType.hpp"

class DenCreature {
	public:
		DenCreature(std::string type, int count, std::string tag, double data);

		std::string type;
		int count;
		std::string tag;
		double data;

		DenCreature *lineageTo;
		double lineageChance;
};

class DenLineage : public DenCreature {
	public:
		DenLineage(std::string type, int count, std::string tag, double data);

		bool timelinesMatch(const DenLineage *other) const;

		std::set<std::string> timelines;
		TimelineType timelineType;
};

class Den {
	public:
		Den();

		std::vector<DenLineage> creatures;
};

class GarbageWormDen {
	public:
		GarbageWormDen();

		std::string creatureType;
		int count;
		bool timelinesMatch(const GarbageWormDen *other) const;

		std::set<std::string> timelines;
		TimelineType timelineType;
};