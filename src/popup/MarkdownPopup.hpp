#pragma once

#include <fstream>

#include "../math/Quadruple.hpp"
#include "Popups.hpp"
#include "../font/Fonts.hpp"
#include "../Settings.hpp"

struct MDStyledText {
	std::string text;
	bool italic = false;
	bool bold = false;
	bool underline = false;
	bool strikethrough = false;
	bool code = false;
	std::string url = "";
};

enum class MDType {
	TEXT,
	H1,
	H2,
	H3,
	QUOTE,
	HORIZONTAL_RULE
};

class MarkdownPopup : public Popup {
	public:
		MarkdownPopup(std::filesystem::path path);

		void draw() override;

		void close() override;

		std::string PopupName() { return "MarkdownPopup"; }

	private:
		void writeLine(std::vector<MDStyledText> line, double x, double &y, double size);

		void loadFile(std::filesystem::path filePath);

		std::vector<MDStyledText> parseStyledText(const std::string &line);

		void clampScroll();

		static void scrollCallback(void *object, double deltaX, double deltaY);

		std::ifstream file;

		std::vector<std::pair<MDType, std::vector<MDStyledText>>> lines;

		std::vector<Quadruple<double, double, std::string, Vector2>> links;

		double currentScroll = 0.0;
		double targetScroll = 0.0;
		double maxScroll = 0.0;
};