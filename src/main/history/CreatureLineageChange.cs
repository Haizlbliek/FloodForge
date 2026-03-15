namespace FloodForge.World;

public class CreatureLineageChange : Change {
	protected bool chance = false;
	protected DenCreature creature;
	protected DenCreature? creatureAdd;
	protected float undoChance;
	protected float redoChance;

	public CreatureLineageChange(DenCreature creature, float chance) {
		this.chance = true;
		this.creature = creature;
		this.undoChance = this.creature.lineageChance;
		this.redoChance = chance;
	}

	public CreatureLineageChange(DenCreature creature) {
		this.chance = false;
		this.creature = creature;
		this.creatureAdd = new DenCreature("", 0, "", 0f);
	}

	public override void Undo() {
		if (this.chance) {
			this.creature.lineageChance = this.undoChance;
		}
		else {
			this.creature.lineageTo = null;
		}
	}

	public override void Redo() {
		if (this.chance) {
			this.creature.lineageChance = this.redoChance;
		}
		else {
			this.creature.lineageTo = this.creatureAdd;
		}
	}
}