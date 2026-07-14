using System.Text;
using StbImageWriteSharp;
using Stride.Core.Extensions;

namespace FloodForge.World;

public static class WorldExporter {
	private static string AcronymCasing(string acronym) {
		if (Settings.ForceExportCasing.value == Settings.STForceExportCasing.MatchAcronym) {
			return acronym;
		}

		return Settings.ForceExportCasing.value == Settings.STForceExportCasing.Lower ? acronym.ToLowerInvariant() : acronym.ToUpperInvariant();
	}

	private static string FancyRoomCasing(Room room) {
		return room.data.warpable ? RoomNameCasing(room.name) : OppositeRoomCasing(room.name);
	}

	private static string OppositeRoomCasing(string name) {
		string o = RoomNameCasing(name);
		string result = "";

		foreach (char c in o) {
			result += char.IsUpper(c) ? char.ToLower(c) : char.ToUpper(c);
		}

		return result;
	}

	private static string RoomNameCasing(string name) {
		if (name.StartsWith("gate_", StringComparison.InvariantCultureIgnoreCase)) {
			string[] regions = name.Split('_');
			string gateName = Settings.ForceExportCasing.value == Settings.STForceExportCasing.Lower ? "gate_" : "GATE_";

			if (regions[1].Equals(WorldWindow.region.acronym, StringComparison.InvariantCultureIgnoreCase)) {
				gateName += AcronymCasing(WorldWindow.region.acronym);
			}
			else {
				gateName += AcronymCasing(WorldWindow.region.FindAcronym(regions[1]));
			}

			gateName += "_";

			if (regions[2].Equals(WorldWindow.region.acronym, StringComparison.InvariantCultureIgnoreCase)) {
				gateName += AcronymCasing(WorldWindow.region.acronym);
			}
			else {
				gateName += AcronymCasing(WorldWindow.region.FindAcronym(regions[2]));
			}

			return gateName;
		}

		if (Settings.ForceExportCasing.value == Settings.STForceExportCasing.Lower) {
			return name.ToLowerInvariant();
		}
		if (Settings.ForceExportCasing.value == Settings.STForceExportCasing.Upper) {
			return name.ToUpperInvariant();
		}
		if (Settings.ForceExportCasing.value == Settings.STForceExportCasing.MatchAcronym && name.ToLowerInvariant().StartsWith(WorldWindow.region.acronym.ToLowerInvariant())) {
			return WorldWindow.region.acronym + name[WorldWindow.region.acronym.Length..];
		}
		return name;
	}

	public static void ExportMapFile() {
		Logger.Info("Exporting map file");

		Logger.Info("Getting timelines"); // REVIEW - consolidate the timeline gathering into one place. Who knows, maybe WorldWindow already does that lmao.
		HashSet<string> timelinesInRegion = [];
		foreach (Room room in WorldWindow.region.rooms) {
			if(room.timeline.timelineType != TimelineType.All) {
				foreach (string timelineEntry in room.timeline.timelines) {
					timelinesInRegion.Add(timelineEntry);
				}
			}
			foreach (Connection connection in room.connections) {
				if (connection.timeline.timelineType != TimelineType.All) {
					foreach (string timelineEntry in room.timeline.timelines) {
						timelinesInRegion.Add(timelineEntry);
					}
				}
			}
		}
		string timelinesLogger = "";
		foreach (string timeline in timelinesInRegion) {
			timelinesLogger += (timelinesLogger != "" ? ", " : "") + timeline;
		}
		Logger.Info("Final timelines: " + timelinesLogger);

		string fileName = $"map_{WorldWindow.region.acronym}.txt";
		string path = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, fileName);

		Backup.File(path);

		try {
			using StreamWriter writer = new StreamWriter(path, false);
			
			// delete existing timeline-specifying files in case the new export doesn't use those
			foreach (string timelineMapPath in Directory.GetFiles(WorldWindow.region.exportPath)) {
				if (!timelineMapPath.EndsWith(fileName) && timelineMapPath.StartsWith(Path.Combine(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}-")) && path.EndsWith(".txt")) {
					Backup.File(timelineMapPath);
					File.Delete(timelineMapPath);
				}
			}
			
			Dictionary<string, StreamWriter> timelineMapWriters = [];
			foreach (string timeline in timelinesInRegion) {
				string timelineFileName = $"map_{WorldWindow.region.acronym}-{timeline}.txt";
				try {
					string timelinePath = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, timelineFileName);
					StreamWriter timelineWriter = new StreamWriter(timelinePath, false);
					timelineMapWriters.Add(timeline, timelineWriter);
				}
				catch (Exception) {
					Logger.Info($"Error opening {timelineFileName}");
				}
			}
			Logger.Info("- Rooms");

			foreach (Room room in WorldWindow.region.rooms) {
				Vector2 canonPosition = new Vector2(
					(room.CanonPosition.x + room.width * 0.5f) * 3.0f,
					(room.CanonPosition.y - room.height * 0.5f) * 3.0f
				);
				Vector2 devPosition = new Vector2(
					(room.DevPosition.x + room.width * 0.5f) * 3.0f,
					(room.DevPosition.y - room.height * 0.5f) * 3.0f
				);

				string line = $"{FancyRoomCasing(room)}: " +
							$"{canonPosition.x:G12}><{canonPosition.y:G12}><" +
							$"{devPosition.x:G12}><{devPosition.y:G12}><" +
							$"{room.data.layer}><";

				if (room.data.subregion > -1) {
					line += WorldWindow.region.subregions[room.data.subregion];
				}
				
				if (room.timeline.timelineType != TimelineType.Only)
					writer.WriteLine(line);
				foreach (KeyValuePair<string, StreamWriter> timelineMapWriter in timelineMapWriters) {
					if (room.timeline.OverlapsWith(timelineMapWriter.Key)) {
						timelineMapWriter.Value.WriteLine(line);
					}
				}
			}

			Logger.Info("- FloodForge Data");
			foreach (Room room in WorldWindow.region.rooms) {
				if (room is OffscreenRoom || !room.data.ExtraFlags)
					continue;

				writer.Write($"//FloodForge;ROOM|{RoomNameCasing(room.name)}");
				if (room.data.hidden != 0)
					writer.Write("|hidden=" + room.data.hidden);
				if (!room.data.merge)
					writer.Write("|nomerge");
				if (!room.data.warpable)
					writer.Write("|nowarp");
				writer.WriteLine(); // floodforge notes can stay in one map file for ease of import.
			}

			Logger.Info("- Connections");
			foreach (Connection connection in WorldWindow.region.connections) {
				if (connection.roomA.data.hidden == 2 || connection.roomB.data.hidden == 2)
					continue;

				Vector2i connA = connection.roomA.GetShortcutEntranceRoomPoint(connection.roomAExitID);
				Vector2i connB = connection.roomB.GetShortcutEntranceRoomPoint(connection.roomBExitID);

				connA = new Vector2i(connA.x, connection.roomA.height - connA.y - 1);
				connB = new Vector2i(connB.x, connection.roomB.height - connB.y - 1);

				string line = $"Connection: " +
					$"{FancyRoomCasing(connection.roomA)}," +
					$"{FancyRoomCasing(connection.roomB)}," +
					$"{connA.x},{connA.y}," +
					$"{connB.x},{connB.y}," +
					$"{(int) connection.roomA.GetShortcutEntranceDirectionInt(connection.roomAExitID)}," +
					$"{(int) connection.roomB.GetShortcutEntranceDirectionInt(connection.roomBExitID)}";
				if (connection.timeline.timelineType != TimelineType.Only)
					writer.WriteLine(line);
				foreach (KeyValuePair<string, StreamWriter> timelineMapWriter in timelineMapWriters) {
					if (connection.timeline.OverlapsWith(timelineMapWriter.Key)) {
						timelineMapWriter.Value.WriteLine(line);
					}
				}
			}

			writer.Write(WorldWindow.region.extraMap);

			foreach (StreamWriter timelineWriter in timelineMapWriters.Values) {
				timelineWriter?.Dispose();
			}
		}
		catch (Exception) {
			Logger.Info($"Error opening {fileName}");
		}
	}

	private static void ParseConditionalLinkConnection(TextWriter conditionalBufferWriter, Room parsingRoom, Connection connectionToParse, List<string> foundTimelines, Dictionary<string, List<(Room? connectedRoom, bool second)>> timelineStates, List<(Room? connectedRoom, bool second)> defaultTimelineState) {
		Room? connectedToRoom;
		int parsingRoomExitID;

		if (connectionToParse.roomA == parsingRoom) {
			connectedToRoom = connectionToParse.roomB;
			parsingRoomExitID = (int) connectionToParse.roomAExitID;
		}
		else {
			connectedToRoom = connectionToParse.roomA;
			parsingRoomExitID = (int) connectionToParse.roomBExitID;
		}

		if (connectedToRoom == null || parsingRoomExitID == -1)
			return;

		string preProcessorConditionsString = "";
		bool first = true;
		foreach (string condition in connectionToParse.preProcessorConditions) {
			if (!first)
				preProcessorConditionsString += ',';
			first = false;
			preProcessorConditionsString += condition;
		}
		if (!string.IsNullOrEmpty(preProcessorConditionsString))
			preProcessorConditionsString = $"{{{preProcessorConditionsString}}}";

		foreach (string connectionTimeline in connectionToParse.timeline.timelines) {
			if (!timelineStates.ContainsKey(connectionTimeline)) {
				timelineStates[connectionTimeline] = [.. defaultTimelineState];
				foundTimelines.Add(connectionTimeline);
			}

			if (connectionToParse.timeline.timelineType == TimelineType.Only) {
				conditionalBufferWriter.Write($"{preProcessorConditionsString}{connectionTimeline} : {RoomNameCasing(parsingRoom.name)} : ");

				if (timelineStates[connectionTimeline][parsingRoomExitID].connectedRoom == null) {
					int disconnectedBefore = 0;
					for (int i = 0; i < parsingRoomExitID; i++) {
						if (defaultTimelineState[i].connectedRoom == null)
							disconnectedBefore++;
					}
					conditionalBufferWriter.Write(disconnectedBefore + 1);
				}
				else {
					conditionalBufferWriter.Write(timelineStates[connectionTimeline][parsingRoomExitID].connectedRoom);
				}
				conditionalBufferWriter.WriteLine($" : {RoomNameCasing(connectedToRoom.name)}");

				if (connectedToRoom != timelineStates[connectionTimeline][parsingRoomExitID].connectedRoom) {
					timelineStates[connectionTimeline][parsingRoomExitID] = (connectedToRoom, true);
				}
			}
			else if (connectionToParse.timeline.timelineType == TimelineType.Except) {
				foreach (string otherTimeline in foundTimelines) {
					if (otherTimeline == connectionTimeline) {
						continue;
					}
					if (!timelineStates[otherTimeline][parsingRoomExitID].second) {
						continue;
					}

					conditionalBufferWriter.Write($"{preProcessorConditionsString}{otherTimeline} : {RoomNameCasing(parsingRoom.name)} : ");
					if (timelineStates[otherTimeline][parsingRoomExitID].connectedRoom == null) {
						int disconnectedBefore = 0;
						for (int i = 0; i < parsingRoomExitID; i++) {
							if (timelineStates[otherTimeline][i].connectedRoom == null)
								disconnectedBefore++;
						}
						conditionalBufferWriter.Write(disconnectedBefore + 1);
					}
					else {
						conditionalBufferWriter.Write(timelineStates[otherTimeline][parsingRoomExitID].connectedRoom);
					}
					conditionalBufferWriter.WriteLine($" : {RoomNameCasing(connectedToRoom.name)}");
				}

				conditionalBufferWriter.Write($"{preProcessorConditionsString}{connectionTimeline} : {RoomNameCasing(parsingRoom.name)} : ");
				if (timelineStates[connectionTimeline][parsingRoomExitID].second) {
					if (timelineStates[connectionTimeline][parsingRoomExitID].connectedRoom == null) {
						int disconnectedBefore = 0;
						for (int i = 0; i < parsingRoomExitID; i++) {
							if (timelineStates[connectionTimeline][i].connectedRoom == null)
								disconnectedBefore++;
						}
						conditionalBufferWriter.Write(disconnectedBefore + 1);
					}
					else {
						conditionalBufferWriter.Write(timelineStates[connectionTimeline][parsingRoomExitID].connectedRoom);
					}
				}
				else {
					conditionalBufferWriter.Write(RoomNameCasing(connectedToRoom.name));
				}
				conditionalBufferWriter.WriteLine($" : {(defaultTimelineState[parsingRoomExitID].connectedRoom == null ? "DISCONNECTED" : RoomNameCasing(defaultTimelineState[parsingRoomExitID].connectedRoom!.name))}");

				if (connectedToRoom != defaultTimelineState[parsingRoomExitID].connectedRoom) {
					defaultTimelineState[parsingRoomExitID] = (connectedToRoom, false);
				}
			}
		}
	}

	private static void ExportCreatureTags(DenCreature creature, StreamWriter writer) {
		if (creature.tags.Count <= 0) {
			return;
		}

		writer.Write("-{");
		bool first = true;
		foreach (DenCreature.Tag tag in creature.tags) {
			if (!first) writer.Write(",");
			first = false;

			// TODO: Handle dynamically?
			if (tag.id.displayType == Mods.DisplayType.None) {
				writer.Write(tag.id.id);
			}
			else {
				string name = Mods.ExportTagName(tag.id.id) + ":";
				if (tag.id == Mods.tags["polemimic_length"] || tag.id == Mods.tags["centipede_length"]) name = "";
				writer.Write($"{name}{(tag is DenCreature.IntegerTag intTag ? intTag.data : (tag is DenCreature.FloatTag floatTag ? floatTag.data : (tag is DenCreature.StringTag stringTag ? stringTag.data : "IDK LOL")))}");
			}
		}
		writer.Write("}");
	}

	private static string ExportCreatureTags(DenCreature creature) {
		if (creature.tags.Count <= 0) {
			return "";
		}

		string finalTags = "";
		finalTags += "-{";
		bool first = true;
		foreach (DenCreature.Tag tag in creature.tags) {
			if (!first)
				finalTags += ",";
			first = false;

			// TODO: Handle dynamically?
			if (tag.id.displayType == Mods.DisplayType.None) {
				finalTags += tag.id.id;
			}
			else {
				string name = Mods.ExportTagName(tag.id.id) + ":";
				if (tag.id == Mods.tags["polemimic_length"] || tag.id == Mods.tags["centipede_length"]) name = "";
				finalTags += $"{name}{(tag is DenCreature.IntegerTag intTag ? intTag.data : (tag is DenCreature.FloatTag floatTag ? floatTag.data : (tag is DenCreature.StringTag stringTag ? stringTag.data : "IDK LOL")))}";
			}
		}
		return finalTags + "}";
	}

	public static bool KhyExporter() { // TODO - make this a hundred times more compact (this is a naive implementation)
		Logger.Info("");
		Logger.Info("========================================");
		Logger.Info("Starting KhyExporter");
		bool CEEE = WorldWindow.connectionExtensionsEnabled;
		if (CEEE)
			Logger.Info("    connectionExtensions enabled!");
		Logger.Info("");
		Logger.Info("Rooms:");
		List<ExportRoom> allRooms = [];
		foreach (Room room in WorldWindow.region.rooms.OrderBy(r => r.data.tags.Contains("GATE")).ThenBy(r => r.data.tags.Contains("SHELTER")).ThenBy(r => r.name)) {
			Logger.Info($"    {room.name}");
			if (room is OffscreenRoom){
				Logger.Info($"        offscreenroom, skipping");
				continue;
			}
			allRooms.Add(new(room.name, room.data.tags, room.data.subregion, room.data.cameras.Count, new IDExit[room.roomExits.Count], room.timeline));
		}
		Logger.Info("Connections:");
		List<ExportConnection> allConnections = [];
		foreach (Connection connection in WorldWindow.region.connections) {
			ExportConnection newExportConnection = new(new (connection.roomA.name, (int)connection.roomAExitID), new (connection.roomB.name, (int)connection.roomBExitID), connection.timeline);
			Logger.Info($"    {newExportConnection}");
			allConnections.Add(newExportConnection);
		}
		Logger.Info("");
		List<ExportConnection> defaultConnections = [];
		List<ExportConnection> conditionalConnections = [];
		Logger.Info("Finding defaultConnections & conditionalConnections");
		foreach (ExportConnection connection in allConnections) {
			if (connection.timeline.timelineType != TimelineType.Only) {
				Logger.Info($"    added defaultConnection: {connection}");
				defaultConnections.Add(connection);
			}
			if (connection.timeline.timelineType != TimelineType.All) {
				Logger.Info($"    added conditionalConnection: {connection}");
				conditionalConnections.Add(connection);
			}
		}
		Logger.Info("");
		Logger.Info("Populating ExportRoom default connections");
		foreach (ExportRoom exportRoom in allRooms) {
			Logger.Info($"    Populating {exportRoom.name}");
			for (int i = 0; i < exportRoom.connections.Length; i++) {
				foreach (ExportConnection exportConnection in defaultConnections) {
					if((exportConnection.roomA.roomName == exportRoom.name && exportConnection.roomA.exitID == i) ||
					(exportConnection.roomB.roomName == exportRoom.name && exportConnection.roomB.exitID == i)) {
						bool otherRoomisRoomB = exportConnection.roomA.roomName == exportRoom.name && exportConnection.roomA.exitID == i;
						IDExit idExitToSet = otherRoomisRoomB ? exportConnection.roomB : exportConnection.roomA;
						Logger.Info($"        Found: {exportConnection}; set [{i}] -> {(idExitToSet.roomName.IsNullOrEmpty() ? "DISCONNECTED" : idExitToSet)}");
						exportRoom.connections[i] = idExitToSet;
						break;
					}
				}
			}
		}
		Logger.Info("");
		Logger.Info("Checking default state for special connections"); // TODO - apply this to conditionals too
		Dictionary<string, List<string>> defaultSpecifyLists = [];
		bool anythingSpecified = false;
		foreach (ExportRoom exportRoom in allRooms)
			defaultSpecifyLists.Add(exportRoom.name, []);
		foreach (ExportRoom exportRoom in allRooms) {
			Logger.Info($"    Checking {exportRoom.name}");
			List<string> namesEncountered = [];
			for (int exportRoomExit = 0; exportRoomExit < exportRoom.connections.Length; exportRoomExit++) {
				string foundName = exportRoom.connections[exportRoomExit].roomName;
				Logger.Info($"        {exportRoomExit}: {foundName ?? "-"}");
				if (foundName == null)
					continue;
				if (namesEncountered.Contains(foundName)) {
					Logger.Info($"            Duplicate found, added {exportRoom.name} to {foundName} specify list");
					defaultSpecifyLists[foundName].Add(exportRoom.name);
					anythingSpecified = true;
				}
				else
					namesEncountered.Add(foundName);
			}
		}
		if (!CEEE && anythingSpecified) {
			Logger.Info("");
			Logger.Warn("Special connections found with connectionExtension support disabled."); // TODO - give user the option to enable at this point
		}
		Logger.Info("");
		Logger.Info("Finding Room Conditionals"); // TODO - add replaceroom exporting with new replaceRoom rework implementation
		List<ExportRoom> hideRooms = [];
		List<ExportRoom> exclusiveRooms = [];
		foreach (ExportRoom exportRoom in allRooms) {
			if (exportRoom.timeline.timelineType == TimelineType.Only) {
				Logger.Info($"    Found: {exportRoom}; add to exclusiveRooms");
				exclusiveRooms.Add(exportRoom);
			}
			else if (exportRoom.timeline.timelineType == TimelineType.Except) {
				Logger.Info($"    Found: {exportRoom}; add to hideRooms");
				hideRooms.Add(exportRoom);
			}
		}
		Logger.Info("");
		Logger.Info("Finding encountered timelines");
		List<string> timelines = [];
		Logger.Info("    Checking Rooms");
		foreach (ExportRoom exportRoom in allRooms) {
			foreach (string roomTimeline in exportRoom.timeline.timelines) {
				if (!timelines.Contains(roomTimeline)) {
					Logger.Info($"        Found: {roomTimeline}");
					timelines.Add(roomTimeline);
				}
			}
		}
		Logger.Info("    Checking connections");
		foreach (ExportConnection exportConnection in allConnections) {
			foreach (string connectionTimeline in exportConnection.timeline.timelines) {
				if (!timelines.Contains(connectionTimeline)) {
					Logger.Info($"        Found: {connectionTimeline}");
					timelines.Add(connectionTimeline);
				}
			}
		}
		Logger.Info("");
		Logger.Info("Initialising timelineSpecifyLists");
		Dictionary<string, Dictionary<string, List<string>>> timelineSpecifyLists = [];
		Logger.Info("");
		Logger.Info("Running through found timelines");
		List<SpecifiedChange> specifiedChanges = [];
		foreach (string worldTimeline in timelines) {
			Logger.Info($"");
			Logger.Info($"Checking {worldTimeline}");
			List<ExportRoom> roomsInTimeline = [];
			Logger.Info($"    Finding timeline rooms");
			foreach (ExportRoom exportRoom in allRooms) {
				if (exportRoom.timeline.OverlapsWith(TimelineType.Only, [worldTimeline])) {
					Logger.Info($"        added {exportRoom}");
					roomsInTimeline.Add(new (exportRoom.name, [..exportRoom.tags], exportRoom.subregion, exportRoom.cameraCount, new IDExit[exportRoom.connections.Length], exportRoom.timeline)); // new instance to avoid modifying default
				}
			}
			List<ExportConnection> connectionsInTimeline = [];
			Logger.Info($"    Finding timeline connections");
			foreach (ExportConnection exportConnection in allConnections) {
				if (exportConnection.timeline.OverlapsWith(TimelineType.Only, [worldTimeline])) {
					Logger.Info($"        added {exportConnection}");
					connectionsInTimeline.Add(exportConnection);
				}
			}
			Logger.Info("");
			Logger.Info($"    Populating ExportRoom connections for {worldTimeline}");
			foreach (ExportRoom timelineRoom in roomsInTimeline) {
				Logger.Info($"        Populating {timelineRoom.name}");
				for (int exitID = 0; exitID < timelineRoom.connections.Length; exitID++) {
					foreach (ExportConnection timelineConnection in connectionsInTimeline) {						
						if((timelineConnection.roomA.roomName == timelineRoom.name && timelineConnection.roomA.exitID == exitID) ||
						(timelineConnection.roomB.roomName == timelineRoom.name && timelineConnection.roomB.exitID == exitID)) {
							bool otherRoomisRoomB = timelineConnection.roomA.roomName == timelineRoom.name && timelineConnection.roomA.exitID == exitID;
							IDExit idExitToSet = otherRoomisRoomB ? timelineConnection.roomB : timelineConnection.roomA;
							Logger.Info($"            Found: {timelineConnection}; set [{exitID}] -> {(idExitToSet.roomName.IsNullOrEmpty() ? "DISCONNECTED" : idExitToSet)}");
							timelineRoom.connections[exitID] = idExitToSet;
							break;
						}
					}
				}
			}
			Logger.Info("");
			Logger.Info($"    Checking {worldTimeline} for special connections"); // TODO - apply this to conditionals too
			timelineSpecifyLists.Add(worldTimeline, []);
			foreach (ExportRoom exportRoom in allRooms) // add all rooms to handle pair-fix changes later on
				timelineSpecifyLists[worldTimeline].Add(exportRoom.name, []);
			foreach (ExportRoom timelineRoom in roomsInTimeline) {
				Logger.Info($"        Checking {timelineRoom.name}");
				List<string> namesEncountered = [];
				for (int timelineRoomExit = 0; timelineRoomExit < timelineRoom.connections.Length; timelineRoomExit++) {
					string foundName = timelineRoom.connections[timelineRoomExit].roomName;
					Logger.Info($"        {timelineRoomExit}: {foundName ?? "-"}");
					if (foundName == null)
						continue;
					if (namesEncountered.Contains(foundName)) {
						Logger.Info($"            Duplicate found, added {timelineRoom.name} to {foundName} specify list");
						timelineSpecifyLists[worldTimeline][foundName].Add(timelineRoom.name);
					}
					else
						namesEncountered.Add(foundName);
				}
			}
			Logger.Info("");
			Logger.Info($"    Finding changes");
			List<TLChange> changes = [];
			foreach (ExportRoom timelineRoom in roomsInTimeline) {
				Logger.Info($"        Checking {timelineRoom}");
				ExportRoom matchingDefaultRoom = allRooms.First(r => r.name == timelineRoom.name);
				for (int exitIndex = 0; exitIndex < timelineRoom.connections.Length; exitIndex++) {
					IDExit defaultConnection = matchingDefaultRoom.connections[exitIndex];
					defaultConnection.roomName ??= "DISCONNECTED";
					IDExit newConnection = timelineRoom.connections[exitIndex];
					newConnection.roomName ??= "DISCONNECTED";
					if (defaultConnection.roomName != newConnection.roomName || (CEEE && (defaultConnection.exitID != newConnection.exitID))) {
						Logger.Info($"            Change at [{exitIndex}]: {defaultConnection} -> {newConnection}; added");
						changes.Add(new(timelineRoom.name, exitIndex, defaultConnection, newConnection));
					}
					else {
						Logger.Info($"            No change at [{exitIndex}]: {defaultConnection} -> {newConnection};");
					}
				}
			}
			Logger.Info("");
			Logger.Info("    Finding unpaired disconnections"); // Probably not really needed, but i'd rather be safe than have pathfinding get a one-way and break.
			List<TLChange> pairChanges = [];
			foreach (TLChange possibleUnpairedChange in changes) {
				Logger.Info($"        Checking {possibleUnpairedChange}");
				if (possibleUnpairedChange.oldConnection.roomName == "DISCONNECTED") {
					Logger.Info("            DISCONNECTED by default, skipping");
					continue;
				}
				ExportRoom? pairedRoom = roomsInTimeline.FirstOrDefault(x => x.name == possibleUnpairedChange.oldConnection.roomName);
				if (pairedRoom == null) {
					Logger.Info($"            Found unpaired disconnection");
					ExportRoom unpairedRoom = allRooms.First(x => x.name == possibleUnpairedChange.oldConnection.roomName);
					for (int i = 0; i < unpairedRoom.connections.Length; i++) {
						if (unpairedRoom.connections[i].roomName == possibleUnpairedChange.affectedRoom && (!CEEE || (unpairedRoom.connections[i].exitID == possibleUnpairedChange.exitID))) {
							TLChange changeToAdd = new (unpairedRoom.name, i, unpairedRoom.connections[i], new("DISCONNECTED", 0));
							Logger.Info($"            Added {changeToAdd}");
							pairChanges.Add(changeToAdd);
							break;
						}
					}
				}
				else
					Logger.Info($"            Found pairedRoom {pairedRoom.name}");
			}
			Logger.Info("    Adding pair-resolving changes");
			foreach (TLChange pairChange in pairChanges) {
				changes.Add(pairChange);
			}
			Logger.Info("");
			Logger.Info("    Ordering changes");
			List<TLChange> orderedChanges = [];
			for (int changeIndex = 0; changeIndex < changes.Count;) {
				TLChange changeToOrder = changes[changeIndex];
				Logger.Info($"        Looking for conditionals related to: {changeToOrder}");
				List<TLChange> groupedChanges = [changeToOrder];
				for (int compareIndex = changeIndex + 1; compareIndex < changes.Count;) {
					TLChange changeToCompare = changes[compareIndex];
					if ((changeToOrder.oldConnection.roomName == changeToCompare.affectedRoom && (!CEEE || changeToOrder.oldConnection.exitID == changeToCompare.exitID)) || 
						changeToOrder.newConnection.roomName == changeToCompare.affectedRoom && (!CEEE || changeToOrder.newConnection.exitID == changeToCompare.exitID)) {
						Logger.Info($"            Found: {changeToCompare}");
						groupedChanges.Add(changeToCompare);
						changes.RemoveAt(compareIndex);
					}
					else {
						compareIndex++;
					}
				}
				if (groupedChanges.Count > 1) {
					Logger.Info("        Adding ordered change");
					changes.RemoveAt(changeIndex);
					foreach (TLChange change in groupedChanges) {
						orderedChanges.Add(change);
					}
				}
				else {
					changeIndex++;
				}
			}
			Logger.Info("    Adding unordered changes");
			foreach (TLChange change in changes) {
				orderedChanges.Add(change);
			}
			Logger.Info("");
			Logger.Info($"    Adding changes to specifiedChanges");
			foreach(TLChange change in orderedChanges) {
				specifiedChanges.Add(new (change, new (TimelineType.Only, [worldTimeline])));
			}
		}
		bool mergeSimilarChanges = false;
		Logger.Info("");
		Logger.Info("Merging similar changes"); // Review - make this optional?
		if (!mergeSimilarChanges) {
			Logger.Info("    mergeSimilarChanges set to false, skipping");
		}
		List<SpecifiedChange> mergedSpecifiedChanges = [];
		for (int changeID = 0; changeID < specifiedChanges.Count; changeID++) {
			SpecifiedChange changeToMerge = specifiedChanges[changeID];
			if (mergeSimilarChanges) {
				Logger.Info($"    looking at ID {changeID}: {changeToMerge}");
				for (int otherID = changeID + 1; otherID < specifiedChanges.Count;) {
					SpecifiedChange otherChange = specifiedChanges[otherID];
					if (changeToMerge.Matches(otherChange, CEEE)) {
						Logger.Info($"        matched with ID {otherID}: {otherChange}");
						otherChange.timeline.timelines.ForEach(x => changeToMerge.timeline.timelines.Add(x));
						specifiedChanges.RemoveAt(otherID);
					}
					else {
						otherID++;
					}
				}
				Logger.Info($"    end");
			}
			mergedSpecifiedChanges.Add(changeToMerge);
		}
		// TODO - decide where it's necessary to specify exitID in case of connectionExtensions (so we don't clutter the world file with unnecessary specificity)
		Logger.Info("");
		Logger.Info("Composing world_XX.txt file");
		List<string> finalWorldFile = [];
		finalWorldFile.Add("CONDITIONAL LINKS");
		foreach (ExportRoom hideRoom in hideRooms) {
			finalWorldFile.Add($"{hideRoom.timeline.Inverted()} : HIDEROOM : {hideRoom.name}");
		}
		finalWorldFile.Add("");
		foreach (ExportRoom exclusiveRoom in exclusiveRooms) {
			finalWorldFile.Add($"{exclusiveRoom.timeline} : EXCLUSIVEROOM : {exclusiveRoom.name}");
		}
		finalWorldFile.Add("");
		foreach (SpecifiedChange specifiedChange in mergedSpecifiedChanges) {
			string finalLine = $"{specifiedChange.timeline} : {specifiedChange.affectedRoom} : ";
			if (specifiedChange.oldConnection.roomName.IsNullOrEmpty() || specifiedChange.oldConnection.roomName == "DISCONNECTED") {
				ExportRoom affectedRoom = allRooms.First(r => r.name == specifiedChange.affectedRoom);
				int disconnectedBeforeExit = 0;
				for (int exitID = 0; exitID < specifiedChange.exitID; exitID++) {
					if (affectedRoom.connections[exitID].roomName.IsNullOrEmpty())
						disconnectedBeforeExit++;
				}
				finalLine += $"{disconnectedBeforeExit + 1} : ";
			}
			else {
				finalLine += $"{specifiedChange.oldConnection.roomName + (CEEE && defaultSpecifyLists[specifiedChange.oldConnection.roomName].Contains(specifiedChange.affectedRoom) ? $"<{specifiedChange.oldConnection.exitID}>" : "")} : ";
			}
			bool specifyExitID = false;
			foreach (string timeline in specifiedChange.timeline.timelines) {
				if (timelineSpecifyLists[timeline][specifiedChange.affectedRoom].Contains(specifiedChange.newConnection.roomName)) {
					specifyExitID = true;
					break;
				}
			}
			finalLine += specifiedChange.newConnection.roomName + (CEEE && specifyExitID ? $"<{specifiedChange.newConnection.exitID}>" : "");
			finalWorldFile.Add(finalLine);
		}
		finalWorldFile.Add("END CONDITIONAL LINKS");
		finalWorldFile.Add("");
		finalWorldFile.Add("ROOMS");
		foreach (ExportRoom exportRoom in allRooms.OrderBy(r => r.tags.Contains("GATE") ? 0 : 1)
					.ThenBy(r => r.subregion)
					.ThenBy(room => room.tags.Contains("SHELTER") ? 0 : 1)
					.ThenBy(room => room.cameraCount)
					.ThenBy(room => room.name, StringComparer.OrdinalIgnoreCase)) {
			string finalLine = exportRoom.name + " : ";
			for (int i = 0; i < exportRoom.connections.Length; i++)
				finalLine += (i > 0 ? ", " : "") + (exportRoom.connections[i].roomName.IsNullOrEmpty() ? "DISCONNECTED" : (exportRoom.connections[i].roomName + ((CEEE && defaultSpecifyLists[exportRoom.name].Contains(exportRoom.connections[i].roomName)) ? $"<{exportRoom.connections[i].exitID}>" : "")));
			foreach (string tag in exportRoom.tags)
				finalLine += $" : {tag}";
			finalWorldFile.Add(finalLine);
		}
		finalWorldFile.Add("END ROOMS");
		finalWorldFile.Add("");
		finalWorldFile.Add("CREATURES"); // this section is directly taken from the old exporter.
		foreach (Room room in WorldWindow.region.rooms) {
			for (int i = 0; i < room.dens.Count; i++) {
				List<DenLineage?> nonLineageCreatures = [];

				Den den = room.GetDen01(i);
				foreach (DenLineage creature in den.creatures) {
					if (creature.lineageTo != null)
						continue;

					if (string.IsNullOrEmpty(creature.type) || creature.count == 0)
						continue;

					nonLineageCreatures.Add(creature);
				}

				for (int j = 0; j < nonLineageCreatures.Count; j++) {
					string finalCreature = "";
					DenLineage? mainCreature = nonLineageCreatures[j];
					if (mainCreature == null)
						continue;

					List<DenLineage> sameTimelineCreatures = [mainCreature];
					nonLineageCreatures[j] = null;
					for (int k = j + 1; k < nonLineageCreatures.Count; k++) {
						DenLineage? otherCreature = nonLineageCreatures[k];
						if (otherCreature == null)
							continue;

						if (mainCreature.timeline.Match(otherCreature.timeline)) {
							sameTimelineCreatures.Add(otherCreature);
							nonLineageCreatures[k] = null;
						}
					}

					if (mainCreature.timeline.timelineType != TimelineType.All) {
						finalCreature += $"({mainCreature.timeline})";
					}

					if (mainCreature.preProcessorConditions.Length != 0) {
						string text = "";
						bool first1 = true;
						foreach (string preProcessor in mainCreature.preProcessorConditions) {
							if (!first1)
								text += ",";
							first1 = false;
							text += preProcessor;
						}
						finalCreature += $"{{{text}}}";
					}

					if (room == WorldWindow.region.offscreenDen) {
						finalCreature += "OFFSCREEN : ";
					}
					else {
						finalCreature += $"{RoomNameCasing(room.name)} : ";
					}

					bool first = true;

					foreach (DenLineage creature in sameTimelineCreatures) {
						if (!first)
							finalCreature += ", ";
						first = false;

						if (room == WorldWindow.region.offscreenDen) {
							finalCreature += $"0-{Mods.ExportCreatureName(creature.type)}";
						}
						else {
							finalCreature += $"{i + room.nonDenExitCount}-{Mods.ExportCreatureName(creature.type)}";
						}
						finalCreature += ExportCreatureTags(creature);
						if (creature.count > 1)
							finalCreature += $"-{creature.count}";
					}

					finalWorldFile.Add(finalCreature);
				}
			}

			for (int i = 0; i < room.dens.Count; i++) {
				Den den = room.GetDen01(i);
				string finalDen = "";
				foreach (DenLineage lineage in den.creatures) {
					DenCreature creature = lineage;

					if (creature.lineageTo == null)
						continue;

					if (lineage.timeline.timelineType != TimelineType.All && lineage.timeline.timelines.Count > 0) {
						finalDen += "(";
						finalDen += lineage.timeline;
						finalDen += ")";
					}

					if (lineage.preProcessorConditions.Length != 0) {
						string text = "";
						bool first = true;
						foreach (string preProcessor in lineage.preProcessorConditions) {
							if (!first)
								text += ",";
							first = false;
							text += preProcessor;
						}
						finalDen += $"{{{text}}}";
					}

					finalDen += "LINEAGE : ";

					if (room == WorldWindow.region.offscreenDen) {
						finalDen += "OFFSCREEN : ";
					}
					else {
						finalDen += $"{RoomNameCasing(room.name)} : ";
					}

					if (room == WorldWindow.region.offscreenDen) {
						finalDen += "0 : ";
					}
					else {
						finalDen += $"{i + room.nonDenExitCount} : ";
					}

					DenCreature current = creature;
					while (current != null) {
						finalDen += string.IsNullOrEmpty(current.type) || current.count == 0 ? "NONE" : Mods.ExportCreatureName(current.type);

						finalDen += ExportCreatureTags(current);

						if (current.lineageTo == null) {
							finalWorldFile.Add(finalDen + "-0");
							break;
						}
						finalDen += $"-{Math.Clamp(current.lineageChance, 0.0f, 1.0f)}, ";

						current = current.lineageTo;
					}
				}
			}

			if (room == WorldWindow.region.offscreenDen)
				continue;

			foreach (GarbageWormDen worm in room.garbageWormDens) {
				string finalWorm = "";
				if (worm.timeline.timelineType != TimelineType.All) {
					finalWorm += $"({worm.timeline})";
				}

				if (worm.preProcessorConditions.Length != 0) {
					finalWorm += "{";
					bool first = true;
					foreach (string preProcessor in worm.preProcessorConditions) {
						if (!first)
							finalWorm += ",";
						first = false;
						finalWorm += preProcessor;
					}
					finalWorm += "}";
				}

				finalWorm += $"{RoomNameCasing(room.name)} : {room.GarbageWormDenIndex}-{Mods.ExportCreatureName(worm.type)}";
				if (worm.count > 1)
					finalWorm += $"-{worm.count}";
				finalWorldFile.Add(finalWorm);
			}
		}

		if (!WorldWindow.region.extraWorldCreatures.IsNullOrEmpty())
			WorldWindow.region.extraWorldCreatures.Split('\n').ForEach(finalWorldFile.Add);
		finalWorldFile.Add("END CREATURES");
		IOrderedEnumerable<Room> sortedMigrationRooms = WorldWindow.region.rooms
			.Where(room => room is not OffscreenRoom && room.data.blockedBatMigration)
			.OrderBy(room => room.data.tags.Contains("GATE") ? 0 : 1)
			.ThenBy(room => room.data.subregion)
			.ThenBy(room => room.data.tags.Contains("SHELTER") ? 0 : 1)
			.ThenBy(room => room.data.cameras.Count)
			.ThenBy(room => room.name, StringComparer.OrdinalIgnoreCase);

		if (sortedMigrationRooms.Any()) {
			finalWorldFile.Add("");
			finalWorldFile.Add("BAT MIGRATION BLOCKAGES");
			foreach (Room room in sortedMigrationRooms) {
				finalWorldFile.Add($"{FancyRoomCasing(room)}");
			}
			finalWorldFile.Add("END BAT MIGRATION BLOCKAGES");
		}
		if (!WorldWindow.region.extraWorld.IsNullOrEmpty())
			WorldWindow.region.extraWorld.Split('\n').ForEach(finalWorldFile.Add);;
		
		string worldFileStringified = "";
		finalWorldFile.ForEach(l => worldFileStringified += "\n" + l);
		Logger.Info($"\n---------------------------{worldFileStringified}\n---------------------------");
		Logger.Info("");
		Logger.Info("End KhyExporter!");
		Logger.Info("========================================");
		Logger.Info("");
		return true;
	}

	public struct TLChange(string affectedRoom, int exitID, IDExit oldConnection, IDExit newConnection) {
		public string affectedRoom = affectedRoom;
		public int exitID = exitID;
		public IDExit oldConnection = oldConnection;
		public IDExit newConnection = newConnection;
		public readonly override string ToString() {
			return $"{this.affectedRoom}({this.exitID}) {this.oldConnection} -> {this.newConnection}";
		}
	}

	public struct SpecifiedChange(TLChange change, Timeline timeline) {
		public string affectedRoom = change.affectedRoom;
		public int exitID = change.exitID;
		public IDExit oldConnection = change.oldConnection;
		public IDExit newConnection = change.newConnection;
		public Timeline timeline = timeline;
		public readonly bool Matches (SpecifiedChange otherChange, bool CEEE) {
			return this.affectedRoom == otherChange.affectedRoom && this.exitID == otherChange.exitID && this.oldConnection.roomName == otherChange.oldConnection.roomName && this.newConnection.roomName == otherChange.newConnection.roomName
				&& (!CEEE || this.oldConnection.exitID == otherChange.oldConnection.exitID && this.newConnection.exitID == otherChange.newConnection.exitID);
		}
		public readonly override string ToString() {
			return $"{this.affectedRoom}({this.exitID}) {this.oldConnection} -> {this.newConnection}";
		}
	}

	public struct IDExit(string room, int exitID) {
		public string roomName = room;
		public int exitID = exitID;
		public override readonly string ToString() {
			return $"{this.roomName ?? "DISCONNECTED"}{((this.roomName == null || this.roomName == "DISCONNECTED" )? "" : $"<{this.exitID}>")}";
		}
	}

	public class ExportRoom(string name, HashSet<string> tags, int subregion, int cameraCount, IDExit[] connections, Timeline timeline) {
		public string name = name;
		public IDExit[] connections = connections;
		public Timeline timeline = timeline;
		public HashSet<string> tags = tags;
		
		// ordering information
		public int subregion = subregion;
		public int cameraCount = cameraCount;
		public override string ToString() {
			return (this.timeline.timelineType == TimelineType.All ? "" : $"({this.timeline}) ") + $"{this.name}";
		}
	}

	public class ExportConnection(IDExit roomA, IDExit roomB, Timeline timeline) {
		public IDExit roomA = roomA;
		public IDExit roomB = roomB;
		public Timeline timeline = timeline;
		public override string ToString() {
			return (this.timeline.timelineType == TimelineType.All ? "" : $"({this.timeline}) ") + $"{this.roomA.roomName}({this.roomA.exitID}) - {this.roomB.roomName}({this.roomB.exitID})";
		}
	}

	public static void ExportWorldFile() {
		Logger.Info("Exporting world file");
		
		if (KhyExporter())
			return;

		string fileName = $"world_{WorldWindow.region.acronym}.txt";
		string path = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, fileName);
		Backup.File(path);

		try {
			using StreamWriter worldFileWriter = new StreamWriter(path, false);

			Dictionary<string, List<(Room?, bool)>> roomDefaultStates = [];

			Logger.Info("- Conditional Links");
			StringBuilder conditionalLinksBuffer = new StringBuilder();
			using (StringWriter conditionalBufferWriter = new StringWriter(conditionalLinksBuffer)) {
				
				foreach (Room roomToParse in WorldWindow.region.rooms) {
					if (roomToParse is OffscreenRoom)
						continue;

					List<string> detectedTimelines = [];
					Dictionary<string, List<(Room?, bool)>> timelineStates = [];
					List<(Room?, bool)> defaultTimelineState = [];
					for (int i = 0; i < roomToParse.roomExits.Count; i++) {
						defaultTimelineState.Add((null, false));
					}

					foreach (Connection roomConnection in roomToParse.connections) {
						if (roomConnection.timeline.timelineType != TimelineType.All)
							continue;

						if (roomConnection.roomA == roomToParse) {
							defaultTimelineState[(int) roomConnection.roomAExitID] = (roomConnection.roomB, false);
						}
						else {
							defaultTimelineState[(int) roomConnection.roomBExitID] = (roomConnection.roomA, false);
						}
					}

					foreach (Connection exceptConnection in roomToParse.connections) {
						if (exceptConnection.timeline.timelineType != TimelineType.Except || exceptConnection.timeline.timelines.Count == 0)
							continue;

						ParseConditionalLinkConnection(conditionalBufferWriter, roomToParse, exceptConnection, detectedTimelines, timelineStates, defaultTimelineState);
					}

					foreach (Connection onlyConnection in roomToParse.connections) {
						if (onlyConnection.timeline.timelineType != TimelineType.Only || onlyConnection.timeline.timelines.Count == 0)
							continue;

						ParseConditionalLinkConnection(conditionalBufferWriter, roomToParse, onlyConnection, detectedTimelines, timelineStates, defaultTimelineState);
					}

					foreach (Connection allConnection in roomToParse.connections) {
						if (allConnection.timeline.timelineType != TimelineType.All)
							continue;
						
						// i don't understand why this one is parsed too
						ParseConditionalLinkConnection(conditionalBufferWriter, roomToParse, allConnection, detectedTimelines, timelineStates, defaultTimelineState);
					}

					roomDefaultStates[RoomNameCasing(roomToParse.name)] = defaultTimelineState;

					if ((roomToParse.timeline.timelineType == TimelineType.All || roomToParse.timeline.timelines.Count == 0) && roomToParse.preProcessorConditions.Length == 0) {
						continue;
					}

					Timeline virtualTimeline = roomToParse.timeline;

					// REVIEW - check whether this works with preprocessorconditions?
					foreach (Room replacingRoom in roomToParse.replacingRooms) {
						Timeline resultingTimeline = replacingRoom.timeline.Inverted().And(roomToParse.timeline.Inverted()).Inverted(); // this is cursed but should work (XOR)
						virtualTimeline = resultingTimeline;
					}

					if ((virtualTimeline.timelineType == TimelineType.All || virtualTimeline.timelines.Count == 0) && roomToParse.preProcessorConditions.Length == 0) {
						continue;
					}

					if (roomToParse.preProcessorConditions.Length != 0) {
						conditionalBufferWriter.Write("{");
						bool first1 = true;
						foreach (string preProcessor in roomToParse.preProcessorConditions) {
							if (!first1)
								conditionalBufferWriter.Write(",");
							first1 = false;
							conditionalBufferWriter.Write(preProcessor);
						}
						conditionalBufferWriter.Write("}");
					}

					if (virtualTimeline.timelineType == TimelineType.All || virtualTimeline.timelines.Count == 0) {
						continue;
					}

					bool first = true;
					foreach (string timeline in virtualTimeline.timelines) {
						if (!first)
							conditionalBufferWriter.Write(",");
						first = false;
						conditionalBufferWriter.Write(timeline);
					}

					conditionalBufferWriter.Write(" : ");
					if (roomToParse.replacedRoom != null)
						conditionalBufferWriter.Write($"REPLACEROOM : {roomToParse.replacedRoom.name}");
					else
						conditionalBufferWriter.Write((virtualTimeline.timelineType == TimelineType.Only) ? "EXCLUSIVEROOM" : "HIDEROOM");
					conditionalBufferWriter.WriteLine($" : {RoomNameCasing(roomToParse.name)}");
				}
			}

			if (conditionalLinksBuffer.Length > 0) {
				worldFileWriter.WriteLine("CONDITIONAL LINKS");
				worldFileWriter.Write(conditionalLinksBuffer.ToString());
				worldFileWriter.WriteLine("END CONDITIONAL LINKS");
				worldFileWriter.WriteLine();
			}

			Logger.Info("- Rooms");
			worldFileWriter.WriteLine("ROOMS");

			IOrderedEnumerable<Room> sortedRooms = WorldWindow.region.rooms
					.Where(room => room is not OffscreenRoom)
					.OrderBy(room => room.data.tags.Contains("GATE") ? 0 : 1)
					.ThenBy(room => room.data.subregion)
					.ThenBy(room => room.data.tags.Contains("SHELTER") ? 0 : 1)
					.ThenBy(room => room.data.cameras.Count)
					.ThenBy(room => room.name, StringComparer.OrdinalIgnoreCase);

			int? lastSubregion = null;
			bool isFirstRoom = true;
			bool wasGate = false;

			foreach (Room room in sortedRooms) {
				if (room.replacedRoom != null)
					continue;

				bool isGate = room.data.tags.Contains("GATE");

				if (!isFirstRoom && ((wasGate && !isGate) || (!isGate && room.data.subregion != lastSubregion))) {
					worldFileWriter.WriteLine();
				}
				
				isFirstRoom = false;
				wasGate = isGate;
				lastSubregion = room.data.subregion;

				worldFileWriter.Write($"{FancyRoomCasing(room)} : ");

				List<(Room?, bool)> connections = roomDefaultStates[RoomNameCasing(room.name)];

				for (int i = 0; i < room.roomExits.Count; i++) {
					if (i > 0) worldFileWriter.Write(", ");

					worldFileWriter.Write(connections[i].Item1 == null ? "DISCONNECTED" : FancyRoomCasing(connections[i].Item1!));
				}

				foreach (string tag in room.data.tags) {
					worldFileWriter.Write($" : {tag}");
				}

				worldFileWriter.WriteLine();
			}
			worldFileWriter.WriteLine("END ROOMS");
			worldFileWriter.WriteLine();

			Logger.Info("- Creatures");
			worldFileWriter.WriteLine("CREATURES");

			foreach (Room room in WorldWindow.region.rooms) {
				for (int i = 0; i < room.dens.Count; i++) {
					List<DenLineage?> nonLineageCreatures = [];

					Den den = room.GetDen01(i);
					foreach (DenLineage creature in den.creatures) {
						if (creature.lineageTo != null)
							continue;

						if (string.IsNullOrEmpty(creature.type) || creature.count == 0)
							continue;

						nonLineageCreatures.Add(creature);
					}

					for (int j = 0; j < nonLineageCreatures.Count; j++) {
						DenLineage? mainCreature = nonLineageCreatures[j];
						if (mainCreature == null)
							continue;

						List<DenLineage> sameTimelineCreatures = [mainCreature];
						nonLineageCreatures[j] = null;
						for (int k = j + 1; k < nonLineageCreatures.Count; k++) {
							DenLineage? otherCreature = nonLineageCreatures[k];
							if (otherCreature == null)
								continue;

							if (mainCreature.timeline.Match(otherCreature.timeline)) {
								sameTimelineCreatures.Add(otherCreature);
								nonLineageCreatures[k] = null;
							}
						}

						if (mainCreature.timeline.timelineType != TimelineType.All) {
							worldFileWriter.Write("(");
							worldFileWriter.Write(mainCreature.timeline.ToString());
							worldFileWriter.Write(")");
						}

						if (mainCreature.preProcessorConditions.Length != 0) {
							string text = "{";
							bool first1 = true;
							foreach (string preProcessor in mainCreature.preProcessorConditions) {
								if (!first1)
									text += ",";
								first1 = false;
								text += preProcessor;
							}
							text += "}";
							worldFileWriter.Write(text);
						}

						if (room == WorldWindow.region.offscreenDen) {
							worldFileWriter.Write("OFFSCREEN : ");
						}
						else {
							worldFileWriter.Write($"{RoomNameCasing(room.name)} : ");
						}

						bool first = true;

						foreach (DenLineage creature in sameTimelineCreatures) {
							if (!first)
								worldFileWriter.Write(", ");
							first = false;

							if (room == WorldWindow.region.offscreenDen) {
								worldFileWriter.Write($"0-{Mods.ExportCreatureName(creature.type)}");
							}
							else {
								worldFileWriter.Write($"{i + room.nonDenExitCount}-{Mods.ExportCreatureName(creature.type)}");
							}
							ExportCreatureTags(creature, worldFileWriter);
							if (creature.count > 1)
								worldFileWriter.Write($"-{creature.count}");
						}

						worldFileWriter.WriteLine();
					}
				}

				for (int i = 0; i < room.dens.Count; i++) {
					Den den = room.GetDen01(i);
					foreach (DenLineage lineage in den.creatures) {
						DenCreature creature = lineage;

						if (creature.lineageTo == null)
							continue;

						if (lineage.timeline.timelineType != TimelineType.All && lineage.timeline.timelines.Count > 0) {
							worldFileWriter.Write("(");
							worldFileWriter.Write(lineage.timeline);
							worldFileWriter.Write(")");
						}

						if (lineage.preProcessorConditions.Length != 0) {
							string text = "{";
							bool first = true;
							foreach (string preProcessor in lineage.preProcessorConditions) {
								if (!first)
									text += ",";
								first = false;
								text += preProcessor;
							}
							text += "}";
							worldFileWriter.Write(text);
						}

						worldFileWriter.Write("LINEAGE : ");

						if (room == WorldWindow.region.offscreenDen) {
							worldFileWriter.Write("OFFSCREEN : ");
						}
						else {
							worldFileWriter.Write($"{RoomNameCasing(room.name)} : ");
						}

						if (room == WorldWindow.region.offscreenDen) {
							worldFileWriter.Write("0 : ");
						}
						else {
							worldFileWriter.Write($"{i + room.nonDenExitCount} : ");
						}

						DenCreature current = creature;
						while (current != null) {
							worldFileWriter.Write(string.IsNullOrEmpty(current.type) || current.count == 0 ? "NONE" : Mods.ExportCreatureName(current.type));

							ExportCreatureTags(current, worldFileWriter);

							if (current.lineageTo == null) {
								worldFileWriter.WriteLine("-0");
								break;
							}
							worldFileWriter.Write($"-{Math.Clamp(current.lineageChance, 0.0f, 1.0f)}, ");

							current = current.lineageTo;
						}
					}
				}

				if (room == WorldWindow.region.offscreenDen)
					continue;

				foreach (GarbageWormDen worm in room.garbageWormDens) {
					if (worm.timeline.timelineType != TimelineType.All) {
						worldFileWriter.Write("(");
						worldFileWriter.Write(worm.timeline);
						worldFileWriter.Write(")");
					}

					if (worm.preProcessorConditions.Length != 0) {
						worldFileWriter.Write("{");
						bool first = true;
						foreach (string preProcessor in worm.preProcessorConditions) {
							if (!first)
								worldFileWriter.Write(",");
							first = false;
							worldFileWriter.Write(preProcessor);
						}
						worldFileWriter.Write("}");
					}

					worldFileWriter.Write($"{RoomNameCasing(room.name)} : {room.GarbageWormDenIndex}-{Mods.ExportCreatureName(worm.type)}");
					if (worm.count > 1)
						worldFileWriter.Write($"-{worm.count}");
					worldFileWriter.WriteLine();
				}
			}

			worldFileWriter.Write(WorldWindow.region.extraWorldCreatures);
			worldFileWriter.WriteLine("END CREATURES");

			Logger.Info("- Bat migration blockages");

			IOrderedEnumerable<Room> sortedMigrationRooms = WorldWindow.region.rooms
				.Where(room => room is not OffscreenRoom && room.data.blockedBatMigration)
				.OrderBy(room => room.data.tags.Contains("GATE") ? 0 : 1)
				.ThenBy(room => room.data.subregion)
				.ThenBy(room => room.data.tags.Contains("SHELTER") ? 0 : 1)
				.ThenBy(room => room.data.cameras.Count)
				.ThenBy(room => room.name, StringComparer.OrdinalIgnoreCase);

			if (sortedMigrationRooms.Any()) {
				worldFileWriter.WriteLine();
				worldFileWriter.WriteLine("BAT MIGRATION BLOCKAGES");
				foreach (Room room in sortedMigrationRooms) {
					worldFileWriter.WriteLine($"{FancyRoomCasing(room)}");
				}
				worldFileWriter.WriteLine("END BAT MIGRATION BLOCKAGES");
			}

			worldFileWriter.Write(WorldWindow.region.extraWorld);
		}
		catch (Exception exception) {
			Logger.Info($"Error opening world_{WorldWindow.region.acronym}.txt");
			Logger.Info($"> {exception}");
		}
	}

	// REVIEW: this does not take into account preprocessorconditions - for example, Watcher's WAUA does not contain a "map_WAUA-Watcher.png",
	// whereas this method does end up creating one.
	// One possible solution might be to have the timeline getting simply ignore any rooms and connections that have preprocessorconditions?
	// but there's multiple other checks that would have to be added/changed to make sure it works in all cases, which is why I'm leaving this for REVIEW.
	public static void ExportImageFile(string outputPath) {
		Logger.Info("Exporting image file");

		Logger.Info("Getting timelines");
		HashSet<string> timelinesInRegion = [];
		foreach (Room room in WorldWindow.region.rooms) {
			if(room.timeline.timelineType != TimelineType.All) {
				foreach (string timelineEntry in room.timeline.timelines) {
					timelinesInRegion.Add(timelineEntry);
				}
			}
			foreach (Connection connection in room.connections) {
				if (connection.timeline.timelineType != TimelineType.All) {
					foreach (string timelineEntry in room.timeline.timelines) {
						timelinesInRegion.Add(timelineEntry);
					}
				}
			}
		}
		string timelinesLogger = "";
		foreach (string timeline in timelinesInRegion) {
			timelinesLogger += (timelinesLogger != "" ? ", " : "") + timeline;
		}
		Logger.Info("Final timelines: " + timelinesLogger);

		string mapPath = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_image_{WorldWindow.region.acronym}.txt");
		Backup.File(mapPath);

		StreamWriter? mapFile = null;

		try {
			mapFile = new StreamWriter(mapPath, false);
		}
		catch (Exception) {
			Logger.Info($"Error creating map_image_{WorldWindow.region.acronym}.txt");
		}

		// delete existing timeline-specifying files in case the new export doesn't use those
		foreach (string path in Directory.GetFiles(WorldWindow.region.exportPath)) {
			if (path != mapPath && path.StartsWith(Path.Combine(WorldWindow.region.exportPath, $"map_image_{WorldWindow.region.acronym}-")) && path.EndsWith(".txt")) {
				Backup.File(path);
				File.Delete(path);
			}
		}

		Dictionary<string, StreamWriter?> timelineMapFiles = [];
		foreach (string timeline in timelinesInRegion) {
			string timelineMapPath = PathUtil.FindOrAssumeDirectory(WorldWindow.region.exportPath, $"map_image_{WorldWindow.region.acronym}-{timeline}.txt");
			Backup.File(timelineMapPath);

			try {
				StreamWriter? timelineMapFile = new StreamWriter(timelineMapPath, false);
				timelineMapFiles.Add(timeline, timelineMapFile);
			}
			catch (Exception) {
				Logger.Info($"Error creating map_image_{WorldWindow.region.acronym}-{timeline}.txt");
			}
		}

		Vector2 topLeft = new Vector2(float.PositiveInfinity, float.PositiveInfinity);
		Vector2 bottomRight = new Vector2(float.NegativeInfinity, float.NegativeInfinity);

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom)
				continue;

			float left = room.CanonPosition.x;
			float right = room.CanonPosition.x + room.width;
			float top = room.CanonPosition.y - room.height;
			float bottom = room.CanonPosition.y;

			topLeft.x = Math.Min(topLeft.x, left);
			bottomRight.x = Math.Max(bottomRight.x, right);
			topLeft.y = Math.Min(topLeft.y, top);
			bottomRight.y = Math.Max(bottomRight.y, bottom);
		}

		int layerHeight = Math.Max((int) (bottomRight.y - topLeft.y) + 20, 20);
		int textureWidth = Math.Max((int) (bottomRight.x - topLeft.x) + 20, 20);
		int textureHeight = layerHeight * 3;

		byte[] imageData = new byte[textureWidth * textureHeight * 3];

		Dictionary<string, byte[]> timelineImageData = [];
		foreach (string timeline in timelinesInRegion) {
			timelineImageData.Add(timeline, new byte[textureWidth * textureHeight * 3]);
		}

		for (int y = 0; y < textureHeight; y++) {
			for (int x = 0; x < textureWidth; x++) {
				int i = (y * textureWidth + x) * 3;
				if (Settings.DEBUGVisibleOutputPadding && (x < 10 || (y % layerHeight) < 10 || x >= textureWidth - 10 || (y % layerHeight) >= layerHeight - 10)) {
					imageData[i] = 0;
					imageData[i + 1] = 255;
					imageData[i + 2] = 255;
					foreach (byte[] image in timelineImageData.Values) {
						image[i] = 0;
						image[i + 1] = 255;
						image[i + 2] = 255;
					}
				}
				else {
					imageData[i] = 0;
					imageData[i + 1] = 255;
					imageData[i + 2] = 0;
					foreach (byte[] image in timelineImageData.Values) {
						image[i] = 0;
						image[i + 1] = 255;
						image[i + 2] = 0;
					}
				}
			}
		}

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom || room.data.hidden == 2)
				continue;

			Vector2i roomPosition = new Vector2i(
				(int) (room.CanonPosition.x - topLeft.x),
				(int) (bottomRight.y - room.CanonPosition.y)
			);

			int layerXOffset = 10;
			int layerYOffset = (2 - room.data.layer) * layerHeight + 10;

			if (room.data.hidden == 1) {
				for (int ox = 0; ox < room.width; ox++) {
					for (int oy = 0; oy < room.height; oy++) {
						int targetX = roomPosition.x + ox + layerXOffset;
						int targetY = roomPosition.y + oy + layerYOffset;

						if (targetX < 0 || targetX >= textureWidth || targetY < 0 || targetY >= textureHeight)
							continue;

						int i = (targetY * textureWidth + targetX) * 3;

						if (room.timeline.timelineType != TimelineType.Only) { // only rooms set to "only" don't appear in the default map
							bool pixelIsGreen = imageData[i] == 0 && imageData[i + 2] == 0;
							if (!room.data.merge || pixelIsGreen) {
								imageData[i] = 0;
								imageData[i + 1] = 0;
								imageData[i + 2] = 0;
							}
						}

						foreach (KeyValuePair<string, StreamWriter?> timelineWriter in timelineMapFiles) {
							if (room.timeline.OverlapsWith(timelineWriter.Key)) {
								bool pixelIsGreen = timelineImageData[timelineWriter.Key][i] == 0 && timelineImageData[timelineWriter.Key][i + 2] == 0;
								if (!room.data.merge || pixelIsGreen) {
									timelineImageData[timelineWriter.Key][i] = 0;
									timelineImageData[timelineWriter.Key][i + 1] = 0;
									timelineImageData[timelineWriter.Key][i + 2] = 0;
								}
							}
						}
					}
				}

				continue;
			}

			int mapfileRoomYPos = textureHeight - roomPosition.y - layerYOffset - room.height;

			if (room.timeline.timelineType != TimelineType.Only) { // only rooms set to "only" don't appear in the default map
				mapFile?.WriteLine($"{RoomNameCasing(room.name)}: {roomPosition.x + layerXOffset},{mapfileRoomYPos},{room.width},{room.height}");
			}
			foreach (KeyValuePair<string, StreamWriter?> timelineWriter in timelineMapFiles) {
				if (room.timeline.OverlapsWith(timelineWriter.Key)) {
					timelineWriter.Value?.WriteLine($"{RoomNameCasing(room.name)}: {roomPosition.x + layerXOffset},{mapfileRoomYPos},{room.width},{room.height}");
				}
			}

			for (int ox = 0; ox < room.width; ox++) {
				for (int oy = 0; oy < room.height; oy++) {
					int targetX = roomPosition.x + ox + layerXOffset;
					int targetY = roomPosition.y + oy + layerYOffset;

					if (targetX < 0 || targetX >= textureWidth || targetY < 0 || targetY >= textureHeight)
						continue;

					int i = (targetY * textureWidth + targetX) * 3;
					uint tile = room.GetTile(ox, oy);
					uint tileType = tile & 15;

					byte r = 0, g = 0, b = 0;

					if (tileType == 0 || tileType == 4 || tileType == 5) {
						r = 255;
						g = 0;
					}
					if (tileType == 1) {
						r = 0;
						g = 0;
					}
					if (tileType == 2 || tileType == 3 || (tile & Room.FLAG_HORIZONTAL_POLE) != 0 || (tile & Room.FLAG_VERTICAL_POLE) != 0) {
						r = 153;
						g = 0;
					}

					if (room.visuals.UnderTerrain(ox, oy, out bool slope)) {
						g = 0;
						if (slope) {
							r = Math.Min(r, (byte) 153);
						} else {
							r = 0;
						}
					}

					if (r > 0 && room.visuals.Underwater(ox, oy)) {
						b = 255;
					}

					bool isBlack = r == 0 && g == 0 && b == 0;

					if (room.timeline.timelineType != TimelineType.Only) { // only rooms set to "only" don't appear in the default map
						bool pixelIsGreen = imageData[i] == 0 && imageData[i + 2] == 0;
						if (!room.data.merge || !isBlack || pixelIsGreen) {
							imageData[i] = r;
							imageData[i + 1] = g;
							imageData[i + 2] = b;
						}
					}
					foreach (KeyValuePair<string, StreamWriter?> timelineWriter in timelineMapFiles) {
						if (room.timeline.OverlapsWith(timelineWriter.Key)) {
							bool pixelIsGreen = timelineImageData[timelineWriter.Key][i] == 0 && timelineImageData[timelineWriter.Key][i + 2] == 0;
							if (!room.data.merge || !isBlack || pixelIsGreen) {
								timelineImageData[timelineWriter.Key][i] = r;
								timelineImageData[timelineWriter.Key][i + 1] = g;
								timelineImageData[timelineWriter.Key][i + 2] = b;
							}
						}
					}
				}
			}
		}

		mapFile?.Dispose();
		foreach (StreamWriter? writer in timelineMapFiles.Values) {
			writer?.Dispose();
		}

		Backup.File(outputPath);
		try {
			{
				using Stream stream = File.OpenWrite(outputPath);
				ImageWriter writer = new ImageWriter();
				writer.WritePng(imageData, textureWidth, textureHeight, ColorComponents.RedGreenBlue, stream);
			}
			if (Settings.ExportPsdFiles) {
				string psdPath = Path.ChangeExtension(outputPath, ".psd");
				ImageUtil.WritePsd(psdPath, imageData, textureWidth, textureHeight);
			}

			foreach (KeyValuePair<string, byte[]> item in timelineImageData) {
				string image = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}-{item.Key}.png");

				using Stream stream = File.OpenWrite(image);
				ImageWriter writer = new ImageWriter();
				writer.WritePng(item.Value, textureWidth, textureHeight, ColorComponents.RedGreenBlue, stream);

				if (Settings.ExportPsdFiles) {
					string timelinePsdPath = Path.ChangeExtension(image, ".psd");
					ImageUtil.WritePsd(timelinePsdPath, item.Value, textureWidth, textureHeight);
				}
			}
			Logger.Info("Image file exported");
		}
		catch (Exception e) {
			Logger.Error($"Exporting image failed: {e.Message}");
		}
	}

	private static void ExportRoomAttr(StreamWriter writer, string name, Dictionary<string, RoomAttractiveness> attrs) {
		writer.Write($"Room_Attr: {name}: ");
		foreach (KeyValuePair<string, RoomAttractiveness> attr in attrs) {
			writer.Write(Mods.ExportCreatureName(attr.Key) + "-");
			if (attr.Value != RoomAttractiveness.Default)
				writer.Write(attr.Value.ToString());
			writer.Write(",");
		}
		writer.Write("\n");
	}

	public static void ExportPropertiesFile(string outputPath) {
		Logger.Info("Exporting properties file");

		Backup.File(outputPath);

		using StreamWriter writer = new StreamWriter(outputPath, false);
		writer.Write(WorldWindow.region.extraProperties);

		foreach (string subregion in WorldWindow.region.subregions) {
			writer.WriteLine($"Subregion: {subregion}");
		}

		if (!WorldWindow.region.defaultAttractiveness.IsNullOrEmpty()) {
			ExportRoomAttr(writer, "Default", WorldWindow.region.defaultAttractiveness);
		}

		foreach (Room room in WorldWindow.region.rooms) {
			if (room is OffscreenRoom)
				continue;
			if (room.data.attractiveness.IsNullOrEmpty())
				continue;

			ExportRoomAttr(writer, RoomNameCasing(room.name), room.data.attractiveness);
		}

		foreach (KeyValuePair<int, Color> item in WorldWindow.region.overrideSubregionColors) {
			writer.WriteLine($"//FloodForge|SubregionColorOverride|{item.Key}|{item.Value}");
		}
	}

	public static void ExportDisplayName(string outputPath) {
		Logger.Info("Exporting displayname file");
		
		if (WorldWindow.region.displayName != "") {
			Backup.File(outputPath);

			File.WriteAllLines(outputPath, [WorldWindow.region.displayName]);
		}
	}
}