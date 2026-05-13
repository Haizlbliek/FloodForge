using FloodForge.Popups;

namespace FloodForge.World;

public class VisibleTimelinePopup : TimelinePopup {
	public SettingsPopup.BoolSettingContainer setNewConnectionsContainer;

	public VisibleTimelinePopup(Timeline timeline, Action<TimelineType> onTimelineTypeChangeCallback, Action<bool, string> onSelectionChangeCallback, ref Action<Timeline>? updateEvent) : base(timeline, onTimelineTypeChangeCallback, onSelectionChangeCallback, ref updateEvent) {
		this.setNewConnectionsContainer = new("Set new connections to timeline", WorldWindow.setNewConnectionTimeline, b => {
			WorldWindow.setNewConnectionTimeline = b;
		});
	}

	public override void Draw() {
		base.Draw();

		this.setNewConnectionsContainer.Draw(new Rect(this.bounds.x0 + 0.01f, this.bounds.y0 + 0.01f, this.bounds.x1 - 0.01f, this.bounds.y0 + 0.05f));
	}
}