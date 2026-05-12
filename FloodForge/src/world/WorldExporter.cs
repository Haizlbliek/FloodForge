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

				string line = $"{RoomNameCasing(room.name)}: " +
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
				if (room.data.hidden)
					writer.Write("|hidden");
				if (!room.data.merge)
					writer.Write("|nomerge");
				writer.WriteLine(); // floodforge notes can stay in one map file for ease of import.
			}

			Logger.Info("- Connections");
			foreach (Connection connection in WorldWindow.region.connections) {
				if (connection.roomA.data.hidden || connection.roomB.data.hidden)
					continue;

				Vector2i connA = connection.roomA.GetShortcutEntranceRoomPoint(connection.roomAExitID);
				Vector2i connB = connection.roomB.GetShortcutEntranceRoomPoint(connection.roomBExitID);

				connA = new Vector2i(connA.x, connection.roomA.height - connA.y - 1);
				connB = new Vector2i(connB.x, connection.roomB.height - connB.y - 1);

				string line = $"Connection: " +
					$"{RoomNameCasing(connection.roomA.name)}," +
					$"{RoomNameCasing(connection.roomB.name)}," +
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

	private static void ParseConditionalLinkConnection(StreamWriter writer, Room room, Connection connection, List<string> timelines, Dictionary<string, List<(string first, bool second)>> state, List<(string first, bool second)> defaultState) {
		Room? otherRoom;
		int connectionId;

		if (connection.roomA == room) {
			otherRoom = connection.roomB;
			connectionId = (int) connection.roomAExitID;
		}
		else {
			otherRoom = connection.roomA;
			connectionId = (int) connection.roomBExitID;
		}

		if (otherRoom == null || connectionId == -1)
			return;

		foreach (string timeline in connection.timeline.timelines) {
			if (!state.ContainsKey(timeline)) {
				state[timeline] = [.. defaultState];
				timelines.Add(timeline);
			}

			// REVIEW - index is not exported correctly
			if (connection.timeline.timelineType == TimelineType.Only) {
				writer.Write($"{timeline} : {RoomNameCasing(room.name)} : ");

				if (state[timeline][connectionId].first == "DISCONNECTED") {
					int disconnectedBefore = 0;
					for (int i = 0; i < connectionId; i++) {
						if (state[timeline][i].first == "DISCONNECTED")
							disconnectedBefore++;
					}
					writer.Write(disconnectedBefore + 1);
				}
				else {
					writer.Write(state[timeline][connectionId].first);
				}
				writer.WriteLine($" : {RoomNameCasing(otherRoom.name)}");

				if (RoomNameCasing(otherRoom.name) != state[timeline][connectionId].first) {
					state[timeline][connectionId] = (RoomNameCasing(otherRoom.name), true);
				}
			}
			else if (connection.timeline.timelineType == TimelineType.Except) {
				foreach (string otherTimeline in timelines) {
					if (otherTimeline == timeline)
						continue;
					if (!state[otherTimeline][connectionId].second)
						continue;

					writer.Write($"{otherTimeline} : {RoomNameCasing(room.name)} : ");
					if (state[otherTimeline][connectionId].first == "DISCONNECTED") {
						int disconnectedBefore = 0;
						for (int i = 0; i < connectionId; i++) {
							if (state[otherTimeline][i].first == "DISCONNECTED")
								disconnectedBefore++;
						}
						writer.Write(disconnectedBefore + 1);
					}
					else {
						writer.Write(state[otherTimeline][connectionId].first);
					}
					writer.WriteLine($" : {RoomNameCasing(otherRoom.name)}");
				}

				writer.Write($"{timeline} : {RoomNameCasing(room.name)} : ");
				if (state[timeline][connectionId].second) {
					if (state[timeline][connectionId].first == "DISCONNECTED") {
						int disconnectedBefore = 0;
						for (int i = 0; i < connectionId; i++) {
							if (state[timeline][i].first == "DISCONNECTED")
								disconnectedBefore++;
						}
						writer.Write(disconnectedBefore + 1);
					}
					else {
						writer.Write(state[timeline][connectionId].first);
					}
				}
				else {
					writer.Write(RoomNameCasing(otherRoom.name));
				}
				writer.WriteLine($" : {defaultState[connectionId].first}");

				if (RoomNameCasing(otherRoom.name) != defaultState[connectionId].first) {
					defaultState[connectionId] = (RoomNameCasing(otherRoom.name), false);
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
				string name = Mods.ExportTagName(tag.id.id);
				if (tag.id == Mods.tags["polemimic_length"] || tag.id == Mods.tags["centipede_length"]) name = "Mean";
				writer.Write($"{name}:{(tag is DenCreature.IntegerTag intTag ? intTag.data : (tag is DenCreature.FloatTag floatTag ? floatTag.data : (tag is DenCreature.StringTag stringTag ? stringTag.data : "IDK LOL")))}");
			}
			// if (tag.id == CreatureTags.Mean) {
			// 	writer.Write($"Mean:{((DenCreature.FloatTag) tag).data}");
			// }
			// else if (tag.id == CreatureTags.POLEMIMIC_LENGTH) {
			// 	writer.Write($"{((DenCreature.IntegerTag) tag).data}");
			// }
			// else if (tag.id == CreatureTags.CENTIPEDE_LENGTH) {
			// 	writer.Write($"{((DenCreature.FloatTag) tag).data}");
			// }
			// else if (tag.id == CreatureTags.Seed) {
			// 	writer.Write($"Seed:{((DenCreature.IntegerTag) tag).data}");
			// }
			// else if (tag.id == CreatureTags.RotType) {
			// 	writer.Write($"RotType:{((DenCreature.IntegerTag) tag).data}");
			// }
			// else if (tag.id == CreatureTags.NamedAttr) {
			// 	writer.Write($"NamedAttr:{((DenCreature.StringTag) tag).data}");
			// }
			// else {
			// 	writer.Write($"{tag.id.id}");
			// }
		}
		writer.Write("}");
	}

	public static void ExportWorldFile() {
		Logger.Info("Exporting world file");

		string fileName = $"world_{WorldWindow.region.acronym}.txt";
		string path = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, fileName);
		Backup.File(path);

		try {
			using StreamWriter writer = new StreamWriter(path, false);

			Dictionary<string, List<(string, bool)>> roomDefaultStates = [];

			Logger.Info("- Conditional Links");
			writer.WriteLine("CONDITIONAL LINKS");
			foreach (Room room in WorldWindow.region.rooms) {
				if (room is OffscreenRoom)
					continue;

				List<string> timelines = [];
				Dictionary<string, List<(string, bool)>> state = [];
				List<(string, bool)> defaultState = [];
				for (int i = 0; i < room.roomExits.Count; i++) {
					defaultState.Add(("DISCONNECTED", false));
				}
				foreach (Connection connection in room.connections) {
					if (connection.timeline.timelineType != TimelineType.All)
						continue;

					if (connection.roomA == room) {
						defaultState[(int) connection.roomAExitID] = (RoomNameCasing(connection.roomB.name), false);
					}
					else {
						defaultState[(int) connection.roomBExitID] = (RoomNameCasing(connection.roomA.name), false);
					}
				}

				foreach (Connection connection in room.connections) {
					if (connection.timeline.timelineType != TimelineType.Except || connection.timeline.timelines.Count == 0)
						continue;

					ParseConditionalLinkConnection(writer, room, connection, timelines, state, defaultState);
				}

				foreach (Connection connection in room.connections) {
					if (connection.timeline.timelineType != TimelineType.Only || connection.timeline.timelines.Count == 0)
						continue;

					ParseConditionalLinkConnection(writer, room, connection, timelines, state, defaultState);
				}

				roomDefaultStates[RoomNameCasing(room.name)] = defaultState;

				if (room.timeline.timelineType == TimelineType.All || room.timeline.timelines.Count == 0) {
					continue;
				}

				bool first = true;
				foreach (string timeline in room.timeline.timelines) {
					if (!first)
						writer.Write(",");
					first = false;
					writer.Write(timeline);
				}

				writer.Write(" : ");
				writer.Write((room.timeline.timelineType == TimelineType.Only) ? "EXCLUSIVEROOM" : "HIDEROOM");
				writer.WriteLine($" : {RoomNameCasing(room.name)}");
			}
			writer.WriteLine("END CONDITIONAL LINKS");
			writer.WriteLine();

			Logger.Info("- Rooms");
			writer.WriteLine("ROOMS");
			foreach (Room room in WorldWindow.region.rooms) {
				if (room is OffscreenRoom)
					continue;

				writer.Write($"{RoomNameCasing(room.name)} : ");
				// Logger.Info($"{RoomNameCasing(room.name)} : ");

				// Logger.Info($"roomDefaultStates.Count: {roomDefaultStates};");
				List<(string, bool)> connections = roomDefaultStates[RoomNameCasing(room.name)];

				for (int i = 0; i < room.roomExits.Count; i++) {
					if (i > 0) writer.Write(", ");

					writer.Write(connections[i].Item1);
				}

				foreach (string tag in room.data.tags) {
					writer.Write($" : {tag}");
				}

				writer.WriteLine();
			}
			writer.WriteLine("END ROOMS");
			writer.WriteLine();

			Logger.Info("- Creatures");
			writer.WriteLine("CREATURES");

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
							writer.Write("(");
							writer.Write(mainCreature.timeline.ToString());
							writer.Write(")");
						}

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("OFFSCREEN : ");
						}
						else {
							writer.Write($"{RoomNameCasing(room.name)} : ");
						}

						bool first = true;

						foreach (DenLineage creature in sameTimelineCreatures) {
							if (!first)
								writer.Write(", ");
							first = false;

							if (room == WorldWindow.region.offscreenDen) {
								writer.Write($"0-{Mods.ExportCreatureName(creature.type)}");
							}
							else {
								writer.Write($"{i + room.nonDenExitCount}-{Mods.ExportCreatureName(creature.type)}");
							}
							ExportCreatureTags(creature, writer);
							if (creature.count > 1)
								writer.Write($"-{creature.count}");
						}

						writer.WriteLine();
					}
				}

				for (int i = 0; i < room.dens.Count; i++) {
					Den den = room.GetDen01(i);
					foreach (DenLineage lineage in den.creatures) {
						DenCreature creature = lineage;

						if (creature.lineageTo == null)
							continue;

						if (lineage.timeline.timelineType != TimelineType.All && lineage.timeline.timelines.Count > 0) {
							writer.Write("(");
							writer.Write(lineage.timeline);
							writer.Write(")");
						}

						writer.Write("LINEAGE : ");

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("OFFSCREEN : ");
						}
						else {
							writer.Write($"{RoomNameCasing(room.name)} : ");
						}

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("0 : ");
						}
						else {
							writer.Write($"{i + room.nonDenExitCount} : ");
						}

						DenCreature current = creature;
						while (current != null) {
							writer.Write(string.IsNullOrEmpty(current.type) || current.count == 0 ? "NONE" : Mods.ExportCreatureName(current.type));

							ExportCreatureTags(current, writer);

							if (current.lineageTo == null) {
								writer.WriteLine("-0");
								break;
							}
							writer.Write($"-{Math.Clamp(current.lineageChance, 0.0f, 1.0f)}, ");

							current = current.lineageTo;
						}
					}
				}

				if (room == WorldWindow.region.offscreenDen)
					continue;

				foreach (GarbageWormDen worm in room.garbageWormDens) {
					if (worm.timeline.timelineType != TimelineType.All) {
						writer.Write("(");
						writer.Write(worm.timeline);
						writer.Write(")");
					}

					writer.Write($"{RoomNameCasing(room.name)} : {room.GarbageWormDenIndex}-{Mods.ExportCreatureName(worm.type)}");
					if (worm.count > 1)
						writer.Write($"-{worm.count}");
					writer.WriteLine();
				}
			}

			writer.Write(WorldWindow.region.extraWorldCreatures);
			writer.WriteLine("END CREATURES");
			writer.Write(WorldWindow.region.extraWorld);
		}
		catch (Exception exception) {
			Logger.Info($"Error opening world_{WorldWindow.region.acronym}.txt");
			Logger.Info($"> {exception}");
		}
	}

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
			if (room is OffscreenRoom || room.data.hidden)
				continue;

			Vector2i roomPosition = new Vector2i(
				(int) (room.CanonPosition.x - topLeft.x),
				(int) (bottomRight.y - room.CanonPosition.y)
			);

			int layerXOffset = 10;
			int layerYOffset = (2 - room.data.layer) * layerHeight - 10;

			if (room.timeline.timelineType != TimelineType.Only) { // only rooms set to "only" don't appear in the default map
				mapFile?.WriteLine($"{RoomNameCasing(room.name)}: {roomPosition.x + layerXOffset},{roomPosition.y + layerYOffset - 0.5f},{room.width},{room.height}");
			}
			foreach (KeyValuePair<string, StreamWriter?> timelineWriter in timelineMapFiles) {
				if (room.timeline.OverlapsWith(timelineWriter.Key)) {
					timelineWriter.Value?.WriteLine($"{RoomNameCasing(room.name)}: {roomPosition.x + layerXOffset},{roomPosition.y + layerYOffset},{room.width},{room.height}");
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
			{ using Stream stream = File.OpenWrite(outputPath);
				ImageWriter writer = new ImageWriter();
				writer.WritePng(imageData, textureWidth, textureHeight, ColorComponents.RedGreenBlue, stream);
			}
			foreach (KeyValuePair<string, byte[]> item in timelineImageData) {
				string image = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_{WorldWindow.region.acronym}-{item.Key}.png");
				using Stream stream = File.OpenWrite(image);
				ImageWriter writer = new ImageWriter();
				writer.WritePng(item.Value, textureWidth, textureHeight, ColorComponents.RedGreenBlue, stream);
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
}