namespace FloodForge.World;

public class CreatureDataChange : Change {
	protected DenCreature creature;
	public string undoType, redoType;
	public int undoCount, redoCount;
	public string undoTag, redoTag;
	public float undoData, redoData;

	public CreatureDataChange(DenCreature creature, string type, int count, string tag, float data) {
		this.creature = creature;
		this.undoType = creature.type;
		this.redoType = type;
		this.undoCount = creature.count;
		this.redoCount = count;
		this.undoTag = creature.tag;
		this.redoTag = tag;
		this.undoData = creature.data;
		this.redoData = data;
	}

	public override void Undo() {
		this.creature.type = this.undoType;
		this.creature.count = this.undoCount;
		this.creature.tag = this.undoTag;
		this.creature.data = this.undoData;
	}

	public override void Redo() {
		this.creature.type = this.redoType;
		this.creature.count = this.redoCount;
		this.creature.tag = this.redoTag;
		this.creature.data = this.redoData;
	}
}