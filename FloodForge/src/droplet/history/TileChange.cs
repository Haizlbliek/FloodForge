using FloodForge.Droplet;

namespace FloodForge.History;

public class TileChange : Change {
    int index;
    uint oldValue;
    uint newValue;

    public TileChange(Vector2i tile, uint oldValue, uint newValue) {
        this.index = tile.x * DropletWindow.Room.height + tile.y;
        this.oldValue = oldValue;
        this.newValue = newValue;
    }

    public override void Redo() {
        DropletWindow.Room.geometry[this.index] = this.newValue;
    }

    public override void Undo() {
        DropletWindow.Room.geometry[this.index] = this.oldValue;
    }
}