#include "DenPopup.hpp"

#include <algorithm>

#include "../../Theme.hpp"
#include "../../font/Fonts.hpp"
#include "../../Settings.hpp"
#include "../../ui/UI.hpp"

#include "ConditionalPopup.hpp"

DenPopup::DenPopup(Den &den) : Popup(), den(den) {
	bounds = Rect(-0.35, -0.35, 0.375 + 0.1, 0.35);
	scrollA = 0.0;
	scrollATo = 0.0;
	scrollB = 0.0;
	scrollBTo = 0.0;
	scrollL = 0.0;
	scrollLTo = 0.0;

	mouseClickSlider = false;

	UI::window->addScrollCallback(this, scrollCallback);
	UI::window->addKeyCallback(this, keyCallback);

	this->selectedCreature = 0;

	if (this->den.creatures.size() == 0) {
		this->den.creatures.push_back(DenLineage("", 0, "", 0.0));
	}
	this->selectedLineage = &this->den.creatures[0];
	this->selectedLineageChance = nullptr;
	ensureFlag(*this->selectedLineage);
}

void DenPopup::draw() {
	lineageSidebarWidth = EditorState::denPopupLineageExtended ? 0.22 : 0.0;

	std::string hoverText = "";

	mouseSection = (UI::mouse.x > (bounds.x0 + 0.6 + lineageSidebarWidth)) ? 2 : (UI::mouse.x > bounds.x0 + lineageSidebarWidth) ? 1 : 0;

	DenCreature *creature = selectedLineage;
	bool unknown = creature == nullptr ? false : !CreatureTextures::known(creature->type);

	hasSlider = creature == nullptr ? false : (creature->tag == "MEAN" || creature->tag == "SEED" || creature->tag == "LENGTH" || creature->tag == "RotType");

	bounds.x1 = bounds.x0 + 0.6;
	if (EditorState::denPopupTagsExtended) {
		bounds.x1 += 0.2;

		if (hasSlider) {
			bounds.x1 += 0.1;
		}
	} else {
		hasSlider = false;
	}
	if (EditorState::denPopupLineageExtended) {
		bounds.x1 += lineageSidebarWidth;
	}

	Popup::draw();

	if (minimized) return;

	scrollL += (scrollLTo - scrollL) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	scrollA += (scrollATo - scrollA) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	scrollB += (scrollBTo - scrollB) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);

	double mainX = bounds.x0;
	if (EditorState::denPopupLineageExtended) mainX += lineageSidebarWidth;

	double centreX = mainX + 0.305;

	double buttonSize = 1.0 / 14.0;
	double buttonPadding = 0.01;

	double countX = 0.0;
	double countY = 0.0;

	glEnable(GL_SCISSOR_TEST);
	double clipBottom = ((bounds.y0 + 0.01 + buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
	double clipTop = ((bounds.y1 - 0.1 - buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
	glScissor(0, clipBottom, UI::window->Width(), clipTop - clipBottom);
	UI::clip(Rect(-INFINITY, bounds.y0 + 0.01 + buttonPadding, INFINITY, bounds.y1 - 0.1));

	// Draw creatures
	{
		glDisable(GL_SCISSOR_TEST);
		setThemeColour(ThemeColour::Text);
		glLineWidth(1);
		Fonts::rainworld->writeCentered("Creature type:", centreX, bounds.y1 - 0.07, 0.035, CENTER_X);
		glEnable(GL_SCISSOR_TEST);

		int countA = CreatureTextures::creatureOrder.size();
		if (!unknown) countA--;

		if (creature != nullptr) {
			for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
				for (int x = 0; x < CREATURE_ROWS; x++) {
					int id = x + y * CREATURE_ROWS;

					if (id >= countA) break;

					std::string creatureType = CreatureTextures::creatureOrder[id];

					bool isSelected = creature->type == creatureType || (unknown && creatureType == "unknown");

					UVRect rect = UVRect::fromSize(
						centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5,
						(bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollA,
						buttonSize, buttonSize
					);

					GLuint texture = CreatureTextures::getTexture(creatureType);

					int w, h;
					glBindTexture(GL_TEXTURE_2D, texture);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glBindTexture(GL_TEXTURE_2D, 0);


					float ratio = (float(w) / float(h) + 1.0) * 0.5;
					float uvx = 1.0 / ratio;
					float uvy = ratio;
					if (uvx < 1.0) {
						uvy /= uvx;
						uvx = 1.0;
					}
					if (uvy < 1.0) {
						uvx /= uvy;
						uvy = 1.0;
					}
					uvx *= 0.5;
					uvy *= 0.5;

					rect.uv(0.5 - uvx, 0.5 + uvy, 0.5 + uvx, 0.5 - uvy);

					UI::ButtonResponse response = UI::TextureButton(rect, UI::TextureButtonMods().Selected(isSelected).TextureId(texture).TextureColor(isSelected ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5)));


					if (response.clicked) {
						if (creatureType == "clear") {
							creature->type = "";
							creature->count = 0;
						} else {
							if (creature->type == creatureType || creatureType == "unknown") {
								if (UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
									creature->count -= 1;
									if (creature->count <= 0) {
										creature->type = "";
										creature->count = 0;
									}
								} else {
									creature->count += 1;
								}
							} else {
								creature->type = creatureType;
								creature->count = 1;
							}
						}
						ensureFlag(*creature);
						isSelected = true;
					}

					if (response.hovered) {
						hoverText = "Creature - " + creatureType;
					}

					if (isSelected) {
						countX = rect.x1;
						countY = rect.y0;
					}
				}
			}

			if (creature->type != "" && den.creatures[selectedCreature].lineageTo == nullptr) {
				setThemeColour(ThemeColour::Text);
				Fonts::rainworld->writeCentered(std::to_string(creature->count), countX, countY, 0.04, CENTER_XY);
			}
		}
	}

	// Draw tags
	if (EditorState::denPopupTagsExtended) {
		glDisable(GL_SCISSOR_TEST);
		setThemeColour(ThemeColour::Text);
		glLineWidth(1);
		Fonts::rainworld->writeCentered("Tag:", mainX + 0.7, bounds.y1 - 0.07, 0.035, CENTER_X);
		glEnable(GL_SCISSOR_TEST);

		if (hovered) {
			setThemeColour(ThemeColour::BorderHighlight);
		} else {
			setThemeColour(ThemeColour::Border);
		}
		Draw::begin(Draw::LINES);
		Draw::vertex(mainX + 0.6, bounds.y0);
		Draw::vertex(mainX + 0.6, bounds.y1);
		Draw::end();

		if (creature != nullptr) {
			double tagCentreX = mainX + 0.7;
			int countB = CreatureTextures::creatureTags.size();

			for (int y = 0; y <= (countB / 2); y++) {
				for (int x = 0; x < 2; x++) {
					int id = x + y * 2;

					if (id >= countB) break;

					std::string creatureTag = CreatureTextures::creatureTags[id];

					bool isSelected = creature->tag == creatureTag;

					double rectX = tagCentreX + (x - 1.0) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
					double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollB;

					UVRect rect = UVRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);
					UI::ButtonResponse response = UI::TextureButton(rect,
						UI::TextureButtonMods()
						.TextureId(CreatureTextures::getTexture(CreatureTextures::creatureTags[id], false))
						.Selected(isSelected)
						.TextureColor(isSelected ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5))
					);

					if (response.clicked) {
						std::string creatureTag = CreatureTextures::creatureTags[id];

						if (creature->tag == creatureTag) {
							creature->tag = "";
						} else {
							creature->tag = creatureTag;
						}
						ensureFlag(*creature);
						isSelected = true;
					}

					if (response.hovered) {
						hoverText = "Tag - " + creatureTag;
					}
				}
			}

			if (hasSlider) {
				if (UI::mouse.justClicked() && (UI::mouse.x >= mainX + 0.825 && UI::mouse.x <= mainX + 0.875) && (UI::mouse.y >= bounds.y0 + 0.05 && UI::mouse.y <= bounds.y1 - 0.1)) {
					mouseClickSlider = true;
				}

				glDisable(GL_SCISSOR_TEST);

				setThemeColour(ThemeColour::Border);
				Draw::begin(Draw::LINES);
				Draw::vertex(mainX + 0.85, bounds.y0 + 0.05);
				Draw::vertex(mainX + 0.85, bounds.y1 - 0.1);
				Draw::end();

				double progress = (creature->data - sliderMin) / (sliderMax - sliderMin);
				double sliderY = ((bounds.y1 - bounds.y0 - 0.2) * progress) + bounds.y0 + 0.075;
				fillRect(mainX + 0.825, sliderY - 0.005, mainX + 0.875, sliderY + 0.005);

				setThemeColour(ThemeColour::Text);
				float number = creature->data;
				std::ostringstream ss;

				if (creature->tag == "MEAN" ||creature->tag == "LENGTH") {
					ss << std::fixed << std::setprecision(2) << std::setw(3) << number;
				} else if (creature->tag == "SEED" || creature->tag == "RotType") {
					ss << std::setw(5) << static_cast<int>(number);
				}

				std::string valueStr = ss.str();
				double xPos = mainX + 0.82;

				if (creature->tag == "MEAN" || creature->tag == "LENGTH") {
					if (number < 0) {
						xPos = mainX + 0.805;
					}
				} else if (creature->tag == "SEED" || creature->tag == "RotType") {
					xPos = (number < 4) ? mainX + 0.81 : mainX + 0.809;
				}

				Fonts::rainworld->writeCentered(valueStr, xPos, sliderY + 0.028, 0.026, CENTER_Y);
			}
		}
	}

	// Draw lineages
	scrollLMax = 0;
	if (EditorState::denPopupLineageExtended) {
		glDisable(GL_SCISSOR_TEST);
		setThemeColour(ThemeColour::Text);
		glLineWidth(1);
		Fonts::rainworld->writeCentered("Lineages", bounds.x0 + 0.11, bounds.y1 - 0.07, 0.035, CENTER_X);
		glEnable(GL_SCISSOR_TEST);

		setThemeColour(hovered ? ThemeColour::BorderHighlight : ThemeColour::Border);
		Draw::begin(Draw::LINES);
		Draw::vertex(bounds.x0 + lineageSidebarWidth, bounds.y0);
		Draw::vertex(bounds.x0 + lineageSidebarWidth, bounds.y1);
		Draw::end();

		double dotsCenterX = bounds.x0 + 0.11 - (den.creatures.size() - 1) * 0.015;
		double dotsCenterY = bounds.y1 - 0.13;
		for (int i = 0; i < den.creatures.size(); i++) {
			double x = dotsCenterX + i * 0.03;
			setThemeColor(i == selectedCreature ? ThemeColour::BorderHighlight : ThemeColour::Border);
			if (i == selectedCreature) {
				fillRect(x - 0.01, dotsCenterY - 0.01, x + 0.01, dotsCenterY + 0.01);
			} else {
				fillRect(x - 0.0075, dotsCenterY - 0.0075, x + 0.0075, dotsCenterY + 0.0075);
			}
		}

		if (UI::TextureButton(
			UVRect::fromSize(bounds.x0 + 0.01, bounds.y1 - 0.19, 0.04, 0.04).uv(0.5, 0.5, 0.75, 0.75),
			UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(selectedCreature == 0).TextureColor((selectedCreature == 0) ? Color(0.5, 0.5, 0.5) : Color(1.0, 1.0, 1.0))
		)) {
			selectedCreature--;
			if (selectedCreature < 0) {
				selectedCreature = den.creatures.size() - 1;
			}
			selectedLineage = den.creatures.size() == 0 ? nullptr : &den.creatures[selectedCreature];
			ensureFlag(*this->selectedLineage);
		}

		if (UI::TextureButton(
			UVRect::fromSize(bounds.x0 + 0.06, bounds.y1 - 0.19, 0.04, 0.04).uv(0.0, 0.0, 0.25, 0.25),
			UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(den.creatures.size() == 0).TextureColor((den.creatures.size() == 0) ? Color(0.5, 0.5, 0.5) : Color(1.0, 1.0, 1.0))
		)) {
			den.creatures.erase(std::next(den.creatures.begin(), selectedCreature));
			selectedCreature--;
			if (selectedCreature == -1 && den.creatures.size() != 0) selectedCreature = 0;
			selectedLineage = den.creatures.size() == 0 ? nullptr : &den.creatures[selectedCreature];
			if (selectedLineage != nullptr) ensureFlag(*this->selectedLineage);
		}

		if (UI::TextureButton(
			UVRect::fromSize(bounds.x0 + 0.11, bounds.y1 - 0.19, 0.04, 0.04).uv(0.25, 0.5, 0.5, 0.75),
			UI::TextureButtonMods().TextureId(UI::uiTexture)
		)) {
			selectedCreature = den.creatures.size();
			den.creatures.push_back(DenLineage("", 0, "", 0.0));
			selectedLineage = &den.creatures[selectedCreature];
			ensureFlag(*this->selectedLineage);
		}

		if (UI::TextureButton(
			UVRect::fromSize(bounds.x0 + 0.16, bounds.y1 - 0.19, 0.04, 0.04).uv(0.75, 0.5, 1.0, 0.75),
			UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(selectedCreature >= den.creatures.size() - 1).TextureColor((selectedCreature >= den.creatures.size() - 1) ? Color(0.5, 0.5, 0.5) : Color(1.0, 1.0, 1.0))
		)) {
			selectedCreature++;
			if (selectedCreature >= den.creatures.size()) {
				selectedCreature = 0;
			}
			selectedLineage = &den.creatures[selectedCreature];
			ensureFlag(*this->selectedLineage);
		}

		double clipBottom = ((bounds.y0 + 0.01 + buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
		double clipTop = ((bounds.y1 - 0.185 - buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
		glScissor(0, clipBottom, UI::window->Width(), clipTop - clipBottom);
		UI::clip(Rect(-INFINITY, bounds.y0 + 0.01 + buttonPadding, INFINITY, bounds.y1 - 0.185));

		if (den.creatures.size() != 0 && selectedCreature != -1) {
			DenCreature *lastCreature = nullptr;
			DenCreature *creature = &den.creatures[selectedCreature];
			int j = 0;
			while (true) {
				scrollLMax = std::max(scrollLMax, j);
				UVRect creatureRect = UVRect::fromSize(bounds.x0 + 0.01, bounds.y1 - scrollL - 0.19 - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize);
				bool selected = creature == selectedLineage;

				UI::ButtonResponse response;

				GLuint texture = CreatureTextures::getTexture(creature->type);
				if (texture != 0) {
					int w, h;
					glBindTexture(GL_TEXTURE_2D, texture);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
					glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glBindTexture(GL_TEXTURE_2D, 0);

					float ratio = (float(w) / float(h) + 1.0) * 0.5;
					float uvx = 1.0 / ratio;
					float uvy = ratio;
					if (uvx < 1.0) {
						uvy /= uvx;
						uvx = 1.0;
					}
					if (uvy < 1.0) {
						uvx /= uvy;
						uvy = 1.0;
					}
					uvx *= 0.5;
					uvy *= 0.5;
					creatureRect.uv(0.5 - uvx, 0.5 + uvy, 0.5 + uvx, 0.5 - uvy);
					response = UI::TextureButton(creatureRect, UI::TextureButtonMods().TextureId(texture).Selected(selected).TextureColor(selected ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5)));
				} else {
					response = UI::Button(creatureRect, UI::ButtonMods().Selected(selected));
				}

				if (response.clicked) {
					selectedLineage = creature;
					ensureFlag(*this->selectedLineage);
				}

				if (creature->lineageTo != nullptr) {
					Rect inputRect = Rect::fromSize(creatureRect.x0 + buttonSize + buttonPadding, creatureRect.y0 - (buttonPadding + buttonSize * 0.5) * 0.5, buttonSize, buttonSize * 0.5);;
					bool selected = creature == selectedLineageChance;
					std::string text = std::to_string(int(creature->lineageChance * 100)) + (selected ? "" : "%");

					if (UI::TextButton(inputRect, text, UI::TextButtonMods().Selected(selected))) {
						selected = !selected;
						if (!selected) submitChance();
						selectedLineageChance = selected ? creature : nullptr;
					}
				}

				UVRect deleteRect = UVRect::fromSize(creatureRect.x0 + buttonSize + buttonPadding, creatureRect.y0 + buttonSize * 0.25, buttonSize * 0.5, buttonSize * 0.5);
				deleteRect.uv(0.0, 0.0, 0.25, 0.25);

				if (UI::TextureButton(deleteRect, UI::TextureButtonMods().TextureId(UI::uiTexture))) {
					if (lastCreature == nullptr) {
						if (creature->lineageTo == nullptr) {
							creature->type = "";
							creature->tag = "";
							creature->count = 0;
							creature->data = 0.0;
						} else {
							creature->type = creature->lineageTo->type;
							creature->tag = creature->lineageTo->tag;
							creature->count = creature->lineageTo->count;
							creature->data = creature->lineageTo->data;

							if (selectedLineage == creature->lineageTo) {
								selectedLineage = creature;
							}
							DenCreature *toDelete = creature->lineageTo;
							creature->lineageTo = creature->lineageTo->lineageTo;
							delete toDelete;
						}
					} else {
						if (selectedLineage == creature) {
							selectedLineage = lastCreature;
						}
						lastCreature->lineageTo = creature->lineageTo;
						delete creature;
					}
				}

				lastCreature = creature;
				j++;
				if (creature->lineageTo == nullptr) {
					break;
				} else {
					creature = creature->lineageTo;
				}
			}

			UVRect addRect = UVRect::fromSize(bounds.x0 + 0.01, bounds.y1 - 0.19 - scrollL - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize);
			addRect.uv(0.25, 0.5, 0.5, 0.75);

			if (UI::TextureButton(addRect, UI::TextureButtonMods().TextureId(UI::uiTexture))) {
				creature->lineageTo = new DenCreature("", 0, "", 0.0);
				selectedLineage = creature->lineageTo;
			}

			UVRect moreRect = UVRect::fromSize(bounds.x0 + 0.01 + buttonSize + buttonPadding, bounds.y1 - 0.19 - scrollL - (j + 1) * (buttonSize + buttonPadding), buttonSize, buttonSize);
			moreRect.uv(0.75, 0.0, 1.0, 0.25);
			UI::ButtonResponse response = UI::TextureButton(moreRect, UI::TextureButtonMods().TextureId(UI::uiTexture));

			if (response.hovered) {
				hoverText = "Edit conditionals";
			}

			if (response.clicked) {
				Popups::addPopup(new ConditionalPopup(&den.creatures[selectedCreature]));
			}
		}
	}

	glDisable(GL_SCISSOR_TEST);
	UI::clip();

	// Draw expand buttons
	{
		UVRect rectLineage = UVRect(mainX, bounds.y1 - 0.05, mainX + 0.05, bounds.y1 - 0.1);
		if (EditorState::denPopupLineageExtended) {
			rectLineage.uv(0.0, 0.5, 0.25, 0.75);
		} else {
			rectLineage.uv(0.25, 0.5, 0.5, 0.75);
		}

		UVRect rectTags = UVRect(mainX + 0.55, bounds.y1 - 0.05, mainX + 0.6, bounds.y1 - 0.1);
		if (EditorState::denPopupTagsExtended) {
			rectTags.uv(0.0, 0.5, 0.25, 0.75);
		} else {
			rectTags.uv(0.25, 0.5, 0.5, 0.75);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (UI::TextureButton(rectLineage, UI::TextureButtonMods().TextureId(UI::uiTexture))) {
			EditorState::denPopupLineageExtended = !EditorState::denPopupLineageExtended;
			if (EditorState::denPopupLineageExtended) {
				bounds.x0 -= 0.22;
			} else {
				bounds.x0 += 0.22;
			}
		}

		if (UI::TextureButton(rectTags, UI::TextureButtonMods().TextureId(UI::uiTexture))) {
			EditorState::denPopupTagsExtended = !EditorState::denPopupTagsExtended;
		}

		glDisable(GL_BLEND);
	}

	if (hasSlider && mouseClickSlider && creature != nullptr) {
		if (!UI::window->GetMouse()->Left()) {
			mouseClickSlider = false;
		}

		double P = (UI::mouse.y - bounds.y0 - 0.075) / (bounds.y1 - bounds.y0 - 0.2);
		P = std::clamp(P, 0.0, 1.0);
		creature->data = P * (sliderMax - sliderMin) + sliderMin;
		if (sliderType == SliderType::SLIDER_INT) {
			creature->data = round(creature->data);
		}
	}

	// Hovers
	if (!hoverText.empty() && hovered) {
		double width = Fonts::rainworld->getTextWidth(hoverText, 0.04) + 0.02;
		Rect rect = Rect::fromSize(UI::mouse.x, UI::mouse.y, width, 0.06);
		setThemeColour(ThemeColour::Popup);
		fillRect(rect);
		setThemeColour(ThemeColour::Border);
		strokeRect(rect);
		setThemeColour(ThemeColour::Text);
		Fonts::rainworld->writeCentered(hoverText, UI::mouse.x + 0.01, UI::mouse.y + 0.03, 0.04, CENTER_Y);
	}
}

void DenPopup::accept() {}

void DenPopup::close() {
	Popup::close();

	UI::window->removeScrollCallback(this, scrollCallback);
	UI::window->removeKeyCallback(this, keyCallback);
}

void DenPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	DenPopup *popup = static_cast<DenPopup*>(object);

	if (!popup->hovered) return;

	if (popup->mouseSection == 0) {
		popup->scrollLTo += deltaY * 0.06;
	} else if (popup->mouseSection == 1) {
		popup->scrollATo += deltaY * 0.06;
	} else {
		popup->scrollBTo += deltaY * 0.06;
	}

	popup->clampScroll();
}

void DenPopup::clampScroll() {
	double buttonSize = 1.0 / 14.0;
	double buttonPadding = 0.01;

	int itemsA = CreatureTextures::creatures.size() / CREATURE_ROWS - 1;
	double sizeA = itemsA * (buttonSize + buttonPadding);

	if (scrollATo < -sizeA) {
		scrollATo = -sizeA;
		if (scrollA <= -sizeA + 0.06) {
			scrollA = -sizeA - 0.03;
		}
	}

	if (scrollATo > 0) {
		scrollATo = 0;
		if (scrollA >= -0.06) {
			scrollA = 0.03;
		}
	}

	int itemsB = CreatureTextures::creatureTags.size() / 2;
	double sizeB = itemsB * (buttonSize + buttonPadding);

	if (scrollBTo < -sizeB) {
		scrollBTo = -sizeB;
		if (scrollB <= -sizeB + 0.06) {
			scrollB = -sizeB - 0.03;
		}
	}

	if (scrollBTo > 0) {
		scrollBTo = 0;
		if (scrollB >= -0.06) {
			scrollB = 0.03;
		}
	}

	double sizeL = (scrollLMax + 1) * (buttonSize + buttonPadding);
	if (scrollLTo < -sizeL) {
		scrollLTo = -sizeL;
		if (scrollL <= -sizeL + 0.06) {
			scrollL = -sizeL - 0.03;
		}
	}

	if (scrollLTo > 0) {
		scrollLTo = 0;
		if (scrollL >= -0.06) {
			scrollL = 0.03;
		}
	}
}

void DenPopup::ensureFlag(DenCreature &creature) {
	std::string creatureType = toLower(creature.type);

	if (creatureType.empty()) {
		creature.tag = "";
		creature.data = 0.0;
	}

	bool isNotLizard =
		creatureType != "blacklizard" &&
		creatureType != "bluelizard" &&
		creatureType != "cyanlizard" &&
		creatureType != "greenlizard" &&
		creatureType != "pinklizard" &&
		creatureType != "redlizard" &&
		creatureType != "whitelizard" &&
		creatureType != "yellowlizard" &&
		creatureType != "salamander" &&
		creatureType != "eellizard" &&
		creatureType != "spitlizard" &&
		creatureType != "trainlizard" &&
		creatureType != "zooplizard" &&
		creatureType != "basilisklizard" &&
		creatureType != "blizzardlizard" &&
		creatureType != "indigolizard";

	if (creature.tag == "MEAN") {
		if (isNotLizard) {
			creature.tag = "";
		}
	}

	if (creature.tag == "LENGTH") {
		if (creatureType != "polemimic" && creatureType != "centipede") {
			creature.tag = "";
		}
	}

	if (creature.tag == "Winter") {
		if (creatureType != "bigspider" && creatureType != "spitterspider" && creatureType != "yeek" && isNotLizard) {
			creature.tag = "";
		}
	}

	if (creature.tag == "Voidsea") {
		if (creatureType != "redlizard" && creatureType != "redcentipede" && creatureType != "bigspider" && creatureType != "daddylonglegs" && creatureType != "brotherlonglegs" && creatureType != "terrorlonglegs" && creatureType != "bigeel" && creatureType != "cyanlizard") {
			creature.tag = "";
		}
	}

	if (creature.tag != "MEAN" && creature.tag != "LENGTH" && creature.tag != "SEED") creature.data = 0.0;

	sliderType = SliderType::SLIDER_FLOAT;
	if (creature.tag == "MEAN") {
		sliderMin = -1.0;
		sliderMax = 1.0;
	} else if (creature.tag == "LENGTH") {
		if (creatureType == "centipede") {
			sliderMin = 0.1;
			sliderMax = 1.0;
		} else {
			sliderMin = 1;
			sliderMax = 32;
		}
	} else if (creature.tag == "SEED") {
		sliderMin = 0;
		sliderMax = 65536;
		sliderType = SliderType::SLIDER_INT;
	} else if (creature.tag == "RotType") {
		if (isNotLizard) {
			creature.tag = "";
		} else {
			sliderMin = 0;
			sliderMax = 3;
		}
		sliderType = SliderType::SLIDER_INT;
	}
}

void DenPopup::submitChance() {
	if (selectedLineageChance == nullptr) return;

	if (selectedLineageChance->lineageChance > 1) {
		selectedLineageChance->lineageChance = 1;
	}
	if (selectedLineageChance->lineageChance < 0) {
		selectedLineageChance->lineageChance = 0;
	}
	selectedLineageChance = nullptr;
}

void DenPopup::keyCallback(void *object, int action, int key) {
	DenPopup *denWindow = static_cast<DenPopup*>(object);

	if (denWindow->minimized) return;
	if (denWindow->selectedLineageChance == nullptr) return;

	int chance = int(denWindow->selectedLineageChance->lineageChance * 100.0);

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key >= 48 && key <= 57) {
			int number = key - 48;
			if (chance > 99) return;
			if (chance == 0) {
				chance = number;
			} else {
				chance = (chance * 10) + number;
			}
		}

		if (key == GLFW_KEY_ENTER) {
			denWindow->submitChance();
			return;
		}

		if (key == GLFW_KEY_BACKSPACE) {
			if (chance > 9) {
				chance /= 10;
			} else {
				chance = 0;
			}
		}
	}

	denWindow->selectedLineageChance->lineageChance = chance / 100.0;
}