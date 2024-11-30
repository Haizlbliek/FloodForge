#include "Room.hpp"

void Room::draw(Vector2 mousePosition, double lineSize) {
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
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);


    glUseProgram(Shaders::roomShader);

    GLuint projLoc = glGetUniformLocation(Shaders::roomShader, "projection");
    GLuint modelLoc = glGetUniformLocation(Shaders::roomShader, "model");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix(cameraOffset, cameraScale).m);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix(position->x, position->y).m);

    glColor(Colour(1.0).mix(tint, 0.5));
    glTranslatef(position->x, position->y, 0.0f);

    glDrawArrays(GL_QUADS, 0, vertices.size());
    glUseProgram(0);

    glTranslatef(-position->x, -position->y, 0.0f);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    if (water != -1) {
        glColor(Colour(0.0, 0.0, 0.5, 0.5));
        fillRect(position->x, position->y - (height - std::min(water, height)), position->x + width, position->y - height);
    }

    if (inside(mousePosition)) {
        glColor(Colour(0.00, 0.75, 0.00));
    } else {
        glColor(Colour(0.75, 0.75, 0.75));
    }
    strokeRect(position->x, position->y, position->x + width, position->y - height);
}