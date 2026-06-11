namespace FloodForge.World;

// REVIEW - turn this class into the base for all connection visuals, only taking in from- and to-point when drawing and a gradient when relevant
public class ConnectionVisual {
	public Room roomA;
	public Room roomB;

	bool drawStriped = true;

	public uint roomAExitID;
	public uint roomBExitID;

	public int segments;
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
	public Rect AABB {
		get {
			return this.PaddedAABB;
		}
	}

	public Vector2[] BezierPoints = [];
	public bool recalculateBezier = true;
	
	public ConnectionVisual(Room roomA, Room roomB, uint connectionA, uint connectionB) {
		this.roomA = roomA;
		this.roomB = roomB;
		this.roomAExitID = connectionA;
		this.roomBExitID = connectionB;
	}

	public void RecalculateBezier() {
		Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
		Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);
		this.segments = Math.Clamp((int) ((pointA - pointB).Length / 2f), 4, 100);
		if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
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

	public void Draw() {
		if (this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			this.RecalculateBezier();
		}
		if (WorldWindow.CullTest(this.fittedAABB)) {
			bool aVisible = WorldWindow.VisibleLayers[this.roomA.data.layer] && this.roomA.timeline.OverlapsWith(WorldWindow.VisibleTimeline);
			bool bVisible = WorldWindow.VisibleLayers[this.roomB.data.layer] && this.roomB.timeline.OverlapsWith(WorldWindow.VisibleTimeline);
			float opacity = Settings.ConnectionOpacity;
			if (!aVisible && !bVisible || opacity < 0.01f)
				return;

			Color connectionColorA;
			Color connectionColorB;
		bool blendColors = false;

			connectionColorA = Themes.RoomConnection;
			connectionColorB = Themes.RoomConnection;

			if (WorldWindow.ColorType != WorldWindow.RoomColors.None) {
				connectionColorA = this.roomA.GetTintColor();
				connectionColorB = this.roomB.GetTintColor();
				connectionColorA = Color.Lerp(Themes.RoomAir, connectionColorA, Settings.RoomTintStrength);
				connectionColorB = Color.Lerp(Themes.RoomAir, connectionColorB, Settings.RoomTintStrength);
			}
			if (!connectionColorA.Equals(connectionColorB)) {
				blendColors = true;
			}
			if (!blendColors) {
				Immediate.Color(connectionColorA);
			}

			float alphaA = aVisible ? opacity : 0f;
			float alphaB = bVisible ? opacity : 0f;
			if (opacity <= 0.999f || aVisible != bVisible) {
				Program.gl.Enable(EnableCap.Blend);
			}

			Vector2 pointA = this.roomA.GetConnectionConnectPoint(this.roomAExitID);
			Vector2 pointB = this.roomB.GetConnectionConnectPoint(this.roomBExitID);
			if (Settings.ConnectionType.value == Settings.STConnectionType.Linear) {
				this.DrawCustomLine(pointA.x, pointA.y, pointB.x, pointB.y, alphaA, alphaB);
			}
			else {
				Vector2 lastPoint = this.BezierPoints![0];
				int curveLength = this.BezierPoints.Length;
				bool drawSegment = true;
				for (int i = 1; i < curveLength; i++) {
					float curveProgress = i / (float) curveLength;
					if (blendColors) {
						Immediate.Color(Color.Lerp(connectionColorA, connectionColorB, curveProgress));
					}
					Vector2 point = this.BezierPoints[i];
					float lastCurveProgress = curveProgress - (1f / curveLength);
					if (drawSegment) {
						float lerpedAlphaA = Mathf.LerpUnclamped(alphaA, alphaB, lastCurveProgress);
						float lerpedAlphaB = Mathf.LerpUnclamped(alphaA, alphaB, curveProgress);
						this.DrawCustomLine(lastPoint.x, lastPoint.y, point.x, point.y, lerpedAlphaA, lerpedAlphaB);
					}
					lastPoint = point;
					drawSegment = !(drawSegment && this.drawStriped);
				}
			}

			Program.gl.Disable(EnableCap.Blend);

			return;
		}
	}
}