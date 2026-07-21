namespace FloodForge.Popups;

/// <summary>
/// A class intended for more convenient construction of popups.
/// </summary>
public class ModularPopup : SettingsPopup {
	private List<SettingContainer> settingsQueue;
	public ModularPopup() : base([]) {
		this.settingsQueue = [];
	}

	protected void AddQueuedSettings() {
		List<SettingContainer> finalList = [.. this.settingContainers];
		this.settingsQueue.ForEach(finalList.Add);
		this.settingsQueue = [];
		this.settingContainers = [.. finalList];
		this.RecalculateBounds();
	}

	protected void AddToQueue(SettingContainer container) {
		this.settingsQueue.Add(container);
	}

	protected void Remove(SettingContainer container) {
		List<SettingContainer> finalList = [.. this.settingContainers];
		finalList.Remove(container);
		this.settingContainers = [.. finalList];
	}
}