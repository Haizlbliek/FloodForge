using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

public class ConditionalPopup : Popup {
	protected const float buttonSize = 1f / 14f;
	protected const float buttonPadding = 0.02f;
	protected const int TimelineRows = 8;

	protected Connection? connection;
	protected HashSet<Room>? rooms;
	protected DenLineage? lineage;

	protected float scroll;

	protected TimelineType TimelineType {
		get {
			return this.connection?.timelineType ?? this?.lineage?.timelineType ?? this.rooms!.First().TimelineType;
		}

		set {
			TimelineTypeChange change = new TimelineTypeChange(value);

			if (this.connection != null) change.AddConnection(this.connection);
			if (this.lineage != null) change.AddLineage(this.lineage);
			this.rooms?.ForEach(change.AddRoom);

			History.Apply(change);
		}
	}

	private ConditionalPopup() {
		this.bounds = new Rect(-0.4f, -0.4f, 0.4f, 0.4f);
	}

	public ConditionalPopup(Connection connection) : this() {
		this.connection = connection;
	}

	public ConditionalPopup(IEnumerable<Room> rooms) : this() {
		this.rooms = [ ..rooms ];
	}

	public ConditionalPopup(DenLineage lineage) : this() {
		this.lineage = lineage;
	}

	protected void DrawButton(Rect rect, string text, TimelineType type) {
		if (UI.TextButton(text, rect, new UI.TextButtonMods { selected = this.TimelineType == type })) {
			this.TimelineType = type;
		}
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		float centerX = this.bounds.CenterX;
		string hover = "";
		float buttonY = this.bounds.y1 - 0.1f;
		Room? firstRoom = this.rooms?.FirstOrDefault();

		if (this.connection != null || this.lineage != null) {
			this.DrawButton(Rect.FromSize(this.bounds.x0 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "ALL", TimelineType.All);
			this.DrawButton(Rect.FromSize(centerX - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "ONLY", TimelineType.Only);
			this.DrawButton(Rect.FromSize(this.bounds.x1 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "EXCEPT", TimelineType.Except);
		}
		else {
			this.DrawButton(Rect.FromSize(this.bounds.x0 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "DEFAULT", TimelineType.All);
			this.DrawButton(Rect.FromSize(centerX - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "EXCLUSIVE", TimelineType.Only);
			this.DrawButton(Rect.FromSize(this.bounds.x1 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "HIDE", TimelineType.Except);
		}

		if (this.TimelineType != TimelineType.All) {
			string timeline;
			HashSet<string> timelines = this.connection?.timelines ?? this.lineage?.timelines ?? firstRoom?.Timelines ?? [];
			List<string> unknowns = [.. timelines.Where(t => !ConditionalTimelineTextures.HasTimeline(t))];
			int count = ConditionalTimelineTextures.timelines.Count + unknowns.Count;
			for (int y = 0; y <= (count / TimelineRows); y++) {
				for (int x = 0; x < TimelineRows; x++) {
					int id = x + y * TimelineRows;
					if (id >= count) break;
					bool unknown = id >= ConditionalTimelineTextures.timelines.Count;
					timeline = unknown ? unknowns[id - ConditionalTimelineTextures.timelines.Count] : ConditionalTimelineTextures.timelines[id];
					bool selected = timelines.Contains(timeline);
					Texture texture = ConditionalTimelineTextures.GetTexture(timeline);
					UVRect rect = UVRect.FromSize(
						centerX + (x - 0.5f * TimelineRows) * (buttonSize + buttonPadding) + buttonPadding * 0.5f,
						this.bounds.y1 - 0.12f - buttonPadding * 0.5f - (y + 1) * (buttonSize + buttonPadding) - this.scroll,
						buttonSize, buttonSize
					);
					UI.CenteredUV(texture, ref rect);
					UI.ButtonResponse response = UI.TextureButton(rect, new UI.TextureButtonMods { selected = selected, texture = texture, textureColor = selected ? Color.White : Color.Grey });

					if (response.clicked) {
						TimelineChange change = new TimelineChange(!selected, timeline);
						if (this.connection != null) change.AddConnection(this.connection);
						if (this.lineage != null) change.AddLineage(this.lineage);
						this.rooms?.ForEach(change.AddRoom);
						History.Apply(change);
					}

					if (response.hovered) {
						hover = timeline;
					}
				}
			}
		}

		if (!hover.IsNullOrEmpty() && this.hovered) {
			float width = UI.font.Measure(hover, 0.04f).x + 0.02f;
			Rect rect = Rect.FromSize(Mouse.X, Mouse.Y, width, 0.06f);
			Immediate.Color(Themes.Popup);
			UI.FillRect(rect);
			Immediate.Color(Themes.Border);
			UI.StrokeRect(rect);
			Immediate.Color(Themes.Text);
			UI.font.Write(hover, Mouse.X + 0.01f, Mouse.Y + 0.03f, 0.04f, Font.Align.MiddleLeft);
		}
	}
}