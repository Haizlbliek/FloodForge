namespace FloodForge.World;

public class LineageChange : Change {
	protected bool creating;
	protected Den den;
	protected DenLineage lineage;
	protected int index;

	public LineageChange(Den den) {
		this.den = den;
		this.lineage = new DenLineage("", 0);
		this.index = -1;
		this.creating = true;
	}

	public LineageChange(Den den, int index) {
		this.den = den;
		this.lineage = this.den.creatures[index];
		this.index = index;
		this.creating = false;
	}

	public override void Undo() {
		if (this.creating) {
			this.den.creatures.RemoveAt(this.den.creatures.Count - 1);
		}
		else {
			this.den.creatures.Insert(this.index, this.lineage);
		}
	}

	public override void Redo() {
		if (this.creating) {
			this.den.creatures.Add(this.lineage);
		}
		else {
			this.den.creatures.RemoveAt(this.index);
		}
	}
}