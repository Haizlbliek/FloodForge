namespace FloodForge.World;

public class Connection {
	public Room roomA;
	public Room roomB;

	public uint roomAExitID;
	public uint roomBExitID;

	public HashSet<string> timelines = [];
	public TimelineType timelineType = TimelineType.All;

	protected int segments;
	protected float directionStrength;

	public Rect fittedAABB;
	public Rect paddedAABB {
		get {
			float padding = WorldWindow.SelectorScale / 4f;
			return new Rect(
				this.fittedAABB.x0 - padding,
				this.fittedAABB.y0 - padding,
				this.fittedAABB.x1 + padding,
				this.fittedAABB.y1 + padding
			);
		}
	}

	public Connection(Room roomA, Room roomB, uint connectionA, uint connectionB) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.roomAExitID = connectionA;
		this.roomBExitID = connectionB;
	}

	public Connection(Room roomA, uint connectionA, Room roomB, uint connectionB) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.roomAExitID = connectionA;
		this.roomBExitID = connectionB;
	}

	public bool AllowsTimeline(string timeline) {
		return this.timelineType switch {
			TimelineType.All => true,
			TimelineType.Only => this.timelines.Contains(timeline),
			TimelineType.Except => !this.timelines.Contains(timeline),
			_ => false,
		};
	}

	// Not perfect, but it works
	public Rect AABB {
		get {
			return this.paddedAABB;
		}
	}

	Vector2[] BezierPoints = [];
	Vector2 BezierCenter;
	public bool recalculateBezier = true;

	public void RecalculateBezier() {
		Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
		Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);
		this.segments = Math.Clamp((int) ((pointA - pointB).Length / 2f), 4, 100);
		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
			this.BezierCenter = (pointA + pointB) * 0.5f;
			this.BezierPoints = [pointA, pointB];
			this.fittedAABB = new Rect(pointA, pointB);
		}
		else {
			Vector2 directionA = this.roomA.GetConnectionConnectDirection(this.roomAExitID);
			Vector2 directionB = this.roomB.GetConnectionConnectDirection(this.roomBExitID);

			this.directionStrength = (pointA - pointB).Length;
			if (this.directionStrength > 300f) {
				this.directionStrength = this.directionStrength * 0.5f + 150f;
			}
			if (directionA.x == -directionB.x || directionA.y == -directionB.y) { // increases directionStrength if shortcuts both face the same direction
				this.directionStrength *= 0.3333f;
			}
			else {
				this.directionStrength *= 0.6666f;
			}
			directionA *= this.directionStrength;
			directionB *= this.directionStrength;
			
			float overSegments = 1f / this.segments;
			List<Vector2> bezierPoints = [];
			Rect bounds = new(pointA, pointB);

			bezierPoints.Add(pointA);
			for (float t = overSegments; t < 1 + overSegments; t += overSegments) {
				t = Mathf.Clamp01(t);
				Vector2 point = MathUtil.BezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);
				bezierPoints.Add(point);
				bounds = new(
					Math.Min(bounds.x0, point.x),
					Math.Min(bounds.y0, point.y),
					Math.Max(bounds.x1, point.x),
					Math.Max(bounds.y1, point.y)
				);
				if(t==1) break;
			}
			this.BezierPoints = bezierPoints.ToArray();
			this.fittedAABB = bounds;
		}
		this.recalculateBezier = false;
	}

	public bool Hovered {
		get {
			if (!this.AABB.Inside(WorldWindow.worldMouse)) return false;

			float lineDist = WorldWindow.SelectorScale / 4f;

			Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
			Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);

			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				return MathUtil.LineDistance(WorldWindow.worldMouse, pointA, pointB) < lineDist;
			}

			Vector2 lastPoint = pointA;
			foreach (Vector2 point in this.BezierPoints) {
				if (MathUtil.LineDistance(WorldWindow.worldMouse, lastPoint, point) < lineDist)
					return true;

				lastPoint = point;
			}
			return false;
		}
	}

	protected void DrawCustomLine(float x0, float y0, float x1, float y1, float alpha0 = 1f, float alpha1 = 1f) {
		float thickness = WorldWindow.SelectorScale / 16f;
		float angle = MathF.Atan2(y1 - y0, x1 - x0);

		float a0x = x0 + Mathf.Cos(angle - Mathf.PI_2) * thickness;
		float a0y = y0 + Mathf.Sin(angle - Mathf.PI_2) * thickness;
		float b0x = x0 + Mathf.Cos(angle + Mathf.PI_2) * thickness;
		float b0y = y0 + Mathf.Sin(angle + Mathf.PI_2) * thickness;
		float a1x = x1 + Mathf.Cos(angle - Mathf.PI_2) * thickness;
		float a1y = y1 + Mathf.Sin(angle - Mathf.PI_2) * thickness;
		float b1x = x1 + Mathf.Cos(angle + Mathf.PI_2) * thickness;
		float b1y = y1 + Mathf.Sin(angle + Mathf.PI_2) * thickness;

		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.Alpha(alpha0);
		Immediate.Vertex(a0x, a0y);
		Immediate.Vertex(b0x, b0y);
		Immediate.Alpha(alpha1);
		Immediate.Vertex(b1x, b1y);
		Immediate.Vertex(a1x, a1y);
		Immediate.End();
	}

	protected void DrawTexturedRect(Texture texture, Rect rect) {
		Program.gl.Enable(EnableCap.Blend);
		Immediate.Color(1f, 1f, 1f);
		Immediate.UseTexture(texture);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);

		float ratio = (texture.width / (float) texture.height + 1f) * 0.5f;
		float uvx = 1f / ratio;
		float uvy = ratio;
		if (uvx < 1f) {
			uvy /= uvx;
			uvx = 1f;
		}
		if (uvy < 1f) {
			uvx /= uvy;
			uvy = 1f;
		}
		uvx *= 0.5f;
		uvy *= 0.5f;

		Immediate.TexCoord(0.5f - uvx, 0.5f + uvy); Immediate.Vertex(rect.x0, rect.y0);
		Immediate.TexCoord(0.5f + uvx, 0.5f + uvy); Immediate.Vertex(rect.x1, rect.y0);
		Immediate.TexCoord(0.5f + uvx, 0.5f - uvy); Immediate.Vertex(rect.x1, rect.y1);
		Immediate.TexCoord(0.5f - uvx, 0.5f - uvy); Immediate.Vertex(rect.x0, rect.y1);

		Immediate.End();
		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);
	}

	public void Draw() {
		if(this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			this.RecalculateBezier();
		}
		if(WorldWindow.CullTest(this.fittedAABB)) {
			bool aVisible = WorldWindow.VisibleLayers[this.roomA.data.layer];
			bool bVisible = WorldWindow.VisibleLayers[this.roomB.data.layer];
			float opacity = Settings.ConnectionOpacity;
			if (!aVisible && !bVisible || opacity < 0.01f) return;
			bool hovered = this.Hovered || Keys.Modifier(Keys.Modifiers.Shift);

			bool roomConnectionHoverColor = aVisible && bVisible && hovered;
			Color connectionColorA;
			Color connectionColorB;
			bool blendColors = false;

			if (roomConnectionHoverColor) {
				connectionColorA = Themes.RoomConnectionHover;
				connectionColorB = Themes.RoomConnectionHover;
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
			if (!connectionColorA.Equals(connectionColorB)) {
				blendColors = true;
			}
			if (!blendColors){
				Immediate.Color(connectionColorA);
			}

			float alphaA = aVisible ? opacity : 0f;
			float alphaB = bVisible ? opacity : 0f;
			if (opacity <= 0.999f || aVisible != bVisible) {
				Program.gl.Enable(EnableCap.Blend);
			}

			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
				Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);
				this.DrawCustomLine(pointA.x, pointA.y, pointB.x, pointB.y, alphaA, alphaB);
			}
			else {
				Vector2 lastPoint = this.BezierPoints![0];
				int curveLength = this.BezierPoints.Length;
				for (int i = 1; i < curveLength; i++) {
					float curveProgress = i / (float)curveLength;
					if (blendColors) {
						Immediate.Color(Color.Lerp(connectionColorA, connectionColorB, curveProgress));
					}
					Vector2 point = this.BezierPoints[i];
					this.DrawCustomLine(lastPoint.x, lastPoint.y, point.x, point.y, Mathf.Lerp(alphaA, alphaB, curveProgress - (1f / curveLength)), Mathf.Lerp(alphaA, alphaB, curveProgress));
					lastPoint = point;
				}
			}

			Program.gl.Disable(EnableCap.Blend);

			if (!aVisible || !bVisible) return;
			if (this.timelines.Count == 0 || this.timelineType == TimelineType.All) return;

			if (this.timelineType == TimelineType.Except) {
				Immediate.Color(1f, 0f, 0f);
				float xSize = 0.035f * WorldWindow.SelectorScale;
				UI.Line(this.BezierCenter.x - xSize, this.BezierCenter.y - xSize, this.BezierCenter.x + xSize, this.BezierCenter.y + xSize, 0.25f * WorldWindow.SelectorScale);
				UI.Line(this.BezierCenter.x + xSize, this.BezierCenter.y - xSize, this.BezierCenter.x - xSize, this.BezierCenter.y + xSize, 0.25f * WorldWindow.SelectorScale);
			}

			float size = 0.03125f * WorldWindow.SelectorScale;
			int width = Math.Max(Mathf.RoundToInt(MathF.Log2(this.timelines.Count)), 1);
			int height = Math.Max(Mathf.CeilToInt(this.timelines.Count / width), 1);

			HashSet<string>.Enumerator it = this.timelines.GetEnumerator();
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					if (!it.MoveNext()) break;

					float ox = (width * -0.5f + x + 0.5f) * size * 2.2f;
					float oy = (height * -0.5f + y + 0.5f) * size * 2.2f;
					Rect rect = Rect.FromSize(this.BezierCenter.x - size - ox, this.BezierCenter.y - size - oy, size * 2f, size * 2f);
					this.DrawTexturedRect(ConditionalTimelineTextures.GetTexture(it.Current), rect);
				}
			}
		}
	}
}