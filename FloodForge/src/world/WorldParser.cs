using System.Globalization;
using System.Text.RegularExpressions;
using Stride.Core;
using Stride.Core.Extensions;

namespace FloodForge.World;

public static class WorldParser {
	private static readonly List<(string, Dictionary<string, RoomAttractiveness>)> roomAttractiveness = [];

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
				if (room.Equals("default", StringComparison.InvariantCultureIgnoreCase)) {
					WorldWindow.region.defaultAttractiveness = attractiveness;
				}
				else {
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

		if (roomName.StartsWith("gate", StringComparison.InvariantCultureIgnoreCase)) {
			roomPath = PathUtil.FindDirectory(PathUtil.Combine(roomPath, ".."), "gates") ?? "";
			if (roomPath.IsNullOrEmpty()) {
				Logger.Warn("Failed to load gate! Missing gates folder");
				return true;
			}
		}

		string? filePath = PathUtil.FindFile(roomPath, roomName + ".txt");

		Room room;
		if (roomName.StartsWith("offscreenden", StringComparison.InvariantCultureIgnoreCase)) {
			if (WorldWindow.region.offscreenDen == null) {
				WorldWindow.region.offscreenDen = new OffscreenRoom(roomName, roomName);
				WorldWindow.region.rooms.Add(WorldWindow.region.offscreenDen);
			}

			room = WorldWindow.region.offscreenDen;
		}
		else {
			if (filePath == null) {
				Logger.Info("File '", Path.Combine(roomPath, roomName), ".txt' could not be found");
			}

			room = new Room(filePath ?? "", roomName);
			WorldWindow.region.rooms.Add(room);
		}

		string[] data = [.. line[(line.IndexOf(':') + 1)..].Split('>').Select(x => x.Replace("<", "").Trim())];
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
		}
		else {
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
			else if (line.StartsWith("SpawnMigrationStream: ") || line.StartsWith("SpawnMigrationStreamMidpoint: ") || line.StartsWith("Def_Mat: ") || line.StartsWith("R: ") || line.StartsWith("[REFERENCE]") || line.StartsWith("I: ") || line.StartsWith("[IMAGE]")) {
				WorldWindow.region.extraMap += line + "\n";
				// LATER
			}
			else {
				if (!ParseMapRoom(line)) return false;
			}
		}

		foreach (var pair in extraRoomData) {
			Room room = WorldWindow.region.rooms.First(x => x.name.Equals(pair.Key, StringComparison.InvariantCultureIgnoreCase));
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
		public uint roomAExitID;
		public Room? roomB = null;
		public string roomBName = "";
		public uint? roomBExitID = null;

		public ConnectionToAdd(Room roomA, uint connectionA, string roomBName) {
			this.roomA = roomA;
			this.roomAExitID = connectionA;
			this.roomBName = roomBName;
		}
	}

	private struct ConditionalConnection {
		public Room roomA;
		public uint roomAExitID;
		public Room? roomB = null;
		public string roomBName = "";
		public uint? roomBExitID = null;
		public HashSet<string> timelines = [];
		public TimelineType timelineType;

		public ConditionalConnection(Room roomA, uint connectionA, string roomBName) {
			this.roomA = roomA;
			this.roomAExitID = connectionA;
			this.roomBName = roomBName;
		}
	}

	private static void ParseWorldRoom(string line, ref List<ConnectionToAdd> connectionsToAdd) {
		string[] data = line.Split(':', StringSplitOptions.TrimEntries);
		if (data.Length < 2) return;

		string roomName = data[0];
		string[] connections = data[1].Split(',', StringSplitOptions.TrimEntries);
		string[] tags = data[2..];

		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
		if (room == null) {
			if (roomName.StartsWith("offscreenden", StringComparison.InvariantCultureIgnoreCase)) {
				room = new OffscreenRoom(roomName, roomName);
			}
			else {
				string path = WorldWindow.region.roomsPath;
				if (roomName.StartsWith("gate", StringComparison.InvariantCultureIgnoreCase)) {
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
		foreach (string connection in connections) { // go through every room-connection
			if (connection.IsNullOrEmpty())
			{
				continue;
			}
			if (connection.ToLowerInvariant() == "disconnected") {
				connectionId++;
				continue;
			}

			bool alreadyExists = false;
			for (int i = 0; i < connectionsToAdd.Count; i++) { // look through the connections that have already been found
				ConnectionToAdd connectionData = connectionsToAdd[i];
				if (connectionData.roomB != null) continue; // if a connection has already found its other side, skip that connection

				// otherwise, check if the found connection: - comes from the room we're looking for, and: - is looking for this room
				// this is probably where the ConnectionExtensions compatibility has to be first implemented, so it also looks for the right connection index.
				if (connectionData.roomA.name.Equals(connection, StringComparison.InvariantCultureIgnoreCase) && connectionData.roomBName.Equals(roomName, StringComparison.InvariantCultureIgnoreCase)) {
					connectionsToAdd[i] = connectionData with { roomB = room, roomBExitID = connectionId };
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
		tags.ForEach(room.data.tags.Toggle);
	}

	private static DenCreature.Tag ParseCreatureTag(string tag, string type) {
		if (tag.StartsWith("Mean")) {
			return new DenCreature.FloatTag(CreatureTags.Mean, float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("Seed")) {
			return new DenCreature.IntegerTag(CreatureTags.Seed, int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("RotType")) {
			return new DenCreature.IntegerTag(CreatureTags.RotType, int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}

		if (!tag.Contains(':')) {
			try {
				if (type == "polemimic") {
					return new DenCreature.IntegerTag(CreatureTags.POLEMIMIC_LENGTH, int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
				}
				else {
					return new DenCreature.FloatTag(CreatureTags.CENTIPEDE_LENGTH, float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
				}
			}
			catch (FormatException) {}
		}

		return new DenCreature.Tag(CreatureTags.GetOrCreate(tag));
	}

	private static bool ParseWorldCreatureLineage(string[] splits, Room room, TimelineType timelineType, HashSet<string> timelines) {
		int denId = int.Parse(splits[2]);

		if (room is OffscreenRoom offscreenRoom) {
			denId = 0;
			offscreenRoom.GetDen();
		}

		if (!room.HasDen(denId)) {
			Logger.Warn($"{room.name} missing den {denId}");
			return false;
		}

		Den den = room.GetDen(denId);
		DenLineage lineage = new DenLineage("", 0) {
			timelineType = timelineType,
			timelines = timelines
		};
		den.creatures.Add(lineage);

		DenCreature creature = lineage;
		bool first = true;
		foreach (string creatureInDen in Regex.Split(splits[3], @",(?![^{]*})").Select(s => s.Trim())) {
			if (!first) {
				creature.lineageTo = new DenCreature("", 0);
				creature = creature.lineageTo;
			}
			first = false;

			string[] sections = Regex.Split(creatureInDen, @"-(?![^{]*})");
			creature.type = CreatureTextures.Parse(sections[0]);
			creature.count = 1;

			for (int i = 1; i < sections.Length; i++) {
				string section = sections[i];

				if (section[0] != '{') {
					creature.lineageChance = float.Parse(section);
					continue;
				}

				section = section[1..^1];
				string[] tags = section.Split(',', '|');
				foreach (string tagStr in tags) {
					creature.AddTag(ParseCreatureTag(tagStr, creature.type));
				}
			}
		}

		return true;
	}

	private static bool ParseWorldCreatureNormal(string[] splits, Room room, TimelineType timelineType, HashSet<string> timelines) {
		foreach (string creatureInDen in Regex.Split(splits[1], @",(?![^{]*})").Select(s => s.Trim())) {
			string[] sections = Regex.Split(creatureInDen, @"-(?![^{]*})");
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
				Logger.Warn($"{room.name} missing den {denId}");
				return false;
			}

			Den den = room.GetDen(denId);
			DenLineage lineage = new DenLineage(CreatureTextures.Parse(creature), 1) {
				timelineType = timelineType,
				timelines = timelines
			};
			den.creatures.Add(lineage);

			for (int i = 2; i < sections.Length; i++) {
				string section = sections[i];

				if (section[0] != '{') {
					lineage.count = int.Parse(section);
					continue;
				}

				section = section[1..^1];
				string[] tags = section.Split(',', '|');
				foreach (string tagStr in tags) {
					lineage.AddTag(ParseCreatureTag(tagStr, lineage.type));
				}
			}
		}

		return true;
	}

	private static bool ParseWorldCreature(string line) {
		try {
			string[] splits = line.Split(" : ", StringSplitOptions.TrimEntries);
			TimelineType timelineType = TimelineType.All;
			HashSet<string> timelines = [];

			if (splits[0][0] == '(') {
				string v = splits[0][1..splits[0].IndexOf(')')];
				splits[0] = splits[0][(splits[0].IndexOf(')') + 1)..].Trim();
				if (v.StartsWith("x-", StringComparison.InvariantCultureIgnoreCase)) {
					timelineType = TimelineType.Except;
					v = v[2..];
				}
				else {
					timelineType = TimelineType.Only;
				}
				timelines = [.. v.Split(',')];
			}

			bool lineage = splits[0].Equals("lineage", StringComparison.InvariantCultureIgnoreCase);
			string roomName = lineage ? splits[1] : splits[0];
			Room? room = roomName.Equals("offscreen", StringComparison.InvariantCultureIgnoreCase)
				? WorldWindow.region.offscreenDen
				: WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));

			if (room == null) {
				Logger.Warn($"No room {roomName}({lineage}) for creature");
				return false;
			}

			if (lineage) {
				if (!ParseWorldCreatureLineage(splits, room, timelineType, timelines)) return false;
			}
			else {
				if (!ParseWorldCreatureNormal(splits, room, timelineType, timelines)) return false;
			}
		}
		catch (Exception e) {
			Logger.Warn(e);
			return false;
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
			Room? room2 = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName2, StringComparison.InvariantCultureIgnoreCase));
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
		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
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

			return otherRoom.name.Equals(currentConnection, StringComparison.InvariantCultureIgnoreCase);
		});

		if (toConnection.Equals("disconnected", StringComparison.InvariantCultureIgnoreCase)) {
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
			bool[] connected = new bool[room.roomExits.Count];
			foreach (Connection connection2 in room.connections) {
				if (!connection2.AllowsTimeline(timeline)) continue;

				connected[connection2.roomA == room ? connection2.roomAExitID : connection2.roomBExitID] = true;
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
			}
			else {
				connectionId = (int) ((connection.roomA == room) ? connection.roomAExitID : connection.roomBExitID);

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

			if (connectionData.roomB == null && connectionData.roomA.name.Equals(toConnection, StringComparison.InvariantCultureIgnoreCase) && connectionData.roomBName.Equals(room.name, StringComparison.InvariantCultureIgnoreCase)) {
				conditionalConnectionsToAdd[i] = connectionData with {
					roomB = room,
					roomBExitID = (uint) connectionId
				};
				return true;
			}
		}

		conditionalConnectionsToAdd.Add(new ConditionalConnection() {
			roomA = room,
			roomAExitID = (uint) connectionId,
			roomBName = toConnection,
			roomB = null,
			roomBExitID = null,
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
					WorldWindow.invalidCreaturesEncountered = true;
					continue;
				}
			}
			else if (parseState == WorldParseState.ConditionalLinks) {
				conditionalLinks.Add(line);
			}
		}

		Logger.Info("Loading connections");

		foreach (ConnectionToAdd connectionData in connectionsToAdd) {
			// Logger.Info($"connectionData - roomA: {connectionData.roomA.name} roomB: {connectionData.roomB?.name} roomAExitID: {connectionData.roomAExitID} roomBExitID: {connectionData.roomBExitID}");
			if (connectionData.roomB == null || connectionData.roomBExitID == null) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.name} to {connectionData.roomB?.name ?? connectionData.roomBName}");
				continue;
			}
			
			if (!connectionData.roomA.ValidConnection(connectionData.roomAExitID) || !connectionData.roomB.ValidConnection(connectionData.roomBExitID.Value)) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.name} to {connectionData.roomB?.name ?? connectionData.roomBName} - Not valid connections");
				continue;
			}

			Connection connection = new Connection(connectionData.roomA, connectionData.roomAExitID, connectionData.roomB, connectionData.roomBExitID.Value);
			WorldWindow.region.connections.Add(connection);
			connectionData.roomA.Connect(connection);
			connectionData.roomB.Connect(connection);
		}

		Logger.Info("Loaded connections");
		Logger.Info("Loading conditional links");

		List<ConditionalConnection> conditionalConnectionsToAdd = [];
			Logger.Info("Checking links");
		foreach (string link in conditionalLinks) {
			Logger.Info("Link: " + link);
			if (!ParseWorldConditionalLink(link, ref conditionalConnectionsToAdd)) return false;
		}

		foreach (ConditionalConnection connectionData in conditionalConnectionsToAdd) {
			if (connectionData.roomB == null) {
				Logger.Warn("Conditional connection failed to load - missing other room");
				Logger.Warn($"> {connectionData.roomA.name} {connectionData.roomAExitID} - {connectionData.roomBName}");
				continue;
			}

			if (connectionData.roomBExitID == null) {
				Logger.Warn("Conditional connection failed to load - missing other connection");
				Logger.Warn($"> {connectionData.roomA.name} {connectionData.roomAExitID} - {connectionData.roomBName}");
				continue;
			}

			if (!connectionData.roomA.ValidConnection(connectionData.roomAExitID) || !connectionData.roomB.ValidConnection(connectionData.roomBExitID.Value)) {
				Logger.Warn("Conditional connection failed to load - invalid connection indices");
				Logger.Warn($"> {connectionData.roomA.name} {connectionData.roomAExitID} - {connectionData.roomB.name} {connectionData.roomBExitID}");
				continue;
			}

			Connection connection = new Connection(connectionData.roomA, connectionData.roomAExitID, connectionData.roomB, connectionData.roomBExitID.Value) {
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

	public static string GetRegionDisplayname(string worldPath) {
		string? displaynamePath = PathUtil.FindFile(PathUtil.Parent(worldPath), "displayname.txt");

		return displaynamePath == null ? "" : File.ReadAllText(displaynamePath).Trim();
	}

	public static bool ImportWorldFile(string worldPath) {
		History.Clear();
		RecentFiles.AddPath(worldPath);
		roomAttractiveness.Clear();
		WorldWindow.Reset();
		WorldWindow.region.exportPath = PathUtil.Parent(worldPath);
		WorldWindow.region.acronym = Path.GetFileNameWithoutExtension(worldPath);
		WorldWindow.region.acronym = WorldWindow.region.acronym[(WorldWindow.region.acronym.IndexOfReverse('_') + 1)..];

		{
			// `world/xx/world_xx.txt` -> `world/regions.txt`
			string? regionsPath = Path.GetFileNameWithoutExtension(PathUtil.Parent(WorldWindow.region.exportPath))?.ToLowerInvariant() == "world"
				? PathUtil.FindFile(PathUtil.Parent(WorldWindow.region.exportPath), "regions.txt")
				: null;
			if (regionsPath != null)
				WorldWindow.region.regionsPaths.Add(regionsPath);

			string main = PathUtil.Parent(Path.Combine(WorldWindow.region.exportPath, ".."));

			// `world/xx/world_xx.txt` -> `modify/world/regions.txt`
			regionsPath = PathUtil.FindDirectory(main, "modify");
			if (regionsPath != null) {
				regionsPath = PathUtil.FindDirectory(regionsPath, "world");
			}
			if (regionsPath != null) {
				regionsPath = PathUtil.FindFile(regionsPath, "regions.txt");
				if (regionsPath != null)
					WorldWindow.region.regionsPaths.Add(regionsPath);
			}

			// `mods/MOD/world/xx/world_xx.txt` -> `world/regions.txt`
			if (Path.GetFileNameWithoutExtension(PathUtil.Parent(main))?.ToLowerInvariant() == "mods") {
				regionsPath = PathUtil.FindDirectory(PathUtil.Parent(Path.Combine(main, "..")), "world");
				if (regionsPath != null) {
					regionsPath = PathUtil.FindFile(regionsPath, "regions.txt");
					if (regionsPath != null)
						WorldWindow.region.regionsPaths.Add(regionsPath);
				}
			}

			Logger.Info(string.Join(", ", WorldWindow.region.regionsPaths));
		}

		WorldWindow.region.acronym = WorldWindow.region.FindAcronym(WorldWindow.region.acronym);

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
		}
		else {
			Logger.Info("Map file not found");
		}

		Logger.Info("Loading world");

		if (!ParseWorld(worldPath)) return false;

		Logger.Info("Loading extra room data");

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom) continue;

			foreach (var attr in roomAttractiveness) {
				if (!attr.Item1.Equals(room.name, StringComparison.InvariantCultureIgnoreCase)) continue;

				room.data.attractiveness = attr.Item2;
			}
		}

		Logger.Info("Searching for display name");
		WorldWindow.region.displayName = GetRegionDisplayname(worldPath);

		Logger.Info("World file imported");

		return true;
	}
}