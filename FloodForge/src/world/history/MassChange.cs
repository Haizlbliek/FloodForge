namespace FloodForge.World;

public class MassChange : Change {
    readonly Change[] changes;
    public MassChange(Change[] changes) {
        this.changes = changes;
    }

    public override void Redo() {
        foreach (Change change in this.changes) {
            change.Redo();
        }
    }

    public override void Undo() {
        foreach (Change change in this.changes) {
            change.Undo();
        }
    }
}