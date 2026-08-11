using System.Globalization;
using System.Text.RegularExpressions;
using FloodForge.Popups;
using Stride.Core;
using Stride.Core.Extensions;

namespace FloodForge.World;

// REVIEW: Make sure to load correctly capitalized room name

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
					creature = Mods.ParseCreature(creature);
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

	public static bool ParseMapRoom(string line, out MapDraggable? offsetDraggable, string? timeline) {
		offsetDraggable = null;
		string? roomName = line[..line.IndexOf(':')];
		string roomPath = WorldWindow.region.roomsPath;

		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, (StringComparison)3));
		
		if (roomName.StartsWith("offscreenden", (StringComparison) 3)) {
			room = WorldWindow.region.rooms.FirstOrDefault(x => x is OffscreenRoom);
		}

		if (room == null) {
			Logger.Warn($"MapRoom {roomName} not found!");
			return true;
		}

		Room dataTarget = room;
		MapDraggable positionTarget = room;
		Room sizeSource = room;
		if (timeline != null) {
			ReplaceRoom? relevantReplaceRoom = room.replaceRooms.LastOrDefault(x => x.timeline.OverlapsWith(timeline));
			if (relevantReplaceRoom != null) {
				dataTarget = relevantReplaceRoom.replacedRoom;
				positionTarget = relevantReplaceRoom;
				sizeSource = relevantReplaceRoom.replacingRoom;
			}
		}
		
		string[] data = [.. line[(line.IndexOf(':') + 1)..].Split('>').Select(x => x.Replace("<", "").Trim())];
		float canonX = float.Parse(data[0]) / 3f;
		float canonY = float.Parse(data[1]) / 3f;
		float devX = float.Parse(data[2]) / 2f;
		float devY = float.Parse(data[3]) / 2f;
		int layer = data[4].IsNullOrEmpty() ? 0 : int.Parse(data[4]);
		string subregion = data[5];

		if (positionTarget.CanonPosition != Vector2.Zero) {
			totalAvgOffset = (totalAvgOffset * avgCount + (new Vector2(canonX - sizeSource.width * 0.5f, canonY + sizeSource.height * 0.5f) - positionTarget.CanonPosition)) / (avgCount + 1);
			avgCount++;
			return true;
		}
		offsetDraggable = positionTarget;

		positionTarget.CanonPosition.x = canonX - sizeSource.width * 0.5f;
		positionTarget.CanonPosition.y = canonY + sizeSource.height * 0.5f;
		positionTarget.DevPosition.x = devX - sizeSource.width * 0.5f;
		positionTarget.DevPosition.y = devY + sizeSource.height * 0.5f;

		if (gridOffsets.FirstOrDefault(x => x.Item1 == room).Item1 == null)
			gridOffsets.Add((room, new (Mathf.Mod1(positionTarget.CanonPosition.x), Mathf.Mod1(positionTarget.CanonPosition.y))));

		dataTarget.data.layer = layer;
		if (subregion.IsNullOrEmpty()) {
			dataTarget.data.subregion = -1;
		}
		else {
			int idx = WorldWindow.region.subregions.IndexOf(subregion);
			if (idx != -1) {
				dataTarget.data.subregion = idx;
			}
			else {
				dataTarget.data.subregion = WorldWindow.region.subregions.Count;
				WorldWindow.region.subregions.Add(subregion);
			}
		}

		return true;
	}

	// REVIEW - improve offset calculation robustness (and/or find out why it happens in the first place)
	// (it probably happens because of differing map sizes or differing map bounds, therefore different map origins... hmm)
	private static Vector2 totalAvgOffset;
	private static int avgCount;
	
	private static List<(Room, Vector2)> gridOffsets = [];
	public static bool ParseMap(string path) {
		gridOffsets = [];

		Dictionary<string, (int hidden, bool warpable, bool merge)> extraRoomData = [];
		List<(string? timeline, string mapPath)> allMaps = [(null, path)];
		
		Logger.Info("Looking for alternate maps");
		string cutMapPath = path[..path.IndexOfReverse('.')];
		foreach (string alternatePath in Directory.GetFiles(WorldWindow.region.exportPath)) {
			if (alternatePath.StartsWith(cutMapPath) && alternatePath.EndsWith(".txt") && alternatePath != path) {
				string fileName = Path.GetFileNameWithoutExtension(alternatePath);
				Logger.Info($"found alternate map: {fileName}");
				string timeline = fileName.Split('-').Last();
				allMaps.Add((timeline, alternatePath));
			}
		}

		foreach ((string? timeline, string mapPath) in allMaps) {
			totalAvgOffset = Vector2.Zero;
			avgCount = 0;
			List<MapDraggable> offsetDraggables = [];
			foreach (string line in File.ReadAllLines(mapPath)) {
				if (line.IsNullOrEmpty()) continue;

				if (line.StartsWith("//FloodForge;")) {
					string[] data = line[(line.IndexOf(';') + 1)..].Split('|');
					if (data[0] == "ROOM") {
						(int hidden, bool warpable, bool merge) extra = (0, true, true);

						for (int i = 2; i < data.Length; i++) {
							string key = data[i];
							if (key.StartsWith("hidden=") && int.TryParse(key[7..], out extra.hidden)) {
							}
							else if (key == "hidden") {
								extra.hidden = 2;
							}
							else if (key == "nomerge") {
								extra.merge = false;
							}
							else if (key == "nowarp") {
								extra.warpable = false;
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
					try {
						if (!ParseMapRoom(line, out MapDraggable? affectedOffsetDraggable, timeline)) {
							return false;
						}
						if (affectedOffsetDraggable != null)
							offsetDraggables.Add(affectedOffsetDraggable);
					}
					catch (Exception e) {
						Logger.Warn($"Issue encountered while parsing {Path.GetFileName(mapPath)}");
						Logger.Warn($"> {line}");
						Logger.Warn(e);
					}
				}
			}
			if (offsetDraggables.Count != 0 && avgCount != 0 && totalAvgOffset != Vector2.Zero) {
				Logger.Info($"map offset detected in map {Path.GetFileNameWithoutExtension(mapPath)}, fixing");
				Logger.Info($"offsetDraggables.Count: {offsetDraggables.Count}; avg offset: ({totalAvgOffset.x};{totalAvgOffset.y})");
				foreach (MapDraggable offsetDraggable in offsetDraggables) {
					offsetDraggable.CanonPosition -= totalAvgOffset;
				}
			}
		}

		foreach (KeyValuePair<string, (int hidden, bool warpable, bool merge)> pair in extraRoomData) {
			Room room = WorldWindow.region.rooms.First(x => x.name.Equals(pair.Key, StringComparison.InvariantCultureIgnoreCase));

			room.data.hidden = pair.Value.hidden;
			room.data.warpable = pair.Value.warpable;
			room.data.merge = pair.Value.merge;
		}

		List<(float, int)> xOffsetCounters = [];
		List<(float, int)> yOffsetCounters = [];
		foreach ((Room room, Vector2 offset) in gridOffsets) {
			bool found = false;
			for (int i = 0; i < xOffsetCounters.Count; i++) {
				(float existingXOffset, int count) = xOffsetCounters[i];
				if (Math.Round(existingXOffset, 4) == Math.Round(offset.x, 4)) {
					xOffsetCounters[i] = (offset.x, count + 1);
					found = true;
					break;
				}
			}
			if (!found)
				xOffsetCounters.Add((offset.x, 1));
			found = false;
			for (int i = 0; i < yOffsetCounters.Count; i++) {
				(float existingYOffset, int count) = yOffsetCounters[i];
				if (Math.Round(existingYOffset, 4) == Math.Round(offset.y, 4)) {
					yOffsetCounters[i] = (offset.y, count + 1);
					found = true;
					break;
				}
			}
			if (!found)
				yOffsetCounters.Add((offset.y, 1));
		}
		List<(float, int)> orderedXOffsetCounters = [.. xOffsetCounters.OrderByDescending(x => x.Item2)];
		List<(float, int)> orderedYOffsetCounters = [.. yOffsetCounters.OrderByDescending(x => x.Item2)];
		Vector2 commonDir = new(orderedXOffsetCounters.First().Item1, orderedYOffsetCounters.First().Item1);
		if (commonDir != Vector2.Zero) {
			Vector2 reverseOffset = -commonDir;
			foreach (Room room in WorldWindow.region.rooms) {
				room.CanonPosition += reverseOffset;
				room.DevPosition += reverseOffset;
				foreach (ReplaceRoom replaceRoom in room.replaceRooms) {
					replaceRoom.CanonPosition += reverseOffset;
					replaceRoom.DevPosition += reverseOffset;
				}
			}
		}

		return true;
	}

	private enum WorldParseState {
		None,
		ConditionalLinks,
		Rooms,
		Creatures,
		BatMigrationBlockages,
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
		public string originLine = ""; // this is only there for debug purposes
		public Room roomA;
		public uint roomAExitID;
		public Room? roomB = null;
		public string roomBName = "";
		public uint? roomBExitID = null;
		public Timeline timeline = new();
		public string[] preProcessorConditions = [];

		public ConditionalConnection(Room roomA, uint connectionA, string roomBName, string originLine = "") {
			this.roomA = roomA;
			this.roomAExitID = connectionA;
			this.roomBName = roomBName;
			this.originLine = originLine;
		}
	}

	private static Room CreateRoom(string name) {
		Room returnValue;
		if (name.StartsWith("offscreenden", StringComparison.InvariantCultureIgnoreCase)) {
			returnValue = new OffscreenRoom(name, name);
		}
		else {
			string path = WorldWindow.region.roomsPath;
			if (name.StartsWith("gate", StringComparison.InvariantCultureIgnoreCase)) {
				string parentPath = PathUtil.Parent(path);
				path = PathUtil.FindDirectory(parentPath, "gates") ?? "";
				if (path.IsNullOrEmpty()) {
					Logger.Warn($"Couldn't find gates folder in {parentPath}");
				}
			}

			string filePath = PathUtil.FindFile(path, name + ".txt") ?? "";
			if (filePath.IsNullOrEmpty()) {
				Logger.Warn($"Room file {path}/{name}.txt could not be found");
			}

			returnValue = new Room(filePath, name);
		}
		return returnValue;
	}

	private static void ParseWorldRoom(string line, ref List<ConnectionToAdd> connectionsToAdd) {
		LogParser($"Checking line {line}");
		string[] data = line.Split(':', StringSplitOptions.TrimEntries);
		if (data.Length < 2) return;

		string roomName = data[0];
		string[] connections = data[1].Split(',', StringSplitOptions.TrimEntries);
		string[] tags = data[2..];

		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
		if (room == null) {
			room = CreateRoom(roomName);
			WorldWindow.region.rooms.Add(room);
		}

		uint connectionId = 0;
		for (int j = 0; j < connections.Length; j++) {
			string connection = connections[j];
			LogParser($"connections[{j}] - {connection}");
			if (connection.IsNullOrEmpty()) {
				continue;
			}
			if (connection.Equals("disconnected", StringComparison.InvariantCultureIgnoreCase)) {
				connectionId++;
				continue;
			}

			int openArrow = connection.IndexOf('<');
			int closeArrow = connection.IndexOf('>');
			int foundIndex = -1;
			if (openArrow != -1 && closeArrow != -1) {
				if (int.TryParse(connection[(openArrow + 1)..closeArrow], out int outcome)) {
					if (!WorldWindow.connectionExtensionsEnabled) {
						// REVIEW - add popup to allow user to decide?
						WorldWindow.connectionExtensionsEnabled = true;
						Logger.Info($"ConnectionExtensions syntax detected in room {room.name}, set to enabled");
					}
					foundIndex = outcome;
					connection = connection[..openArrow];
				}
				else {
					Logger.Info($"Failed to parse connection {connection} index ({connection[(openArrow + 1)..closeArrow]}) to int");
				}
			}
			LogParser($"foundIndex: - {foundIndex}; connection: {connection}");

			if (connection == roomName && foundIndex == -1 || foundIndex == j) {
				connectionsToAdd.Add(new ConnectionToAdd() {
					roomA = room,
					roomAExitID = (uint)j,
					roomB = room,
					roomBExitID = (uint)j,
					roomBName = roomName
				});
			}

			bool alreadyExists = false;
			for (int i = 0; i < connectionsToAdd.Count; i++) { // look through the connections that have already been found
				ConnectionToAdd connectionData = connectionsToAdd[i];
				if (connectionData.roomB != null && !(connectionData.roomB == room)) continue; // if a connection has already found its other side, skip that connection
				// otherwise, check if the found connection: - comes from the room we're looking for, and: - is looking for this room

				if (connectionData.roomA.name.Equals(connection, StringComparison.InvariantCultureIgnoreCase) && connectionData.roomBName.Equals(roomName, StringComparison.InvariantCultureIgnoreCase)) {
					if (WorldWindow.connectionExtensionsEnabled) {
						if (connectionData.roomBExitID != null && connectionData.roomBExitID != connectionId)
							continue;
					}
					connectionsToAdd[i] = connectionData with { roomB = room, roomBExitID = connectionId };
					alreadyExists = true;
					if (foundIndex != -1)
						break;
				}
			}

			if (alreadyExists) {
				connectionId++;
				continue;
			}

			ConnectionToAdd connectionToAdd = new ConnectionToAdd(room, connectionId, connection);
			if (foundIndex != -1)
				connectionToAdd.roomBExitID = (uint)foundIndex;
			connectionsToAdd.Add(connectionToAdd);
			connectionId++;
		}

		room.data.tags = [ ];
		tags.ForEach(room.data.tags.Toggle);
	}

	// TODO: Handle dynamically
	private static DenCreature.Tag ParseCreatureTag(string tag, string type) {
		if (tag.StartsWith("Mean")) {
			return new DenCreature.FloatTag(Mods.tags["mean"], float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("Seed")) {
			return new DenCreature.IntegerTag(Mods.tags["seed"], int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}
		else if (tag.StartsWith("RotType")) {
			return new DenCreature.IntegerTag(Mods.tags["rottype"], int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
		}

		if (!tag.Contains(':')) {
			try {
				if (type == "polemimic") {
					return new DenCreature.IntegerTag(Mods.tags["polemimic_length"], int.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
				}
				else {
					return new DenCreature.FloatTag(Mods.tags["centipede_length"], float.Parse(tag[(tag.IndexOf(':') + 1)..], NumberStyles.Any, CultureInfo.InvariantCulture));
				}
			}
			catch (FormatException) {}
		}

		return new DenCreature.Tag(Mods.GetOrCreateTag(tag));
	}

	private static bool ParseWorldCreatureLineage(string[] splits, Room room, Timeline timeline, string[] preProcessorConditions) {
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
			timeline = timeline,
			preProcessorConditions = preProcessorConditions
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
			creature.type = Mods.ParseCreature(sections[0]);
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

	private static bool ParseWorldCreatureNormal(string[] splits, Room room, Timeline timeline, string[] preProcessorConditions) {
		foreach (string creatureInDen in Regex.Split(splits[1], @",(?![^{]*})").Select(s => s.Trim())) {
			string[] sections = Regex.Split(creatureInDen, @"-(?![^{]*})");
			int denId = int.Parse(sections[0], NumberStyles.Any, CultureInfo.InvariantCulture);
			string creature = sections[1];

			if (room is OffscreenRoom offscreenDen) {
				denId = 0;
				offscreenDen.GetDen();
			}

			if (denId >= room.nonDenExitCount + room.denShortcutEntrances.Count && denId < room.GarbageWormDenIndex) {
				GarbageWormDen worm = new GarbageWormDen() {
					type = Mods.ParseCreature(creature),
					timeline = timeline,
					preProcessorConditions = preProcessorConditions,
					count = sections.Length < 3 ? 1 : int.Parse(sections[2])
				};
				room.garbageWormDens.Add(worm);
				continue;
			}

			if (!room.HasDen(denId)) {
				Logger.Warn($"{room.name} missing den {denId}");
				return false;
			}

			Den den = room.GetDen(denId);
			DenLineage lineage = new DenLineage(Mods.ParseCreature(creature), 1) {
				timeline = timeline,
				preProcessorConditions = preProcessorConditions
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
			Timeline timeline = new();
			string[] preProcessorConditions = [];

			if (splits[0][0] == '(') {
				int closingBracketPosition = splits[0].IndexOf(')');
				string v = splits[0][1..closingBracketPosition];
				splits[0] = splits[0][(closingBracketPosition + 1)..].Trim();
				if (v.StartsWith("x-", StringComparison.InvariantCultureIgnoreCase)) {
					timeline.timelineType = TimelineType.Except;
					v = v[2..];
				}
				else {
					timeline.timelineType = TimelineType.Only;
				}
				timeline.timelines = [.. v.Split(',')];
			}
			if (splits[0][0] == '{') {
				int closingBracketPosition = splits[0].IndexOf('}');
				string conditions = splits[0][1..closingBracketPosition];
				splits[0] = splits[0][(closingBracketPosition + 1)..].Trim();
				preProcessorConditions = [.. conditions.Split(',')];
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
				if (!ParseWorldCreatureLineage(splits, room, timeline, preProcessorConditions)) return false;
			}
			else {
				if (!ParseWorldCreatureNormal(splits, room, timeline, preProcessorConditions)) return false;
			}
		}
		catch (Exception e) {
			Logger.Warn(e);
			return false;
		}

		return true;
	}

	private static string[] SplitTopLevel(string item, char separator, char[] openers, char[] closers, StringSplitOptions options) {
		List<string> items = [];
		string currentItem = "";
		Stack<int> depth = [];
		for (int i = 0; i < item.Length; i++) {
			if (openers.Contains(item[i])) {
				depth.Push(openers.IndexOf(item[i]));
				currentItem += item[i];
			}
			else if (closers.Contains(item[i]) && depth.Count > 0 && closers[depth.Peek()] == item[i]) {
				depth.Pop();
				currentItem += item[i];
			}
			else {
				if (item[i] == separator && depth.Count == 0) {
					if (options == StringSplitOptions.TrimEntries) {
						currentItem = currentItem.Trim();
					}
					if (currentItem != "" || options != StringSplitOptions.RemoveEmptyEntries) {
						items.Add(currentItem);
					}
					currentItem = "";
				}
				else {
					currentItem += item[i];
				}
			}
		}
		if (currentItem != "") {
			if (options == StringSplitOptions.TrimEntries) {
				currentItem = currentItem.Trim();
			}
			items.Add(currentItem);
		}
		return [.. items];
	}

	// TODO - Add to settings
	private static bool enableLogParser = false;
	private static void LogParser(string message) {
		if (enableLogParser)
			Logger.Info(message);
	}

	private static bool ParseWorldConditionalLink(string link, ref List<ConditionalConnection> conditionalConnectionsToAdd) {
		string[] parts = SplitTopLevel(link, ':', ['(', '{'], [')', '}'], StringSplitOptions.TrimEntries);
		if (parts.Length < 3 || parts.Length > 4) {
			Logger.Warn("Skipping line due to improper length");
			Logger.Warn($"> {link}");
			return false;
		}

		string[] preProcessorConditions = [];

		if (parts[0].Length == 0) {
			Logger.Warn($"Skipping line due to invalid conditional/timeline");
			Logger.Warn($"> {link}");
			return false;
		}
		if (parts[0][0] == '{') {
			int closingBracketPosition = parts[0].IndexOf('}');
			string conditions = parts[0][1..closingBracketPosition];
			parts[0] = parts[0][(closingBracketPosition + 1)..].Trim();
			preProcessorConditions = [.. conditions.Split(',')];
		}

		bool xminus = parts[0].StartsWith("x-", StringComparison.InvariantCultureIgnoreCase);
		if (xminus)
			parts[0] = parts[0][2..];
		string[] timelines = parts[0].Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
		Timeline timeline = new(TimelineType.Only, [.. timelines]);

		string mod = parts[1].ToLowerInvariant();

		if (parts.Length == 3) {
			string roomName2 = parts[2];
			Room? room2 = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName2, StringComparison.InvariantCultureIgnoreCase));
			if (room2 == null) {
				Logger.Warn($"Skipping line due to missing room {roomName2}");
				Logger.Warn($"> {link}");
				return false;
			}

			// REVIEW - this fails to correctly parse a case such as:
			// 		{condition}Watcher : HIDEROOM : XX_A01
            // 		Rivulet : HIDEROOM : XX_A01
			// overwriting the preprocessorcondition for both conditionals to be the same, practically merging two distinct cases
			room2.preProcessorConditions = preProcessorConditions;

			if (mod == "exclusiveroom" || (mod == "hideroom" && xminus)) {
				if (room2.timeline.timelineType == TimelineType.Except) {
					Logger.Warn($"Skipping line due to invalid EXCLUSIVEROOM {roomName2}");
					Logger.Warn($"> {link}");
					return false;
				}

				room2.timeline.timelineType = TimelineType.Only;
				timelines.ForEach(x => room2.timeline.timelines.Add(x));
			}
			else if (mod == "hideroom" || (mod == "exclusiveroom" && xminus)) {
				if (room2.timeline.timelineType == TimelineType.Only) {
					Logger.Warn($"Skipping line due to invalid HIDEROOM {roomName2}");
					Logger.Warn($"> {link}");
					return false;
				}

				room2.timeline.timelineType = TimelineType.Except;
				timelines.ForEach(x => room2.timeline.timelines.Add(x));
			}

			return true;
		}
		else if (parts.Length == 4 && mod == "replaceroom") {
			LogParser($"Parsing REPLACEROOM: {link}");
			ReplaceRoom? similarReplaceRoom = WorldWindow.replaceRooms.FirstOrDefault(x => x.replacedRoom.name == parts[2] && x.replacingRoom.name == parts[3] && x.preProcessorConditions.Length == 0 && preProcessorConditions.Length == 0 && !((x.timeline.timelineType == TimelineType.Except) ^ xminus));
			if (similarReplaceRoom != null) {
				LogParser($"Existing replaceRoom found matching parameters.");
				foreach (string timelineEntry in timelines) {
					similarReplaceRoom.timeline.timelines.Add(timelineEntry);
				}
			}
			else {
				Room? foundRoom = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(parts[2], StringComparison.InvariantCultureIgnoreCase));
				if (foundRoom != null) {
					LogParser($"Found room {foundRoom.name}!");
					Room? replacingRoom = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(parts[3], (StringComparison)3));
					replacingRoom ??= WorldWindow.replaceReferenceRooms.FirstOrDefault(x => x.name.Equals(parts[3], (StringComparison)3));
					if (replacingRoom == null) {
						replacingRoom = CreateRoom(parts[3]);
						replacingRoom.isVirtualRoom = true;
						WorldWindow.replaceReferenceRooms.Add(replacingRoom);
					}
					ReplaceRoom newReplaceRoom = new ReplaceRoom(replacingRoom, foundRoom, new(xminus ? TimelineType.Except : TimelineType.Only, [..timelines]), []);
					replacingRoom.referencingReplaceRooms.Add(newReplaceRoom);
					foundRoom.replaceRooms.Add(newReplaceRoom);
					WorldWindow.replaceRooms.Add(newReplaceRoom);
				}
				else {
					Logger.Warn($"No room {parts[2]} found!");
				}
			}
			return true;
		}

		LogParser($"LINK: {link}");
		LogParser($"isConnection");

		string roomName = parts[1];
		LogParser($"roomName: {roomName}");
		Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(roomName, StringComparison.InvariantCultureIgnoreCase));
		if (room == null) {
			Logger.Warn($"Skipping line due to missing room {roomName}");
			Logger.Warn($"> {link}");
			return false;
		}

		string currentConnection = parts[2];
		int currentConnectionId = -1;
		{
			int openArrow = currentConnection.IndexOf('<');
			int closeArrow = currentConnection.IndexOf('>');
			if (openArrow != -1 && closeArrow != -1) {
				if (int.TryParse(currentConnection[(openArrow + 1)..closeArrow], out int outcome)) {
					if (!WorldWindow.connectionExtensionsEnabled) {
						WorldWindow.connectionExtensionsEnabled = true;
						Logger.Info($"ConnectionExtensions syntax detected in link {link}, set to enabled");
					}
					currentConnectionId = outcome;
					currentConnection = currentConnection[..openArrow];
				}
				else {
					Logger.Error($"Failed to parse connection {currentConnection} index ({currentConnection[(openArrow + 1)..closeArrow]}) to int");
					return false;
				}
			}
		}
		int disconnectedId = -1;
		bool isOriginallyDisconnected = int.TryParse(currentConnection, NumberStyles.Any, CultureInfo.InvariantCulture, out disconnectedId);
		LogParser($"currentConnection: {currentConnection}({currentConnectionId}){(isOriginallyDisconnected ? " (Disconnected)" : "")}");

		string toConnection = parts[3];
		int toConnectionId = -1;
		{
			int openArrow = toConnection.IndexOf('<');
			int closeArrow = toConnection.IndexOf('>');
			if (openArrow != -1 && closeArrow != -1) {
				if (int.TryParse(toConnection[(openArrow + 1)..closeArrow], out int outcome)) {
					if (!WorldWindow.connectionExtensionsEnabled) {
						WorldWindow.connectionExtensionsEnabled = true;
						Logger.Info($"ConnectionExtensions syntax detected in link {link}, set to enabled");
					}
					toConnectionId = outcome;
					toConnection = toConnection[..openArrow];
				}
				else {
					Logger.Error($"Failed to parse connection {toConnection} index ({toConnection[(openArrow + 1)..closeArrow]}) to int");
					return false;
				}
			}
		}
		LogParser($"toConnection: {toConnection}({toConnectionId})");

		if (currentConnection.Equals(toConnection, StringComparison.InvariantCultureIgnoreCase) && (!WorldWindow.connectionExtensionsEnabled || currentConnectionId == toConnectionId)) {
			Logger.Warn("Skipping line due to no change");
			Logger.Warn($"> {link}");
			return false;
		}

		Connection? connection = room.connections.FirstOrDefault(otherConnection => {
			bool roomAMatches = otherConnection.roomA.name.Equals(currentConnection) && (currentConnectionId == -1 || otherConnection.roomAExitID == currentConnectionId);
			bool roomBMatches = otherConnection.roomB.name.Equals(currentConnection) && (currentConnectionId == -1 || otherConnection.roomBExitID == currentConnectionId);
			return roomAMatches || roomBMatches;
		});
		LogParser($"foundConnection: {connection?.roomA.name}<{connection?.roomAExitID}>-{connection?.roomB.name}<{connection?.roomBExitID}>");

		if (toConnection.Equals("disconnected", StringComparison.InvariantCultureIgnoreCase)) {
			LogParser($"to == disconnected, setting timelines to exclude");
			if (connection == null) {
				Logger.Warn("Skipping line due to missing connection");
				Logger.Warn($"> {link}");
				return false;
			}

			if (connection.timeline.timelineType == TimelineType.Only) {
				timelines.ForEach(x => connection.timeline.timelines.Remove(x));
			}
			else {
				connection.timeline.timelineType = TimelineType.Except;
				timelines.ForEach(x => connection.timeline.timelines.Add(x));
			}
			
			connection.preProcessorConditions = preProcessorConditions;
			return true;
		}

		int originConnectionId = -1;
		if (isOriginallyDisconnected) {
			LogParser($"trying to infer originConnectionId through disconnection id"); // here lies the problem.
			string timelineEntry = timelines[0]; // LATER: Figure out what this does and clean up
			bool[] connected = new bool[room.roomExits.Count];
			LogParser($"creating connectedList");
			for (int index = 0; index < connected.Length; index++) {
				foreach (Connection connection2 in room.connections) {
					if (connection2.timeline.timelineType == TimelineType.Only)
						continue;
					if (connection2.roomAExitID != index && connection2.roomBExitID != index)
						continue;
					bool sideARelevant = connection2.roomA == room && connection2.roomAExitID == index;
					bool sideBRelevant = connection2.roomB == room && connection2.roomBExitID == index;
					if (!sideARelevant && !sideBRelevant)
						continue;
					connected[index] = true;
				}
			}
			LogParser($"final connectedList:");
			string finalList = "";
			connected.ForEach(b => finalList += (finalList == "" ? "" : ", ") + b);
			LogParser(finalList);

			for (int i = 0; i < connected.Length; i++) {
				if (connected[i]) continue;

				disconnectedId--;
				if (disconnectedId == 0) {
					originConnectionId = i;
					break;
				}
			}
			LogParser($"result: {originConnectionId}");
		}
		else {
			LogParser($"taking originConnectionId from existing connection");
			if (connection == null) {
				Logger.Warn("Link missing connection, adding new connection anyways");
				Logger.Warn($"> {link}");
			}
			else {
				bool originalConnectionIsRoomA = connection.roomB.name.Equals(currentConnection, (StringComparison)3) && (currentConnectionId == -1 || connection.roomBExitID == currentConnectionId);
				originConnectionId = (int)(originalConnectionIsRoomA ? connection.roomAExitID : connection.roomBExitID);
			}
		}
		LogParser($"final originConnectionId: {originConnectionId}");

		if (connection != null) {
			LogParser($"pre-existing connection found, setting timelines");
			if (connection.timeline.timelineType == TimelineType.Only) {
				timelines.ForEach(x => connection.timeline.timelines.Remove(x));
			}
			else {
				connection.timeline.timelineType = TimelineType.Except;
				timelines.ForEach(x => connection.timeline.timelines.Add(x));
			}

			if (room.name == toConnection && (!WorldWindow.connectionExtensionsEnabled || originConnectionId == toConnectionId)) {
				// self-connection
				conditionalConnectionsToAdd.Add(new ConditionalConnection(room, (uint)originConnectionId, toConnection, link) with {
					roomB = room,
					roomBExitID = (uint)originConnectionId,
					timeline = timeline,
					preProcessorConditions = preProcessorConditions
				});
				return true;
			}

			Room roomA = connection.roomA == room ? connection.roomA : connection.roomB;
			uint roomAID = connection.roomA == room && connection.roomAExitID == originConnectionId ? connection.roomAExitID : connection.roomBExitID;

			LogParser($"Looking for matching unpaired connections");
			for (int i = 0; i < conditionalConnectionsToAdd.Count; i++) {
				ConditionalConnection potentialMatch = conditionalConnectionsToAdd[i];
				if (!potentialMatch.timeline.Match(timeline) || !preProcessorConditions.SequenceEqual(potentialMatch.preProcessorConditions))
					continue;
				LogParser($"checking {potentialMatch.roomA.name}<{potentialMatch.roomAExitID}> - {potentialMatch.roomB?.name ?? $"'{potentialMatch.roomBName}'"}<{potentialMatch.roomBExitID}>");
				bool roomAExitIDMatches = !WorldWindow.connectionExtensionsEnabled || toConnectionId == -1 || potentialMatch.roomAExitID == (uint)toConnectionId;
				bool roomBExitIDMatches = !WorldWindow.connectionExtensionsEnabled || potentialMatch.roomBExitID == null || originConnectionId == -1 || potentialMatch.roomBExitID == (uint)originConnectionId;
				if (potentialMatch.roomB == null && potentialMatch.roomBName.Equals(roomA.name, (StringComparison)3) && potentialMatch.roomA.name.Equals(toConnection, (StringComparison)3) && roomAExitIDMatches && roomBExitIDMatches) {
					LogParser($"Found matching unpaired connection. Setting roomB to {room.name}({originConnectionId})");
					conditionalConnectionsToAdd[i] = potentialMatch with {
						roomB = room,
						roomBExitID = (uint) originConnectionId
					};
					return true;
				}
			}

			LogParser($"Adding to unpaired connections: {roomA.name}<{originConnectionId}> - '{toConnection}'<{(toConnectionId == -1 ? null : (uint)toConnectionId)}>");
			ConditionalConnection conditionalConnection = new(roomA, (uint)originConnectionId, toConnection, link) {
				roomBExitID = toConnectionId == -1 ? null : (uint)toConnectionId,
				timeline = timeline,
				preProcessorConditions = preProcessorConditions
			};
			conditionalConnectionsToAdd.Add(conditionalConnection);

			return true;
		}
		LogParser($"No pre-existing connection found");

		if (originConnectionId == -1) {
			Logger.Warn("originConnectionId cannot be inferred");
			Logger.Warn($"> {link}");
			return false;
		}

		if (room.name == toConnection && (!WorldWindow.connectionExtensionsEnabled || originConnectionId == toConnectionId)) {
			// self-connection
			conditionalConnectionsToAdd.Add(new ConditionalConnection(room, (uint)originConnectionId, toConnection, link) with {
				roomB = room,
				roomBExitID = (uint)originConnectionId,
				timeline = timeline,
				preProcessorConditions = preProcessorConditions
			});
			return true;
		}

		LogParser($"Looking for matching unpaired connections");
		for (int i = 0; i < conditionalConnectionsToAdd.Count; i++) {
			ConditionalConnection connectionData = conditionalConnectionsToAdd[i];
			if (!connectionData.timeline.Match(timeline) || !preProcessorConditions.SequenceEqual(connectionData.preProcessorConditions))
				continue;
			LogParser($"checking {connectionData.roomA.name}<{connectionData.roomAExitID}> - {connectionData.roomB?.name ?? $"'{connectionData.roomBName}'"}<{connectionData.roomBExitID}>");
			bool roomAExitIDMatches = !WorldWindow.connectionExtensionsEnabled || toConnectionId == -1 || connectionData.roomAExitID == (uint)toConnectionId;
			bool roomBExitIDMatches = !WorldWindow.connectionExtensionsEnabled || connectionData.roomBExitID == null || originConnectionId == -1 || connectionData.roomBExitID == (uint)originConnectionId;
			if (connectionData.roomB == null && connectionData.roomA.name.Equals(toConnection, (StringComparison)3) && connectionData.roomBName.Equals(room.name, (StringComparison)3) && roomAExitIDMatches && roomBExitIDMatches) {
				LogParser($"Found matching unpaired connection. Setting roomB to {room.name}({originConnectionId})");
				conditionalConnectionsToAdd[i] = connectionData with {
					roomB = room,
					roomBExitID = (uint) originConnectionId
				};
				return true;
			}
		}

		LogParser($"Adding to unpaired connections: {room.name}<{originConnectionId}> - '{toConnection}'<{(toConnectionId == -1 ? null : (uint)toConnectionId)}>");
		conditionalConnectionsToAdd.Add(new ConditionalConnection() {
			roomA = room,
			roomAExitID = (uint) originConnectionId,
			roomBName = toConnection,
			roomB = null,
			roomBExitID = toConnectionId == -1 ? null : (uint)toConnectionId,
			timeline = new Timeline(TimelineType.Only, [..timelines]),
			preProcessorConditions = preProcessorConditions,
			originLine = link
		});

		return true;
	}

	public static bool ParseWorld(string path) {
		List<ConnectionToAdd> connectionsToAdd = [];
		List<string> conditionalLinks = [];
		WorldParseState parseState = WorldParseState.None;
		WorldWindow.invalidCreaturesEncountered = false;

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

			if (line == "BAT MIGRATION BLOCKAGES") {
				if (parseState != WorldParseState.None) {
					Logger.Warn("Invalid world file. Failed to close " + parseState);
					return false;
				}

				parseState = WorldParseState.BatMigrationBlockages;
				Logger.Info("World - Bat Migration Blockages");
				continue;
			}

			if (line == "END BAT MIGRATION BLOCKAGES") {
				if (parseState != WorldParseState.BatMigrationBlockages) {
					Logger.Warn("Invalid world file. END BAT MIGRATION BLOCKAGES without matching BAT MIGRATION BLOCKAGES");
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
			else if (parseState == WorldParseState.BatMigrationBlockages) {
				Room? room = WorldWindow.region.rooms.FirstOrDefault(x => x.name.Equals(line, StringComparison.InvariantCultureIgnoreCase));
				if (room == null) {
					Logger.Warn($"No room {line} in bat migration blockages");
					continue;
				}

				room.data.blockedBatMigration = true;
			}
		}

		Logger.Info("Loading connections");

		foreach (ConnectionToAdd connectionData in connectionsToAdd) {
			// Logger.Info($"connectionData - roomA: {connectionData.roomA.name} roomB: {connectionData.roomB?.name} roomAExitID: {connectionData.roomAExitID} roomBExitID: {connectionData.roomBExitID}");
			if (connectionData.roomB == null || connectionData.roomBExitID == null) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.name}({connectionData.roomAExitID}) to {connectionData.roomB?.name ?? $"'{connectionData.roomBName}'"}({connectionData.roomBExitID})");
				continue;
			}
			
			if (!connectionData.roomA.ValidConnection(connectionData.roomAExitID) || !connectionData.roomB.ValidConnection(connectionData.roomBExitID.Value)) {
				Logger.Warn($"Failed to load connection from {connectionData.roomA.name}({connectionData.roomAExitID}) to {connectionData.roomB?.name ?? $"'{connectionData.roomBName}'"}({connectionData.roomBExitID}) - Invalid connection indices");
				continue;
			}

			Connection connection = new Connection(connectionData.roomA, connectionData.roomAExitID, connectionData.roomB, connectionData.roomBExitID.Value);
			WorldWindow.region.connections.Add(connection);
			connectionData.roomA.Connect(connection);
			if (connection.roomA != connection.roomB)
				connectionData.roomB.Connect(connection);
		}

		Logger.Info("Loaded connections");
		Logger.Info("Loading conditional links");

		List<ConditionalConnection> conditionalConnectionsToAdd = [];
		bool success = true;
		foreach (string link in conditionalLinks) {
			if (!ParseWorldConditionalLink(link, ref conditionalConnectionsToAdd)) {
				success = false;
				Logger.Warn($"Parse failed on link {link}");
			}
		}

		foreach (ConditionalConnection connectionData in conditionalConnectionsToAdd) {
			if (connectionData.roomB == null) {
				Logger.Warn("Conditional connection failed to load - missing other room");
				Logger.Warn($"Line: {connectionData.originLine}\n> {connectionData.roomA.name}({connectionData.roomAExitID}) - {connectionData.roomBName}({connectionData.roomBExitID})");
				continue;
			}

			if (connectionData.roomBExitID == null) {
				Logger.Warn("Conditional connection failed to load - missing other connection");
				Logger.Warn($"Line: {connectionData.originLine}\n> {connectionData.roomA.name}({connectionData.roomAExitID}) - {connectionData.roomBName}({connectionData.roomBExitID})");
				continue;
			}

			if (!connectionData.roomA.ValidConnection(connectionData.roomAExitID) || !connectionData.roomB.ValidConnection(connectionData.roomBExitID.Value)) {
				Logger.Warn("Conditional connection failed to load - invalid connection indices");
				Logger.Warn($"Line: {connectionData.originLine}\n> {connectionData.roomA.name}({connectionData.roomAExitID}) - {connectionData.roomB.name}({connectionData.roomBExitID})");
				continue;
			}

			Connection? mergeWithConnection = null;
			foreach (Connection existingConnection in connectionData.roomA.connections) {
				if (((existingConnection.roomA == connectionData.roomA && existingConnection.roomAExitID == connectionData.roomAExitID && 
				existingConnection.roomB == connectionData.roomB && existingConnection.roomBExitID == connectionData.roomBExitID) ||
				(existingConnection.roomA == connectionData.roomB && existingConnection.roomAExitID == connectionData.roomBExitID && 
				existingConnection.roomB == connectionData.roomA && existingConnection.roomBExitID == connectionData.roomAExitID)) &&
				existingConnection.preProcessorConditions.SequenceEqual(connectionData.preProcessorConditions)) {
					if (existingConnection.timeline.timelineType == connectionData.timeline.timelineType) {
						mergeWithConnection = existingConnection;
						break;
					}
				}
			}
			if (mergeWithConnection != null) {
				connectionData.timeline.timelines.ForEach(x => mergeWithConnection.timeline.timelines.Add(x));
			}
			else {
				Connection connection = new Connection(connectionData.roomA, connectionData.roomAExitID, connectionData.roomB, connectionData.roomBExitID.Value) {
					timeline = connectionData.timeline,
					preProcessorConditions = connectionData.preProcessorConditions
				};
				WorldWindow.region.connections.Add(connection);
				connectionData.roomA.Connect(connection);
				connectionData.roomB.Connect(connection);
			}
		}

		Logger.Info("Loaded conditional links");
		if (!success)
			PopupManager.Add(new InfoPopup("Issue encountered\nwhile loading links\ncheck log.txt for more info"));

		return true;
	}

	public static string GetRegionDisplayname(string worldPath) {
		string? displaynamePath = PathUtil.FindFile(PathUtil.Parent(worldPath), "displayname.txt");

		return displaynamePath == null ? "" : File.ReadAllText(displaynamePath).Trim();
	}

	public static bool ImportWorldFile(string worldPath, out string? message) {
		message = null;
		WorldWindow.importIncomplete = true;
		if (!File.Exists(worldPath)) {
			message = "Cannot find world_XX.txt";
			Logger.Error(message);
			return false;
		}
		WorldWindow.worldHistory.Clear();
		RecentFiles.AddPath(worldPath);
		
		Logger.Info($"File path: {worldPath}");
		string exportPath = PathUtil.Parent(worldPath);
		if (Path.GetFileNameWithoutExtension(PathUtil.Parent(PathUtil.Parent(exportPath))).Equals("modify", StringComparison.InvariantCultureIgnoreCase)) {
			message = $"Cannot load world from inside /modify folder";
			Logger.Error(message);
			return false;
		}

		roomAttractiveness.Clear();
		WorldWindow.Reset();
		WorldWindow.region.exportPath = exportPath;
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

			if(WorldWindow.region.regionsPaths.Count != 0)
				Logger.Info(string.Join(", ", WorldWindow.region.regionsPaths));
			else
				Logger.Info("'No regions.txt paths found");
		}

		WorldWindow.region.acronym = WorldWindow.region.FindAcronym(WorldWindow.region.acronym);

		Logger.Info("Opening world ", WorldWindow.region.acronym);

		string? roomsPath = PathUtil.FindDirectory(PathUtil.Parent(WorldWindow.region.exportPath), WorldWindow.region.acronym + "-rooms");
		Logger.Info($"RoomsPath: {roomsPath ?? "NULL"}");
		if (roomsPath == null || roomsPath == "") {
			message = "Cannot find rooms directory";
			Logger.Error(message);
			return false;
		}
		WorldWindow.region.roomsPath = roomsPath;

		string? propertiesPath = PathUtil.FindFile(WorldWindow.region.exportPath, "properties.txt");
		if (propertiesPath != null) {
			Logger.Info("Loading properties");
			if (!ParseProperties(propertiesPath)) return false; // Does ParseProperties at any point even have the capacity to return false??
		}

		Logger.Info("Loading world");

		if (!ParseWorld(worldPath)) return false;

		string? mapPath = PathUtil.FindFile(WorldWindow.region.exportPath, "map_" + WorldWindow.region.acronym + ".txt");
		if (mapPath != null) {
			Logger.Info("Loading map");
			if (!ParseMap(mapPath)) return false;
		}
		else {
			Logger.Info("Map file not found");
		}

		Logger.Info("Loading extra room data");

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom) continue;

			foreach ((string, Dictionary<string, RoomAttractiveness>) attr in roomAttractiveness) {
				if (!attr.Item1.Equals(room.name, StringComparison.InvariantCultureIgnoreCase)) continue;

				room.data.attractiveness = attr.Item2;
			}
		}

		Logger.Info("Searching for display name");
		WorldWindow.region.displayName = GetRegionDisplayname(worldPath);

		Logger.Info("Retrieving persistent data");
		PersistentData.GetPersistentData(WorldWindow.region.acronym);

		Logger.Info("World file imported");

		WorldWindow.importIncomplete = false;
		return true;
	}
}