namespace FloodForge.History;

public class VariableChange<T> : Change {
	readonly T oldValue;
	readonly T newValue;
	readonly Action<T> onChangeCallback;
    public VariableChange(T oldValue, T newValue, Action<T> onChangeCallback) {
        this.oldValue = oldValue;
        this.newValue = newValue;
        this.onChangeCallback = onChangeCallback;
    }
	public override void Redo() {
        this.onChangeCallback(this.newValue);
    }
    public override void Undo() {
        this.onChangeCallback(this.oldValue);
    }
}