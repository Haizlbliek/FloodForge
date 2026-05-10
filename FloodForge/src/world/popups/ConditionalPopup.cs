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
			return this.connection?.timeline.timelineType ?? this?.lineage?.timeline.timelineType ?? this.rooms!.First().timeline.timelineType;
		}

		set {
			TimelineTypeChange change = new TimelineTypeChange(value);

			if (this.connection != null) change.AddConnection(this.connection);
			if (this.lineage != null) change.AddLineage(this.lineage);
			this.rooms?.ForEach(change.AddRoom);

			WorldWindow.worldHistory.Apply(change);
		}
	}

	// REVIEW - check if a timeline change makes sense - for example, a connection set to "X-Red" while it connects to a room with timeline "Red" doesn't make sense
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
	
	public void InvokeOnTimelineChange(Timeline timeline) {
		UpdateOnTimelineChange?.Invoke(timeline);
	}

	public event Action<Timeline>? UpdateOnTimelineChange;

	private ConditionalPopup(Connection? connection = null, IEnumerable<Room>? rooms = null, DenLineage? lineage = null) : base(new (), (_)=>{}, (_,_)=>{}) {
		this.timeline.timelines = connection?.timeline.timelines ?? lineage?.timeline.timelines ?? rooms?.First()?.timeline.timelines ?? [];
		this.onTimelineTypeChangeCallback = this.TimelineChangeCallback;
		this.onSelectionChangeCallback = this.SelectionChangeCallback;
		this.bounds = new Rect(-0.4f, -0.4f, 0.4f, 0.4f);
		this.UpdateOnTimelineChange += this.UpdateTimeline;
		this.UpdateTimeline(new (connection?.timeline.timelineType ?? lineage?.timeline.timelineType ?? rooms!.First().timeline.timelineType, this.timeline.timelines));
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