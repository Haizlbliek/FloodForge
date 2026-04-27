using FloodForge.Popups;
using FloodForge.History;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class ConditionalPopup : TimelinePopup {
	protected Connection? connection;
	protected HashSet<Room>? rooms;
	protected DenLineage? lineage;

	protected TimelineType ConditionalTimelineType {
		get {
			return this.connection?.timelineType ?? this?.lineage?.timelineType ?? this.rooms!.First().TimelineType;
		}

		set {
			TimelineTypeChange change = new TimelineTypeChange(value);

			if (this.connection != null) change.AddConnection(this.connection);
			if (this.lineage != null) change.AddLineage(this.lineage);
			this.rooms?.ForEach(change.AddRoom);

			WorldWindow.worldHistory.Apply(change);
		}
	}

	public void TimelineChangeCallback(TimelineType timelineType) {
		this.ConditionalTimelineType = timelineType;
	}

	public void SelectionChangeCallback(bool selected, string timeline) {
		TimelineChange change = new TimelineChange(!selected, timeline);
		if (this.connection != null) change.AddConnection(this.connection);
		if (this.lineage != null) change.AddLineage(this.lineage);
		this.rooms?.ForEach(change.AddRoom);
		WorldWindow.worldHistory.Apply(change);
	}
	
	public void InvokeOnTimelineChange(TimelineType timelineType, HashSet<string> timelines) {
		UpdateOnTimelineChange?.Invoke(timelineType, timelines);
	}
	public event Action<TimelineType, HashSet<string>>? UpdateOnTimelineChange;

	private ConditionalPopup(Connection? connection = null, IEnumerable<Room>? rooms = null, DenLineage? lineage = null) : base(TimelineType.All, [], (_)=>{}, (_,_)=>{}) {
		this.Timelines = connection?.timelines ?? lineage?.timelines ?? rooms?.First()?.Timelines ?? [];
		this.onTimelineTypeChangeCallback = this.TimelineChangeCallback;
		this.onSelectionChangeCallback = this.SelectionChangeCallback;
		this.bounds = new Rect(-0.4f, -0.4f, 0.4f, 0.4f);
		this.UpdateOnTimelineChange += this.UpdateTimeline;
		this.UpdateTimeline(connection?.timelineType ?? lineage?.timelineType ?? rooms!.First().TimelineType, this.Timelines);
	}

	public ConditionalPopup(Connection connection) : this(connection, null, null) {
		this.connection = connection;
	}

	public ConditionalPopup(IEnumerable<Room> rooms) : this(null, rooms, null) {
		this.rooms = [ ..rooms ];
	}

	public ConditionalPopup(DenLineage lineage) : this(null, null, lineage) {
		this.lineage = lineage;
	}

	public override void DrawButtons(float centerX, float buttonY) {
		if (this.connection != null || this.lineage != null) {
			base.DrawButtons(centerX, buttonY);
		}
		else {
			this.DrawButton(Rect.FromSize(this.bounds.x0 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "DEFAULT", TimelineType.All);
			this.DrawButton(Rect.FromSize(centerX - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "EXCLUSIVE", TimelineType.Only);
			this.DrawButton(Rect.FromSize(this.bounds.x1 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "HIDE", TimelineType.Except);
		}
	}
}