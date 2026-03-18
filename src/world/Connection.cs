namespace FloodForge.World;

public class Connection {
	public Room roomA;
	public Room roomB;

	public uint connectionA;
	public uint connectionB;

	public HashSet<string> timelines = [];
	public TimelineType timelineType = TimelineType.All;

	protected int segments;
	protected float directionStrength;

	public Connection(Room roomA, Room roomB, uint connectionA, uint connectionB) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.connectionA = connectionA;
		this.connectionB = connectionB;
	}

	public Connection(Room roomA, uint connectionA, Room roomB, uint connectionB) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.connectionA = connectionA;
		this.connectionB = connectionB;
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
			float padding = WorldWindow.SelectorScale / 4f;
			Vector2 pointA = this.roomA.GetRoomEntrancePosition(this.connectionA);
			Vector2 pointB = this.roomB.GetRoomEntrancePosition(this.connectionB);

			Vector2 min, max;

			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				min = Vector2.Min(pointA, pointB);
				max = Vector2.Max(pointA, pointB);
			} else {
				Vector2 directionA = this.roomA.GetRoomEntranceDirectionVector(this.connectionA) * this.directionStrength;
				Vector2 directionB = this.roomB.GetRoomEntranceDirectionVector(this.connectionB) * this.directionStrength;

				Vector2 cp1 = pointA + directionA;
				Vector2 cp2 = pointB + directionB;

				min = Vector2.Min(Vector2.Min(pointA, pointB), Vector2.Min(cp1, cp2));
				max = Vector2.Max(Vector2.Max(pointA, pointB), Vector2.Max(cp1, cp2));
			}

			return new Rect(
				min.x - padding,
				min.y - padding,
				max.x + padding,
				max.y + padding
			);
		}
	}

	public bool Hovered {
		get {
			if (!this.AABB.Inside(WorldWindow.worldMouse)) return false;

			float lineDist = WorldWindow.SelectorScale / 4f;

			Vector2 pointA = this.roomA.GetRoomEntrancePosition(this.connectionA);
			Vector2 pointB = this.roomB.GetRoomEntrancePosition(this.connectionB);

			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				return MathUtil.LineDistance(WorldWindow.worldMouse, pointA, pointB) < lineDist;
			}

			Vector2 directionA = this.roomA.GetRoomEntranceDirectionVector(this.connectionA) * this.directionStrength;
			Vector2 directionB = this.roomB.GetRoomEntranceDirectionVector(this.connectionB) * this.directionStrength;

			Vector2 lastPoint = MathUtil.BezierCubic(0f, pointA, pointA + directionA, pointB + directionB, pointB);
			float overSegments = 1f / this.segments;
			for (float t = overSegments; t <= 1.01f; t += overSegments) {
				Vector2 point = MathUtil.BezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);

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
		bool aVisible = WorldWindow.VisibleLayers[this.roomA.data.layer];
		bool bVisible = WorldWindow.VisibleLayers[this.roomB.data.layer];
		float opacity = Settings.ConnectionOpacity;
		if (!aVisible && !bVisible || opacity < 0.01f) return;
		bool hovered = this.Hovered;
		if (aVisible && bVisible && hovered) {
			Immediate.Color(Themes.RoomConnectionHover);
		} else {
			Immediate.Color(Themes.RoomConnection);
		}
		float alphaA = aVisible ? opacity : 0f;
		float alphaB = bVisible ? opacity : 0f;
		if (opacity <= 0.999f || aVisible != bVisible) {
			Program.gl.Enable(EnableCap.Blend);
		}

		Vector2 pointA = this.roomA.GetRoomEntrancePosition(this.connectionA);
		Vector2 pointB = this.roomB.GetRoomEntrancePosition(this.connectionB);
		this.segments = Math.Clamp((int) ((pointA - pointB).Length / 2f), 4, 100);
		this.directionStrength = (pointA - pointB).Length;
		if (this.directionStrength > 300f) {
			this.directionStrength = this.directionStrength * 0.5f + 150f;
		}

		//Vector2 pointEndA = (this.roomA.roomExits[(int) this.connectionA] + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1);
		//Vector2 pointEndB = (this.roomB.roomExits[(int) this.connectionB] + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1);
		Vector2 center;
		/*if (hovered) {
			if (this.roomA.shortcutPaths.ContainsKey(this.roomA.GetRoomEntranceShortcutPosition(this.connectionA))) {
				Immediate.Color(Themes.RoomConnectionHover);
				foreach (Vector2i dot in this.roomA.shortcutPaths[this.roomA.GetRoomEntranceShortcutPosition(this.connectionA)]) { // DRAWING SHORTCUT PATH
					UI.FillCircle((dot + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1) + this.roomA.Position, 0.3f, 8);
				}// end of shortcut path draw
			}
			if (this.roomB.shortcutPaths.ContainsKey(this.roomB.GetRoomEntranceShortcutPosition(this.connectionB))) {
				foreach (Vector2i dot in this.roomB.shortcutPaths[this.roomB.GetRoomEntranceShortcutPosition(this.connectionB)]) { // DRAWING SHORTCUT PATH
					UI.FillCircle((dot + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1) + this.roomB.Position, 0.3f, 8);
				}// end of shortcut path draw
			}
		}*/

		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
			this.DrawCustomLine(pointA.x, pointA.y, pointB.x, pointB.y, alphaA, alphaB);
			center = (pointA + pointB) * 0.5f;
		} else {
			Vector2 directionA = this.roomA.GetRoomEntranceDirectionVector(this.connectionA);
			Vector2 directionB = this.roomB.GetRoomEntranceDirectionVector(this.connectionB);

			if (directionA.x == -directionB.x || directionA.y == -directionB.y) { // increases directionStrength if shortcuts both face the same direction
				this.directionStrength *= 0.3333f;
			} else {
				this.directionStrength *= 0.6666f;
			}

			directionA *= this.directionStrength;
			directionB *= this.directionStrength;

			Vector2 lastPoint = MathUtil.BezierCubic(0f, pointA, pointA + directionA, pointB + directionB, pointB);
			float overSegments = 1f / this.segments;
			for (float t = overSegments; t <= 1.01f; t += overSegments) {
				Vector2 point = MathUtil.BezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);
				this.DrawCustomLine(lastPoint.x, lastPoint.y, point.x, point.y, Mathf.Lerp(alphaA, alphaB, t - overSegments), Mathf.Lerp(alphaA, alphaB, t));
				lastPoint = point;
			}

			center = MathUtil.BezierCubic(0.5f, pointA, pointA + directionA, pointB + directionB, pointB);
		}

		Program.gl.Disable(EnableCap.Blend);

		if (!aVisible || !bVisible) return;
		if (this.timelines.Count == 0 || this.timelineType == TimelineType.All) return;

		if (this.timelineType == TimelineType.Except) {
			Immediate.Color(1f, 0f, 0f);
			float xSize = 0.035f * WorldWindow.SelectorScale;
			UI.Line(center.x - xSize, center.y - xSize, center.x + xSize, center.y + xSize, 0.25f * WorldWindow.SelectorScale);
			UI.Line(center.x + xSize, center.y - xSize, center.x - xSize, center.y + xSize, 0.25f * WorldWindow.SelectorScale);
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
				Rect rect = Rect.FromSize(center.x - size - ox, center.y - size - oy, size * 2f, size * 2f);
				this.DrawTexturedRect(ConditionalTimelineTextures.GetTexture(it.Current), rect);
			}
		}
	}
}