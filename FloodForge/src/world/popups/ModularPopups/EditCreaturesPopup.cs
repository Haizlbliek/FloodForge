using FloodForge.Popups;

namespace FloodForge.World;

public class EditCreaturesPopup : ModularPopup {
	private Room relevantRoom;
	
	private VerticalElement denListContainer;

	// TODO - add garbage worm editing to this
	public EditCreaturesPopup(Room relevantRoom) {
		this.relevantRoom = relevantRoom;
		this.popupTitle = $"Edit Creatures - {relevantRoom.name}";
		this.denListContainer = new VerticalElement([]);
		this.AddToQueue(this.denListContainer);
		this.AddQueuedSettings();
		this.UpdateDenList();
	}

	public void UpdateDenList() {
		int denIndex = 0;
		foreach (Den den in this.relevantRoom.dens) {
			this.denListContainer.AddSetting($"{denIndex}", new ButtonContainer($"View den {denIndex}" + (den.creatures.Count == 0 ? "(empty)" : ""), () => { PopupManager.Add(new DenPopup(den)); }));
			denIndex++;
		}
		this.RecalculateBounds(true, true);
	}
}