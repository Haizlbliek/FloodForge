#pragma once

class PhysicalObject {
    public:
        PhysicalObject() {}

        double gravity = 0.9;
        double airFriction = 0.999;
        double surfaceFriction = 0.3;
};