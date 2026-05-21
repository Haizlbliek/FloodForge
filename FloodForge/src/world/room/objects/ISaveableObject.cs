namespace FloodForge.World;

public interface ISaveableObject {
	public string SaveKey { get; }

	public void Load(Vector2 pos, string[] splits);
	public string Save(string[] splits);
}
