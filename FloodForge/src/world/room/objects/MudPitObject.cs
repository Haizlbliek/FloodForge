namespace FloodForge.World;

public class MudPitObject : DevObject {
	public MudPitObject() {
		this.AddNode(new Vector2(200f, 30f), this.nodes[0]);
	}

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
		Immediate.Alpha(0.25f);
		Program.gl.Enable(EnableCap.Blend);
		UI.FillRect(rect);
		Program.gl.Disable(EnableCap.Blend);
	}
}