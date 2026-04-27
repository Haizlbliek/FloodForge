using FloodForge.World;

namespace FloodForge.History;

public class CreatureDataChange : Change {
	protected DenCreature creature;
	public string undoType, redoType;
	public int undoCount, redoCount;
	public IEnumerable<DenCreature.Tag> undoTags, redoTags;

	public CreatureDataChange(DenCreature creature, string type, int count, List<DenCreature.Tag> tags) {
		this.creature = creature;
		this.undoType = creature.type;
		this.redoType = type;
		this.undoCount = creature.count;
		this.redoCount = count;
		this.undoTags = creature.tags.Select(x => x.Clone());
		this.redoTags = tags.Select(x => x.Clone());
	}

	public override void Undo() {
		this.creature.type = this.undoType;
		this.creature.count = this.undoCount;
		this.creature.tags.Clear();
		foreach (DenCreature.Tag tag in this.undoTags) {
			this.creature.tags.Add(tag.Clone());
		}
	}

	public override void Redo() {
		this.creature.type = this.redoType;
		this.creature.count = this.redoCount;
		this.creature.tags.Clear();
		foreach (DenCreature.Tag tag in this.redoTags) {
			this.creature.tags.Add(tag.Clone());
		}
	}
}