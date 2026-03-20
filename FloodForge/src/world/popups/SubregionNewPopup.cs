using Stride.Core.Extensions;

namespace FloodForge.World;

public class SubregionNewPopup : AcronymPopup {
	protected HashSet<Room> rooms;
	protected int? editIndex;

	protected override int MinLength => 1;
	protected override string BanLetters => ":<>";

	public SubregionNewPopup(IEnumerable<Room> rooms, int? editIndex = null) {
		if (rooms.IsNullOrEmpty()) throw new NotImplementedException("SubregionPopup must have at least 1 room");

		this.bounds = new Rect(-0.4f, -0.08f, 0.4f, 0.25f);
		this.rooms = [..rooms];
		this.editIndex = editIndex;
		if (editIndex != null) this.setTo = WorldWindow.region.subregions[(int) editIndex];
	}

	protected override void Submit(string subregion) {
		if (subregion.Length == 0) return;

		this.Close();
		if (this.editIndex == null) {
			if (WorldWindow.region.subregions.Contains(subregion)) return;

			int subregionIndex = WorldWindow.region.subregions.Count;
			SubregionChange change = new SubregionChange(subregion);
			this.rooms.ForEach(r => change.AddRoom(r, subregionIndex));
			History.Apply(change);
		}
		else {
			if (WorldWindow.region.subregions.Contains(subregion)) return;

			SubregionChange change = new SubregionChange((int) this.editIndex, subregion);
			History.Apply(change);
		}
	}
}