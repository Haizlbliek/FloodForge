using FloodForge.Droplet;

namespace FloodForge.History;

public class ResizeChange : Change {
	readonly uint[] oldGeo;
	readonly uint[] newGeo;
	Vector2i oldSize;
	Vector2i newSize;

	public ResizeChange(uint[] oldGeo, uint[] newGeo, Vector2i oldSize, Vector2i newSize) {
		this.oldGeo = oldGeo;
		this.newGeo = newGeo;
		this.oldSize = oldSize;
		this.newSize = newSize;
	}
	public override void Redo() {
		DropletWindow.Room.width = this.newSize.x;
		DropletWindow.Room.height = this.newSize.y;
		DropletWindow.Room.geometry = this.newGeo;
	}

	public override void Undo() {
		DropletWindow.Room.width = this.oldSize.x;
		DropletWindow.Room.height = this.oldSize.y;
		DropletWindow.Room.geometry = this.oldGeo;
	}
}