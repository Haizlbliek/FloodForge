namespace FloodForge.World;

public class Region {
	public string acronym = "";
	public string displayName = "";

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

	public List<string> regionsPaths = [];

	private static string? FindAcronym(string regionsPath, string acronym) {
		foreach (string l in File.ReadAllLines(regionsPath)) {
			string line = (l.StartsWith("[ADD]") ? l[5..] : l).Trim();

			if (line.Equals(acronym, StringComparison.InvariantCultureIgnoreCase)) {
				return line;
			}
		}

		return null;
	}

	public string FindAcronym(string acronym) {
		foreach (string path in WorldWindow.region.regionsPaths) {
			string? p = FindAcronym(path, acronym);

			if (p != null) {
				return p;
			}
		}

		return acronym;
	}
}