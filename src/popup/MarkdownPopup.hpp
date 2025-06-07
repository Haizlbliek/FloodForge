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

enum MDType {
	TEXT,
	H1,
	H2,
	H3,
	QUOTE,
	HORIZONTAL_RULE
};

class MarkdownPopup : public Popup {
	public:
		MarkdownPopup(Window *window, std::string path)
		: Popup(window) {
			bounds = Rect(-0.8, -0.8, 0.8, 0.8);
			
			loadFile(path);
			
			window->addScrollCallback(this, scrollCallback);
		}
		
		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) override {
			Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
			
			if (minimized) return;
			
			links.clear();
			
			int windowWidth;
			int windowHeight;
			glfwGetWindowSize(window->getGLFWWindow(), &windowWidth, &windowHeight);
			
			double padding = 0.01;
			glEnable(GL_SCISSOR_TEST);
			glScissor(
				((bounds.X0() + padding) / screenBounds.x + 1.0) * 0.5 * windowWidth,
 				((bounds.Y0() + padding) / screenBounds.y + 1.0) * 0.5 * windowHeight,
				(((bounds.X1() - bounds.X0()) - padding * 2) / screenBounds.x) * 0.5 * windowWidth,
				(((bounds.Y1() - bounds.Y0()) - padding * 2 - 0.05) / screenBounds.y) * 0.5 * windowHeight
			);
			
			scroll += (scrollTo - scroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
			
			double x = bounds.X0();
			double y = 0.75 + bounds.Y0() + 0.8f + scroll;
			
			for (std::pair<MDType, std::vector<MDStyledText>> line : lines) {
				if (line.first == MDType::TEXT) {
					Draw::color(1.0, 1.0, 1.0);
					writeLine(line.second, x + 0.02, y, 0.03);
					y -= 0.05;
				} else if (line.first == MDType::QUOTE) {
					Draw::color(0.7, 0.7, 0.7);
					Draw::begin(Draw::LINES);
					Draw::vertex(-0.77, y);
					Draw::vertex(-0.77, y - 0.04);
					Draw::end();
					Draw::color(1.0, 1.0, 1.0);
					writeLine(line.second, x + 0.05, y, 0.03);
					y -= 0.04;
				} else if (line.first == MDType::H1) {
					Draw::color(1.0, 1.0, 1.0);
					y -= 0.02;
					writeLine(line.second, x + 0.03, y, 0.08);
					y -= 0.11;
				} else if (line.first == MDType::H2) {
					Draw::color(1.0, 1.0, 1.0);
					y -= 0.02;
					writeLine(line.second, x + 0.02, y, 0.05);
					y -= 0.08;
				} else if (line.first == MDType::H3) {
					Draw::color(0.8, 0.8, 0.8);
					y -= 0.01;
					writeLine(line.second, x + 0.02, y, 0.04);
					y -= 0.06;
				} else if (line.first == MDType::HORIZONTAL_RULE) {
					y -= 0.03;
					Draw::color(0.7, 0.7, 0.7);
					Draw::begin(Draw::LINES);
					Draw::vertex(bounds.X0() + 0.01, y);
					Draw::vertex(bounds.X1() - 0.01, y);
					Draw::end();
					y -= 0.03;
				}
			}
			
			maxScroll = -(y - scroll);
			
			glDisable(GL_SCISSOR_TEST);
		}
		
		void close() override {
			Popup::close();
			
			window->removeScrollCallback(this, scrollCallback);
		}
		
		void mouseClick(double mouseX, double mouseY) override {
			Popup::mouseClick(mouseX, mouseY);
			
			for (Quadruple<double, double, std::string, Vector2> link : links) {
				if (
					(mouseX >= link.first && mouseX <= link.first + link.fourth.x) &&
					(mouseY >= link.second && mouseY <= link.second + link.fourth.y)
				) {
					openURL(link.third);
				}
			}
		}
		
		std::string PopupName() { return "MarkdownPopup"; }
	
	private:
		void writeLine(std::vector<MDStyledText> line, double x, double &y, double size) {
			for (MDStyledText &seg : line) {
				double width = Fonts::rainworld->getTextWidth(seg.text, size);

				float matrix[16] = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, (float) (y - size), 0, 1
				};
				if (seg.italic) {
					matrix[4] = 0.2f;
				}
				Draw::pushMatrix();
				Draw::multMatrix(matrix);
				
				if (seg.code) {
					x += 0.02;
					strokeRect(x - 0.01, -0.005, x + width + 0.01, size + 0.005);
				}

				if (seg.bold) {
					Fonts::rainworld->write(seg.text, x + 0.003, size, size);
					Fonts::rainworld->write(seg.text, x, size, size);
				} else {
					Fonts::rainworld->write(seg.text, x, size, size);
				}
				
				if (seg.strikethrough) {
					drawLine(x, size * 0.4, x + width, size * 0.4, 0.1);
				}

				if (seg.underline) {
					drawLine(x, 0.0, x + width, 0.0, 0.1);
				}
				
				if (!seg.url.empty()) {
					links.push_back(Quadruple<double, double, std::string, Vector2>( x, y - size, seg.url, Vector2(width, size) ));
				}
				
				if (seg.code) {
					x += 0.02;
				}

				Draw::popMatrix();
				
				x += width;
			}
		}
		
		void loadFile(std::string filePath) {
			file = std::ifstream(filePath);
			if (!file.is_open() || !std::filesystem::exists(filePath)) {
				Logger::log("No file found '", filePath, "'");
				close();
				return;
			}
			
			int addNewline = 0;
			std::string line;
			while (std::getline(file, line)) {
				MDType type = MDType::TEXT;
				if (line.empty()) {
					if (addNewline == 1) {
						addNewline = 2;
					} else {
						addNewline = 0;
					}
					
					continue;
				}

				if (startsWith(line, "# ")) {
					type = MDType::H1;
					line = line.substr(2);
					addNewline = 0;
				} else if (startsWith(line, "## ")) {
					type = MDType::H2;
					line = line.substr(3);
					addNewline = 0;
				} else if (startsWith(line, "### ")) {
					type = MDType::H3;
					line = line.substr(4);
					addNewline = 0;
				} else if (startsWith(line, "> ")) {
					if (addNewline == 2) {
						lines.push_back({ MDType::TEXT, {} });
					}
					addNewline = 1;

					type = MDType::QUOTE;
					line = line.substr(2);
				} else if (startsWith(line, "---") && (line.find_first_not_of('-') == std::string::npos)) {
					lines.push_back({ MDType::HORIZONTAL_RULE, {} });
					addNewline = 0;
					continue;
				} else if (startsWith(line, "***") && (line.find_first_not_of('*') == std::string::npos)) {
					lines.push_back({ MDType::HORIZONTAL_RULE, {} });
					addNewline = 0;
					continue;
				} else if (startsWith(line, "___") && (line.find_first_not_of('_') == std::string::npos)) {
					lines.push_back({ MDType::HORIZONTAL_RULE, {} });
					addNewline = 0;
					continue;
				} else {
					if (addNewline == 2) {
						lines.push_back({ MDType::TEXT, {} });
					}
					addNewline = 1;
				}
				
				lines.push_back({ type, parseStyledText(line) });
			}
			
			file.close();
		}
		
		std::vector<MDStyledText> parseStyledText(const std::string &line) {
			std::vector<MDStyledText> result;
			
			bool bold = false, italic = false, underline = false, strikethrough = false, code = false;
			std::string current = "";
			
			bool inLink = false;

			for (int i = 0; i < line.length(); i++) {
				if (line[i] == '\\') {
					if (i + 1 < line.length()) {
						current += line[i + 1];
						i++;
					}
				} else if (code) {
					if (line[i] == '`') {
						code = false;
						result.push_back({ current, false, false, false, false, true });
						current = "";
					} else {
						current += line[i];
					}
				} else {
					if (line[i] == '`') {
						result.push_back({ current, italic, bold, underline, strikethrough, false });
						current = "";
						code = true;
					} else if (line[i] == '*' && i + 1 < line.length() && line[i + 1] == '*') {
						result.push_back({ current, italic, bold, underline, strikethrough, false });
						current = "";
						bold = !bold;
						i++;
					} else if (line[i] == '~' && i + 1 < line.length() && line[i + 1] == '~') {
						result.push_back({ current, italic, bold, underline, strikethrough, false });
						current = "";
						strikethrough = !strikethrough;
						i++;
					} else if (line[i] == '_' && i + 1 < line.length() && line[i + 1] == '_') {
						result.push_back({ current, italic, bold, underline, strikethrough, false });
						current = "";
						underline = !underline;
						i++;
					} else if (line[i] == '*' || line[i] == '_') {
						result.push_back({ current, italic, bold, underline, strikethrough, false });
						current = "";
						italic = !italic;
					} else if (line[i] == '[') {
						result.push_back({ current + " ", italic, bold, underline, strikethrough, false });
						current = "";
						inLink = true;
					} else if (line[i] == ']') {
						if (!inLink) {
							current += ']';
							continue;
						}
	
						result.push_back({ current });
					} else if (line[i] == '(') {
						if (!inLink) {
							current += '(';
							continue;
						}
						
						current = "";
					} else if (line[i] == ')') {
						if (!inLink) {
							current += ')';
							continue;
						}
						
						MDStyledText last = result.back();
						result.pop_back();
						result.push_back({ (last.text), italic, bold, true, strikethrough, false, current });
						current = "";
						inLink = false;
					} else {
						current += line[i];
					}
				}
			}
			
			if (!current.empty()) result.push_back({ current, false, false, false, false });
		
			return result;
		}
		
		void clampScroll() {
			if (scrollTo < 0) {
				scrollTo = 0;
			}
			if (scrollTo >= maxScroll) {
				scrollTo = maxScroll;
			}
		}

		static void scrollCallback(void *object, double deltaX, double deltaY) {
			MarkdownPopup *popup = static_cast<MarkdownPopup*>(object);

			if (!popup->hovered) return;
			
			popup->scrollTo -= deltaY * 0.1;
			
			popup->clampScroll();
		}
	
		std::ifstream file;

		std::vector<std::pair<MDType, std::vector<MDStyledText>>> lines;
		
		std::vector<Quadruple<double, double, std::string, Vector2>> links;
		
		double scroll = 0.0;
		double scrollTo = 0.0;
		double maxScroll = 0.0;
};