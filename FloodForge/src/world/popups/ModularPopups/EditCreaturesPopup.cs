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
		// IDEA - add timeline icon to lineages?
		foreach (Den den in this.relevantRoom.dens) {
			HorizontalElement denImages = new HorizontalElement([], null, false, true);
			foreach (DenLineage lineage in den.creatures) {
				// IDEA - add lineage creatures behind first creature?
				denImages.AddSetting("", new ImageContainer(Mods.GetCreatureTexture(lineage.type)));
			}
			if (den.creatures.Count == 0) {
				denImages.AddSetting("", new LabelContainer("EMPTY", align: Font.Align.MiddleCenter));
			}
			this.denListContainer.AddSetting($"{denIndex}", new HorizontalElement([ ("", new ButtonContainer($"den {denIndex}", () => { PopupManager.Add(new DenPopup(den)); })), ("", denImages) ], null, true, true));
			denIndex++;
		}
		this.RecalculateBounds(true, true);
	}
}