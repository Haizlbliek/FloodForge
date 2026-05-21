using System.Globalization;
using System.Text;

namespace FloodForge.World;

public class LocalTerrainObject : DevObject, ISaveableObject {
	private readonly Node bottom;
	private readonly Node nodeB;

	private Vector2 PosB {
		get => this.nodes[1].position;
		set => this.nodes[1].position = value;
	}
	private Vector2 HandleA {
		get => this.nodes[2].position;
		set => this.nodes[2].position = value;
	}
	private Vector2 HandleB {
		get => this.PosB + this.nodes[3].position;
		set => this.nodes[3].position = value - this.PosB;
	}
	public List<SplineMidpoint> midpoints = [];

	public LocalTerrainObject() {
		this.nodeB = this.AddNode(new Vector2(100f, 0f), this.nodes[0]);
		this.AddNode(new Vector2(0f, 100f), this.nodes[0]);
		this.AddNode(new Vector2(0f, -100f), this.nodeB);
		this.bottom = this.AddNode(new Vector2(0f, -100f), this.nodes[0]);
	}

	public virtual string SaveKey => "LocalTerrain";


	public override LocalTerrainObject Clone() {
		LocalTerrainObject clone = new LocalTerrainObject();
		DevObject.SetNodes(this, clone);
		clone.midpoints = [.. this.midpoints];
		return clone;
	}

	public override void Draw(Vector2 offset) {
		this.bottom.position.x = 0f;
		this.bottom.position.y = MathF.Min(this.bottom.position.y, -10f);

		Vector2 screenA = offset + this.nodes[0].position / 20f;
		Vector2 screenB = offset + (this.nodes[0].position + this.PosB) / 20f;
		Vector2 screenHA = offset + (this.nodes[0].position + this.HandleA) / 20f;
		Vector2 screenHB = offset + (this.nodes[0].position + this.HandleB) / 20f;
		Vector2 screenBottom = offset + (this.nodes[0].position + this.bottom.position) / 20f;

		float minX = MathF.Min(screenA.x, screenB.x);
		float maxX = MathF.Max(screenA.x, screenB.x);
		int segments = 24;

		Program.gl.Enable(GLEnum.Blend);
		Immediate.Color(0.3f, 0.5f, 0.8f, 0.25f);
		Immediate.Begin(Immediate.PrimitiveType.TRIANGLES);

		for (int i = 0; i < segments; i++) {
			float t1 = i / (float) segments;
			float t2 = (i + 1) / (float) segments;

			Vector2 topL = MathUtil.BezierCubic(t1, screenA, screenHA, screenHB, screenB);
			Vector2 topR = MathUtil.BezierCubic(t2, screenA, screenHA, screenHB, screenB);

			topL.x = Mathf.Clamp(topL.x, minX, maxX);
			topR.x = Mathf.Clamp(topR.x, minX, maxX);

			Vector2 botL = new Vector2(topL.x, screenBottom.y);
			Vector2 botR = new Vector2(topR.x, screenBottom.y);

			Immediate.Vertex(botL);
			Immediate.Vertex(topL);
			Immediate.Vertex(topR);

			Immediate.Vertex(botL);
			Immediate.Vertex(topR);
			Immediate.Vertex(botR);
		}

		Immediate.End();
		Program.gl.Disable(GLEnum.Blend);

		Immediate.Color(0.3f, 0.5f, 0.8f);
		UI.Line(screenA, screenHA);
		UI.Line(screenB, screenHB);

		Immediate.Color(0.8f, 0.3f, 0.3f);
		UI.Line(new Vector2(screenA.x, screenBottom.y), new Vector2(screenB.x, screenBottom.y));
		UI.Line(screenA, new Vector2(screenA.x, screenBottom.y));
		UI.Line(screenB, new Vector2(screenB.x, screenBottom.y));

		Immediate.Color(0.5f, 0.5f, 0.5f);
		Vector2 prevPoint = screenA;
		for (int i = 1; i <= segments; i++) {
			float t = i / (float) segments;
			Vector2 curvePoint = MathUtil.BezierCubic(t, screenA, screenHA, screenHB, screenB);
			UI.Line(prevPoint, curvePoint);
			prevPoint = curvePoint;
		}
	}


	private string SerializeVector(Vector2 v) => $"{v.x.ToString(CultureInfo.InvariantCulture)}^{v.y.ToString(CultureInfo.InvariantCulture)}";

	private Vector2 ParseVector(string s) {
		string[] parts = s.Split('^');
		return new Vector2(
			float.Parse(parts[0], NumberStyles.Any, CultureInfo.InvariantCulture),
			float.Parse(parts[1], NumberStyles.Any, CultureInfo.InvariantCulture)
		);
	}

	public virtual string Save(string[] splits) {
		StringBuilder sb = new StringBuilder();
		sb.Append($"{this.SaveKey}><{this.nodes[0].position.x.ToString(CultureInfo.InvariantCulture)}><{this.nodes[0].position.y.ToString(CultureInfo.InvariantCulture)}><");

		List<string> segments = [
			(-this.bottom.position.y).ToString(CultureInfo.InvariantCulture),
			this.SerializeVector(this.PosB),
			this.SerializeVector(this.HandleA),
			this.SerializeVector(this.HandleB),
			string.Join("|", this.midpoints)
		];

		sb.Append(string.Join("~", segments));
		return sb.ToString();
	}

	public virtual void Load(Vector2 basePosition, string[] splits) {
		this.nodes[0].position = basePosition;
		if (splits.Length < 4)
			return;

		try {
			this.bottom.position.y = -float.Parse(splits[0], CultureInfo.InvariantCulture);

			this.PosB = this.ParseVector(splits[1]);
			this.HandleA = this.ParseVector(splits[2]);
			this.HandleB = this.ParseVector(splits[3]);

			this.midpoints.Clear();
			if (splits.Length > 4 && !string.IsNullOrWhiteSpace(splits[4])) {
				string[] midpointStrings = splits[4].Split('|');
				foreach (string mpStr in midpointStrings) {
					if (string.IsNullOrWhiteSpace(mpStr))
						continue;
					this.midpoints.Add(SplineMidpoint.FromString(mpStr));
				}
			}
		}
		catch (Exception ex) {
			Logger.Warn($"Failed parsing LocalTerrain data layout: {ex.Message}");
		}
	}

	public struct SplineMidpoint {
		public Vector2 position;
		public Vector2 handleA;
		public Vector2 handleB;

		public static SplineMidpoint FromString(string s) {
			string[] parts = s.Split('^');
			if (parts.Length < 6)
				return default;

			return new SplineMidpoint {
				position = new Vector2(float.Parse(parts[0], CultureInfo.InvariantCulture), float.Parse(parts[1], CultureInfo.InvariantCulture)),
				handleA = new Vector2(float.Parse(parts[2], CultureInfo.InvariantCulture), float.Parse(parts[3], CultureInfo.InvariantCulture)),
				handleB = new Vector2(float.Parse(parts[4], CultureInfo.InvariantCulture), float.Parse(parts[5], CultureInfo.InvariantCulture))
			};
		}

		public override readonly string ToString() {
			return $"{this.position.x}^{this.position.y}^{this.handleA.x}^{this.handleA.y}^{this.handleB.x}^{this.handleB.y}";
		}
	}

}
