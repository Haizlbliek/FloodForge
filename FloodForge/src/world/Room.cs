using Stride.Core.Extensions;

namespace FloodForge.World;

public class Room {
	public const uint FLAG_VERTICAL_POLE = 16;
	public const uint FLAG_HORIZONTAL_POLE = 32;
	public const uint FLAG_ROOM_EXIT = 64;
	public const uint FLAG_SHORTCUT = 128;
	public const uint FLAG_DEN = 256;
	public const uint FLAG_BACKGROUND_SOLID = 512;
	// 1024 and 2048 are for slopes
	public const uint FLAG_SCAVENGER_DEN = 4096;
	public const uint FLAG_WACK_A_MOLE_HOLE = 8192;
	public const uint FLAG_GARBAGE_WORM_HOLE = 16384;
	public const uint FLAG_WORMGRASS = 32768;
	public const uint FLAG_BATFLY_HIVE = 65536;
	public const uint FLAG_WATERFALL = 131072;
	public const uint FLAG_ROCK = 262144;
	public const uint FLAG_SPEAR = 524288;

	public string Path;
	public string Name;
	public TimelineType TimelineType;
	public HashSet<string> Timelines = [];
	public Vector2 CanonPosition;
	public Vector2 DevPosition;
	public Vector2 CanonVel;
	public Vector2 DevVel;
	public int width;
	public int height;
	public bool valid;
	public RoomData data;
	public uint[] geometry = null!;
	public List<(ShortcutType, Vector2i)> shortcutExits = [];
	public Dictionary<Vector2i, (Vector2i[], Vector2i)> shortcutPaths = [];
	public List<Vector2i> roomExits = [];
	public List<Vector2i> roomShortcutEntrances = [];
	public List<Vector2i> denShortcutEntrances = [];
	public List<Den> dens = [];
	public List<GarbageWormDen> garbageWormDens = [];
	public int hoveredDen = -1; // LATER: Remove / improve
	public int hoveredRoomExit = -1; // LATER: Remove / improve

	public List<Connection> connections = [];


	private int specialExitCount = 0;
	public int GarbageWormDenIndex => this.specialExitCount + this.roomShortcutEntrances.Count + this.denShortcutEntrances.Count;

	public bool Visible => WorldWindow.VisibleLayers[this.data.layer];

	public Room(string path, string name) {
		this.Path = path;
		this.Name = name;
		this.TimelineType = TimelineType.All;

		this.CanonPosition = Vector2.Zero;
		this.DevPosition = Vector2.Zero;
		this.width = 1;
		this.height = 1;
		this.valid = false;

		this.data = new RoomData();

		this.LoadGeometry();
		this.GenerateMesh();
		this.CheckImages();
	}

	public bool HasDen(int id) {
		return this.HasDen01(id - this.roomShortcutEntrances.Count) || id == this.GarbageWormDenIndex;
	}

	public bool HasDen01(int id) {
		return id >= 0 && id < this.dens.Count;
	}

	public Den GetDen(int id) {
		return this.GetDen01(id - this.roomShortcutEntrances.Count);
	}

	public int GetDenId(Vector2i pos) {
		return this.denShortcutEntrances.IndexOf(pos) + this.roomShortcutEntrances.Count;
	}

	public int GetDenId01(Vector2i pos) {
		return this.denShortcutEntrances.IndexOf(pos);
	}

	public Den GetDen01(int id) {
		if (id < 0 || id >= this.dens.Count) {
			throw new Exception($"Invalid Den {id} for {this.Name}");
		}

		return this.dens[id];
	}

	public bool ValidConnection(uint index) {
		return index < this.shortcutExits.Count;
	}

	public void Connect(Connection connection) {
		this.connections.Add(connection);
	}

	public void Disconnect(Connection connection) {
		this.connections.Remove(connection);
	}

	private static void SetCameraAngle(string from, ref Vector2 angle) {
		try {
			int commaIndex = from.IndexOf(',');
			if (commaIndex == -1) throw new FormatException();

			double theta = double.Parse(from[..commaIndex]) * (Math.PI / 180.0);
			double radius = double.Parse(from[(commaIndex + 1)..]);

			angle.x = (float)(Math.Sin(theta) * radius);
			angle.y = (float)(Math.Cos(theta) * radius);
		} catch (Exception) {
			Logger.Warn("Failed parsing camera angle: " + from);
		}
	}

	protected virtual void LoadGeometry() {
		if (!File.Exists(this.Path)) {
			Logger.Warn($"Failed to load '{this.Name}'. File '{this.Path}' doesn't exist");
			this.width = 72;
			this.height = 43;
			this.geometry = new uint[this.width * this.height];
			this.valid = false;
			return;
		}

		string[] lines = File.ReadAllLines(this.Path);

		string[] levelData = lines[1].Split('|');
		this.width = int.Parse(levelData[0][..levelData[0].IndexOf('*')]);
		this.height = int.Parse(levelData[0][(levelData[0].IndexOf('*') + 1)..]);
		this.geometry = new uint[this.width * this.height];
		if (levelData.Length == 1) {
			this.data.waterHeight = -1;
			this.data.waterInFront = false;
		} else {
			this.data.waterHeight = int.Parse(levelData[1]);
			this.data.waterInFront = int.Parse(levelData[2]) == 1;
		}

		string[] camerasData = lines[3].Split('|', StringSplitOptions.RemoveEmptyEntries);
		foreach (string cameraData in camerasData) {
			string[] parts = cameraData.Split(',');
			if (parts.Length < 2) {
				Logger.Warn($"Room has invalid camera position count ({cameraData})");
				continue;
			}

			int x = 0, y = 0;
			try {
				x = int.Parse(parts[0]);
				y = int.Parse(parts[1]);
			} catch {
				Logger.Warn($"Room has invalid camera position ({cameraData})");
			}
			this.data.cameras.Add(new RoomData.Camera() {
				position = new Vector2(x, y)
			});
		}

		this.data.enclosedRoom = lines[4].Contains("Solid");

		if (lines.Length >= 13 && lines[12].StartsWith("camera angles:")) {
			string[] angleData = lines[12][(lines[12].IndexOf(':') + 1)..].Split('|');
			for (int i = 0; i < this.data.cameras.Count; i++) {
				if (i >= angleData.Length) break;

				string[] angles = angleData[i].Split(';');
				if (angles.Length != 4) {
					Logger.Warn($"Failed to parse camera {i}; Not enough camera angles");
					continue;
				}

				SetCameraAngle(angles[0], ref this.data.cameras[i].angles[0]);
				SetCameraAngle(angles[1], ref this.data.cameras[i].angles[1]);
				SetCameraAngle(angles[2], ref this.data.cameras[i].angles[2]);
				SetCameraAngle(angles[3], ref this.data.cameras[i].angles[3]);
			}
		}

		string[] objectData = lines[5].Split('|', StringSplitOptions.RemoveEmptyEntries);

		string[] geometryData = lines[11].Split('|', StringSplitOptions.RemoveEmptyEntries);
		int idx = 0;
		foreach (string tile in geometryData) {
			byte[] data = [.. tile.Split(',').Select(byte.Parse)];
			this.geometry[idx] = data[0];
			if (data[0] == 4) this.geometry[idx] |= FLAG_SHORTCUT;

			for (int i = 1; i < data.Length; i++) {
				switch (data[i]) {
					case 1: this.geometry[idx] |= FLAG_VERTICAL_POLE; break;
					case 2: this.geometry[idx] |= FLAG_HORIZONTAL_POLE; break;
					case 3: this.geometry[idx] |= FLAG_SHORTCUT; break;
					case 6: this.geometry[idx] |= FLAG_BACKGROUND_SOLID; break;
					case 7: this.geometry[idx] |= FLAG_BATFLY_HIVE; break;
					case 8: this.geometry[idx] |= FLAG_WATERFALL; break;
					case 9: this.geometry[idx] |= FLAG_WACK_A_MOLE_HOLE; break;
					case 10: this.geometry[idx] |= FLAG_GARBAGE_WORM_HOLE; break;
					case 11: this.geometry[idx] |= FLAG_WORMGRASS; break;

					case 4:
						this.geometry[idx] = this.geometry[idx] | FLAG_ROOM_EXIT | FLAG_SHORTCUT;
						this.shortcutExits.Add((ShortcutType.Room, new Vector2i(idx / this.height, idx % this.height)));
						break;

					case 5:
						this.geometry[idx] = this.geometry[idx] | FLAG_DEN | FLAG_SHORTCUT;
						this.shortcutExits.Add((ShortcutType.Den, new Vector2i(idx / this.height, idx % this.height)));
						break;

					case 12:
						this.geometry[idx] = this.geometry[idx] | FLAG_SCAVENGER_DEN | FLAG_SHORTCUT;
						this.shortcutExits.Add((ShortcutType.Scavenger, new Vector2i(idx / this.height, idx % this.height)));
						break;
				}
			}

			idx++;
		}

		this.valid = true;

		idx = 0;
		for (int x = 0; x < this.width; x++) {
			for (int y = 0; y < this.height; y++) {
				if ((this.geometry[idx] & 15) == 2) {
					int bits = 0;
					bits += (this.GetTile(x - 1, y) == 1u) ? 1 : 0;
					bits += (this.GetTile(x + 1, y) == 1u) ? 2 : 0;
					bits += (this.GetTile(x, y - 1) == 1u) ? 4 : 0;
					bits += (this.GetTile(x, y + 1) == 1u) ? 8 : 0;
					int type = -1;

					if (bits == 1 + 4) type = 0;
					else if (bits == 1 + 8) type = 1;
					else if (bits == 2 + 4) type = 2;
					else if (bits == 2 + 8) type = 3;

					if (type == -1) {
						Logger.Note($"Invalid slope type {this.Name}({x}, {y})");
					} else {
						this.geometry[idx] += (uint) (1024 * type);
					}
				}

				idx++;
			}
		}

		foreach (string obj in objectData) {
			string[] item = obj.Split(',');
			if (item.Length != 3) {
				Logger.Warn("Failed to parse object: " + obj);
				continue;
			}

			if (!int.TryParse(item[1], out int x)) {
				Logger.Warn("Failed to parse object: " + obj);
			}
			if (!int.TryParse(item[2], out int y)) {
				Logger.Warn("Failed to parse object: " + obj);
			}

			this.geometry[y - 1 + (x - 1) * this.height] += item[0] == "0" ? FLAG_ROCK : FLAG_SPEAR;
		}

		this.EnsureConnections();

		foreach (Vector2i den in this.denShortcutEntrances) {
			this.dens.Add(new Den());
		}
	}

	private struct VerifiedConnection {
		public ShortcutType type;
		public Vector2i entrancePosition;
		public Vector2i exitPosition;

		public VerifiedConnection(ShortcutType type, Vector2i entrancePosition, Vector2i exitPosition) {
			this.type = type;
			this.entrancePosition = entrancePosition;
			this.exitPosition = exitPosition;
		}
	}

	protected void EnsureConnections() {
		this.specialExitCount = 0;

		List<VerifiedConnection> verifiedConnections = [];
		List<(ShortcutType, Vector2i)> verifiedShortcuts = [];

		for (int i = 0; i < this.shortcutExits.Count; i++) {
			Vector2i forwardDirection = Vector2i.Zero;
			Vector2i initialPosition = this.shortcutExits[i].Item2;
			Vector2i currentPosition = initialPosition;
			bool hasDirection = true;
			if (this.TileIsShortcut(currentPosition.x - 1, currentPosition.y)) {
				forwardDirection.x = -1;
			}
			else if (this.TileIsShortcut(currentPosition.x, currentPosition.y + 1)) {
				forwardDirection.y = 1;
			}
			else if (this.TileIsShortcut(currentPosition.x + 1, currentPosition.y)) {
				forwardDirection.x = 1;
			}
			else if (this.TileIsShortcut(currentPosition.x, currentPosition.y - 1)) {
				forwardDirection.y = -1;
			}

			if (forwardDirection.x == 0 && forwardDirection.y == 0) {
				Logger.Warn($"Couldn't load shortcut {this.Name}({currentPosition.x}, {currentPosition.y})");
				continue;
			}
			Vector2i initialDirection = forwardDirection * new Vector2i(-1, 1);

			List<Vector2i> exitPath = [];
			exitPath.Add(currentPosition);
			for (int runs = 0; runs < 10000; runs++) {
				currentPosition += forwardDirection;

				if (!this.TileIsShortcut(currentPosition.x + forwardDirection.x, currentPosition.y + forwardDirection.y)) {
					Vector2i lastDirection = forwardDirection;

					forwardDirection.x = 0;
					forwardDirection.y = 0;
					hasDirection = false;
					if (lastDirection.x != 1 && this.TileIsShortcut(currentPosition.x - 1, currentPosition.y)) {
						forwardDirection.x = -1;
						hasDirection = true;
					}
					else if (lastDirection.y != -1 && this.TileIsShortcut(currentPosition.x, currentPosition.y + 1)) {
						forwardDirection.y = 1;
						hasDirection = true;
					}
					else if (lastDirection.x != -1 && this.TileIsShortcut(currentPosition.x + 1, currentPosition.y)) {
						forwardDirection.x = 1;
						hasDirection = true;
					}
					else if (lastDirection.y != 1 && this.TileIsShortcut(currentPosition.x, currentPosition.y - 1)) {
						forwardDirection.y = -1;
						hasDirection = true;
					}
				}

				if ((this.GetTile(currentPosition.x, currentPosition.y) & 15) == 4) {
					hasDirection = true;
					exitPath.Add(currentPosition);
					break;
				}

				if (!hasDirection) break;
				exitPath.Add(currentPosition);
			}

			if (hasDirection) {
				verifiedConnections.Add(new VerifiedConnection(this.shortcutExits[i].Item1, currentPosition, initialPosition));
				verifiedShortcuts.Add(this.shortcutExits[i]);
				this.shortcutPaths[currentPosition] = (exitPath.ToArray(), initialDirection);
			}
			string outString = "";
			foreach(Vector2i item in exitPath) {
				outString += string.Format("item: {0}; {1}\n", item.x, item.y);
			}
			Logger.Info("Final exitPath for index " + i + " with exitTile position " + currentPosition + ":\n" + outString);
		}

		verifiedConnections.Reverse();
		verifiedShortcuts.Reverse();

		for (int i = 0; i < verifiedShortcuts.Count; i++) {
			for (int j = 0; j + 1 < verifiedShortcuts.Count - i; j++) {
				Vector2i a = verifiedShortcuts[j].Item2;
				Vector2i b = verifiedShortcuts[j + 1].Item2;

				if (a.y > b.y || (a.y == b.y && a.x > b.x)) {
					(verifiedShortcuts[j], verifiedShortcuts[j + 1]) = (verifiedShortcuts[j + 1], verifiedShortcuts[j]);
					(verifiedConnections[j], verifiedConnections[j + 1]) = (verifiedConnections[j + 1], verifiedConnections[j]);
				}
			}
		}

		foreach (VerifiedConnection connection in verifiedConnections) {
			if (connection.type == ShortcutType.Room) {
				this.roomShortcutEntrances.Add(connection.entrancePosition);
				this.roomExits.Add(connection.exitPosition);
			}
			else if (connection.type == ShortcutType.Den) {
				this.denShortcutEntrances.Add(connection.entrancePosition);
			}
			else {
				this.specialExitCount++;
			}
		}

		// Side Exits
		bool wasL = false, wasR = false;
		for (int y = 0; y < this.height; y++) {
			bool airL = (this.GetTile(0, y) & 15) != 1;
			bool airR = (this.GetTile(this.width - 1, y) & 15) != 1;
			if (airL && !wasL)
				this.specialExitCount++;
			if (airR && !wasR)
				this.specialExitCount++;
			wasL = airL;
			wasR = airR;
		}

		// Sky & Sea Exits
		wasL = false; wasR = false;
		for (int x = 0; x < this.width; x++) {
			bool airL = (this.GetTile(x, 0) & 15) != 1;
			bool airR = (this.GetTile(x, this.height - 1) & 15) != 1;
			if (airL && !wasL)
				this.specialExitCount++;
			if (airR && !wasR && this.data.waterHeight >= 0)
				this.specialExitCount++;
			wasL = airL;
			wasR = airR;
		}

		// Batfly Hives
		for (int y = 0; y < this.height; y++) {
			wasL = false;
			for (int x = 0; x < this.width; x++) {
				bool hive = (this.GetTile(x, y) & FLAG_BATFLY_HIVE) > 0;
				if (!wasL && hive)
					this.specialExitCount++;
				wasL = hive;
			}
		}
	}

	protected void CheckImages() {
		if (!Settings.WarnMissingImages) return;

		string path = PathUtil.Parent(this.Path);
		for (int i = 0; i < this.data.cameras.Count; i++) {
			string imageFile = $"{this.Name}_{i + 1}.png";

			if (PathUtil.FindFile(path, imageFile) == null) {
				Logger.Warn($"{this.Name} is missing image {imageFile}");
			}
		}
	}

	public void RegenerateGeometry() {
		this.shortcutExits.Clear();
		this.denShortcutEntrances.Clear();
		this.roomShortcutEntrances.Clear();
		this.roomExits.Clear();

		int idx = 0;
		for (int x = 0; x < this.width; x++) {
			for (int y = 0; y < this.height; y++) {
				if ((this.geometry[idx] & FLAG_ROOM_EXIT) > 0) {
					this.shortcutExits.Add((ShortcutType.Room, new Vector2i(x, y)));
				}
				if ((this.geometry[idx] & FLAG_DEN) > 0) {
					this.shortcutExits.Add((ShortcutType.Den, new Vector2i(x, y)));
				}
				if ((this.geometry[idx] & FLAG_SCAVENGER_DEN) > 0) {
					this.shortcutExits.Add((ShortcutType.Scavenger, new Vector2i(x, y)));
				}

				idx++;
			}
		}

		this.EnsureConnections();

		// LATER: Parse dens
		while (this.dens.Count < this.denShortcutEntrances.Count) {
			this.dens.Add(new Den());
		}
		while (this.dens.Count > this.denShortcutEntrances.Count) {
			this.dens.RemoveAt(0);
		}

		this.GenerateMesh();
	}

	public uint GetTile(Vector2i pos) {
		return this.GetTile(pos.x, pos.y);
	}

	public uint GetTile(int x, int y) {
		if (!this.valid) return 1u;
		if (x < 0 || y < 0 || x >= this.width || y >= this.height) return 1u;

		return this.geometry[x * this.height + y];
	}

	public bool TileIsShortcut(int x, int y) {
		uint tile = this.GetTile(x, y);

		return (tile & (FLAG_SHORTCUT | FLAG_ROOM_EXIT | FLAG_SCAVENGER_DEN)) > 0;
	}

	public Vector2 GetRoomEntranceOffsetPosition(uint i) {
		Vector2i connection = (Settings.ConnectionPoint.value == Settings.STConnectionPoint.Entrance) ? this.roomShortcutEntrances[(int) i] : this.roomExits[(int) i];

		return new Vector2(
			connection.x + 0.5f,
			-connection.y - 0.5f
		);
	}

	public Vector2 GetRoomEntrancePosition(uint i) {
		Vector2 position = WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
		Vector2i connection = (Settings.ConnectionPoint.value == Settings.STConnectionPoint.Entrance) ? this.roomShortcutEntrances[(int) i] : this.roomExits[(int) i];

		return new Vector2(
			position.x + connection.x + 0.5f,
			position.y - connection.y - 0.5f
		);
	}

	public bool AnyConnectionConnectedTo(uint i) {
		if (this.shortcutExits.Count <= i) return false;

		foreach (Connection connection in this.connections) {
			if (connection.roomA == this && connection.connectionA == i) return true;
			if (connection.roomB == this && connection.connectionB == i) return true;
		}

		return false;
	}

	public uint GetRoomEntranceDirection(uint i) {
		Vector2i connection = this.roomShortcutEntrances[(int) i];

		if (this.TileIsShortcut(connection.x - 1, connection.y)) return Direction.Left;
		else if (this.TileIsShortcut(connection.x, connection.y + 1)) return Direction.Down;
		else if (this.TileIsShortcut(connection.x + 1, connection.y)) return Direction.Right;
		else if (this.TileIsShortcut(connection.x, connection.y - 1)) return Direction.Up;

		return Direction.Unknown;
	}

	public Vector2i GetRoomExitPosFromShortcut(Vector2i shortcutPos) {
		foreach (KeyValuePair<Vector2i, (Vector2i[], Vector2i)> pair in this.shortcutPaths) {
			if (pair.Value.Item1[^1] == shortcutPos) {
				return pair.Value.Item1[0];
			}
		}
		return new();
	}

	public Vector2 GetRoomExitPosition(uint i) {
		return (this.GetRoomExitPosFromShortcut(this.GetRoomEntranceShortcutPosition(i)) + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1) + this.Position;
	}

	public Vector2 GetConfiguredRoomEntrancePosition(uint i) {
		return WorldWindow.changeConnectBehaviour ? this.GetRoomExitPosition(i) : this.GetRoomEntrancePosition(i);
	}

	public Vector2i GetRoomEntranceShortcutPosition(uint i) {
		Vector2i connection = this.roomShortcutEntrances[(int) i];
		return connection;
	}

	public Vector2 GetRoomEntranceDirectionVector(uint i) {
		return Direction.ToVector(this.GetRoomEntranceDirection(i));
	}

	public Vector2 GetRoomExitDirectionFromShortcut(Vector2i shortcutPos) {
		foreach (KeyValuePair<Vector2i, (Vector2i[], Vector2i)> pair in this.shortcutPaths) {
			if (pair.Value.Item1[^1] == shortcutPos) {
				return pair.Value.Item2;
			}
		}
		return new();
	}

	public Vector2 GetConfiguredRoomEntranceDirection(uint i) {
		return WorldWindow.changeConnectBehaviour ? this.GetRoomExitDirectionFromShortcut(this.GetRoomEntranceShortcutPosition(i)) : this.GetRoomEntranceDirectionVector(i);
	}

	public Vector2 Position {
		get {
			return WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
		}

		set {
			if (WorldWindow.PositionType == WorldWindow.RoomPosition.Canon) {
				this.CanonPosition = value;
			} else {
				this.DevPosition = value;
			}
		}
	}

	public Vector2 InactivePosition {
		get {
			return WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.DevPosition : this.CanonPosition;
		}

		set {
			if (WorldWindow.PositionType == WorldWindow.RoomPosition.Canon) {
				this.DevPosition = value;
			} else {
				this.CanonPosition = value;
			}
		}
	}

	public bool Inside(Vector2 pos) {
		Vector2 position = this.Position;
		return pos.x >= position.x && pos.y >= position.y - this.height && pos.x < position.x + this.width && pos.y <= position.y;
	}

	public bool Inside(int x, int y) {
		return x >= 0 && x < this.width && y >= 0 && y < this.height;
	}

	public bool Inside(Vector2i pos) {
		return pos.x >= 0 && pos.x < this.width && pos.y >= 0 && pos.y < this.height;
	}

	public bool Intersects(Vector2 from, Vector2 to) {
		Vector2 position = this.Position;
		Vector2 cornerMin = Vector2.Min(from, to);
		Vector2 cornerMax = Vector2.Max(from, to);

		return cornerMax.x >= position.x && cornerMax.y >= position.y - this.height && cornerMin.x < position.x + this.width && cornerMin.y <= position.y;
	}

#region Rendering

	[StructLayout(LayoutKind.Sequential)]
	protected readonly struct Vertex {
		public readonly float x, y;
		public readonly float r, g, b, a;

		public Vertex(float x, float y, Color color) {
			this.x = x;
			this.y = y;
			this.r = color.r;
			this.g = color.g;
			this.b = color.b;
			this.a = color.a;
		}

		public Vertex(float x, float y, float r, float g, float b, float a = 1f) {
			this.x = x;
			this.y = y;
			this.r = r;
			this.g = g;
			this.b = b;
			this.a = a;
		}
	}

	protected uint _vao = 0;
	protected uint _vbo = 0;
	protected uint _ebo = 0;
	protected int _projLoc, _modelLoc, _tintLoc, _tintStrengthLoc;

	protected uint currentIndex = 0;
	protected List<Vertex> vertices = [];
	protected List<uint> indices = [];

	protected void AddQuad(float xPos, float yPos, Themes.ThemeColor theme) {
		this.AddQuad(new Vector2(xPos, yPos), Vector2.One, theme);
	}

	protected void AddQuad(Vector2 centerPosition, Themes.ThemeColor theme) {
		this.AddQuad(centerPosition, Vector2.One, theme);
	}

	protected void AddQuad(float xPos, float yPos, float scale, Themes.ThemeColor theme) {
		this.AddQuad(new Vector2(xPos, yPos), Vector2.One * scale, theme);
	}

	protected void AddQuad(Vector2 centerPosition, float scale, Themes.ThemeColor theme) {
		this.AddQuad(centerPosition, Vector2.One * scale, theme);
	}

	protected void AddQuad(float xPos, float yPos, Vector2 scale, Themes.ThemeColor theme) {
		this.AddQuad(new(xPos, yPos), scale, theme);
	}

	protected void AddQuad(Vector2 centerPosition, Vector2 scale, Themes.ThemeColor theme) {
		float x0 = centerPosition.x - (scale.x/2);
		float x1 = centerPosition.x + (scale.x/2);
		float y0 = centerPosition.y + (scale.y / 2);
		float y1 = centerPosition.y - (scale.y / 2);
		this.AddQuad(
			new Vertex(x0, y0, theme),
			new Vertex(x1, y0, theme),
			new Vertex(x1, y1, theme),
			new Vertex(x0, y1, theme)
		);
	}

	protected void AddQuad(Vertex a, Vertex b, Vertex c, Vertex d) {
		this.vertices.Add(a);
		this.vertices.Add(b);
		this.vertices.Add(c);
		this.vertices.Add(d);
		this.indices.Add(this.currentIndex + 0);
		this.indices.Add(this.currentIndex + 1);
		this.indices.Add(this.currentIndex + 2);
		this.indices.Add(this.currentIndex + 2);
		this.indices.Add(this.currentIndex + 3);
		this.indices.Add(this.currentIndex + 0);
		this.currentIndex += 4;
	}

	protected void AddTriangle(Vertex a, Vertex b, Vertex c) {
		this.vertices.Add(a);
		this.vertices.Add(b);
		this.vertices.Add(c);
		this.indices.Add(this.currentIndex + 0);
		this.indices.Add(this.currentIndex + 1);
		this.indices.Add(this.currentIndex + 2);
		this.currentIndex += 3;
	}

	List<Vector2i> allShortcutEntrances = [];
	protected unsafe virtual void GenerateMesh() {
		this.vertices.Clear();
		this.indices.Clear();
		this.allShortcutEntrances.Clear();
		this.currentIndex = 0;

		// Background
		/*this.AddQuad(
			new Vertex(0, 0, Themes.RoomSolid),
			new Vertex(this.width, 0, Themes.RoomSolid),
			new Vertex(this.width, -this.height, Themes.RoomSolid),
			new Vertex(0, -this.height, Themes.RoomSolid)
		);*/

		for (int x = 0; x < this.width; x++) {
			for (int y = 0; y < this.height; y++) {
				uint tile = this.GetTile(x, y);
				uint type = tile & 15;

				float x0 = x;
				float y0 = -y;
				float x1 = x + 1;
				float y1 = -y - 1;
				float x2 = (x0 + x1) * 0.5f;
				float y2 = (y0 + y1) * 0.5f;

				if (type == 4) {
					this.allShortcutEntrances.Add(new(x, y));
					this.AddQuad(x2, y2, Themes.RoomShortcutEntrance);
				}
				else if (type != 1) { // draws air if the tile isn't fully solid.
					Themes.ThemeColor air = ((tile & FLAG_BACKGROUND_SOLID) > 0) ? Themes.RoomLayer2Solid : Themes.RoomAir;

					if (type == 2) {
						uint direction = (tile >> 10) & 3;
						if (direction == 0) {
							this.AddTriangle(
								new Vertex(x1, y1, air),
								new Vertex(x0, y1, air),
								new Vertex(x1, y0, air)
							);
						}
						else if (direction == 1) {
							this.AddTriangle(
								new Vertex(x1, y0, air),
								new Vertex(x0, y0, air),
								new Vertex(x1, y1, air)
							);
						}
						else if (direction == 2) {
							this.AddTriangle(
								new Vertex(x0, y1, air),
								new Vertex(x1, y1, air),
								new Vertex(x0, y0, air)
							);
						}
						else if (direction == 3) {
							this.AddTriangle(
								new Vertex(x0, y0, air),
								new Vertex(x1, y0, air),
								new Vertex(x0, y1, air)
							);
						}
					}
					else {
						this.AddQuad(x2, y2, air); // possibility for greedy meshing, since right now every tile of air now gets its own quad.
					}
				}

				if (type == 3) {
					this.AddQuad(
						new Vertex(x0, y0, Themes.RoomPlatform),
						new Vertex(x1, y0, Themes.RoomPlatform),
						new Vertex(x1, y2, Themes.RoomPlatform),
						new Vertex(x0, y2, Themes.RoomPlatform)
					);
				}

				if ((tile & FLAG_VERTICAL_POLE) > 0) {
					this.AddQuad(x2, y2, new Vector2(0.25f, 1f), Themes.RoomPole);
				}

				if ((tile & FLAG_HORIZONTAL_POLE) > 0) {
					this.AddQuad(x2, y2, new Vector2(1f, 0.25f), Themes.RoomPole);
				}

				if ((tile & FLAG_SHORTCUT) > 0 && tile != 4) {
					// NOTE - Might add outlining RoomSolid if it looks weird

					this.AddQuad(x2, y2, 0.1875f, Themes.RoomShortcutDot);
				}
			}
		}

		foreach (Vector2i entrance in (Settings.ConnectionPoint.value == Settings.STConnectionPoint.Entrance) ? this.roomShortcutEntrances : this.roomExits) {
			int direction = 0;
			int x = entrance.x;
			int y = entrance.y;
			float x0 = x + 0.25f;
			float y0 = -y - 0.25f;
			float x1 = x + 0.75f;
			float y1 = -y - 0.75f;
			float x2 = (x0 + x1) * 0.5f;
			float y2 = (y0 + y1) * 0.5f;
			if ((this.GetTile(x + 1, y) & FLAG_SHORTCUT) > 0) {
				direction = 1;
			}
			if ((this.GetTile(x, y - 1) & FLAG_SHORTCUT) > 0) {
				direction = direction != 0 ? 128 : 2;
			}
			if ((this.GetTile(x - 1, y) & FLAG_SHORTCUT) > 0) {
				direction = direction != 0 ? 128 : 3;
			}
			if ((this.GetTile(x, y + 1) & FLAG_SHORTCUT) > 0) {
				direction = direction != 0 ? 128 : 4;
			}
			Themes.ThemeColor color = Themes.Layer1Color;
			if (direction != 0) {
				if (direction == 1) {
					this.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x0, y1, color),
						new Vertex(x1, y2, color)
					);
				}
				else if (direction == 2) {
					this.AddTriangle(
						new Vertex(x0, y1, color),
						new Vertex(x1, y1, color),
						new Vertex(x2, y0, color)
					);
				}
				else if (direction == 3) {
					this.AddTriangle(
						new Vertex(x1, y0, color),
						new Vertex(x1, y1, color),
						new Vertex(x0, y2, color)
					);
				}
				else if (direction == 4) {
					this.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x1, y0, color),
						new Vertex(x2, y1, color)
					);
				}
			}
			else
				this.AddQuad(
					new Vertex(x0, y0, color),
					new Vertex(x1, y0, color),
					new Vertex(x1, y1, color),
					new Vertex(x0, y1, color)
				);
		}

		foreach (Vector2i entrance in this.denShortcutEntrances) {
			this.AddQuad(
				new Vertex(entrance.x + 0.25f, -entrance.y - 0.25f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.75f, -entrance.y - 0.25f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.75f, -entrance.y - 0.75f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.25f, -entrance.y - 0.75f, Themes.RoomShortcutDen)
			);
		}

		foreach (Vector2i entrance in this.allShortcutEntrances) {
			if (!this.denShortcutEntrances.Contains(entrance) & !this.roomShortcutEntrances.Contains(entrance)) {
				int direction = 0;
				int x = entrance.x;
				int y = entrance.y;
				float x0 = x + 0.25f;
				float y0 = -y - 0.25f;
				float x1 = x + 0.75f;
				float y1 = -y - 0.75f;
				float x2 = (x0 + x1) * 0.5f;
				float y2 = (y0 + y1) * 0.5f;
				if ((this.GetTile(x + 1, y) & FLAG_SHORTCUT) > 0) {
					direction = 1;
				}
				if ((this.GetTile(x, y - 1) & FLAG_SHORTCUT) > 0) {
					direction = direction != 0 ? 128 : 2;
				}
				if ((this.GetTile(x - 1, y) & FLAG_SHORTCUT) > 0) {
					direction = direction != 0 ? 128 : 3;
				}
				if ((this.GetTile(x, y + 1) & FLAG_SHORTCUT) > 0) {
					direction = direction != 0 ? 128 : 4;
				}
				Themes.ThemeColor color = Themes.RoomShortcutRoom;
				if (direction != 0) {
					if (direction == 1) {
						this.AddTriangle(
							new Vertex(x0, y0, color),
							new Vertex(x0, y1, color),
							new Vertex(x1, y2, color)
						);
					}
					else if (direction == 2) {
						this.AddTriangle(
							new Vertex(x0, y1, color),
							new Vertex(x1, y1, color),
							new Vertex(x2, y0, color)
						);
					}
					else if (direction == 3) {
						this.AddTriangle(
							new Vertex(x1, y0, color),
							new Vertex(x1, y1, color),
							new Vertex(x0, y2, color)
						);
					}
					else if (direction == 4) {
						this.AddTriangle(
							new Vertex(x0, y0, color),
							new Vertex(x1, y0, color),
							new Vertex(x2, y1, color)
						);
					}
				}
				else {
					this.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x0, y1, color),
						new Vertex(x2, y2, color)
					);
					this.AddTriangle(
						new Vertex(x2, y0, color),
						new Vertex(x2, y1, color),
						new Vertex(x1, y2, color)
					);
				}
			}
		}

		if (this._vao == 0) {
			this._vao = Program.gl.GenVertexArray();
			this._vbo = Program.gl.GenBuffer();
			this._ebo = Program.gl.GenBuffer();
		}

		Span<Vertex> vertices = CollectionsMarshal.AsSpan(this.vertices);
		Span<uint> indices = CollectionsMarshal.AsSpan(this.indices);

		Program.gl.BindVertexArray(this._vao);

		Program.gl.BindBuffer(BufferTargetARB.ArrayBuffer, this._vbo);
		fixed (Vertex* ptr = vertices) {
			Program.gl.BufferData(BufferTargetARB.ArrayBuffer, (nuint) (vertices.Length * sizeof(Vertex)), ptr, BufferUsageARB.StaticDraw);
		}

		Program.gl.BindBuffer(BufferTargetARB.ElementArrayBuffer, this._ebo);
		fixed (uint* ptr = indices) {
			Program.gl.BufferData(BufferTargetARB.ElementArrayBuffer, (nuint) (indices.Length * sizeof(uint)), ptr, BufferUsageARB.StaticDraw);
		}

		Program.gl.VertexAttribPointer(0, 2, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) 0);
		Program.gl.EnableVertexAttribArray(0);

		Program.gl.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, (uint) sizeof(Vertex), (void*) (sizeof(float) * 2));
		Program.gl.EnableVertexAttribArray(1);

		Program.gl.UseProgram(Preload.RoomShader);
		this._projLoc = Program.gl.GetUniformLocation(Preload.RoomShader, "projection");
		this._modelLoc = Program.gl.GetUniformLocation(Preload.RoomShader, "model");
		this._tintLoc = Program.gl.GetUniformLocation(Preload.RoomShader, "tintColor");
		this._tintStrengthLoc = Program.gl.GetUniformLocation(Preload.RoomShader, "tintStrength");
		Program.gl.UseProgram(0);

		Program.gl.BindVertexArray(0);
		Program.gl.BindBuffer(BufferTargetARB.ArrayBuffer, 0);
		Program.gl.BindBuffer(BufferTargetARB.ElementArrayBuffer, 0);
	}

	public void DrawBlack(WorldWindow.RoomPosition positionType) {
		Immediate.Color(Themes.RoomSolid);
		if (this.data.hidden) {
			Immediate.Alpha(0.5f);
		}

		Vector2 position = positionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
		UI.FillRect(position.x, position.y - this.height, position.x + this.width, position.y);
	}
	bool drawWireFrames = false;

	public unsafe virtual void Draw(WorldWindow.RoomPosition positionType) {
		if (this.drawWireFrames) {
			Program.gl.PolygonMode(GLEnum.FrontAndBack, GLEnum.Line);
		}
		Vector2 position = positionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;

		if (!this.valid) {
			Immediate.Color(1f, 0f, 0f);
			Immediate.Begin(Immediate.PrimitiveType.LINES);
			Immediate.Vertex(position.x, position.y);
			Immediate.Vertex(position.x + this.width, position.y - this.height);
			Immediate.Vertex(position.x + this.width, position.y);
			Immediate.Vertex(position.x, position.y - this.height);
			Immediate.End();

			UI.StrokeRect(position.x, position.y, position.x + this.width, position.y - this.height);

			return;
		}

		Program.gl.Enable(EnableCap.Blend);
		if (this.data.waterHeight != -1 && !this.data.waterInFront) {
			Immediate.Color(Themes.RoomWater);
			UI.FillRect(position.x, position.y - (this.height - MathF.Min(this.data.waterHeight + 0.5f, this.height)), position.x + this.width, position.y - this.height);
		}

		Color tint = this.GetTintColor();

		Program.gl.BindVertexArray(this._vao);
		Program.gl.UseProgram(Preload.RoomShader);

		Vector2 matrixPos = WorldWindow.cameraOffset;
		Vector2 matrixScale = WorldWindow.cameraScale * Main.screenBounds;
		Program.gl.UniformMatrix4(this._projLoc, false, [..Matrix4X4.CreateOrthographicOffCenter(-matrixScale.x + matrixPos.x, matrixScale.x + matrixPos.x, -matrixScale.y + matrixPos.y, matrixScale.y + matrixPos.y, 0f, 1f)]);
		Program.gl.UniformMatrix4(this._modelLoc, false, [..Matrix4X4.CreateTranslation(position.x, position.y, 0f)]);

		float alpha = this.data.hidden ? 0.5f : tint.a;
		if (positionType != WorldWindow.PositionType) {
			alpha *= 0.5f;
		}

		Program.gl.Uniform4(this._tintLoc, tint.r, tint.g, tint.b, alpha);
		Program.gl.Uniform1(this._tintStrengthLoc, Settings.RoomTintStrength);

		Program.gl.DrawElements(PrimitiveType.Triangles, (uint) this.indices.Count, DrawElementsType.UnsignedInt, (void*) 0);

		Program.gl.BindVertexArray(0);
		Program.gl.UseProgram(0);

		if (this.data.waterHeight != -1 && this.data.waterInFront) {
			Immediate.Color(Themes.RoomWater);
			UI.FillRect(position.x, position.y - (this.height - MathF.Min(this.data.waterHeight + 0.5f, this.height)), position.x + this.width, position.y - this.height);
		}
		if (this.drawWireFrames) {
			Program.gl.PolygonMode(GLEnum.FrontAndBack, GLEnum.Fill);
		}

		Program.gl.Disable(EnableCap.Blend);

		if (positionType == WorldWindow.PositionType) {
			if (WorldWindow.VisibleDevItems) {
				foreach (RoomData.DevItem item in this.data.devItems) {
					if (item.texture == null) continue;

					float x = position.x + item.position.x;
					float y = position.y - this.height + item.position.y;
					UI.CenteredTexture(item.texture, x, y, 0.5f);
				}
			}

			for (int i = 0; i < this.denShortcutEntrances.Count; i++) {
				this.DrawDen(this.dens[i], position.x + this.denShortcutEntrances[i].x, position.y - this.denShortcutEntrances[i].y, i == this.hoveredDen);
			}

			for (uint i = 0; i < this.roomShortcutEntrances.Count; i++) {
				Vector2 entrancePos = this.GetRoomEntrancePosition(i);
				Vector2 exitPos = this.GetRoomExitPosition(i);
				bool connected = this.AnyConnectionConnectedTo(i);

				// Shortcut Entrance
				Immediate.Color(connected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
				if (WorldWindow.changeConnectBehaviour)
					UI.StrokeCircle(entrancePos, WorldWindow.SelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
				else
					UI.FillCircle(entrancePos, WorldWindow.SelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);

				// Room Exit
				Immediate.Color(connected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
				if (WorldWindow.changeConnectBehaviour)
					UI.FillCircle(exitPos, WorldWindow.SelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
				else
					UI.StrokeCircle(exitPos, WorldWindow.SelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);

				// Find the index of the connection associated with this RoomExit (if it's connected to something)
				int getConnectionIndex = 0;
				bool connectionFound = false;
				if (connected) {
					for (int j = 0; j < this.connections.Count; j++) {
						int connection = this.connections[j].roomA == this ? (int)this.connections[j].connectionA : (int)this.connections[j].connectionB;
						if (connection == i) {
							connectionFound = true;
							getConnectionIndex = j;
							break;
						}
					}
				}

				// Draws shortcutpath if either the associated exit or connection is hovered over.
				if(i == this.hoveredRoomExit || connectionFound && this.connections[getConnectionIndex].Hovered || Keys.Modifier(Silk.NET.SDL.Keymod.Shift)) {
					Vector2i roomEntranceShortcutPosition = this.GetRoomEntranceShortcutPosition(i);
					if (this.shortcutPaths.TryGetValue(roomEntranceShortcutPosition, out var result)) {
						Immediate.Color(Themes.RoomConnectionHover);
						foreach (Vector2i dot in result.Item1) { // DRAWING SHORTCUT PATH
							UI.FillCircle((dot + new Vector2(0.5f, 0.5f)) * new Vector2(1, -1) + this.Position, this.roomExits.Contains(dot) ? 0.4f : 0.3f , 8);
						}
					}
				}
			}
		}

		if (this.TimelineType != TimelineType.All) {
			int i = 0;
			foreach (string timeline in this.Timelines) {
				UI.CenteredTexture(ConditionalTimelineTextures.GetTexture(timeline), (float) (position.x + i * 4f + 1.5f), (float) (position.y - 1.5f), WorldWindow.SelectorScale);
				i++;
			}

			if (this.Timelines.Count > 0 && this.TimelineType == TimelineType.Except) {
				Immediate.Color(1f, 0f, 0f);
				UI.Line(position.x + 2f - WorldWindow.SelectorScale * 0.5f, position.y - 2f, position.x + 2f + WorldWindow.SelectorScale * 0.5f + (this.Timelines.Count - 1) * 4f, position.y - 2f, WorldWindow.SelectorScale * 4f);
			}
		}

		Vector2 o = WorldWindow.worldMouse - position;
		bool hovered = o.x >= 0f && o.y <= 0f && o.x <= this.width && o.y >= -this.height;
		Immediate.Color(hovered ? Themes.RoomBorderHighlight : Themes.RoomBorder);
		UI.StrokeRect(position.x, position.y, position.x + this.width, position.y - this.height);
	}

	protected void DrawDen(Den den, float x, float y, bool hovered) {
		bool denEmpty = true;

		for (int i = 0; i < den.creatures.Count; i++) {
			DenCreature creature = den.creatures[i];
			if (creature.type.IsNullOrEmpty() && creature.lineageTo == null) continue;

			float scale = WorldWindow.SelectorScale;
			float rectX = x + i * scale - (den.creatures.Count - 1f) * 0.5f * scale;
			float rectY = y;

			if (hovered) scale *= 1.5f;

			if (!creature.type.IsNullOrEmpty()) {
				denEmpty = false;
				UI.CenteredTexture(CreatureTextures.GetTexture(creature.type), rectX, rectY, scale);
			}

			if (creature.lineageTo == null) {
				Immediate.Color(Color.White);
				UI.font.Write(creature.count.ToString(), rectX + 0.5f + scale * 0.25f, rectY - 0.5f - scale * 0.5f, 0.5f * scale, Font.Align.MiddleCenter);
			} else {
				while (creature.lineageTo != null) {
					float chance = creature.lineageChance;
					creature = creature.lineageTo;
					rectY -= WorldWindow.SelectorScale;
					if (!creature.type.IsNullOrEmpty()) {
						UI.CenteredTexture(CreatureTextures.GetTexture(creature.type), rectX, rectY, scale);
					}
					Immediate.Color(Color.White);
					UI.font.Write((int) (chance * 100f) + "%", rectX + 0.5f + scale * 0.25f, rectY + WorldWindow.SelectorScale - 0.4f - scale * 0.5f, 0.3f * scale, Font.Align.MiddleCenter);
				}
			}
		}

		if (denEmpty) {
			Immediate.Color(Themes.RoomShortcutDen);
			UI.FillCircle(x + 0.5f, y - 0.5f, WorldWindow.SelectorScale * (hovered ? 1.5f : 1f) * 0.25f, 8);
		}
	}

	protected Color GetTintColor() {
		switch (WorldWindow.ColorType) {

			case WorldWindow.RoomColors.Layer:
				return this.data.layer switch {
					0 => Themes.Layer0Color,
					1 => Themes.Layer1Color,
					2 => Themes.Layer2Color,
					_ => Color.White,
				};

			case WorldWindow.RoomColors.Subregion: {
				if (this.data.subregion <= -1) {
					return WorldWindow.region.overrideSubregionColors.TryGetValue(-1, out Color value) ? value : Settings.NoSubregionColor;
				}
				else {
					if (WorldWindow.region.overrideSubregionColors.TryGetValue(this.data.subregion, out Color value)) {
						return value;
					}
					else {
						return Settings.SubregionColors.value.Length == 0
							? Settings.NoSubregionColor
							: Settings.SubregionColors.value[this.data.subregion % Settings.SubregionColors.value.Length];
					}
				}
			}

			default:
				return Color.White;
		}
	}

	#endregion
}