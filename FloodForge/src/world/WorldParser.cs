using System.Globalization;
using Stride.Core;
using Stride.Core.Extensions;

namespace FloodForge.World;

public static class WorldParser {
	private static readonly List<(string, Dictionary<string, RoomAttractiveness>)> roomAttractiveness = [];

	private static string FindAcronym(string regionsPath, string lowerAcronym) {
		foreach (string l in File.ReadAllLines(regionsPath)) {
			string line = (l.StartsWith("[ADD]") ? l[5..] : l).Trim();

			if (line.ToLowerInvariant() == lowerAcronym) {
				return line;
			}
		}

		return lowerAcronym;
	}

	public static RoomAttractiveness ParseRoomAttractiveness(string value) {
		return value switch {
			"neutral" => RoomAttractiveness.Neutral,
			"forbidden" => RoomAttractiveness.Forbidden,
			"avoid" => RoomAttractiveness.Avoid,
			"like" => RoomAttractiveness.Like,
			"stay" => RoomAttractiveness.Stay,
			_ => RoomAttractiveness.Default
		};
	}

	public static bool ParseProperties(string path) {
		foreach (string line in File.ReadAllLines(path)) {
			if (line.IsNullOrEmpty()) continue;

			if (line.StartsWith("Subregion: ")) {
				string subregionName = line[(line.IndexOf(':') + 2)..];
				Logger.Info("Subregion: " + subregionName);
				WorldWindow.region.subregions.Add(subregionName);
			}
			else if (line.StartsWith("Room_Attr: ")) {
				string attr = line[(line.IndexOf(':') + 2)..];
				string room = attr[0..attr.IndexOf(':')];
				string[] states = attr[(attr.IndexOf(':') + 2)..].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
				Dictionary<string, RoomAttractiveness> attractiveness = [];
				foreach (string state in states) {
					int idx = state.IndexOf('-');
					string creature = state[..idx];
					string value = state[(idx+1)..];
					creature = CreatureTextures.Parse(creature);
					attractiveness[creature] = ParseRoomAttractiveness(value.ToLowerInvariant());
				}
				if (room.ToLowerInvariant() == "default") {
					WorldWindow.region.defaultAttractiveness = attractiveness;
				} else {
					roomAttractiveness.Add((room, attractiveness));
				}
			}
			else if (line.StartsWith("//FloodForge|")) {
				string[] splits = line.Split('|');
				try {
					if (splits[1] == "SubregionColorOverride") {
						WorldWindow.region.overrideSubregionColors[int.Parse(splits[2])] = Color.Parse(splits[3]);
					}
				} catch (Exception ex) {
					Logger.Warn("Error while loading property comment: " + ex);
				}
			}
			else {
				WorldWindow.region.extraProperties += line + "\n";
			}
		}

		return true;
	}

	public static bool ParseMapRoom(string line) {
		string? roomName = line[..line.IndexOf(':')];
		string roomPath = WorldWindow.region.roomsPath;

		if (roomName.ToLowerInvariant().StartsWith("gate")) {
			Logger.Info("Found gate " + roomName);
			roomPath = PathUtil.FindDirectory(PathUtil.Combine(roomPath, ".."), "gates") ?? "";
			if (roomPath.IsNullOrEmpty()) {
				Logger.Warn("Failed to load gate! Missing gates folder");
				return true;
			}
		}

		string? filePath = PathUtil.FindFile(roomPath, roomName + ".txt");

		Room room;
		if (roomName.ToLowerInvariant().StartsWith("offscreenden")) {
			if (WorldWindow.region.offscreenDen == null) {
				WorldWindow.region.offscreenDen = new OffscreenRoom(roomName, roomName);
				WorldWindow.region.rooms.Add(WorldWindow.region.offscreenDen);
			}

			room = WorldWindow.region.offscreenDen;
		} else {
			if (filePath == null) {
				Logger.Info("File '", Path.Combine(roomPath, roomName), ".txt' could not be found");
			}

			room = new Room(filePath ?? "", roomName.ToLowerInvariant());
			WorldWindow.region.rooms.Add(room);
		}

		string[] data = [.. line[(line.IndexOf(':') + 1)..].Split('>').Select(x => x.Replace("<", ""))];
		float canonX = float.Parse(data[0]) / 3f;
		float canonY = float.Parse(data[1]) / 3f;
		float devX = float.Parse(data[2]) / 3f;
		float devY = float.Parse(data[3]) / 3f;
		int layer = data[4].IsNullOrEmpty() ? 0 : int.Parse(data[4]);
		string subregion = data[5];

		room.CanonPosition.x = canonX - room.width * 0.5f;
		room.CanonPosition.y = canonY + room.height * 0.5f;
		room.DevPosition.x = devX - room.width * 0.5f;
		room.DevPosition.y = devY + room.height * 0.5f;
		room.data.layer = layer;
		if (subregion.IsNullOrEmpty()) {
			room.data.subregion = -1;
		} else {
			int idx = WorldWindow.region.subregions.IndexOf(subregion);
			if (idx != -1) {
				room.data.subregion = idx;
			}
			else {
				room.data.subregion = WorldWindow.region.subregions.Count;
				WorldWindow.region.subregions.Add(subregion);
			}
		}

		return true;
	}

	public static bool ParseMap(string path) {
		Dictionary<string, (bool hidden, bool merge)> extraRoomData = [];

		foreach (string line in File.ReadAllLines(path)) {
			if (line.IsNullOrEmpty()) continue;

			if (line.StartsWith("//FloodForge;")) {
				string[] data = line[(line.IndexOf(';') + 1)..].Split('|');
				if (data[0] == "ROOM") {
					(bool hidden, bool merge) extra = (false, true);

					for (int i = 2; i < data.Length; i++) {
						string key = data[i];
						if (key == "hidden") {
							extra.hidden = true;
						}
						else if (key == "nomerge") {
							extra.merge = false;
						}
					}
				
					extraRoomData[data[1]] = extra;
				}
			}
			else if (line.StartsWith("//")) {
				WorldWindow.region.extraMap += line + "\n";
			}
			else if (line.StartsWith("Connection: ")) {
				// LATER
			}
			else if (line.StartsWith("SpawnMigrationStream: ") || line.StartsWith("SpawnMigrationStreamMidpoint: ")) {
				WorldWindow.region.extraMap += line + "\n";
				// LATER
			}
			else {
				if (!ParseMapRoom(line)) return false;
			}
		}

		foreach (var pair in extraRoomData) {
			Room room = WorldWindow.region.rooms.First(x => x.Name.Equals(pair.Key, StringComparison.InvariantCultureIgnoreCase));
			room.data.hidden = pair.Value.hidden;
			room.data.merge = pair.Value.merge;
		}

		return true;
	}

	private enum WorldParseState {
		None,
		ConditionalLinks,
		Rooms,
		Creatures
	}

	private struct ConnectionToAdd {
		public Room roomA;
		public uint connectionA;
		public Room? roomB = null;
		public string roomBName = "";
		public uint? connectionB = null;

		public ConnectionToAdd(Room roomA, uint connectionA, string roomBName) {
			this.roomA = roomA;
			this.connectionA = connectionA;
			this.roomBName = roomBName;
		}
	}

	private struct ConditionalConnection {
		public Room roomA;
		public uint connectionA;
		public Room? roomB = null;
		public string roomBName = "";
		public uint? connectionB = null;
		public HashSet<string> timelines = [];
		public TimelineType timelineType;

		public ConditionalConnection(Room roomA, uint connectionA, string roomBName) {
			this.roomA = roomA;
			this.connectionA = connectionA;
			this.roomBName = roomBName;
		}
	}

	private static void ParseWorldRoom(string line, ref List<ConnectionToAdd> connectionsToAdd) {
		string[] data = line.Split(':', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
		string roomName = data[0];
		string[] connections = data[1].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
		string[] tags = data[2..];

		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.Name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
		if (room == null) {
			if (roomName.ToLowerInvariant().StartsWith("offscreenden")) {
				room = new OffscreenRoom(roomName, roomName);
			} else {
				string path = WorldWindow.region.roomsPath;
				if (roomName.ToLowerInvariant().StartsWith("gate")) {
					path = PathUtil.FindDirectory(PathUtil.Parent(path), "gates") ?? "";
					if (path.IsNullOrEmpty()) {
						Logger.Warn($"Couldn't find gates folder in {WorldWindow.region.roomsPath}");
					}
				}

				string filePath = PathUtil.FindFile(path, roomName + ".txt") ?? "";
				if (filePath.IsNullOrEmpty()) {
					Logger.Warn($"Room file {path}/{roomName}.txt could not be found");
				}

				room = new Room(filePath, roomName);
			}

			WorldWindow.region.rooms.Add(room);
		}

		uint connectionId = 0;
		foreach (string connection in connections) {
			if (connection.ToLowerInvariant() == "disconnected") {
				connectionId++;
				continue;
			}

			bool alreadyExists = false;
			for (int i = 0; i < connectionsToAdd.Count; i++) {
				ConnectionToAdd connectionData = connectionsToAdd[i];
				if (connectionData.roomB != null) continue;

				if (connectionData.roomA.Name.Equals(connection, StringComparison.InvariantCultureIgnoreCase) && connectionData.roomBName.Equals(roomName, StringComparison.InvariantCultureIgnoreCase)) {
					connectionsToAdd[i] = connectionData with { roomB = room, connectionB = connectionId };
					alreadyExists = true;
					break;
				}
			}

			if (alreadyExists) {
				connectionId++;
				continue;
			}

			connectionsToAdd.Add(new ConnectionToAdd(room, connectionId, connection));
			connectionId++;
		}

		room.data.tags = [ ];
		tags.ForEach(tag => room.data.tags.Toggle(tag));
	}

	private static (string, float) ParseCreatureTag(string tag) {
		if (tag.StartsWith("Mean")) {
			return ("MEAN", float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("Seed")) {
			return ("SEED", float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("RotType")) {
			return ("RotType", float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.Contains(':')) {
			return ("LENGTH", float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		} else {
			return (tag, 0f);
		}
	}

	private static bool ParseWorldCreatureLineage(string[] splits, Room room, TimelineType timelineType, HashSet<string> timelines) {
		int denId = int.Parse(splits[2]);

		if (room is OffscreenRoom offscreenRoom) {
			denId = 0;
			offscreenRoom.GetDen();
		}

		if (!room.HasDen(denId)) {
			Logger.Warn($"{room.Name} missing den {denId}");
			return false;
		}

		Den den = room.GetDen(denId);
		DenLineage lineage = new DenLineage("", 0, "", 0.0f) {
			timelineType = timelineType,
			timelines = timelines
		};
		den.creatures.Add(lineage);

		DenCreature creature = lineage;
		bool first = true;
		foreach (string creatureInDen in splits[3].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)) {
			if (!first) {
				creature.lineageTo = new DenCreature("", 0, "", 0.0f);
				creature = creature.lineageTo;
			}
			first = false;

			string[] sections = creatureInDen.Split('-', StringSplitOptions.TrimEntries);
			creature.type = CreatureTextures.Parse(sections[0]);
			creature.count = 1;
			creature.lineageChance = float.Parse(sections[^1]);

			if (sections.Length == 3 && sections[1][0] == '{') {
				string tag = sections[1][1..^1];
				(creature.tag, creature.data) = ParseCreatureTag(tag);
			}
		}

		return true;
	}

	private static bool ParseWorldCreatureNormal(string[] splits, Room room, TimelineType timelineType, HashSet<string> timelines) {
		foreach (string creatureInDen in splits[1].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)) {
			string[] sections = creatureInDen.Split('-', StringSplitOptions.TrimEntries);
			int denId = int.Parse(sections[0], NumberStyles.Any, CultureInfo.InvariantCulture);
			string creature = sections[1];

			if (room is OffscreenRoom offscreenDen) {
				denId = 0;
				offscreenDen.GetDen();
			}

			if (denId == room.GarbageWormDenIndex) {
				GarbageWormDen worm = new GarbageWormDen() {
					type = CreatureTextures.Parse(creature),
					timelineType = timelineType,
					timelines = timelines,
					count = sections.Length < 3 ? 1 : int.Parse(sections[2]),
				};
				room.garbageWormDens.Add(worm);
				continue;
			}

			if (!room.HasDen(denId)) {
				Logger.Warn($"{room.Name} missing den {denId}");
				return false;
			}

			Den den = room.GetDen(denId);
			DenLineage lineage = new DenLineage(CreatureTextures.Parse(creature), 0, "", 0.0f) {
				timelineType = timelineType,
				timelines = timelines
			};
			den.creatures.Add(lineage);

			if (sections.Length == 3) {
				if (sections[2][0] == '{') {
					(lineage.tag, lineage.data) = ParseCreatureTag(sections[2][1..^1]);
					lineage.count = 1;
				} else {
					lineage.count = int.Parse(sections[2]);
				}
			}
			else if (sections.Length == 4) {
				bool tagFirst = sections[2][0] == '{';
				string tagString = sections[tagFirst ? 2 : 3];
				string countString = sections[tagFirst ? 3 : 2];
				(lineage.tag, lineage.data) = ParseCreatureTag(tagString[1..^1]);
				lineage.count = int.Parse(countString);
			}
			else {
				lineage.count = 1;
			}
		}

		return true;
	}

	private static bool ParseWorldCreature(string line) {
		string[] splits = line.Split(':', StringSplitOptions.TrimEntries);
		TimelineType timelineType = TimelineType.All;
		HashSet<string> timelines = [];

		if (splits[0][0] == '(') {
			string v = splits[0][1..splits[0].IndexOf(')')];
			splits[0] = splits[0][(splits[0].IndexOf(')') + 1)..].Trim();
			if (v.ToLowerInvariant().StartsWith("x-")) {
				timelineType = TimelineType.Except;
				v = v[2..];
			}
			else {
				timelineType = TimelineType.Only;
			}
			timelines = [.. v.Split(',')];
		}

		bool lineage = splits[0].ToLowerInvariant() == "lineage";
		string roomName = lineage ? splits[1] : splits[0];
		Room? room = (roomName.ToLowerInvariant() == "offscreen")
			? WorldWindow.region.offscreenDen
			: WorldWindow.region.rooms.FirstOrDefault(x => x.Name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));

		if (room == null) {
			Logger.Warn($"No room {roomName}({lineage}) for creature");
			return false;
		}

		if (lineage) {
			if (!ParseWorldCreatureLineage(splits, room, timelineType, timelines)) return false;
		} else {
			if (!ParseWorldCreatureNormal(splits, room, timelineType, timelines)) return false;
		}

		return true;
	}

	private static bool ParseWorldConditionalLink(string link, ref List<ConditionalConnection> conditionalConnectionsToAdd) {
		string[] parts = link.Split(':', StringSplitOptions.TrimEntries);
		if (parts.Length < 3 || parts.Length > 4) {
			Logger.Warn("Skipping line due to improper length");
			Logger.Warn($"> {link}");
			return false;
		}

		string[] timelines = parts[0].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

		// LATER: REPLACEROOM

		if (parts.Length == 3) {
			string roomName2 = parts[2];
			Room? room2 = WorldWindow.region.rooms.FirstOrDefault(x => x.Name.Equals(roomName2, StringComparison.InvariantCultureIgnoreCase));
			if (room2 == null) {
				Logger.Warn($"Skipping line due to missing room {roomName2}");
				Logger.Warn($"> {link}");
				return false;
			}

			string mod = parts[1].ToLowerInvariant();

			if (mod == "exclusiveroom") {
				if (room2.TimelineType == TimelineType.Except) {
					Logger.Warn($"Skipping line due to invalid EXCLUSIVEROOM {roomName2}");
					Logger.Warn($"> {link}");
					return false;
				}

				room2.TimelineType = TimelineType.Only;
				timelines.ForEach(x => room2.Timelines.Add(x));
			}
			else if (mod == "hideroom") {
				if (room2.TimelineType == TimelineType.Only) {
					Logger.Warn($"Skipping line due to invalid HIDEROOM {roomName2}");
					Logger.Warn($"> {link}");
					return false;
				}

				room2.TimelineType = TimelineType.Except;
				timelines.ForEach(x => room2.Timelines.Add(x));
			}

			return true;
		}

		string roomName = parts[1];
		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.Name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
		if (room == null) {
			Logger.Warn($"Skipping line due to missing room {roomName}");
			Logger.Warn($"> {link}");
			return false;
		}

		string currentConnection = parts[2];
		int disconnectedId = -1;
		bool isCurrentDisconnected = int.TryParse(currentConnection, NumberStyles.Any, CultureInfo.InvariantCulture, out disconnectedId);
		string toConnection = parts[3];

		if (currentConnection.Equals(toConnection, StringComparison.InvariantCultureIgnoreCase)) {
			Logger.Warn("Skipping line due to no change");
			Logger.Warn($"> {link}");
			return false;
		}

		Connection? connection = room.connections.FirstOrDefault(otherConnection => {
			Room otherRoom = (otherConnection.roomA == room) ? otherConnection.roomB : otherConnection.roomA;

			return otherRoom.Name.Equals(currentConnection, StringComparison.InvariantCultureIgnoreCase);
		});

		if (toConnection.ToLowerInvariant() == "disconnected") {
			if (connection == null) {
				Logger.Warn("Skipping line due to missing connection");
				Logger.Warn($"> {link}");
				return false;
			}

			if (connection.timelineType == TimelineType.Only) {
				timelines.ForEach(x => connection.timelines.Remove(x));
			}
			else {
				connection.timelineType = TimelineType.Except;
				timelines.ForEach(x => connection.timelines.Add(x));
			}
			return true;
		}

		int connectionId = -1;
		if (isCurrentDisconnected) {
			string timeline = timelines[0]; // LATER: Figure out what this does and clean up
			bool[] connected = new bool[room.roomShortcutEntrances.Count];
			foreach (Connection connection2 in room.connections) {
				if (!connection2.AllowsTimeline(timeline)) continue;

				connected[connection2.roomA == room ? connection2.connectionA : connection2.connectionB] = true;
			}

			for (int i = 0; i < connected.Length; i++) {
				if (connected[i]) continue;

				disconnectedId--;
				if (disconnectedId == 0) {
					connectionId = i;
					break;
				}
			}
		}
		else {
			if (connection == null) {
				Logger.Warn("Link missing connection, adding new connection anyways");
				Logger.Warn($"> {link}");
			} else {
				connectionId = (int) ((connection.roomA == room) ? connection.connectionA : connection.connectionB);

				if (connection.timelineType == TimelineType.Only) {
					timelines.ForEach(x => connection.timelines.Remove(x));
				}
				else {
					connection.timelineType = TimelineType.Except;
					timelines.ForEach(x => connection.timelines.Add(x));
				}
			}
		}

		if (connection != null) {
			if (connection.timelineType == TimelineType.Except) {
				timelines.ForEach(x => connection.timelines.Remove(x));
			}
			else {
				connection.timelineType = TimelineType.Only;
				timelines.ForEach(x => connection.timelines.Add(x));
			}

			return true;
		}

		if (connectionId == -1) {
			Logger.Warn("Connection id cannot be inferred");
			Logger.Warn($"> {link}");
			return false;
		}

		for (int i = 0; i < conditionalConnectionsToAdd.Count; i++) {
			ConditionalConnection connectionData = conditionalConnectionsToAdd[i];

			if (connectionData.roomB == null && connectionData.roomA.Name.Equals(toConnection, StringComparison.InvariantCultureIgnoreCase) && connectionData.roomBName.Equals(room.Name, StringComparison.InvariantCultureIgnoreCase)) {
				conditionalConnectionsToAdd[i] = connectionData with {
					roomB = room,
					connectionB = (uint) connectionId
				};
				return true;
			}
		}

		conditionalConnectionsToAdd.Add(new ConditionalConnection() {
			roomA = room,
			connectionA = (uint) connectionId,
			roomBName = toConnection,
			roomB = null,
			connectionB = null,
			timelines = [.. timelines],
			timelineType = TimelineType.Only
		});

		return true;
	}

	public static bool ParseWorld(string path) {
		List<ConnectionToAdd> connectionsToAdd = [];
		List<string> conditionalLinks = [];
		WorldParseState parseState = WorldParseState.None;

		foreach (string line in File.ReadAllLines(path)) {
			if (line.IsNullOrEmpty() || line.StartsWith("//")) continue;

			if (line == "ROOMS") {
				if (parseState != WorldParseState.None) {
					Logger.Warn("Invalid world file. Failed to close " + parseState);
					return false;
				}

				parseState = WorldParseState.Rooms;
				Logger.Info("World - Rooms");
				continue;
			}

			if (line == "END ROOMS") {
				if (parseState != WorldParseState.Rooms) {
					Logger.Warn("Invalid world file. END ROOMS without matching ROOMS");
					return false;
				}

				parseState = WorldParseState.None;
				if (WorldWindow.region.offscreenDen == null) {
					WorldWindow.region.offscreenDen = new OffscreenRoom("offscreenden" + WorldWindow.region.acronym, "OffscreenDen" + WorldWindow.region.acronym.ToUpperInvariant());
					WorldWindow.region.rooms.Add(WorldWindow.region.offscreenDen);
				}
				continue;
			}

			if (line == "CREATURES") {
				if (parseState != WorldParseState.None) {
					Logger.Warn("Invalid world file. Failed to close " + parseState);
					return false;
				}

				parseState = WorldParseState.Creatures;
				Logger.Info("World - Creatures");
				continue;
			}

			if (line == "END CREATURES") {
				if (parseState != WorldParseState.Creatures) {
					Logger.Warn("Invalid world file. END CREATURES without matching CREATURES");
					return false;
				}

				parseState = WorldParseState.None;
				continue;
			}

			if (line == "CONDITIONAL LINKS") {
				if (parseState != WorldParseState.None) {
					Logger.Warn("Invalid world file. Failed to close " + parseState);
					return false;
				}

				parseState = WorldParseState.ConditionalLinks;
				Logger.Info("World - Conditional Links");
				continue;
			}

			if (line == "END CONDITIONAL LINKS") {
				if (parseState != WorldParseState.ConditionalLinks) {
					Logger.Warn("Invalid world file. END CONDITIONAL LINKS without matching CONDITIONAL LINKS");
					return false;
				}

				parseState = WorldParseState.None;
				continue;
			}

			if (parseState == WorldParseState.None) {
				WorldWindow.region.extraWorld += line + "\n";
			}
			else if (parseState == WorldParseState.Rooms) {
				ParseWorldRoom(line, ref connectionsToAdd);
			}
			else if (parseState == WorldParseState.Creatures) {
				if (!ParseWorldCreature(line)) {
					Logger.Warn("Invalid world creature " + line);
					return false;
				}
			}
			else if (parseState == WorldParseState.ConditionalLinks) {
				conditionalLinks.Add(line);
			}
		}

		Logger.Info("Loading connections");

		foreach (ConnectionToAdd connectionData in connectionsToAdd) {
			if (connectionData.roomB == null || connectionData.connectionB == null) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.Name} to {connectionData.roomB?.Name ?? connectionData.roomBName}");
				continue;
			}

			if (!connectionData.roomA.ValidConnection(connectionData.connectionA) || !connectionData.roomB.ValidConnection(connectionData.connectionB.Value)) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.Name} to {connectionData.roomB?.Name ?? connectionData.roomBName} - Not valid connections");
				continue;
			}

			Connection connection = new Connection(connectionData.roomA, connectionData.connectionA, connectionData.roomB, connectionData.connectionB.Value);
			WorldWindow.region.connections.Add(connection);
			connectionData.roomA.Connect(connection);
			connectionData.roomB.Connect(connection);
		}

		Logger.Info("Loaded connections");
		Logger.Info("Loading conditional links");

		List<ConditionalConnection> conditionalConnectionsToAdd = [];
		foreach (string link in conditionalLinks) {
			if (!ParseWorldConditionalLink(link, ref conditionalConnectionsToAdd)) return false;
		}

		foreach (ConditionalConnection connectionData in conditionalConnectionsToAdd) {
			if (connectionData.roomB == null) {
				Logger.Warn("Conditional connection failed to load - missing other room");
				Logger.Warn($"> {connectionData.roomA.Name} {connectionData.connectionA} - {connectionData.roomBName}");
				continue;
			}

			if (connectionData.connectionB == null) {
				Logger.Warn("Conditional connection failed to load - missing other connection");
				Logger.Warn($"> {connectionData.roomA.Name} {connectionData.connectionA} - {connectionData.roomBName}");
				continue;
			}

			if (!connectionData.roomA.ValidConnection(connectionData.connectionA) || !connectionData.roomB.ValidConnection(connectionData.connectionB.Value)) {
				Logger.Warn("Conditional connection failed to load - invalid connection indices");
				Logger.Warn($"> {connectionData.roomA.Name} {connectionData.connectionA} - {connectionData.roomB.Name} {connectionData.connectionB}");
				continue;
			}

			Connection connection = new Connection(connectionData.roomA, connectionData.connectionA, connectionData.roomB, connectionData.connectionB.Value) {
				timelines = connectionData.timelines,
				timelineType = connectionData.timelineType
			};
			WorldWindow.region.connections.Add(connection);
			connectionData.roomA.Connect(connection);
			connectionData.roomB.Connect(connection);
		}

		Logger.Info("Loaded conditional links");

		return true;
	}

	private static void LoadExtraRoomData(string? path, Room room) {
		if (path == null) return;

		List<RoomData.DevItem> devItems = [];

		foreach (string line in File.ReadAllLines(path)) {
			if (line.IsNullOrEmpty()) continue;
			if (!line.StartsWith("PlacedObjects:")) continue;

			string[] splits = line[(line.IndexOf(':') + 1)..].Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
			foreach (string item in splits) {
				string[] splits2 = item.Split('>');
				string key = splits2[0];

				Texture texture = CreatureTextures.GetTexture($"room-{key}");
				if (texture == CreatureTextures.UnknownCreature) continue;

				RoomData.DevItem devItem = new RoomData.DevItem(key, texture, new Vector2(
					float.Parse(splits2[1][1..]) / 20f,
					float.Parse(splits2[2][1..]) / 20f
				));
				devItems.Add(devItem);
			}
		}

		room.data.devItems = [ ..devItems ];
	}

	public static bool ImportWorldFile(string path) {
		History.Clear();
		RecentFiles.AddPath(path);
		roomAttractiveness.Clear();
		WorldWindow.Reset();
		WorldWindow.region.exportPath = PathUtil.Parent(path);
		WorldWindow.region.acronym = Path.GetFileNameWithoutExtension(path);
		WorldWindow.region.acronym = WorldWindow.region.acronym[(WorldWindow.region.acronym.IndexOfReverse('_') + 1)..];
		string? regionsPath = Path.GetDirectoryName(PathUtil.Parent(WorldWindow.region.exportPath))?.ToLowerInvariant() == "world"
			? PathUtil.FindFile(PathUtil.Parent(WorldWindow.region.exportPath), "regions.txt")
			: null;

		if (regionsPath == null) {
			Logger.Info("../world/regions.txt doesn't exist, checking for modify");
			regionsPath = PathUtil.FindDirectory(PathUtil.Parent(Path.Combine(WorldWindow.region.exportPath, "..")), "modify");
			if (regionsPath != null) {
				Logger.Info("../modify found");
				regionsPath = PathUtil.FindDirectory(regionsPath, "world");
			}
			if (regionsPath != null) {
				Logger.Info("../modify/world found");
				regionsPath = PathUtil.FindFile(regionsPath, "regions.txt");
			}
			else {
				Logger.Info("../modify/world/regions.txt doesn't exist");
			}
		}

		if (regionsPath != null) {
			Logger.Info("Found regions.txt, looking for acronym");
			WorldWindow.region.acronym = FindAcronym(regionsPath, WorldWindow.region.acronym);
		} else {
			Logger.Info("regions.txt not found");
		}

		Logger.Info("Opening world ", WorldWindow.region.acronym);

		string? roomsPath = PathUtil.FindDirectory(PathUtil.Parent(WorldWindow.region.exportPath), WorldWindow.region.acronym + "-rooms");
		if (roomsPath == null) {
			Logger.Error("Cannot find rooms directory");
			return false;
		}
		WorldWindow.region.roomsPath = roomsPath;

		string? propertiesPath = PathUtil.FindFile(WorldWindow.region.exportPath, "properties.txt");
		if (propertiesPath != null) {
			Logger.Info("Loading properties");
			if (!ParseProperties(propertiesPath)) return false;
		}

		string? mapPath = PathUtil.FindFile(WorldWindow.region.exportPath, "map_" + WorldWindow.region.acronym + ".txt");
		if (mapPath != null) {
			Logger.Info("Loading map");
			if (!ParseMap(mapPath)) return false;
		} else {
			Logger.Info("Map file not found");
		}

		Logger.Info("Loading world");

		if (!ParseWorld(path)) return false;

		Logger.Info("Loading extra room data");

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom) continue;

			foreach (var x in roomAttractiveness) {
				if (!x.Item1.Equals(room.Name, StringComparison.InvariantCultureIgnoreCase)) continue;

				room.data.attractiveness = x.Item2;
			}

			LoadExtraRoomData(PathUtil.FindFile(WorldWindow.region.roomsPath, room.Name + "_settings.txt"), room);
		}

		Logger.Info("World file imported");

		return true;
	}
}