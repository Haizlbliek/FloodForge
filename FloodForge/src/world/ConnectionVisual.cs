using FloodForge.Rendering;

namespace FloodForge.World;

// TODO - re-add virtual connection visuals to replaceRooms
public abstract class ConnectionVisual {
	public virtual Vector2 PointA => Vector2.Zero;
	public virtual Vector2i DirectionA => Vector2i.Zero;
	public virtual Vector2 PointB => Vector2.Zero;
	public virtual Vector2i DirectionB => Vector2i.Zero;

	protected int segments;
	protected float directionStrength;

	protected Vector2[] BezierPoints = [];
	public bool recalculateBezier = true;

	protected Mesh connectionMesh = new Mesh();
	protected MeshRenderable? connectionRenderable;

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

	// Not perfect, but it works
	public Rect AABB {
		get {
			return this.PaddedAABB;
		}
	}

	public virtual bool AVisible {
		get {
			return true;
		}
	}

	public virtual bool BVisible {
		get {
			return true;
		}
	}

	public virtual bool ConnectionVisible {
		get {
			return true;
		}
	}

	protected bool drawStriped = true;

	public virtual (Color, Color) GetColorInformation(bool fadeMiddle, bool AVisible, bool BVisible, bool hovered) {
		Color colorA = Themes.RoomConnection;
		Color colorB = Themes.RoomConnection;
		colorA.a = AVisible ? Settings.ConnectionOpacity : 0f;
		colorB.a = BVisible ? Settings.ConnectionOpacity : 0f;
		return (colorA, colorB);
	}

	public ConnectionVisual(bool drawStriped) {
		this.drawStriped = drawStriped;
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

	// REVIEW - generate and interpret bezier points relative to pointA (so that collective movement does not recalculate beziers all the time)
	public void RecalculateBezier(Vector2 pointA, Vector2 pointB, Vector2 directionA, Vector2 directionB, bool createLoop) {
		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
			if (createLoop) {
				pointB += directionA * 3;
			}
			this.BezierPoints = [pointA, pointB];
			this.fittedAABB = new Rect(pointA, pointB);
		}
		else {
			this.directionStrength = (pointA - pointB).Length;
			if (createLoop) {
				this.directionStrength = 10f;
				directionA += new Vector2(-directionA.y, directionA.x);
				directionB += new Vector2(directionB.y, -directionB.x);
			}
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
			this.segments = createLoop ? 10 : Math.Clamp((int) (Math.Max((pointA - pointB).Length, (pointA + directionA - (pointB + directionB)).Length) / 2f), 4, 100);

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
		this.GenerateMesh();
	}

	protected unsafe void GenerateMesh() {
		this.connectionMesh.Clear();
		
		bool drawQuad = true;
		for (int i = 0; i < this.BezierPoints.Length - 1; i++) {
			int j = i + 1;
			Vector2 pointA = this.BezierPoints[i];
			Vector2 pointB = this.BezierPoints[j];

			float curveProgress = j / (float) this.BezierPoints.Length;
			float lastCurveProgress = i / (float) this.BezierPoints.Length;

			if (drawQuad)
				this.connectionMesh.AddQuad(this.CreateConnectionLineQuad(pointA.x, pointA.y, pointB.x, pointB.y, lastCurveProgress, curveProgress));
			drawQuad = !this.drawStriped || !drawQuad;
		}

		// REVIEW - find a way to not have fadeMiddle be converted to a float. because that feels. painful.
		// Convert to int?
		this.connectionRenderable = new MeshRenderable(this.connectionMesh, Preload.ConnectionShader, [
				new (0, 4, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) 0),
				new (1, 4, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 2)),
				new (2, 1, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 4)),
				new (3, 1, VertexAttribPointerType.Byte, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 5))
			], [ "projection", "model", "tintColor", "tintColorB", "widthClip", "fadeMiddle" ]);
	}

	protected static void DrawConnectionMesh(MeshRenderable renderable, Vector2 cameraPosition, Vector2 cameraScale, Vector2 translation, Color color, Color colorB, float widthClip, bool fadeMiddle) {
		renderable.PreDraw();
		renderable.UniformMatrix4("projection", false, [.. Matrix4X4.CreateOrthographicOffCenter(-cameraScale.x + cameraPosition.x, cameraScale.x + cameraPosition.x, -cameraScale.y + cameraPosition.y, cameraScale.y + cameraPosition.y, 0f, 1f)]);
		renderable.UniformMatrix4("model", false, [.. Matrix4X4.CreateTranslation(translation.x, translation.y, 0f)]);
		renderable.Uniform4("tintColor", color.r, color.g, color.b, color.a);
		renderable.Uniform4("tintColorB", colorB.r, colorB.g, colorB.b, colorB.a);
		renderable.Uniform1("widthClip", widthClip);
		renderable.Uniform1("fadeMiddle", fadeMiddle ? 1 : 0);
		renderable.DoDraw();
	}

	public bool Hovered {
		get {
			if (!this.AABB.Inside(WorldWindow.worldMouse))
				return false;

			float lineDist = WorldWindow.SelectorScale / 4f;

			Vector2 lastPoint = this.BezierPoints[0];
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

	public virtual void CheckRecalculateBezier() {
		if (this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			this.RecalculateBezier(this.PointA, this.PointB, this.DirectionA, this.DirectionB, false);
		}
	}

	public virtual void Draw() {
		this.CheckRecalculateBezier();
		if (WorldWindow.CullTest(this.fittedAABB)) {
			bool aVisible = this.AVisible;
			bool bVisible = this.BVisible;
			if (!aVisible && !bVisible || Settings.ConnectionOpacity < 0.01f)
				return;

			bool hovered = this.Hovered || Keys.Modifier(Keys.Modifiers.Shift);

			bool fadeMiddle = aVisible && bVisible && !this.ConnectionVisible;

			Program.gl.Enable(EnableCap.Blend);
			(Color connectionColorA, Color connectionColorB) = this.GetColorInformation(fadeMiddle, aVisible, bVisible, hovered);

			Vector2 matrixPos = WorldWindow.cameraOffset;
			Vector2 matrixScale = WorldWindow.cameraScale * Main.screenBounds;

			if (this.connectionRenderable != null)
				DrawConnectionMesh(this.connectionRenderable, matrixPos, matrixScale, Vector2.Zero, connectionColorA, connectionColorB, WorldWindow.SelectorScale / 66, fadeMiddle);

			Program.gl.Disable(EnableCap.Blend);
		}
	}
}