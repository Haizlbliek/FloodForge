namespace FloodForge.World;

public class FreeConnection : ConnectionVisual {
	protected Vector2 pointA = Vector2.Zero;
	protected Vector2i directionA = Vector2i.Zero;
	protected Vector2 pointB = Vector2.Zero;
	protected Vector2i directionB = Vector2i.Zero;
	protected bool createLoop;

	public override Vector2 PointA => this.pointA;
	public override Vector2i DirectionA => this.directionA;
	public override Vector2 PointB => this.pointB;
	public override Vector2i DirectionB => this.directionB;

	protected Color color;

	protected bool aVisible = true;
	public override bool AVisible {
		get {
			return this.aVisible;
		}
	}

	protected bool bVisible = true;
	public override bool BVisible {
		get {
			return this.bVisible;
		}
	}

	protected bool connectionVisible = true;
	public override bool ConnectionVisible {
		get {
			return this.connectionVisible;
		}
	}

	public void SetVisibilities(bool a, bool b, bool connection) {
		this.aVisible = a;
		this.bVisible = b;
		this.connectionVisible = connection;
	}

	protected override (Color, Color) GetColorInformation(bool fadeMiddle, bool AVisible, bool BVisible, bool hovered) {
		Color colorA = this.color;
		Color colorB = this.color;
		colorA.a = AVisible ? Settings.ConnectionOpacity : 0f;
		colorB.a = BVisible ? Settings.ConnectionOpacity : 0f;
		return (colorA, colorB);
	}

	public FreeConnection(bool createLoopOnInit) : base(false) {
		this.createLoop = createLoopOnInit;
	}

	public override void CheckRecalculateBezier() {
		if (this.BezierPoints == null || this.BezierPoints.Length == 0 || this.recalculateBezier) {
			this.RecalculateBezier(this.PointA, this.PointB, this.DirectionA, this.DirectionB, this.createLoop);
		}
	}

	public void Draw(Vector2 pointA, Vector2 pointB, Vector2i directionA, Vector2i directionB, Color currentColor, bool createLoop) {
		this.color = currentColor;
		if (pointA != this.pointA || pointB != this.pointB || directionA != this.directionA || directionB != this.directionB || createLoop != this.createLoop) {
			this.pointA = pointA;
			this.pointB = pointB;
			this.directionA = directionA;
			this.directionB = directionB;
			this.createLoop = createLoop;
			this.recalculateBezier = true;
		}
		base.Draw();		
	}
}