using FloodForge.World;

namespace FloodForge.Popups;

public class TimelinePopup : Popup {
	protected const float buttonSize = 1f / 14f;
	protected const float buttonPadding = 0.02f;
	protected const int TimelineColumns = 8;

	protected float scroll;

	public TimelineType TimelineType;
	public HashSet<string> Timelines = [];

	public Action<bool, string> onSelectionChangeCallback;
	public Action<TimelineType> onTimelineTypeChangeCallback;

	public TimelinePopup(TimelineType TimelineType, HashSet<string> Timelines, Action<TimelineType> onTimelineTypeChangeCallback, Action<bool, string> onSelectionChangeCallback, ref Action<TimelineType, HashSet<string>>? updateEvent)
	 : this(TimelineType, Timelines, onTimelineTypeChangeCallback, onSelectionChangeCallback) {
		updateEvent += this.UpdateTimeline;
	}

	public TimelinePopup(TimelineType TimelineType, HashSet<string> Timelines, Action<TimelineType> onTimelineTypeChangeCallback, Action<bool, string> onSelectionChangeCallback) {
		this.bounds = new Rect(-0.4f, -0.4f, 0.4f, 0.4f);
		this.onSelectionChangeCallback = onSelectionChangeCallback;
		this.onTimelineTypeChangeCallback = onTimelineTypeChangeCallback;
		this.TimelineType = TimelineType;
		this.Timelines = Timelines;
	}

	protected void DrawButton(Rect rect, string text, TimelineType type) {
		if (UI.TextButton(text, rect, new UI.TextButtonMods { selected = this.TimelineType == type })) {
			this.onTimelineTypeChangeCallback.Invoke(type);
		}
	}

	public virtual void DrawButtons(float centerX, float buttonY) {
		// draw (and check for clicks on) the view type buttons (ALL, ONLY, EXCEPT)
		this.DrawButton(Rect.FromSize(this.bounds.x0 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "ALL", TimelineType.All);
		this.DrawButton(Rect.FromSize(centerX - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "ONLY", TimelineType.Only);
		this.DrawButton(Rect.FromSize(this.bounds.x1 * 0.6f + centerX * 0.4f - 0.1f, buttonY - 0.025f, 0.2f, 0.05f), "EXCEPT", TimelineType.Except);
	}

	public void UpdateTimeline(TimelineType type, HashSet<string> timelines) {
		this.TimelineType = type;
		this.Timelines = timelines;
	}

	public override void Draw() {
		base.Draw();

		if (this.collapsed)
			return;

		float centerX = this.bounds.CenterX;
		string hover = "";
		float buttonY = this.bounds.y1 - 0.1f;

		this.DrawButtons(centerX, buttonY);

		// If the TimelineType is all, we don't care about specifics.
		if (this.TimelineType != TimelineType.All) {
			// otherwise:
			string timeline; // the currently hovered timeline
			HashSet<string> timelines = this.Timelines ?? []; // get the current Timeline selection
			List<string> unknowns = [.. timelines.Where(t => !ConditionalTimelineTextures.HasTimeline(t))]; // create list of all timelines for which no texture exists
			int count = ConditionalTimelineTextures.timelines.Count + unknowns.Count; // total count

			// go through every square in the timeline popup per row and per column (probably not resize-proof)
			for (int row = 0; row <= (count / TimelineColumns); row++) {
				for (int column = 0; column < TimelineColumns; column++) {
					int id = column + row * TimelineColumns;
					if (id >= count)
						break; // Make sure it doesn't try to check nonexistent timelinse

					// if unknown (and as such without texture), get texturename from unknowns instead
					bool unknown = id >= ConditionalTimelineTextures.timelines.Count;
					timeline = unknown ? unknowns[id - ConditionalTimelineTextures.timelines.Count] : ConditionalTimelineTextures.timelines[id];

					// Check if current button is already selected
					bool selected = timelines.Contains(timeline);

					// create button
					Texture texture = ConditionalTimelineTextures.GetTexture(timeline);
					UVRect rect = UVRect.FromSize(
						centerX + (column - 0.5f * TimelineColumns) * (buttonSize + buttonPadding) + buttonPadding * 0.5f,
						this.bounds.y1 - 0.12f - buttonPadding * 0.5f - (row + 1) * (buttonSize + buttonPadding) - this.scroll,
						buttonSize, buttonSize
					);
					UI.CenteredUV(texture, ref rect);
					UI.ButtonResponse response = UI.TextureButton(rect, new UI.TextureButtonMods { selected = selected, texture = texture, textureColor = selected ? Color.White : Color.Grey });

					// if clicked, run the callback.
					if (response.clicked) {
						this.onSelectionChangeCallback.Invoke(selected, timeline);
					}

					if (response.hovered) {
						hover = timeline;
					}
				}
			}
		}

		if (!string.IsNullOrEmpty(hover) && this.isHovered) {
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