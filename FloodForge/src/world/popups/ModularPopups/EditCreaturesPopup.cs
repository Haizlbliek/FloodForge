using FloodForge.Popups;

namespace FloodForge.World;

public class EditCreaturesPopup : ModularPopup {
	private Room relevantRoom;
	
	private VerticalElement denListContainer;

	public EditCreaturesPopup(Room relevantRoom) {
		this.relevantRoom = relevantRoom;
		this.popupTitle = $"Edit Creatures - {relevantRoom.name}";
		this.denListContainer = null!; // is set in RebuildSettings()
		this.RebuildSettings();
	}

	public void RebuildSettings() {
		this.settingContainers = [];
		this.denListContainer = new VerticalElement([]);
		this.AddToQueue(this.denListContainer);
		if (this.relevantRoom.hasGarbageWormHoles) {
			// REVIEW - add support for having multiple dens? I suppose that would be useful for timeline things?
			GarbageWormDen? den = this.relevantRoom.garbageWormDens.FirstOrDefault();
			LabelContainer garbageWormCounter = new LabelContainer($"{(den == null ? "no den" : den.count)}", Font.Align.MiddleCenter);
			this.AddToQueue(new HorizontalElement([
				("", new LabelContainer($"Garbage Worms", Font.Align.MiddleLeft)),
				("", new HorizontalElement([
					("", new TextureButtonContainer("Minus", UI.uiAtlas.UV("Minus"), () => {
							if (den != null){
								den.count = Math.Max(0, den.count - 1);
								garbageWormCounter.settingName = $"{den.count}";
							}
						}).SetContextCheck(_ => den != null)),
					("", garbageWormCounter),
					("", new TextureButtonContainer("Plus", UI.uiAtlas.UV("Plus"), () => {
							if (den != null){
								den.count = Math.Max(0, den.count + 1);
								garbageWormCounter.settingName = $"{den.count}";
							}
						}).SetContextCheck(_ => den != null))
				], [ 0.05f, 0f, 0.05f ], false))
			], forceEqualWidth: true));
			if (den == null) {
				this.AddToQueue(new ButtonContainer("add Worm den", () => {
					this.relevantRoom.garbageWormDens.Add(new GarbageWormDen() {
						count = 0,
						type = Mods.ParseCreature("garbageworm"),
						isInvalidGarbageWormDen = false
					});
					this.RebuildSettings();
				}));
			}
			else {
				if (den.isInvalidGarbageWormDen) {
					this.AddToQueue(new ButtonContainer("fix Worm den", () => {
						den.isInvalidGarbageWormDen = false; // it's that easy (this just tells the exporter it's fine)
						this.RebuildSettings();
					}));
				}
				this.AddToQueue(new ButtonContainer("remove Worm den", () => {
					this.relevantRoom.garbageWormDens.Remove(den);
					this.RebuildSettings();
				}));
			}
		}
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