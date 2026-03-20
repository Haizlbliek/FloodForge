namespace FloodForge.World;

public class SubregionChange : GeneralRoomChange<int> {
	protected Type type;
	protected string previousSubregionName = "";
	protected string subregionName = "";
	protected int subregionIndex;

	private SubregionChange() : base(r => r.data.subregion, (r, i) => r.data.subregion = i) {
	}

	public SubregionChange(string subregionName) : this() {
		this.type = Type.Add;
		this.subregionName = subregionName;
	}

	public SubregionChange(int index, string subregionName) : this() {
		this.type = Type.Rename;
		this.subregionName = subregionName;
		this.subregionIndex = index;
		this.previousSubregionName = WorldWindow.region.subregions[this.subregionIndex];
	}

	public SubregionChange(int index) : this() {
		this.type = Type.Delete;
		this.subregionName = WorldWindow.region.subregions[this.subregionIndex];
		this.subregionIndex = index;
	}

	public override void Undo() {
		base.Undo();

		switch (this.type) {
			case Type.Add:
				WorldWindow.region.subregions.RemoveAt(WorldWindow.region.subregions.Count - 1);
				break;

			case Type.Rename:
				WorldWindow.region.subregions[this.subregionIndex] = this.previousSubregionName;
				break;

			case Type.Delete:
				WorldWindow.region.subregions.Insert(this.subregionIndex, this.subregionName);
				break;
		}
	}

	public override void Redo() {
		base.Redo();

		switch (this.type) {
			case Type.Add:
				WorldWindow.region.subregions.Add(this.subregionName);
				break;

			case Type.Rename:
				WorldWindow.region.subregions[this.subregionIndex] = this.subregionName;
				break;

			case Type.Delete:
				WorldWindow.region.subregions.RemoveAt(this.subregionIndex);
				break;
		}
	}

	protected enum Type {
		Add,
		Delete,
		Rename,
	}
}