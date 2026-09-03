using FloodForge.History;
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
							if (den != null)
								WorldWindow.worldHistory.Apply(new VariableChange<int>(den.count, Math.Max(0, den.count - 1), c => { den.count = c; garbageWormCounter.settingName = $"{den.count}"; }));
						}).SetContextCheck(_ => den != null && den.count != 0)),
					("", garbageWormCounter),
					("", new TextureButtonContainer("Plus", UI.uiAtlas.UV("Plus"), () => {
							if (den != null)
								WorldWindow.worldHistory.Apply(new VariableChange<int>(den.count, den.count + 1, c => { den.count = c; garbageWormCounter.settingName = $"{den.count}"; }));
						}).SetContextCheck(_ => den != null))
				], [ 0.05f, 0f, 0.05f ], false))
			], forceEqualWidth: true));
			if (den == null) {
				this.AddToQueue(new ButtonContainer("add Worm den", () => {
					List<GarbageWormDen> newDenList = [.. this.relevantRoom.garbageWormDens];
					newDenList.Add(new GarbageWormDen() {
						count = 0,
						type = Mods.ParseCreature("garbageworm"),
						isInvalidGarbageWormDen = false
					});
					WorldWindow.worldHistory.Apply(new VariableChange<List<GarbageWormDen>>([.. this.relevantRoom.garbageWormDens], newDenList, l => {
						this.relevantRoom.garbageWormDens = l;
						this.RebuildSettings();
					}));
				}));
			}
			else {
				if (den.isInvalidGarbageWormDen) {
					this.AddToQueue(new ButtonContainer("fix Worm den", () => {
						WorldWindow.worldHistory.Apply(new VariableChange<bool>(true, false, b => {
							den.isInvalidGarbageWormDen = b;
							this.RebuildSettings();
						})); // (this just tells the exporter it's fine)
					}));
				}
				this.AddToQueue(new ButtonContainer("remove Worm den", () => {
					List<GarbageWormDen> newDenList = [.. this.relevantRoom.garbageWormDens];
					newDenList.Remove(den);
					WorldWindow.worldHistory.Apply(new VariableChange<List<GarbageWormDen>>([.. this.relevantRoom.garbageWormDens], newDenList, l => {
						this.relevantRoom.garbageWormDens = l;
						this.RebuildSettings();
					}));
				}));
			}
		}
		this.AddQueuedSettings(true);
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