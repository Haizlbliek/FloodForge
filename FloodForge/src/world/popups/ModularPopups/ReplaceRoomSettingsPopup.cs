using FloodForge.History;
using FloodForge.Popups;

namespace FloodForge.World;

public class ReplaceRoomSettingsPopup : ModularPopup {
	protected ReplaceRoom replaceRoom;
	protected LabelContainer replacesRoomLabel;
	protected ButtonContainer timelineButton;
	protected ButtonContainer deleteButton;
	protected TextureButtonContainer hideButton;
	protected TimelinePopup? timelinePopup = null;

	public ReplaceRoomSettingsPopup(ReplaceRoom replaceRoom) {
		this.replaceRoom = replaceRoom;
		this.popupTitle = "Settings - ReplaceRoom";

		this.replacesRoomLabel = new("");
		this.AddToQueue(this.replacesRoomLabel);

		this.timelineButton = new ButtonContainer("", this.TimelineButton).SetContextCheck(this.UpdateTimelineButton, false);
		this.AddToQueue(this.timelineButton);

		this.deleteButton = new ButtonContainer("Delete", this.DeleteReplaceRoom);
		this.hideButton = new TextureButtonContainer("", UI.uiAtlas.UV(this.replaceRoom.setHidden ? "EyeClosed" : "EyeOpen"), () => replaceRoom.ToggleHide()).SetContextCheck(this.UpdateHideButton);
		this.AddToQueue(new HorizontalElement([("hide", this.hideButton), ("delete", this.deleteButton)], [0.05f, 0f]));

		this.AddQueuedSettings();
		this.UpdateLabels();
	}

	private bool UpdateHideButton(TextureButtonContainer button) {
		button.SetUV(UI.uiAtlas.UV(this.replaceRoom.setHidden ? "EyeClosed" : "EyeOpen"));
		return true;
	}

	private void TimelineButton() {
		this.timelinePopup ??= new TimelinePopup(this.replaceRoom.timeline, _ => {}, this.SelectionChange, true).SetButtons<TimelinePopup>("", "ONLY", "");
		PopupManager.Add(this.timelinePopup);
	}

	private void SelectionChange(bool selected, string timeline) {
		TimelineChange newTimelineChange = new TimelineChange(!selected, timeline);
		newTimelineChange.AddReplaceRoom(this.replaceRoom);
		WorldWindow.worldHistory.Apply(newTimelineChange);
	}

	private bool UpdateTimelineButton(ButtonContainer button) {
		button.settingName = "Timeline" + (this.replaceRoom.timeline.timelines.Count == 0 ? "" : $" - {this.replaceRoom.timeline}");
		this.RecalculateBounds(retainTopLeft: true, tryRetainScale: true);
		return true;
	}

	private void DeleteReplaceRoom() {
		ReplaceRoomChange change = new ReplaceRoomChange(false);
		change.AddReplaceRoom(this.replaceRoom);
		WorldWindow.worldHistory.Apply(change);
		this.timelinePopup?.Close();
		this.Close();
	}

	private void UpdateLabels() {
		this.replacesRoomLabel.settingName = $"Replaces {this.replaceRoom.replacedRoom.name}";
		this.RecalculateBounds(tryRetainScale: true);
	}

	public override void Draw() {
		Immediate.Color(Color.Cyan);
		UI.Line(new(this.bounds.x0, this.bounds.y1), (this.replaceRoom.Position - WorldWindow.cameraOffset) / WorldWindow.cameraScale);
		base.Draw();
	}
}