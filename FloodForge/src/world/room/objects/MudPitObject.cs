namespace FloodForge.World;

public class MudPitObject : DevObject, ISaveableObject {
	public MudPitObject() {
		this.AddNode(new Vector2(200f, 30f), this.nodes[0]);
	}

	public string SaveKey => "MudPit";

	public override MudPitObject Clone() {
		MudPitObject clone = new MudPitObject();
		DevObject.SetNodes(this, clone);
		return clone;
	}

	public override void Draw(Vector2 offset) {
		Rect rect = Rect.FromSize(
			offset + this.nodes[0].position / 20f,
			this.nodes[1].position / 20f
		);

		Immediate.Color(0.478f, 0.282f, 0.196f);
		UI.StrokeRect(rect);
		Immediate.Alpha(0.5f);
		Program.gl.Enable(EnableCap.Blend);
		UI.FillRect(rect);
		Program.gl.Disable(EnableCap.Blend);
	}

	public void Load(Vector2 pos, string[] splits) {
		this.nodes[0].position = pos;
		if (splits.Length >= 2) {
			this.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
		}
	}

	public string Save(string[] splits) {
		string size = splits.Length >= 3 ? splits[2] : "15.0";
		return $"MudPit><{this.nodes[0].position.x}><{this.nodes[0].position.y}><{this.nodes[1].position.x}~{this.nodes[1].position.y}~{size}";
	}
}