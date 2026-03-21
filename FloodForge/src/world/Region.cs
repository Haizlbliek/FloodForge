namespace FloodForge.World;

// TODO: Add display name
public class Region {
	public string acronym = "";

	public string extraProperties = "";
	public string extraWorld = "";
	public string extraMap = "";
	public string extraWorldCreatures = "";

	public string roomsPath = "";
	public string exportPath = "";

	public Dictionary<string, RoomAttractiveness> defaultAttractiveness = [];

	public Dictionary<int, Color> overrideSubregionColors = [];

	public OffscreenRoom? offscreenDen = null;
	public List<Room> rooms = [];
	public List<Connection> connections = [];
	public List<string> subregions = [];

	// public void Reset() {
	// 	this.rooms.Clear();
	// 	this.connections.Clear();
	// 	this.subregions.Clear();
	// 	this.offscreenDen = null;
	// 	this.defaultAttractiveness.Clear();
	// 	this.extraProperties = "";
	// 	this.extraWorld = "";
	// 	this.extraMap = "";
	// 	this.extraWorldCreatures = "";

	// 	this.exportPath = "";
	// 	this.acronym = "";
	// 	this.overrideSubregionColors.Clear();
	// }
}