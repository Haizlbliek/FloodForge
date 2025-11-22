#include "ColorEditPopup.hpp"

#include "../../Draw.hpp"
#include "../../ui/UI.hpp"

#include "../Shaders.hpp"
#include "../Globals.hpp"

#define selectorSize 0.33

ColorEditPopup::ColorEditPopup(Color &color) : Popup(), color(color) {
	oldColor = color;
	bounds = Rect(-0.2, -0.2, 0.2, 0.2);
	Vector3f hsv = color.HSV();
	hue = hsv.x;
	this->callback = nullptr;
}

ColorEditPopup::ColorEditPopup(Color &color, std::function<void(Color oldColor, Color &color)> callback) : ColorEditPopup(color) {
	this->callback = callback;
}

void ColorEditPopup::draw() {
	Popup::draw();

	if (minimized) return;

	Vector3f hsv = color.HSV();

	Rect selectorRect = Rect::fromSize(bounds.x0 + 0.01, bounds.y1 - 0.06, selectorSize, -selectorSize);
	Draw::useProgram(Shaders::colorSquareShader);
	GLuint hueLoc = glGetUniformLocation(Shaders::colorSquareShader, "hue");
	GLuint projLoc = glGetUniformLocation(Shaders::colorSquareShader, "projection");
	GLuint modelLoc = glGetUniformLocation(Shaders::colorSquareShader, "model");
	glUniform1f(hueLoc, hue / 360.0);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix(Vector2(0.0, 0.0), UI::screenBounds).m);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix(0.0, 0.0).m);

	Draw::color(1.0, 1.0, 1.0);
	Draw::begin(Draw::QUADS);
	Draw::texCoord(0.0, 1.0);
	Draw::vertex(selectorRect.x0, selectorRect.y1);
	Draw::texCoord(1.0, 1.0);
	Draw::vertex(selectorRect.x1, selectorRect.y1);
	Draw::texCoord(1.0, 0.0);
	Draw::vertex(selectorRect.x1, selectorRect.y0);
	Draw::texCoord(0.0, 0.0);
	Draw::vertex(selectorRect.x0, selectorRect.y0);
	Draw::end();
	Draw::useProgram(0);

	static bool centerFocused = false;
	if (UI::mouse.justClicked() && selectorRect.inside(UI::mouse)) {
		centerFocused = true;
	}
	if (centerFocused) {
		if (!UI::mouse.leftMouse) centerFocused = false;

		double s = std::clamp((UI::mouse.x - bounds.x0 - 0.01) / selectorSize, 0.0, 1.0);
		double v = std::clamp((UI::mouse.y - (bounds.y1 - 0.06 - selectorSize)) / selectorSize, 0.0, 1.0);
		hsv.x = hue;
		hsv.y = s;
		hsv.z = v;
		color.HSV(hsv);
	}

	Vector2 colorPos = Vector2(hsv.y * selectorSize + 0.01 + bounds.x0, hsv.z * selectorSize + bounds.y1 - 0.06 - selectorSize);
	Rect colorRect = Rect::fromSize(colorPos.x - 0.01, colorPos.y - 0.01, 0.02, 0.02);
	Draw::color(color);
	fillRect(colorRect);
	Draw::color(hsv.z > 0.5 ? 0.0 : 1.0);
	strokeRect(colorRect);

	static bool sliderFocused = false;
	Rect sliderRect = Rect::fromSize(selectorRect.x1 + 0.02, selectorRect.y0, 0.02, selectorSize);

	Draw::useProgram(Shaders::hueSliderShader);
	GLuint projLoc2 = glGetUniformLocation(Shaders::hueSliderShader, "projection");
	GLuint modelLoc2 = glGetUniformLocation(Shaders::hueSliderShader, "model");
	glUniformMatrix4fv(projLoc2, 1, GL_FALSE, projectionMatrix(Vector2(0.0, 0.0), UI::screenBounds).m);
	glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, modelMatrix(0.0, 0.0).m);
	Draw::color(1.0, 1.0, 1.0);
	Draw::begin(Draw::QUADS);
	Draw::texCoord(0.0, 1.0);
	Draw::vertex(sliderRect.x0, sliderRect.y1);
	Draw::texCoord(1.0, 1.0);
	Draw::vertex(sliderRect.x1, sliderRect.y1);
	Draw::texCoord(1.0, 0.0);
	Draw::vertex(sliderRect.x1, sliderRect.y0);
	Draw::texCoord(0.0, 0.0);
	Draw::vertex(sliderRect.x0, sliderRect.y0);
	Draw::end();
	Draw::useProgram(0);

	double hueY = MathUtils::lerp(sliderRect.y0, sliderRect.y1, hue / 360.0);
	Draw::color(1.0);
	drawLine(sliderRect.x0 - 0.01, hueY, sliderRect.x1 + 0.01, hueY);

	if (UI::mouse.justClicked() && sliderRect.inside(UI::mouse)) {
		sliderFocused = true;
	}
	if (sliderFocused) {
		if (!UI::mouse.leftMouse) sliderFocused = false;

		double h = std::clamp((UI::mouse.y - (bounds.y1 - 0.06 - selectorSize)) / selectorSize, 0.0, 1.0) * 360.0;
		hsv.x = h;
		hue = h;
		color.HSV(hsv);
	}
}

void ColorEditPopup::close() {
	Popup::close();

	if (this->callback != nullptr) {
		this->callback(oldColor, color);
	}
}