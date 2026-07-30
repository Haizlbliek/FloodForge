namespace FloodForge.History;

public class ListSwapChange<T> : Change {
	private readonly List<T> listToEdit;
	private readonly int indexA;
	private readonly int indexB;
	public ListSwapChange(List<T> List, int swapA, int swapB) {
		this.listToEdit = List;
		this.indexA = swapA;
		this.indexB = swapB;
	}

	public void Swap() {
		(this.listToEdit[this.indexA], this.listToEdit[this.indexB]) = (this.listToEdit[this.indexB], this.listToEdit[this.indexA]);
	}

	public override void Redo() => this.Swap();

	public override void Undo() => this.Swap();
}