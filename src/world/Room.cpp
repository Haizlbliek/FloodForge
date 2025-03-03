#include "Room.hpp"

#include "DenPopup.hpp"

void Room::draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds) {
    if (!valid) return;
    
    Colour tint = Colour(1.0, 1.0, 1.0);

    if (::roomColours == 1) {
        if (layer == 0) tint = Colour(1.0, 0.0, 0.0);
        if (layer == 1) tint = Colour(1.0, 1.0, 1.0);
        if (layer == 2) tint = Colour(0.0, 1.0, 0.0);
    }
    
    if (::roomColours == 2) {
        if (subregion == -1) tint = Colour(1.0, 1.0, 1.0);
        if (subregion ==  0) tint = Colour(1.0, 0.0, 0.0);
        if (subregion ==  1) tint = Colour(0.0, 1.0, 0.0);
        if (subregion ==  2) tint = Colour(0.0, 0.0, 1.0);
        if (subregion ==  3) tint = Colour(1.0, 1.0, 0.0);
        if (subregion ==  4) tint = Colour(0.0, 1.0, 1.0);
        if (subregion ==  5) tint = Colour(1.0, 0.0, 1.0);
        if (subregion ==  6) tint = Colour(1.0, 0.5, 0.0);
        if (subregion ==  7) tint = Colour(1.0, 1.0, 0.5);
        if (subregion ==  8) tint = Colour(0.5, 1.0, 0.0);
        if (subregion ==  9) tint = Colour(1.0, 1.0, 0.5);
        if (subregion == 10) tint = Colour(0.5, 0.0, 1.0);
        if (subregion == 11) tint = Colour(1.0, 0.5, 1.0);
    }
    
    glBindVertexArray(vao);
    glUseProgram(Shaders::roomShader);

    GLuint projLoc = glGetUniformLocation(Shaders::roomShader, "projection");
    GLuint modelLoc = glGetUniformLocation(Shaders::roomShader, "model");
    GLuint tintLoc = glGetUniformLocation(Shaders::roomShader, "tintColour");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix(cameraOffset, cameraScale * screenBounds).m);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix(position.x, position.y).m);
    if (hidden) {
        glUniform4f(tintLoc, tint.r, tint.g, tint.b, 0.5f);
    } else {
        glUniform4f(tintLoc, tint.r, tint.g, tint.b, tint.a);
    }

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);

    Draw::flushOnEnd = false;
    if (water != -1) {
        Draw::color(0.0, 0.0, 0.5, 0.5);
        fillRect(position.x, position.y - (height - std::min(water, height)), position.x + width, position.y - height);
    }

    Draw::flushOnEnd = true;

    for (int i = 0; i < denEntrances.size(); i++) {
        if (dens[i].type == "" || dens[i].count == 0) continue;

        double rectX = position.x + denEntrances[i].x;
        double rectY = position.y - denEntrances[i].y;

        Draw::color(1.0, 1.0, 1.0);
        GLuint texture = CreatureTextures::getTexture(dens[i].type);
        glEnable(GL_BLEND);
        Draw::useTexture(texture);			
        Draw::begin(Draw::QUADS);

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
        Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(rectX, rectY - 1.0);
        Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(rectX + 1.0, rectY - 1.0);
        Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(rectX + 1.0, rectY);
        Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(rectX, rectY);
        Draw::end();
        Draw::useTexture(0);
        glDisable(GL_BLEND);

        Draw::color(1.0, 0.0, 0.0);
	    Fonts::rainworld->writeCentred(std::to_string(dens[i].count), rectX + 1.0, rectY - 1.0, 0.5, CENTRE_XY);
    }

    if (inside(mousePosition)) {
        setThemeColour(ThemeColour::RoomBorderHighlight);
    } else {
        setThemeColour(ThemeColour::RoomBorder);
    }
    strokeRect(position.x, position.y, position.x + width, position.y - height);
}

void Room::addQuad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d) {
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(d);

    indices.push_back(cur_index + 0);
    indices.push_back(cur_index + 1);
    indices.push_back(cur_index + 2);
    indices.push_back(cur_index + 2);
    indices.push_back(cur_index + 3);
    indices.push_back(cur_index + 0);
    cur_index += 4;
}

void Room::addTri(const Vertex &a, const Vertex &b, const Vertex &c) {
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);

    indices.push_back(cur_index++);
    indices.push_back(cur_index++);
    indices.push_back(cur_index++);
}

void Room::generateVBO() {
    vertices.clear();
    indices.clear();
    cur_index = 0;

    glGenBuffers(2, vbo);
    glGenVertexArrays(2, &vao);

    addQuad(
        { (float) position.x,         (float) position.y,          1.0, 1.0, 1.0 },
        { (float) position.x + width, (float) position.y,          1.0, 1.0, 1.0 },
        { (float) position.x + width, (float) position.y - height, 1.0, 1.0, 1.0 },
        { (float) position.x,         (float) position.y - height, 1.0, 1.0, 1.0 }
    );

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int tileType = getTile(x, y) % 16;
            int tileData = getTile(x, y) / 16;

            float x0 = position.x + x;
            float y0 = position.y - y;
            float x1 = position.x + x + 1;
            float y1 = position.y - y - 1;
            float x2 = (x0 + x1) * 0.5;
            float y2 = (y0 + y1) * 0.5;

            if (tileType == 1) {
                addQuad(
                    { x0, y0, 0.125, 0.125, 0.125 },
                    { x1, y0, 0.125, 0.125, 0.125 },
                    { x1, y1, 0.125, 0.125, 0.125 },
                    { x0, y1, 0.125, 0.125, 0.125 }
                );
            }
            if (tileType == 4) {
                addQuad(
                    { x0, y0, 0.0, 1.0, 1.0 },
                    { x1, y0, 0.0, 1.0, 1.0 },
                    { x1, y1, 0.0, 1.0, 1.0 },
                    { x0, y1, 0.0, 1.0, 1.0 }
                );
            }
            if (tileType == 2) {
                int bits = 0;
                bits += (getTile(x - 1, y) == 1) ? 1 : 0;
                bits += (getTile(x + 1, y) == 1) ? 2 : 0;
                bits += (getTile(x, y - 1) == 1) ? 4 : 0;
                bits += (getTile(x, y + 1) == 1) ? 8 : 0;

                if (bits == 1 + 4) {
                    addQuad(
                        { x0, y0, 1.0, 0.0, 0.0 },
                        { x1, y0, 1.0, 0.0, 0.0 },
                        { x0, y1, 1.0, 0.0, 0.0 },
                        { x0, y0, 1.0, 0.0, 0.0 }
                    );
                } else if (bits == 1 + 8) {
                    addQuad(
                        { x0, y1, 1.0, 0.0, 0.0 },
                        { x1, y1, 1.0, 0.0, 0.0 },
                        { x0, y0, 1.0, 0.0, 0.0 },
                        { x0, y1, 1.0, 0.0, 0.0 }
                    );
                } else if (bits == 2 + 4) {
                    addQuad(
                        { x1, y0, 1.0, 0.0, 0.0 },
                        { x0, y0, 1.0, 0.0, 0.0 },
                        { x1, y1, 1.0, 0.0, 0.0 },
                        { x1, y0, 1.0, 0.0, 0.0 }
                    );
                } else if (bits == 2 + 8) {
                    addQuad(
                        { x1, y1, 1.0, 0.0, 0.0 },
                        { x0, y1, 1.0, 0.0, 0.0 },
                        { x1, y0, 1.0, 0.0, 0.0 },
                        { x1, y1, 1.0, 0.0, 0.0 }
                    );
                }
            }
            if (tileType == 3) {
                addQuad(
                    { x0, y0, 0.0, 1.0, 0.0 },
                    { x1, y0, 0.0, 1.0, 0.0 },
                    { x1, (y0 + y1) * 0.5f, 0.0, 1.0, 0.0 },
                    { x0, (y0 + y1) * 0.5f, 0.0, 1.0, 0.0 }
                );
            }

            if (tileData & 1) { // 16 - Vertical Pole
                addQuad(
                    { x0 + 0.375f, y0, 0.0, 0.0, 1.0 },
                    { x1 - 0.375f, y0, 0.0, 0.0, 1.0 },
                    { x1 - 0.375f, y1, 0.0, 0.0, 1.0 },
                    { x0 + 0.375f, y1, 0.0, 0.0, 1.0 }
                );
            }

            if (tileData & 2) { // 32 - Horizontal Pole
                addQuad(
                    { x0, y0 - 0.375f, 0.0, 0.0, 1.0 },
                    { x1, y0 - 0.375f, 0.0, 0.0, 1.0 },
                    { x1, y1 + 0.375f, 0.0, 0.0, 1.0 },
                    { x0, y1 + 0.375f, 0.0, 0.0, 1.0 }
                );
            }

            if (tileData & 4) { // 64 - Room Exit
                // addQuad(
                //     { x0 + 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 },
                //     { x1 - 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 },
                //     { x1 - 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 },
                //     { x0 + 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 }
                // );
            }

            if (tileData & 8) { // 128 - Shortcut
                addQuad(
                    { x0 + 0.40625f, y0 - 0.40625f, 0.125, 0.125, 0.125 },
                    { x1 - 0.40625f, y0 - 0.40625f, 0.125, 0.125, 0.125 },
                    { x1 - 0.40625f, y1 + 0.40625f, 0.125, 0.125, 0.125 },
                    { x0 + 0.40625f, y1 + 0.40625f, 0.125, 0.125, 0.125 }
                );

                addQuad(
                    { x0 + 0.4375f, y0 - 0.4375f, 1.0, 1.0, 1.0 },
                    { x1 - 0.4375f, y0 - 0.4375f, 1.0, 1.0, 1.0 },
                    { x1 - 0.4375f, y1 + 0.4375f, 1.0, 1.0, 1.0 },
                    { x0 + 0.4375f, y1 + 0.4375f, 1.0, 1.0, 1.0 }
                );
            }
        }
    }

    for (Vector2i &shortcutEntrance : shortcutEntrances) {
        float x0 = position.x + shortcutEntrance.x;
        float y0 = position.y - shortcutEntrance.y;
        float x1 = position.x + shortcutEntrance.x + 1;
        float y1 = position.y - shortcutEntrance.y - 1;
        
        addQuad(
            { x0 + 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 },
            { x1 - 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 },
            { x1 - 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 },
            { x0 + 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 }
        );
    }

    for (Vector2i &shortcutEntrance : denEntrances) {
        float x0 = position.x + shortcutEntrance.x;
        float y0 = position.y - shortcutEntrance.y;
        float x1 = position.x + shortcutEntrance.x + 1;
        float y1 = position.y - shortcutEntrance.y - 1;
        
        addQuad(
            { x0 + 0.25f, y0 - 0.25f, 1.0, 1.0, 0.0 },
            { x1 - 0.25f, y0 - 0.25f, 1.0, 1.0, 0.0 },
            { x1 - 0.25f, y1 + 0.25f, 1.0, 1.0, 0.0 },
            { x0 + 0.25f, y1 + 0.25f, 1.0, 1.0, 0.0 }
        );
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}