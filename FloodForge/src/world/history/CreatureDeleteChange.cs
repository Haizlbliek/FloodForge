using FloodForge.World;

namespace FloodForge.History;

public class CreatureDeleteChange : Change {
	protected DenCreature previous;
	protected DenCreature? lastCreature;
	protected DenCreature creature;

	public CreatureDeleteChange(DenCreature creature, DenCreature? lastCreature) {
		this.creature = creature;
		this.lastCreature = lastCreature;
		if (this.lastCreature == null) {
			this.previous = this.creature.lineageTo!;
		}
		else {
			this.previous = this.lastCreature.lineageTo!;
		}

		if (this.previous == null) {
			throw new InvalidDataException("Creature must have lineageTo");
		}
	}

	public override void Undo() {
		if (this.lastCreature == null) {
			(this.creature.type, this.previous.type) = (this.previous.type, this.creature.type);
			(this.creature.tags, this.previous.tags) = (this.previous.tags, this.creature.tags);
			(this.creature.count, this.previous.count) = (this.previous.count, this.creature.count);
			this.creature.lineageTo = this.previous;
		}
		else {
			this.lastCreature.lineageTo = this.previous;
		}
	}

	public override void Redo() {
		if (this.lastCreature == null) {
			(this.creature.type, this.previous.type) = (this.previous.type, this.creature.type);
			(this.creature.tags, this.previous.tags) = (this.previous.tags, this.creature.tags);
			(this.creature.count, this.previous.count) = (this.previous.count, this.creature.count);
			this.creature.lineageTo = this.previous.lineageTo;
		}
		else {
			this.lastCreature.lineageTo = this.creature.lineageTo;
		}
	}
}