namespace FloodForge.World;

public enum ShortcutType {
	Room,
	Den,
	Scavenger,
}

public class RoomData {
	public int waterHeight = -1;
	public bool waterInFront = false;
	public bool enclosedRoom = false;
	public int subregion = -1;
	public int layer = 0;
	public int cameraCount = 0;
	public bool hidden = false;
	public bool merge = true;
	public DevItem[] devItems = [];
	public Dictionary<string, RoomAttractiveness> attractiveness = [];
	public HashSet<string> tags = [];

	public bool ExtraFlags => this.hidden || !this.merge;

	public class DevItem {
		public string name;
		public Texture texture;
		public Vector2 position;

		public DevItem(string name, Texture texture, Vector2 position) {
			this.name = name;
			this.texture = texture;
			this.position = position;
		}
	}
}