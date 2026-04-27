using FloodForge.World;

namespace FloodForge.History;

public class OverrideSubregionColorChange : Change {
	protected Type type;
	protected int index;
	protected Color undoValue;
	protected Color redoValue;

	public OverrideSubregionColorChange(int index) {
		this.index = index;
		this.undoValue = WorldWindow.region.overrideSubregionColors[this.index];
		this.type = Type.Delete;
	}

	public OverrideSubregionColorChange(int index, Color to) {
		this.index = index;
		this.redoValue = to;
		this.type = Type.Add;
	}

	public OverrideSubregionColorChange(int index, Color from, Color to) {
		this.index = index;
		this.redoValue = to;
		this.undoValue = from;
		this.type = Type.Change;
	}

	public override void Undo() {
		if (this.type == Type.Change || this.type == Type.Delete) {
			WorldWindow.region.overrideSubregionColors[this.index] = this.undoValue;
		}
		else if (this.type == Type.Add) {
			WorldWindow.region.overrideSubregionColors.Remove(this.index);
		}
	}

	public override void Redo() {
		if (this.type == Type.Change || this.type == Type.Add) {
			WorldWindow.region.overrideSubregionColors[this.index] = this.redoValue;
		}
		else if (this.type == Type.Delete) {
			WorldWindow.region.overrideSubregionColors.Remove(this.index);
		}
	}

	public enum Type {
		Change,
		Add,
		Delete
	}
}