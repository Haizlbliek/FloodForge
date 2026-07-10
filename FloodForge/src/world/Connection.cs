using FloodForge.Rendering;

namespace FloodForge.World;

public class Connection {
	public Room roomA;
	public Room roomB;

	public uint roomAExitID;
	public uint roomBExitID;

	public string[] preProcessorConditions = [];
	public List<ConnectionVisual> replacementVirtualConnections = [];
	public void RefreshReplacementVirtualConnections() {
		this.replacementVirtualConnections = [];
		if (this.roomA.replacingRooms.Count == 0 && this.roomB.replacingRooms.Count == 0)
			return;
		foreach (Room replacement in this.roomA.replacingRooms) {
			this.replacementVirtualConnections.Add(new ConnectionVisual(replacement, this.roomB, this.roomAExitID, this.roomBExitID));
		}
		foreach (Room replacement in this.roomB.replacingRooms) {
			this.replacementVirtualConnections.Add(new ConnectionVisual(this.roomA, replacement, this.roomAExitID, this.roomBExitID));
		}
	}
	public void RecalculateReplacementVirtualConnectionBeziers() {
		foreach (ConnectionVisual connectionVisual in this.replacementVirtualConnections) {
			connectionVisual.recalculateBezier = true;
		}
	}
	
	public Timeline timeline;
	public Timeline EffectiveConnectionTimeline {
		get {
			return this.timeline.And(this.roomA.timeline.And(this.roomB.timeline));
		}
	}
	public bool ConnectionVisible {
		get {
			return this.timeline.OverlapsWith(WorldWindow.VisibleTimeline);
		}
	}
	public ConditionalPopup? conditionalPopup;

	protected int segments;
	protected float directionStrength;

	public Rect fittedAABB;
	public Rect PaddedAABB {
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
		this.timeline = new();
	}

	public Connection(Room roomA, uint connectionA, Room roomB, uint connectionB) {
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

	public bool Intersects(Vector2 from, Vector2 to) {
		Vector2 cornerMin = Vector2.Min(from, to);
		Vector2 cornerMax = Vector2.Max(from, to);

		if (!(cornerMax.x >= this.fittedAABB.x0 && cornerMax.y >= this.fittedAABB.y0 && cornerMin.x < this.fittedAABB.x1 && cornerMin.y <= this.fittedAABB.y1))
			return false;
		for (int i = 0; i < this.BezierPoints.Length - 1; i++) {
			Vector2 pointA = this.BezierPoints[i];
			Vector2 pointB = this.BezierPoints[i + 1];
			if (new Rect(from, to).IntersectsLine(pointA, pointB)) {
				return true;
			}
		}
		return false;
	}

	// Not perfect, but it works
	public Rect AABB {
		get {
			return this.PaddedAABB;
		}
	}

	Vector2[] BezierPoints = [];
	Vector2 BezierCenter;
	public bool recalculateBezier = true;

	public void RecalculateBezier() {
		this.RefreshReplacementVirtualConnections();
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
			Rect bounds = new Rect(pointA, pointB);

			bezierPoints.Add(pointA);
			for (float t = overSegments; t < 1 + overSegments; t += overSegments) {
				t = Mathf.Clamp01(t);
				Vector2 point = MathUtil.BezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);
				bezierPoints.Add(point);
				bounds = new Rect(
					Math.Min(bounds.x0, point.x),
					Math.Min(bounds.y0, point.y),
					Math.Max(bounds.x1, point.x),
					Math.Max(bounds.y1, point.y)
				);
				if (t == 1)
					break;
			}
			this.BezierPoints = [.. bezierPoints];
			this.fittedAABB = bounds;
		}
		this.recalculateBezier = false;

		foreach (ConnectionVisual visual in this.replacementVirtualConnections) {
			visual.RecalculateBezier();
		}
		this.GenerateMesh();
	}

	protected Mesh connectionMesh = new Mesh();
	protected MeshRenderable? connectionRenderable;

	protected unsafe void GenerateMesh() {
		this.connectionMesh.Clear();

		for (int i = 0; i < this.BezierPoints.Length - 1; i++) {
			int j = i + 1;
			Vector2 pointA = this.BezierPoints[i];
			Vector2 pointB = this.BezierPoints[j];

			float curveProgress = j / (float) this.BezierPoints.Length;
			float lastCurveProgress = i / (float) this.BezierPoints.Length;

			this.connectionMesh.AddQuad(this.CreateConnectionLineQuad(pointA.x, pointA.y, pointB.x, pointB.y, lastCurveProgress, curveProgress));
		}

		this.connectionRenderable = new MeshRenderable(this.connectionMesh, Preload.ConnectionShader, [
				new (0, 4, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) 0),
				new (1, 4, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 2)),
				new (2, 1, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 4))
			], [ "projection", "model", "tintColor", "tintColorB", "widthClip" ]);
	}

	static void DrawConnectionMesh(MeshRenderable renderable, Vector2 cameraPosition, Vector2 cameraScale, Vector2 translation, Color color, Color colorB, float widthClip) {
		renderable.PreDraw();
		renderable.UniformMatrix4("projection", false, [.. Matrix4X4.CreateOrthographicOffCenter(-cameraScale.x + cameraPosition.x, cameraScale.x + cameraPosition.x, -cameraScale.y + cameraPosition.y, cameraScale.y + cameraPosition.y, 0f, 1f)]);
		renderable.UniformMatrix4("model", false, [.. Matrix4X4.CreateTranslation(translation.x, translation.y, 0f)]);
		renderable.Uniform4("tintColor", color.r, color.g, color.b, color.a);
		renderable.Uniform4("tintColorB", colorB.r, colorB.g, colorB.b, colorB.a);
		renderable.Uniform1("widthClip", widthClip);
		renderable.DoDraw();
	}

	public bool Hovered {
		get {
			if (!this.AABB.Inside(WorldWindow.worldMouse))
				return false;

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

	public Vertex[] CreateConnectionLineQuad(float x0, float y0, float x1, float y1, float progress0, float progress1, float thickness = 5f) {
		// Review - use vector flipping instead of trigonometric functions? (since we're only ever rotating by quarter turns anyway)
		float angle = MathF.Atan2(y1 - y0, x1 - x0);

		// sin(angle + PI/2) == cos(angle);
		// sin(angle - PI/2) == cos(angle - PI);
		// sin(angle + PI) == -sin(angle);
		float sinA = Mathf.Sin(angle) * thickness;
		float cosA = Mathf.Cos(angle) * thickness;

		return [
			new Vertex(x0 + sinA, y0 - cosA, new Color(progress0, 1f, 0f, 0f)),
			new Vertex(x0 - sinA, y0 + cosA, new Color(progress0, 0f, 0f, 0f)),
			new Vertex(x1 - sinA, y1 + cosA, new Color(progress1, 0f, 0f, 0f)),
			new Vertex(x1 + sinA, y1 - cosA, new Color(progress1, 1f, 0f, 0f))
		];
	}

	public void DrawCustomLine(float x0, float y0, float x1, float y1, float alpha0 = 1f, float alpha1 = 1f) {
		float thickness = WorldWindow.SelectorScale / 16f;

		// Review - use vector flipping instead of trigonometric functions (since we're only ever rotating by quarter turns anyway)
		float angle = MathF.Atan2(y1 - y0, x1 - x0);

		// sin(angle + PI/2) == cos(angle);
		// sin(angle - PI/2) == cos(angle - PI);
		// sin(angle + PI) == -sin(angle);
		float sinA = Mathf.Sin(angle) * thickness;
		float cosA = Mathf.Cos(angle) * thickness;

		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.Alpha(alpha0);
		Immediate.Vertex(x0 + sinA, y0 - cosA);
		Immediate.Vertex(x0 - sinA, y0 + cosA);
		Immediate.Alpha(alpha1);
		Immediate.Vertex(x1 - sinA, y1 + cosA);
		Immediate.Vertex(x1 + sinA, y1 - cosA);
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

		Immediate.TexCoord(0.5f - uvx, 0.5f + uvy);
		Immediate.Vertex(rect.x0, rect.y0);
		Immediate.TexCoord(0.5f + uvx, 0.5f + uvy);
		Immediate.Vertex(rect.x1, rect.y0);
		Immediate.TexCoord(0.5f + uvx, 0.5f - uvy);
		Immediate.Vertex(rect.x1, rect.y1);
		Immediate.TexCoord(0.5f - uvx, 0.5f - uvy);
		Immediate.Vertex(rect.x0, rect.y1);

		Immediate.End();
		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);
	}

	// TODO - render connection as a mesh instead of individual quads
	public void Draw() {
		if (this.roomAExitID >= this.roomA.roomExits.Count || this.roomBExitID >= this.roomB.roomExits.Count) {
			Logger.Warn($"Connection {this.roomA.name}[{this.roomAExitID}] - {this.roomB.name}[{this.roomBExitID}] connects to invalid index! Deleting connection.");
			WorldWindow.connectionsToBeRemoved.Add(this);
			return;
		}
		if (this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			this.RecalculateBezier();
		}
		//this.RefreshReplacementVirtualConnections();
		foreach (ConnectionVisual connectionVisual in this.replacementVirtualConnections) {
			connectionVisual.Draw();
		}
		if (WorldWindow.CullTest(this.fittedAABB)) {
			bool aVisible = WorldWindow.VisibleLayers[this.roomA.data.layer] && (this.roomA.timeline.OverlapsWith(WorldWindow.VisibleTimeline) || this.ConnectionVisible);
			bool bVisible = WorldWindow.VisibleLayers[this.roomB.data.layer] && (this.roomB.timeline.OverlapsWith(WorldWindow.VisibleTimeline) || this.ConnectionVisible);
			float opacity = Settings.ConnectionOpacity;
			if (!aVisible && !bVisible || opacity < 0.01f)
				return;

			bool hovered = this.Hovered || Keys.Modifier(Keys.Modifiers.Shift);

			bool fadeMiddle = aVisible && bVisible && !this.ConnectionVisible;
			bool roomConnectionHoverColor = (!fadeMiddle) && aVisible && bVisible && hovered;
			Color connectionColorA;
			Color connectionColorB;
			bool blendColors = false;

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
			if (!connectionColorA.Equals(connectionColorB)) {
				blendColors = true;
			}
			if (!blendColors) {
				Immediate.Color(connectionColorA);
			}

			float alphaA = aVisible ? opacity : 0f;
			float alphaB = bVisible ? opacity : 0f;
			Program.gl.Enable(EnableCap.Blend);
			connectionColorA.a = alphaA;
			connectionColorB.a = alphaB;

			Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
			Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);
			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				Vector2 pointMiddle = (pointA + pointB) / 2;
				float alphaMiddle = fadeMiddle ? 0f : (alphaA + alphaB) / 2;
				this.DrawCustomLine(pointA.x, pointA.y, pointMiddle.x, pointMiddle.y, alphaA, alphaMiddle);
				this.DrawCustomLine(pointMiddle.x, pointMiddle.y, pointB.x, pointB.y, alphaMiddle, alphaB);
			}
			else {
				Vector2 matrixPos = WorldWindow.cameraOffset;
				Vector2 matrixScale = WorldWindow.cameraScale * Main.screenBounds;

				// TODO - re-add middle-fading (negative alpha, abs()'d in the shader?)
				if (this.connectionRenderable != null)
					DrawConnectionMesh(this.connectionRenderable, matrixPos, matrixScale, Vector2.Zero, connectionColorA, connectionColorB, WorldWindow.SelectorScale / 50);
			}

			Program.gl.Disable(EnableCap.Blend);

			if (!aVisible || !bVisible || !this.ConnectionVisible)
				return;
			if (this.timeline.timelines.Count == 0 || this.timeline.timelineType == TimelineType.All)
				return;

			float size = WorldWindow.SelectorScale * (this.Hovered ? 1.5f : 1f);
			int squareWidth = Mathf.CeilToInt(Mathf.Sqrt(this.timeline.timelines.Count));
			int squareHeight = 0;
			while (squareHeight * squareWidth < this.timeline.timelines.Count) {
				squareHeight++;
			}

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
}