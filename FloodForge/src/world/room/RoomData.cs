namespace FloodForge.World;

public enum RoomExitType {
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

	public bool blockedBatMigration = false;
	public int hidden = 0;
	public bool merge = true;
	public bool warpable = true;

	public Dictionary<string, RoomAttractiveness> attractiveness = [];
	public HashSet<string> tags = [];
	public List<Camera> cameras = [];
	public List<DevObject> objects = [];

	public bool ExtraFlags => this.hidden != 0 || !this.merge || !this.warpable;

	public class Camera {
		public Vector2 position;
		public Vector2[] angles = [ Vector2.Zero, Vector2.Zero, Vector2.Zero, Vector2.Zero ];
	}
}