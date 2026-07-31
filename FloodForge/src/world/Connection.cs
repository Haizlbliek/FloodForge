namespace FloodForge.World;

public class Connection : ConnectionVisual{
	public Room roomA;
	public Room roomB;

	public uint roomAExitID;
	public uint roomBExitID;

	public override Vector2 PointA => this.roomA.GetConnectionConnectPoint(this.roomAExitID);
	public override Vector2i DirectionA => this.roomA.GetConnectionConnectDirection(this.roomAExitID);
	public override Vector2 PointB => this.roomB.GetConnectionConnectPoint(this.roomBExitID);
	public override Vector2i DirectionB => this.roomB.GetConnectionConnectDirection(this.roomBExitID);

	private bool VisibleTimelineIsEmptyOnly => WorldWindow.VisibleTimeline.timelineType == TimelineType.Only && WorldWindow.VisibleTimeline.timelines.Count == 0;
	public override bool AVisible => WorldWindow.VisibleLayers[this.roomA.data.layer] && (this.VisibleTimelineIsEmptyOnly || WorldWindow.VisibleTimeline.OverlapsWith(this.roomA.timeline) || this.ConnectionVisible);
	public override bool BVisible => WorldWindow.VisibleLayers[this.roomB.data.layer] && (this.VisibleTimelineIsEmptyOnly || WorldWindow.VisibleTimeline.OverlapsWith(this.roomB.timeline) || this.ConnectionVisible);

	public string[] preProcessorConditions = [];
	
	public Timeline timeline;
	public Timeline EffectiveConnectionTimeline {
		get {
			return this.timeline.And(this.roomA.timeline.And(this.roomB.timeline));
		}
	}
	public override bool ConnectionVisible {
		get {
			return this.timeline.OverlapsWith(WorldWindow.VisibleTimeline) || WorldWindow.VisibleTimeline.timelineType == TimelineType.Only && WorldWindow.VisibleTimeline.timelines.Count == 0;
		}
	}
	public ConditionalPopup? conditionalPopup;

	public Connection(Room roomA, Room roomB, uint connectionA, uint connectionB) : base(false) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.roomAExitID = connectionA;
		this.roomBExitID = connectionB;
		this.timeline = new();
	}

	public Connection(Room roomA, uint connectionA, Room roomB, uint connectionB) : base(false) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.roomAExitID = connectionA;
		this.roomBExitID = connectionB;
		this.timeline = new();
	}

	public bool AllowsTimeline(string timeline) {
		return this.timeline.timelineType switch {
			TimelineType.All => true,
			TimelineType.Only => this.timeline.timelines.Contains(timeline),
			TimelineType.Except => !this.timeline.timelines.Contains(timeline),
			_ => false,
		};
	}

	public override (Color, Color) GetColorInformation(bool fadeMiddle, bool AVisible, bool BVisible, bool hovered) {
		bool roomConnectionHoverColor = (!fadeMiddle) && AVisible && BVisible && hovered;
		Color connectionColorA;
		Color connectionColorB;

		if (roomConnectionHoverColor) {
			Timeline timeline = this.EffectiveConnectionTimeline;
			bool warnConflictingTimelines = timeline.timelineType == TimelineType.Only && timeline.timelines.Count == 0;
			connectionColorA = warnConflictingTimelines ? Themes.TextWarn : Themes.RoomConnectionHover;
			connectionColorB = warnConflictingTimelines ? Themes.TextWarn : Themes.RoomConnectionHover;
		}
		else {
			connectionColorA = Themes.RoomConnection;
			connectionColorB = Themes.RoomConnection;
		}

		if (WorldWindow.ColorType != WorldWindow.RoomColors.None) {
			connectionColorA = this.roomA.GetTintColor();
			connectionColorB = this.roomB.GetTintColor();
			if (!roomConnectionHoverColor) {
				connectionColorA = Color.Lerp(Themes.RoomAir, connectionColorA, Settings.RoomTintStrength);
				connectionColorB = Color.Lerp(Themes.RoomAir, connectionColorB, Settings.RoomTintStrength);
			}
		}

		float alphaA = AVisible ? Settings.ConnectionOpacity : 0f;
		float alphaB = BVisible ? Settings.ConnectionOpacity : 0f;
		connectionColorA.a = alphaA;
		connectionColorB.a = alphaB;
		return (connectionColorA, connectionColorB);
	}

	public override void CheckRecalculateBezier() {
		if (this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			bool isSingleExitConnection = this.roomA == this.roomB && this.roomAExitID == this.roomBExitID;
			this.RecalculateBezier(this.PointA, this.PointB, this.DirectionA, this.DirectionB, isSingleExitConnection);
		}
	}

	public override void Draw() {
		if (this.roomAExitID >= this.roomA.roomExits.Count || this.roomBExitID >= this.roomB.roomExits.Count) {
			Logger.Warn($"Connection {this.roomA.name}[{this.roomAExitID}] - {this.roomB.name}[{this.roomBExitID}] connects to invalid index! Deleting connection.");
			WorldWindow.connectionsToBeRemoved.Add(this);
			return;
		}
		base.Draw();
		if (WorldWindow.CullTest(this.fittedAABB)) {
			if (!this.AVisible || !this.BVisible || !this.ConnectionVisible)
				return;
			if (this.timeline.timelines.Count == 0 || this.timeline.timelineType == TimelineType.All)
				return;

			this.DrawTimelineIcons();
		}
	}

	protected void DrawTimelineIcons() {
		float size = WorldWindow.SelectorScale * (this.Hovered ? 1.5f : 1f);
		int squareWidth = Mathf.CeilToInt(Mathf.Sqrt(this.timeline.timelines.Count));
		int squareHeight = 0;
		while (squareHeight * squareWidth < this.timeline.timelines.Count) {
			squareHeight++;
		}

		Vector2 pointA = this.PointA;
		Vector2 pointB = this.PointB;
		Vector2 pointDirAB = (pointB - pointA).Normalized;
		Vector2 pointDirBA = pointDirAB * -1;
		float dotA = Vector2.Dot(pointDirAB, this.roomA.GetConnectionConnectDirection(this.roomAExitID));
		float dotB = Vector2.Dot(pointDirBA, this.roomB.GetConnectionConnectDirection(this.roomBExitID));
		bool onLeft = dotB > dotA ? (pointB.x < pointA.x) : (pointA.x < pointB.x);
		bool onTop = dotB > dotA ? (pointB.y > pointA.y) : (pointA.y > pointB.y);
		float offsetX0 = (onLeft ? this.fittedAABB.x0 : (this.fittedAABB.x1 - (squareWidth * size))) - 0.5f;
		float offsetY1 = (onTop ? this.fittedAABB.y1 : (this.fittedAABB.y0 + (squareHeight * size))) + 0.5f;

		if (WorldWindow.VisibleTimelineIcons) {
			HashSet<string>.Enumerator timelineEnumerator = this.timeline.timelines.GetEnumerator();
			for (int y = 0; y < squareHeight; y++) {
				for (int x = 0; x < squareWidth; x++) {
					if (!timelineEnumerator.MoveNext())
						break;

					Immediate.Color(1f, 1f, 1f);
					UI.CenteredTexture(Mods.GetTimelineTexture(timelineEnumerator.Current), offsetX0 + (x * size) + size / 2, offsetY1 - (y * size) - size / 2, size);

					if (this.timeline.timelineType == TimelineType.Except) {
						Immediate.Color(1f, 0f, 0f);
						float x0 = offsetX0 + 0.5f + ((x + 0.1f) * size);
						float x1 = offsetX0 + 0.5f + ((x + 0.9f) * size);
						float y0 = offsetY1 - 0.5f - ((y + 0.1f) * size);
						float y1 = offsetY1 - 0.5f - ((y + 0.9f) * size);
						UI.Line(x0, y0, x1, y1, WorldWindow.SelectorScale * 3f);
						UI.Line(x0, y1, x1, y0, WorldWindow.SelectorScale * 3f);
					}
				}

				if (this.preProcessorConditions.Length != 0) {
					Immediate.Color(1f, 1f, 0f);
					float x0 = offsetX0 + 0.5f;
					float y0 = offsetY1 - 0.5f - (y * size);
					float y1 = offsetY1 - 0.5f - ((y + 1) * size);
					UI.Line(x0, y0, x0, y1, WorldWindow.SelectorScale * 3f);
				}
			}
		}
	}
}