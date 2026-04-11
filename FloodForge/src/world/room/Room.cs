using FloodForge.Popups;
using Stride.Core.Extensions;

namespace FloodForge.World;

// TODO: Fix room water behind level rendering
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

	public string path;
	public string name;
	public TimelineType TimelineType;
	public HashSet<string> Timelines = [];
	public Vector2 CanonPosition;
	public Vector2 DevPosition;
	public Vector2 CanonVel;
	public Vector2 DevVel;
	public int width;
	public int height;
	public bool valid;
	public readonly RoomData data;
	public readonly RoomVisuals visuals;
	public uint[] geometry = null!;
	public List<(RoomExitType, Vector2i)> allRoomExitPoints = [];
	public List<Vector2i> allShortcutEntrancePoints = [];
	public List<Vector2i> roomExits = [];
	public Dictionary<Vector2i, RoomConnection> roomExitPaths = [];
	public Dictionary<Vector2i, (RoomConnection, bool matchesWithRoomExitPath)> shortcutEntrancePaths = [];
	public List<Vector2i> denShortcutEntrances = [];
	public int nonDenExitCount = 0;
	public List<Den> dens = [];
	public List<GarbageWormDen> garbageWormDens = [];
	public int hoveredDen = -1; // LATER: Remove / improve
	public int hoveredRoomExit = -1; // LATER: Remove / improve
	public int hoveredShortcutEntrance = -1;

	public List<Connection> connections = [];

	// IDEA: Room alerts/hints? (an exclamation mark that appears above a room's corner if there's something of note - softlocking shortcuts, lack of cameras)
	// then, this could also be added to connections so that a room that connects to the same room multiple times isn't allowed to exist without feedback

	private int specialExitCount = 0;
	public int GarbageWormDenIndex => this.specialExitCount + this.nonDenExitCount + this.denShortcutEntrances.Count;

	public bool Visible => WorldWindow.VisibleLayers[this.data.layer];

	public Room(string path, string name) {
		this.path = path;
		this.name = name;
		this.TimelineType = TimelineType.All;

		this.CanonPosition = Vector2.Zero;
		this.DevPosition = Vector2.Zero;
		this.width = 1;
		this.height = 1;
		this.valid = false;

		this.data = new RoomData();
		this.visuals = new RoomVisuals(this);

		try {
			this.LoadGeometry();
			this.LoadSettings();
			this.visuals.Refresh();
			this.GenerateMesh();
			this.CheckImages();
		}
		catch (Exception e) {
			Logger.Error($"Failed to load {this.name}!\n{e}");
			try { 
				this.SetToInvalidRoom();
				PopupManager.Add(new InfoPopup($"Failed to load {this.name}!")); 
			} catch {}
		}
	}

	public void SetToInvalidRoom() {
		this.valid = false;
		this.width = 72;
		this.height = 43;
		this.geometry = new uint[this.width * this.height];
	}

	public bool HasDen(int id) {
		return this.HasDen01(id - this.nonDenExitCount) || id == this.GarbageWormDenIndex;
	}

	public bool HasDen01(int id) {
		return id >= 0 && id < this.dens.Count;
	}

	public Den GetDen(int id) {
		return this.GetDen01(id - this.nonDenExitCount);
	}

	public int GetDenId(Vector2i pos) {
		return this.denShortcutEntrances.IndexOf(pos) + this.nonDenExitCount;
	}

	public int GetDenId01(Vector2i pos) {
		return this.denShortcutEntrances.IndexOf(pos);
	}

	public Den GetDen01(int id) {
		if (id < 0 || id >= this.dens.Count) {
			throw new Exception($"Invalid Den {id} for {this.name}");
		}

		return this.dens[id];
	}

	public bool ValidConnection(uint index) {
		return index < this.roomExits.Count;
	}

	public void Connect(Connection connection) {
		this.connections.Add(connection);
	}

	public void Disconnect(Connection connection) {
		this.connections.Remove(connection);
	}

	public void MoveUpdate() {
		foreach(Connection connection in this.connections) {
			connection.recalculateBezier = true;
		}
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
		if (!File.Exists(this.path)) {
			Logger.Warn($"Failed to load '{this.name}'. File '{this.path}' doesn't exist");
			this.SetToInvalidRoom();
			return;
		}

		string[] lines = File.ReadAllLines(this.path);

		string[] levelData = lines[1].Split('|');
		if(levelData.Length <= 0) {
			Logger.Warn($"Failed to load '{this.name}'. File contains no leveldata.");
			this.SetToInvalidRoom();
			return;
		}
		this.width = int.Parse(levelData[0][..levelData[0].IndexOf('*')]);
		this.height = int.Parse(levelData[0][(levelData[0].IndexOf('*') + 1)..]);
		this.geometry = new uint[this.width * this.height];
		if (levelData.Length == 1) {
			this.data.waterHeight = -1;
			this.data.waterInFront = false;
		}
		else {
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
						this.allRoomExitPoints.Add((RoomExitType.Room, new Vector2i(idx / this.height, idx % this.height)));
						break;

					case 5:
						this.geometry[idx] = this.geometry[idx] | FLAG_DEN | FLAG_SHORTCUT;
						this.allRoomExitPoints.Add((RoomExitType.Den, new Vector2i(idx / this.height, idx % this.height)));
						break;

					case 12:
						this.geometry[idx] = this.geometry[idx] | FLAG_SCAVENGER_DEN | FLAG_SHORTCUT;
						this.allRoomExitPoints.Add((RoomExitType.Scavenger, new Vector2i(idx / this.height, idx % this.height)));
						break;
				}
			}
			idx++;
		}

		this.valid = true;
		this.CheckShortcutEntrancePoints();

		idx = 0;
		for (int x = 0; x < this.width; x++) {
			for (int y = 0; y < this.height; y++) {
				if ((this.geometry[idx] & 15) == 2) {
					int bits = 0;
					bits += (this.GetTile(x - 1, y) % 16 == 1u) ? 1 : 0;
					bits += (this.GetTile(x + 1, y) % 16 == 1u) ? 2 : 0;
					bits += (this.GetTile(x, y - 1) % 16 == 1u) ? 4 : 0;
					bits += (this.GetTile(x, y + 1) % 16 == 1u) ? 8 : 0;
					int type = -1;

					if (bits == 1 + 4) type = 0;
					else if (bits == 1 + 8) type = 1;
					else if (bits == 2 + 4) type = 2;
					else if (bits == 2 + 8) type = 3;

					if (type == -1) {
						if (Settings.DEBUGLogInvalidSlopes) {
							Logger.Note($"Invalid slope type {this.name}({x}, {y})");
						}
					}
					else {
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

	protected virtual void LoadSettings() {
		if (this.path.IsNullOrEmpty()) return;

		this.data.objects.Clear();

		string folder = Path.GetDirectoryName(this.path)!;
		string? settingsPath = PathUtil.FindFile(folder, this.name + "_settings.txt");
		if (settingsPath == null) return;

		foreach (string line in File.ReadLines(settingsPath)) {
			if (line.StartsWith("PlacedObjects: ")) {
				string data = line["PlacedObjects: ".Length..];
				string[] poData = data.Split([", "], StringSplitOptions.RemoveEmptyEntries);

				foreach (string po in poData) {
					try {
						int start = po.IndexOf('<');
						int next = po.IndexOf('>', start);
						int end = po.IndexOf('>', next + 1);

						string xStr = po.Substring(start + 1, next - start - 1);
						string yStr = po.Substring(next + 2, end - next - 2);
						string last = po[(end + 2)..];

						Vector2 pos = new Vector2(float.Parse(xStr), float.Parse(yStr));
						string[] splits = last.Split('~');

						if (po.StartsWith("TerrainHandle>")) {
							TerrainHandleObject obj = new TerrainHandleObject();
							obj.nodes[0].position = pos;
							if (splits.Length >= 4) {
								obj.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
								obj.nodes[2].position = new Vector2(float.Parse(splits[2]), float.Parse(splits[3]));
							}
							this.data.objects.Add(obj);
						}
						else if (po.StartsWith("MudPit>")) {
							MudPitObject obj = new MudPitObject();
							obj.nodes[0].position = pos;
							if (splits.Length >= 2) {
								obj.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
							}
							this.data.objects.Add(obj);
						}
						else if (po.StartsWith("AirPocket>")) {
							AirPocketObject obj = new AirPocketObject();
							obj.nodes[0].position = pos;
							if (splits.Length >= 6) {
								obj.nodes[1].position = new Vector2(float.Parse(splits[0]), float.Parse(splits[1]));
								obj.nodes[2].position.y = float.Parse(splits[5]);
							}
							this.data.objects.Add(obj);
						}
						else {
							string[] splits2 = po.Split('>');
							string key = splits2[0];

							Texture texture = CreatureTextures.GetTexture($"room-{key}");
							if (texture == CreatureTextures.UnknownCreature) continue;

							GenericItemObject obj = new GenericItemObject(key, texture);
							obj.nodes[0].position = pos;
							this.data.objects.Add(obj);
						}
					} catch {
						Logger.Warn("Failed to parse Placed Object: " + po);
					}
				}
			}
		}
	}

	public struct RoomConnection (RoomPath path, RoomPathEndType startType, RoomPathEndType endType) {
		public RoomPath path = path;
		public RoomPathEndType startType = startType;
		public RoomPathEndType endType = endType;
	}

	public enum RoomPathEndType {
		deadend,
		shortcutEntrance,
		roomExit,
		den,
		wackAMoleHole,
		scavengerDen
	}

	public class RoomPath {
		public Vector2i[] Path;
		public Vector2i StartPosition {
			get {
				return this.Path?.Length > 0 ? this.Path[0] : Vector2i.Zero;
			}
		}
		public Vector2i EndPosition {
			get {
				return this.Path?.Length > 1 ? this.Path[^1] : this.StartPosition;
			}
		}
		public Vector2i StartDirection {
			get {
				return this.Path.Length > 1 ? (this.Path[1] - this.Path[0]) * new Vector2i(-1, 1) : Vector2i.Zero;
			}
		}
		public bool isDeadEnd = false;
		public Vector2i EndDirection => (this.isDeadEnd || this.Path.Length <= 1) ? Vector2i.Zero : (this.Path[^1] - this.Path[^2]);
		public RoomPath(Room room, Vector2i startPosition) {
			Vector2i forwardDirection = Vector2i.Zero;
			Vector2i currentPosition = startPosition;
			bool hasDirection = true;
			if (room.TileIsShortcut(currentPosition.x - 1, currentPosition.y)) {
				forwardDirection.x = -1;
			}
			else if (room.TileIsShortcut(currentPosition.x, currentPosition.y + 1)) {
				forwardDirection.y = 1;
			}
			else if (room.TileIsShortcut(currentPosition.x + 1, currentPosition.y)) {
				forwardDirection.x = 1;
			}
			else if (room.TileIsShortcut(currentPosition.x, currentPosition.y - 1)) {
				forwardDirection.y = -1;
			}

			List<Vector2i> pathTaken = [];
			pathTaken.Add(currentPosition);
			uint pathEndFlag = FLAG_ROOM_EXIT | FLAG_WACK_A_MOLE_HOLE | FLAG_DEN | FLAG_SCAVENGER_DEN;
			for (int runs = 0; runs < 10000; runs++) {
				currentPosition += forwardDirection;
				if(!pathTaken.Contains(currentPosition))
					pathTaken.Add(currentPosition);

				if (!room.TileIsShortcut(currentPosition.x + forwardDirection.x, currentPosition.y + forwardDirection.y)) {
					Vector2i lastDirection = forwardDirection;

					forwardDirection.x = 0;
					forwardDirection.y = 0;
					hasDirection = false;
					if (lastDirection.x != 1 && (room.TileIsShortcut(currentPosition.x - 1, currentPosition.y) || ((room.GetTile(currentPosition.x - 1, currentPosition.y) & pathEndFlag) > 0))) {
						forwardDirection.x = -1;
						hasDirection = true;
					}
					else if (lastDirection.y != -1 && room.TileIsShortcut(currentPosition.x, currentPosition.y + 1) || ((room.GetTile(currentPosition.x, currentPosition.y + 1) & pathEndFlag) > 0)) {
						forwardDirection.y = 1;
						hasDirection = true;
					}
					else if (lastDirection.x != -1 && room.TileIsShortcut(currentPosition.x + 1, currentPosition.y) || ((room.GetTile(currentPosition.x + 1, currentPosition.y) & pathEndFlag) > 0)) {
						forwardDirection.x = 1;
						hasDirection = true;
					}
					else if (lastDirection.y != 1 && room.TileIsShortcut(currentPosition.x, currentPosition.y - 1) || ((room.GetTile(currentPosition.x, currentPosition.y - 1) & pathEndFlag) > 0)) {
						forwardDirection.y = -1;
						hasDirection = true;
					}
				}

				if ((room.GetTile(currentPosition.x, currentPosition.y) & 15) == 4 || (room.GetTile(currentPosition.x, currentPosition.y) & pathEndFlag) > 0) {
					hasDirection = true;
					break;
				}

				if (!hasDirection) break;
				if (runs + 1 == 10000) hasDirection = false;
			}
			this.Path = pathTaken.ToArray();
			this.isDeadEnd = !hasDirection;
		}
	}
	
	void CheckShortcutEntrancePoints() {
		this.allShortcutEntrancePoints.Clear();
		for (int y = 0; y < this.height; y++) {
			for (int x = 0; x < this.width; x++) {
				if ((this.GetTile(x, y) & FLAG_SHORTCUT) > 0) {
					int[] tiles = new int[9];
					
					int index = 0;
					for (int y2 = y - 1; y2 < y + 2; y2++) {
						for(int x2 = x - 1; x2 < x + 2; x2++) {
							int result = ((this.GetTile(x2, y2)&15) == 1) ? 1 : 0;					//  1 == solid
							result += ((this.GetTile(x2, y2)&FLAG_SHORTCUT) > 0) ? 2 : 0;			//  2 == shortcut
							result += ((this.GetTile(x2, y2)&FLAG_ROOM_EXIT) > 0) ? 4 : 0;			//  4 == roomexit
							result += ((this.GetTile(x2, y2)&FLAG_DEN) > 0) ? 8 : 0;				//  8 == den
							result += ((this.GetTile(x2, y2)&FLAG_SCAVENGER_DEN) > 0) ? 16 : 0;		// 16 == scav
							result += ((this.GetTile(x2, y2)&FLAG_WACK_A_MOLE_HOLE) > 0) ? 32 : 0;	// 32 == wack-a-mole-hole
							tiles[index] = result;
							index++;
						}
					}

					int directionCount = 0;
					int airGaps = 0;

					// only check rest if the tile is just a shortcut
					if(tiles[4] == 2 || tiles[4] == 3) {
						// check if all corners are solid
						if((tiles[0] & 1) == 0 || (tiles[2] & 1) == 0 || (tiles[6] & 1) == 0 || (tiles[8] & 1) == 0) airGaps = 99;

						int dirFlags = 0;
						int airFlags = 0;
						if((tiles[1] & 62) > 0) {
							directionCount++; 
							dirFlags |= 1; }
						if((tiles[1] & 1) == 0) {
							airGaps++;
						 	airFlags |= 8; }
						if((tiles[3] & 62) > 0) {
							directionCount++; 
							dirFlags |= 2; }
						if((tiles[3] & 1) == 0) {
							airGaps++;
						 	airFlags |= 4; }
						if((tiles[5] & 62) > 0) {
							directionCount++; 
							dirFlags |= 4; }
						if((tiles[5] & 1) == 0) {
							airGaps++;
						 	airFlags |= 2; }
						if((tiles[7] & 62) > 0) {
							directionCount++; 
							dirFlags |= 8; }
						if((tiles[7] & 1) == 0) {
							airGaps++;
						 	airFlags |= 1; }
						
						// check:
						// - that only one of the sides is air,
						// - that only one of the sides has direction
						// - that the shortcut's direction and the airgap's direction are correct
						// (correct as in opposite, which is why the bit assignments are opposite)
						if((directionCount == 1) && (airGaps == 1) && (airFlags == dirFlags)) {
							this.allShortcutEntrancePoints.Add(new Vector2i(x, y));
						}
					}
				}
			}
		}
	}

	protected void EnsureConnections() {
		this.specialExitCount = 0;
		this.nonDenExitCount = 0;
		this.roomExits.Clear();
		this.roomExitPaths.Clear();
		this.shortcutEntrancePaths.Clear();
		this.denShortcutEntrances.Clear();
		
		List<(RoomExitType type, Vector2i position)> newList = [];
		for (int y = 0; y < this.height; y++) {
			for (int x = 0; x < this.width; x++) {
				foreach ((RoomExitType _, Vector2i position) item in this.allRoomExitPoints) {
					if (item.position == new Vector2i(x, y)) {
						newList.Add(item);
					}
				}
			}
		}
		this.allRoomExitPoints = newList[..];

		for (int i = 0; i < this.allRoomExitPoints.Count; i++) {
			if(this.allRoomExitPoints[i].Item1 == RoomExitType.Room) this.roomExits.Add(this.allRoomExitPoints[i].Item2);

			RoomPath roomExitPath = new(this, this.allRoomExitPoints[i].Item2);

			uint Tile = this.GetTile(roomExitPath.StartPosition);

			RoomPathEndType startType = RoomPathEndType.roomExit;
			if ((Tile & FLAG_DEN) > 0) startType = RoomPathEndType.den;
			else if ((Tile & FLAG_SCAVENGER_DEN) > 0) startType = RoomPathEndType.scavengerDen;
			else if ((Tile & FLAG_WACK_A_MOLE_HOLE) > 0) startType = RoomPathEndType.wackAMoleHole;

			Tile = this.GetTile(roomExitPath.EndPosition);

			RoomPathEndType endType = RoomPathEndType.deadend;
			if (!roomExitPath.isDeadEnd) {
				endType = RoomPathEndType.shortcutEntrance;
				if ((Tile & FLAG_ROOM_EXIT) > 0) endType = RoomPathEndType.roomExit;
				else if ((Tile & FLAG_DEN) > 0) endType = RoomPathEndType.den;
				else if ((Tile & FLAG_SCAVENGER_DEN) > 0) endType = RoomPathEndType.scavengerDen;
				else if ((Tile & FLAG_WACK_A_MOLE_HOLE) > 0) endType = RoomPathEndType.wackAMoleHole;
			}
			
			if(startType == RoomPathEndType.roomExit) {
				this.roomExitPaths.Add(this.allRoomExitPoints[i].Item2, new RoomConnection(roomExitPath, startType, endType));
			}
		}

		for (int i = 0; i < this.allShortcutEntrancePoints.Count; i++) {
			RoomPath shortcutPath = new(this, this.allShortcutEntrancePoints[i]);

			RoomPathEndType startType = RoomPathEndType.shortcutEntrance;

			uint Tile = this.GetTile(shortcutPath.EndPosition);
			RoomPathEndType endType = RoomPathEndType.deadend;
			if (!shortcutPath.isDeadEnd) {
				endType = RoomPathEndType.shortcutEntrance;
				if ((Tile & FLAG_ROOM_EXIT) > 0) endType = RoomPathEndType.roomExit;
				else if ((Tile & FLAG_DEN) > 0) endType = RoomPathEndType.den;
				else if ((Tile & FLAG_SCAVENGER_DEN) > 0) endType = RoomPathEndType.scavengerDen;
				else if ((Tile & FLAG_WACK_A_MOLE_HOLE) > 0) endType = RoomPathEndType.wackAMoleHole;
			}
			bool hasMatchingRoomExit = false;
			if(this.roomExitPaths.TryGetValue(shortcutPath.EndPosition, out RoomConnection value)){
				if(value.endType == RoomPathEndType.shortcutEntrance && value.path.EndPosition == shortcutPath.StartPosition) {
					hasMatchingRoomExit = true;
				}
			}
			this.shortcutEntrancePaths.Add(this.allShortcutEntrancePoints[i], (new RoomConnection(shortcutPath, startType, endType), hasMatchingRoomExit));
			if(endType == RoomPathEndType.den) {
				this.denShortcutEntrances.Add(shortcutPath.StartPosition);
			}
			else if(endType == RoomPathEndType.roomExit) {
				this.nonDenExitCount++;
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

		string path = PathUtil.Parent(this.path);
		for (int i = 0; i < this.data.cameras.Count; i++) {
			string imageFile = $"{this.name}_{i + 1}.png";

			if (PathUtil.FindFile(path, imageFile) == null) {
				Logger.Warn($"{this.name} is missing image {imageFile}");
			}
		}
	}

	public void RegenerateGeometry() {
		this.allRoomExitPoints.Clear();
		this.allShortcutEntrancePoints.Clear();

		int idx = 0;
		for (int x = 0; x < this.width; x++) {
			for (int y = 0; y < this.height; y++) {
				if ((this.geometry[idx] & FLAG_ROOM_EXIT) > 0) {
					this.allRoomExitPoints.Add((RoomExitType.Room, new Vector2i(x, y)));
				}
				if ((this.geometry[idx] & FLAG_DEN) > 0) {
					this.allRoomExitPoints.Add((RoomExitType.Den, new Vector2i(x, y)));
				}
				if ((this.geometry[idx] & FLAG_SCAVENGER_DEN) > 0) {
					this.allRoomExitPoints.Add((RoomExitType.Scavenger, new Vector2i(x, y)));
				}
				idx++;
			}
		}

		this.CheckShortcutEntrancePoints();
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

	public bool AnyConnectionConnectedTo(uint i) {
		if (this.roomExits.Count <= i) return false;

		foreach (Connection connection in this.connections) {
			if (connection.roomA == this && connection.roomAExitID == i) return true;
			if (connection.roomB == this && connection.roomBExitID == i) return true;
		}

		return false;
	}

	#region Connection information methods
	public Vector2 GetConnectionConnectPoint(uint i) {
		if (!WorldWindow.changeConnectBehaviour) {
			RoomConnection connection = this.roomExitPaths[this.roomExits[(int)i]];
			if(connection.endType == RoomPathEndType.shortcutEntrance) {
				return this.RoomPositionToWorldPosition(this.roomExitPaths[this.roomExits[(int)i]].path.EndPosition);
			}	
		}
		return this.RoomPositionToWorldPosition(this.roomExits[(int)i]);
	}
	
	public Vector2i GetConnectionConnectDirection(uint i) {
		if (!WorldWindow.changeConnectBehaviour) {
			RoomConnection connection = this.roomExitPaths[this.roomExits[(int)i]];
			if(connection.endType == RoomPathEndType.shortcutEntrance) {
				return this.roomExitPaths[this.roomExits[(int)i]].path.EndDirection * new Vector2i(-1, 1);
			}	
		}
		return this.roomExitPaths[this.roomExits[(int)i]].path.StartDirection;
	}

	public Vector2 GetShortcutEntranceWorldPoint(uint i) {
		return this.RoomPositionToWorldPosition(this.roomExitPaths[this.roomExits[(int)i]].path.EndPosition);
	}
	public Vector2i GetShortcutEntranceRoomPoint(uint i) {
		return this.roomExitPaths[this.roomExits[(int)i]].path.EndPosition;
	}

	public Vector2i GetShortcutEntranceDirection(uint i) {
		return this.roomExitPaths[this.roomExits[(int)i]].path.EndDirection;
	}

	public uint GetRoomExitIDFromShortcut(uint i) {
		return (uint) this.roomExits.IndexOf(this.shortcutEntrancePaths[this.allShortcutEntrancePoints[(int)i]].Item1.path.EndPosition);
	}

	public uint GetShortcutEntranceDirectionInt(uint i) {
		if (i >= this.allShortcutEntrancePoints.Count) {
			throw new Exception($"Invalid shortcut index {i} for {this.name}");
		}
		Vector2i connection = this.allShortcutEntrancePoints[(int) i];

		if (this.TileIsShortcut(connection.x - 1, connection.y)) return Direction.Left;
		else if (this.TileIsShortcut(connection.x, connection.y + 1)) return Direction.Down;
		else if (this.TileIsShortcut(connection.x + 1, connection.y)) return Direction.Right;
		else if (this.TileIsShortcut(connection.x, connection.y - 1)) return Direction.Up;

		return Direction.Unknown;
	}

	public Vector2 RoomPositionToWorldPosition(Vector2i roomPosition) {
		return roomPosition * new Vector2i(1, -1) + new Vector2(0.5f, -0.5f) + this.Position;
	}
	#endregion

	public Vector2 Position {
		get {
			return WorldWindow.PositionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
		}

		set {
			if (WorldWindow.PositionType == WorldWindow.RoomPosition.Canon) {
				this.CanonPosition = value;
			}
			else {
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
			}
			else {
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
	protected class Mesh () {
		public uint currentIndex = 0;
		public List<Vertex> vertices = [];
		public List<uint> indices = [];

		public void Clear() {
			this.vertices.Clear();
			this.indices.Clear();
			this.currentIndex = 0;
		}
			
		public void AddQuad(float xPos, float yPos, Themes.ThemeColor theme) {
			this.AddQuad(new Vector2(xPos, yPos), Vector2.One, theme);
		}

		public void AddQuad(Vector2 centerPosition, Themes.ThemeColor theme) {
			this.AddQuad(centerPosition, Vector2.One, theme);
		}

		public void AddQuad(float xPos, float yPos, float scale, Themes.ThemeColor theme) {
			this.AddQuad(new Vector2(xPos, yPos), Vector2.One * scale, theme);
		}

		public void AddQuad(Vector2 centerPosition, float scale, Themes.ThemeColor theme) {
			this.AddQuad(centerPosition, Vector2.One * scale, theme);
		}

		public void AddQuad(float xPos, float yPos, Vector2 scale, Themes.ThemeColor theme) {
			this.AddQuad(new(xPos, yPos), scale, theme);
		}

		public void AddQuad(Vector2 centerPosition, Vector2 scale, Themes.ThemeColor theme) {
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

		public void AddQuad(Vertex a, Vertex b, Vertex c, Vertex d) {
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

		public void AddTriangle(Vertex a, Vertex b, Vertex c) {
			this.vertices.Add(a);
			this.vertices.Add(b);
			this.vertices.Add(c);
			this.indices.Add(this.currentIndex + 0);
			this.indices.Add(this.currentIndex + 1);
			this.indices.Add(this.currentIndex + 2);
			this.currentIndex += 3;
		}
	}

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

	protected Mesh roomMesh = new();
	protected uint _vao = 0;
	protected uint _vbo = 0;
	protected uint _ebo = 0;
	protected int _projLoc, _modelLoc, _tintLoc, _tintStrengthLoc;
	List<Vector2i> allShortcutEntrances = [];

	protected unsafe virtual void GenerateMesh() {
		this.roomMesh.Clear();
		this.allShortcutEntrances.Clear();

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
					this.roomMesh.AddQuad(x2, y2, Themes.RoomShortcutEntrance);
				}
				else if (type != 1) { // draws air if the tile isn't fully solid.
					Themes.ThemeColor air = ((tile & FLAG_BACKGROUND_SOLID) > 0) ? Themes.RoomLayer2Solid : Themes.RoomAir;

					if (type == 2) {
						uint direction = (tile >> 10) & 3;
						if (direction == 0) {
							this.roomMesh.AddTriangle(
								new Vertex(x1, y1, air),
								new Vertex(x0, y1, air),
								new Vertex(x1, y0, air)
							);
						}
						else if (direction == 1) {
							this.roomMesh.AddTriangle(
								new Vertex(x1, y0, air),
								new Vertex(x0, y0, air),
								new Vertex(x1, y1, air)
							);
						}
						else if (direction == 2) {
							this.roomMesh.AddTriangle(
								new Vertex(x0, y1, air),
								new Vertex(x1, y1, air),
								new Vertex(x0, y0, air)
							);
						}
						else if (direction == 3) {
							this.roomMesh.AddTriangle(
								new Vertex(x0, y0, air),
								new Vertex(x1, y0, air),
								new Vertex(x0, y1, air)
							);
						}
					}
					else {
						this.roomMesh.AddQuad(x2, y2, air); // possibility for greedy meshing, since right now every tile of air now gets its own quad.
					}
				}

				if (type == 3) {
					this.roomMesh.AddQuad(
						new Vertex(x0, y0, Themes.RoomPlatform),
						new Vertex(x1, y0, Themes.RoomPlatform),
						new Vertex(x1, y2, Themes.RoomPlatform),
						new Vertex(x0, y2, Themes.RoomPlatform)
					);
				}

				if ((tile & FLAG_VERTICAL_POLE) > 0) {
					this.roomMesh.AddQuad(x2, y2, new Vector2(0.25f, 1f), Themes.RoomPole);
				}

				if ((tile & FLAG_HORIZONTAL_POLE) > 0) {
					this.roomMesh.AddQuad(x2, y2, new Vector2(1f, 0.25f), Themes.RoomPole);
				}

				if ((tile & FLAG_SHORTCUT) > 0 && tile != 4) {
					// NOTE - Might add outlining RoomSolid if it looks weird
					this.roomMesh.AddQuad(x2, y2, 0.1875f, Themes.RoomShortcutDot);
				}
			}
		}

		foreach (Vector2i entrance in (Settings.ConnectionPoint.value == Settings.STConnectionPoint.Entrance) ? this.allShortcutEntrances : this.roomExits) {
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
			Color color = Themes.RoomShortcutArrow;
			if (direction != 0) {
				if (direction == 1) {
					this.roomMesh.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x0, y1, color),
						new Vertex(x1, y2, color)
					);
				}
				else if (direction == 2) {
					this.roomMesh.AddTriangle(
						new Vertex(x0, y1, color),
						new Vertex(x1, y1, color),
						new Vertex(x2, y0, color)
					);
				}
				else if (direction == 3) {
					this.roomMesh.AddTriangle(
						new Vertex(x1, y0, color),
						new Vertex(x1, y1, color),
						new Vertex(x0, y2, color)
					);
				}
				else if (direction == 4) {
					this.roomMesh.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x1, y0, color),
						new Vertex(x2, y1, color)
					);
				}
			}
			else
				this.roomMesh.AddQuad(
					new Vertex(x0, y0, color),
					new Vertex(x1, y0, color),
					new Vertex(x1, y1, color),
					new Vertex(x0, y1, color)
				);
		}

		foreach (Vector2i entrance in this.denShortcutEntrances) {
			this.roomMesh.AddQuad(
				new Vertex(entrance.x + 0.25f, -entrance.y - 0.25f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.75f, -entrance.y - 0.25f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.75f, -entrance.y - 0.75f, Themes.RoomShortcutDen),
				new Vertex(entrance.x + 0.25f, -entrance.y - 0.75f, Themes.RoomShortcutDen)
			);
		}

		foreach (Vector2i entrance in this.allShortcutEntrances) {
			if (!this.denShortcutEntrances.Contains(entrance) & !this.allShortcutEntrances.Contains(entrance)) {
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
				Color color = Themes.RoomShortcutArrow;
				if (direction != 0) {
					if (direction == 1) {
						this.roomMesh.AddTriangle(
							new Vertex(x0, y0, color),
							new Vertex(x0, y1, color),
							new Vertex(x1, y2, color)
						);
					}
					else if (direction == 2) {
						this.roomMesh.AddTriangle(
							new Vertex(x0, y1, color),
							new Vertex(x1, y1, color),
							new Vertex(x2, y0, color)
						);
					}
					else if (direction == 3) {
						this.roomMesh.AddTriangle(
							new Vertex(x1, y0, color),
							new Vertex(x1, y1, color),
							new Vertex(x0, y2, color)
						);
					}
					else if (direction == 4) {
						this.roomMesh.AddTriangle(
							new Vertex(x0, y0, color),
							new Vertex(x1, y0, color),
							new Vertex(x2, y1, color)
						);
					}
				}
				else {
					this.roomMesh.AddTriangle(
						new Vertex(x0, y0, color),
						new Vertex(x0, y1, color),
						new Vertex(x2, y2, color)
					);
					this.roomMesh.AddTriangle(
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

		Span<Vertex> vertices = CollectionsMarshal.AsSpan(this.roomMesh.vertices);
		Span<uint> indices = CollectionsMarshal.AsSpan(this.roomMesh.indices);

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

	public virtual void DrawBlack(WorldWindow.RoomPosition positionType) {
		Immediate.Color(Themes.RoomSolid);
		if (this.data.hidden) {
			Immediate.Alpha(0.5f);
		}

		Vector2 position = positionType == WorldWindow.RoomPosition.Canon ? this.CanonPosition : this.DevPosition;
		UI.FillRect(position.x, position.y - this.height, position.x + this.width, position.y);
	}

	private void DrawWater(Vector2 position) {
		Immediate.Color(Themes.RoomWater);
		if (!WorldWindow.VisibleDevItems) {
			UI.FillRect(position.x, position.y - this.height + MathF.Min(this.data.waterHeight + 0.5f, this.height), position.x + this.width, position.y - this.height);
			return;
		}

		Program.gl.Enable(EnableCap.Blend);
		foreach (RoomVisuals.WaterSpot spot in this.visuals.water) {
			Rect waterRect = Rect.FromSize(position.x + spot.pos.x / 20f, position.y - this.height + spot.pos.y / 20f, spot.size.x / 20f, spot.size.y / 20f);
			UI.FillRect(waterRect);
		}
		Program.gl.Disable(EnableCap.Blend);
	}

	public unsafe virtual void Draw(WorldWindow.RoomPosition positionType) {
		if (Settings.DEBUGRoomWireframe) {
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
			this.DrawWater(position);
		}

		Color tint = this.GetTintColor();
		if (WorldWindow.highlightRoom != null && WorldWindow.highlightRoom != this) {
			tint *= 0.25f;
		}

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

		Program.gl.DrawElements(PrimitiveType.Triangles, (uint) this.roomMesh.indices.Count, DrawElementsType.UnsignedInt, (void*) 0);

		Program.gl.BindVertexArray(0);
		Program.gl.UseProgram(0);

		if (this.data.waterHeight != -1 && this.data.waterInFront) {
			this.DrawWater(position);
		}
		if (WorldWindow.VisibleDevItems && this.visuals.hasTerrain && this.visuals.terrain.Count >= 2) {
			Immediate.Color(0f, 1f, 0f);
			Immediate.Begin(Immediate.PrimitiveType.LINE_STRIP);
			foreach (Vector2 point in this.visuals.terrain) {
				Immediate.Vertex(position.x + point.x / 20f, position.y + point.y / 20f - this.height);
			}
			Immediate.End();
		}
		if (Settings.DEBUGRoomWireframe) {
			Program.gl.PolygonMode(GLEnum.FrontAndBack, GLEnum.Fill);
		}

		Program.gl.Disable(EnableCap.Blend);

		if (positionType == WorldWindow.PositionType) {
			float clippedSelectorScale = Math.Min(WorldWindow.SelectorScale, 10f);
			if (WorldWindow.VisibleDevItems) {
				foreach (DevObject devObject in this.data.objects) {
					devObject.Draw(this.Position + new Vector2(0f, -this.height));
				}
			}

			for (int i = 0; i < this.roomExits.Count; i++) {
				Vector2 exitPos = this.RoomPositionToWorldPosition(this.roomExitPaths[this.roomExits[i]].path.StartPosition);
				Vector2 entrancePos = this.RoomPositionToWorldPosition(this.roomExitPaths[this.roomExits[i]].path.EndPosition);
				bool entranceIsShortcutEntrance = this.roomExitPaths[this.roomExits[i]].endType == RoomPathEndType.shortcutEntrance;
				bool connected = this.AnyConnectionConnectedTo((uint)i);

				// Shortcut Entrance
				Immediate.Color(connected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
				if(entranceIsShortcutEntrance) {
					if (WorldWindow.changeConnectBehaviour)
						UI.StrokeCircle(entrancePos, clippedSelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
					else
						UI.FillCircle(entrancePos, clippedSelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
				}

				// Room Exit
				if (WorldWindow.changeConnectBehaviour || !entranceIsShortcutEntrance)
					UI.FillCircle(exitPos, clippedSelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);
				else
					UI.StrokeCircle(exitPos, clippedSelectorScale * (i == this.hoveredRoomExit ? 1.5f : 1f) * (connected ? 0.5f : 1f) * 0.25f, 8);

				// Find the index of the connection associated with this RoomExit (if it's connected to something)
				int getConnectionIndex = 0;
				bool connectionFound = false;
				if (connected) {
					for (int j = 0; j < this.connections.Count; j++) {
						int connection = this.connections[j].roomA == this ? (int)this.connections[j].roomAExitID : (int)this.connections[j].roomBExitID;
						if (connection == i) {
							connectionFound = true;
							getConnectionIndex = j;
							break;
						}
					}
				}

				// Draws shortcutpath if either the associated exit or connection is hovered over.
				bool shouldBeHighlighted = (i == this.hoveredRoomExit || connectionFound && this.connections[getConnectionIndex].Hovered) && this.hoveredShortcutEntrance == -1;
				if(shouldBeHighlighted || Keys.Modifier(Keys.Modifiers.Shift)) {
					if (this.roomExitPaths.TryGetValue(this.roomExits[i], out var result)) {
						this.DrawRoomPath(result, i == this.hoveredRoomExit, shouldBeHighlighted);
					}
				}
			}
			
			if(Settings.DEBUGVisibleShortcutEntranceData){
				foreach((RoomConnection connection, bool isMatchedWithRoomExit) in this.shortcutEntrancePaths.Values) {
					Immediate.Color(isMatchedWithRoomExit ? Color.Black : connection.endType switch { 
						RoomPathEndType.deadend => new(1,0,0), 
						RoomPathEndType.shortcutEntrance => new(1, 1, 1), 
						RoomPathEndType.den => new(1, 1, 0), 
						RoomPathEndType.scavengerDen => new(0, 1, 0), 
						RoomPathEndType.roomExit => new(0.5f, 0.5f, 1), 
						RoomPathEndType.wackAMoleHole => new(0.2f, 0.4f, 0.6f),
						_ => Color.Black});
					UI.StrokeCircle(this.RoomPositionToWorldPosition(connection.path.StartPosition), isMatchedWithRoomExit ? 0.25f : 2f, 8);
					Immediate.Color(isMatchedWithRoomExit ? Color.Black : Color.Magenta);
					UI.StrokeCircle(this.RoomPositionToWorldPosition(connection.path.EndPosition), isMatchedWithRoomExit ? 0.25f : 1f, 8);
				}
			}
			// this bit handles the case where:
			// a shortcut entrance that connects to a roomexit, without said roomexit connecting back to the same entrance
			for (int i = 0; i < this.allShortcutEntrancePoints.Count; i++) {
				if(this.shortcutEntrancePaths.TryGetValue(this.allShortcutEntrancePoints[i], out (RoomConnection connection, bool isMatchedWithRoomExit) value)){
					bool entranceConnectedToRoomExit = value.connection.endType == RoomPathEndType.roomExit;
					if (!value.isMatchedWithRoomExit && entranceConnectedToRoomExit) {
						Vector2 entrancePos = this.RoomPositionToWorldPosition(this.shortcutEntrancePaths[this.allShortcutEntrancePoints[i]].Item1.path.StartPosition);
						Vector2 exitPos = this.RoomPositionToWorldPosition(this.shortcutEntrancePaths[this.allShortcutEntrancePoints[i]].Item1.path.EndPosition);
						uint exitID = this.GetRoomExitIDFromShortcut((uint)i);
						bool roomExitIsConnected = this.AnyConnectionConnectedTo(exitID);

						// Shortcut Entrance
						Immediate.Color(roomExitIsConnected ? Themes.RoomConnection : Themes.RoomShortcutRoom);
						if (WorldWindow.changeConnectBehaviour)
						{
							UI.StrokeCircle(entrancePos, clippedSelectorScale * (i == this.hoveredShortcutEntrance ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
							UI.FillCircle(exitPos, clippedSelectorScale * (i == this.hoveredShortcutEntrance ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
						}
						else
						{
							UI.FillCircle(entrancePos, clippedSelectorScale * (i == this.hoveredShortcutEntrance ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
							UI.StrokeCircle(exitPos, clippedSelectorScale * (i == this.hoveredShortcutEntrance ? 1.5f : 1f) * (roomExitIsConnected ? 0.5f : 1f) * 0.25f, 8);
						}

						// Draws shortcutpath if the connection is hovered over. (since a roomexit isn't related to this entrance
						// (otherwise it'd have been drawn with the roomExits), there is no exit to hover over that should highlight this shortcut entrance)
						bool shouldBeHighlighted = i == this.hoveredShortcutEntrance;
						if(shouldBeHighlighted || Keys.Modifier(Keys.Modifiers.Shift)) {
							this.DrawRoomPath(value.connection, shouldBeHighlighted, shouldBeHighlighted);
						}
					}
				}
			}

			if (WorldWindow.VisibleCreatures) {
				for (int i = 0; i < this.denShortcutEntrances.Count; i++) {
					this.DrawDen(this.dens[i], position.x + this.denShortcutEntrances[i].x, position.y - this.denShortcutEntrances[i].y, i == this.hoveredDen, WorldWindow.HoveringRoom == this);
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
		this.hoveredShortcutEntrance = -1;
	}

	protected void DrawRoomPath(RoomConnection connectionPathToDraw, bool isHovered, bool isHighlighted) {
		Vector2 positionOffset = this.Position + new Vector2(0.5f, -0.5f);
		Immediate.Color(isHovered ? Themes.RoomConnectionHover : Themes.RoomConnection);
		if(WorldWindow.changeConnectBehaviour && isHighlighted && (WorldWindow.cameraScale < 75f || Keys.Pressed(Silk.NET.Input.Key.P))) {
			bool drawnExit = false;
			foreach (Vector2i dot in connectionPathToDraw.path.Path) { // DRAWING SHORTCUT PATH, STARTS FROM ROOMEXIT, WHICH IS WHY IT DRAWS THE FIRST ORB BIGGER
				UI.FillCircle(dot * new Vector2(1, -1) + positionOffset, drawnExit ? 0.4f : 0.5f , 8);
				drawnExit = true; 
			}
		}
		else {
			Vector2i lastDir = Vector2i.Zero;
			Vector2i lastPos = Vector2i.Zero;
			Vector2i newDir;
			Vector2i pointA;
			Vector2i pointB;
			for (int j = 1; j < connectionPathToDraw.path.Path.Length; j++) {
				pointA = connectionPathToDraw.path.Path[j-1] * new Vector2i(1, -1);
				pointB = connectionPathToDraw.path.Path[j] * new Vector2i(1, -1);
				newDir = pointB - pointA;
				if(lastDir == Vector2i.Zero) {
					lastDir = newDir;
					lastPos = pointA;
				}
				else if(newDir != lastDir) {
					UI.Line(lastPos + positionOffset, pointA + positionOffset);
					lastPos = pointA;
					lastDir = newDir;
				}
				if(j == connectionPathToDraw.path.Path.Length - 1) {
					UI.Line(lastPos + positionOffset, pointB + positionOffset);
				}
			}
		}
	}

	protected void DrawDen(Den den, float x, float y, bool hovered, bool roomHovered) {
		bool denEmpty = true;
		bool drawnDen = false;

		float selectorScale = WorldWindow.SelectorScale;
		for (int i = 0; i < den.creatures.Count; i++) {
			DenCreature creature = den.creatures[i];
			if (creature.type.IsNullOrEmpty() && creature.lineageTo == null) continue;

			float scale = selectorScale;
			float rectX = x + i * scale - (den.creatures.Count - 1f) * 0.5f * scale;
			float rectY = y;

			if (hovered) scale *= 1.5f;

			if (!creature.type.IsNullOrEmpty()) {
				denEmpty = false;
			}
			if (WorldWindow.cameraScale < 1000f || roomHovered) {
				drawnDen = true;
				if(!denEmpty) {
					UI.CenteredTexture(CreatureTextures.GetTexture(creature.type), rectX, rectY, scale);
				}
				if (creature.lineageTo == null) {
					Immediate.Color(Color.White);
					UI.font.Write(creature.count.ToString(), rectX + 0.5f + scale * 0.25f, rectY - 0.5f - scale * 0.5f, 0.5f * scale, Font.Align.MiddleCenter);
				}
				else {
					while (creature.lineageTo != null) {
						float chance = creature.lineageChance;
						creature = creature.lineageTo;
						rectY -= selectorScale;
						if (!creature.type.IsNullOrEmpty()) {
							UI.CenteredTexture(CreatureTextures.GetTexture(creature.type), rectX, rectY, scale);
						}
						Immediate.Color(Color.White);
						UI.font.Write((int) (chance * 100f) + "%", rectX + 0.5f + scale * 0.25f, rectY + selectorScale - 0.4f - scale * 0.5f, 0.3f * scale, Font.Align.MiddleCenter);
					}
				}
			}
		}
		if (!drawnDen && (!denEmpty || denEmpty && WorldWindow.cameraScale < 400f || roomHovered)) {
			Immediate.Color(Themes.RoomShortcutDen);
			UI.FillCircle(x + 0.5f, y - 0.5f, selectorScale * (hovered ? 1.5f : 1f) * 0.25f, 8);
		}
	}

	public Color GetTintColor() {
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