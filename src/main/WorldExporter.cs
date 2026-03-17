using System.IO;
using StbImageWriteSharp;
using Stride.Core.Extensions;

namespace FloodForge.World;

public static class WorldExporter {
	private static string RoomNameCasing(string name) {
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

	private static string ExportCreatureName(string creatureId) {
		if (CreatureTextures.exportCreatureNames.TryGetValue(creatureId, out string? o)) {
			return o;
		}

		return creatureId;
	}

	public static void ExportMapFile() {
		Logger.Info("Exporting map file");

		string fileName = $"map_{WorldWindow.region.acronym}.txt";
		string path = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, fileName);

		Backup.File(path);

		try {
			using StreamWriter writer = new StreamWriter(path, false);
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

				string line = $"{RoomNameCasing(room.Name)}: " +
							$"{canonPosition.x:G12}><{canonPosition.y:G12}><" +
							$"{devPosition.x:G12}><{devPosition.y:G12}><" +
							$"{room.data.layer}><";

				if (room.data.subregion > -1) {
					line += WorldWindow.region.subregions[room.data.subregion];
				}

				writer.WriteLine(line);
			}

			Logger.Info("- FloodForge Data");
			foreach (Room room in WorldWindow.region.rooms) {
				if (room is OffscreenRoom || !room.data.ExtraFlags)
					continue;

				writer.Write($"//FloodForge;ROOM|{RoomNameCasing(room.Name)}");
				if (room.data.hidden)
					writer.Write("|hidden");
				if (!room.data.merge)
					writer.Write("|nomerge");
				writer.WriteLine();
			}

			Logger.Info("- Connections");
			foreach (Connection connection in WorldWindow.region.connections) {
				if (connection.roomA.data.hidden || connection.roomB.data.hidden)
					continue;

				Vector2i connA = connection.roomA.GetRoomEntranceShortcutPosition(connection.connectionA);
				Vector2i connB = connection.roomB.GetRoomEntranceShortcutPosition(connection.connectionB);

				connA = new Vector2i(connA.x, connection.roomA.height - connA.y - 1);
				connB = new Vector2i(connB.x, connection.roomB.height - connB.y - 1);

				writer.WriteLine($"Connection: " +
					$"{RoomNameCasing(connection.roomA.Name)}," +
					$"{RoomNameCasing(connection.roomB.Name)}," +
					$"{connA.x},{connA.y}," +
					$"{connB.x},{connB.y}," +
					$"{(int) connection.roomA.GetRoomEntranceDirection(connection.connectionA)}," +
					$"{(int) connection.roomB.GetRoomEntranceDirection(connection.connectionB)}");
			}

			writer.Write(WorldWindow.region.extraMap);
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
			connectionId = (int) connection.connectionA;
		}
		else {
			otherRoom = connection.roomA;
			connectionId = (int) connection.connectionB;
		}

		if (otherRoom == null || connectionId == -1)
			return;

		foreach (string timeline in connection.timelines) {
			if (!state.ContainsKey(timeline)) {
				state[timeline] = [.. defaultState];
				timelines.Add(timeline);
			}

			if (connection.timelineType == TimelineType.Only) {
				writer.Write($"{timeline} : {RoomNameCasing(room.Name)} : ");

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
				writer.WriteLine($" : {RoomNameCasing(otherRoom.Name)}");

				if (RoomNameCasing(otherRoom.Name) != state[timeline][connectionId].first) {
					state[timeline][connectionId] = (RoomNameCasing(otherRoom.Name), true);
				}
			}
			else if (connection.timelineType == TimelineType.Except) {
				foreach (string otherTimeline in timelines) {
					if (otherTimeline == timeline)
						continue;
					if (!state[otherTimeline][connectionId].second)
						continue;

					writer.Write($"{otherTimeline} : {RoomNameCasing(room.Name)} : ");
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
					writer.WriteLine($" : {RoomNameCasing(otherRoom.Name)}");
				}

				writer.Write($"{timeline} : {RoomNameCasing(room.Name)} : ");
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
					writer.Write(RoomNameCasing(otherRoom.Name));
				}
				writer.WriteLine($" : {defaultState[connectionId].first}");

				if (RoomNameCasing(otherRoom.Name) != defaultState[connectionId].first) {
					defaultState[connectionId] = (RoomNameCasing(otherRoom.Name), false);
				}
			}
		}
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
				for (int i = 0; i < room.roomShortcutEntrances.Count; i++) {
					defaultState.Add(("DISCONNECTED", false));
				}
				foreach (Connection connection in room.connections) {
					if (connection.timelineType != TimelineType.All)
						continue;

					if (connection.roomA == room) {
						defaultState[(int) connection.connectionA] = (RoomNameCasing(connection.roomB.Name), false);
					} else {
						defaultState[(int) connection.connectionB] = (RoomNameCasing(connection.roomA.Name), false);
					}
				}

				foreach (Connection connection in room.connections) {
					if (connection.timelineType != TimelineType.Except || connection.timelines.Count == 0)
						continue;

					ParseConditionalLinkConnection(writer, room, connection, timelines, state, defaultState);
				}

				foreach (Connection connection in room.connections) {
					if (connection.timelineType != TimelineType.Only || connection.timelines.Count == 0)
						continue;

					ParseConditionalLinkConnection(writer, room, connection, timelines, state, defaultState);
				}

				roomDefaultStates[RoomNameCasing(room.Name)] = defaultState;

				if (room.TimelineType == TimelineType.All || room.Timelines.Count == 0) {
					continue;
				}

				bool first = true;
				foreach (string timeline in room.Timelines) {
					if (!first)
						writer.Write(",");
					first = false;
					writer.Write(timeline);
				}

				writer.Write(" : ");
				writer.Write((room.TimelineType == TimelineType.Only) ? "EXCLUSIVEROOM" : "HIDEROOM");
				writer.WriteLine($" : {RoomNameCasing(room.Name)}");
			}
			writer.WriteLine("END CONDITIONAL LINKS");
			writer.WriteLine();

			Logger.Info("- Rooms");
			writer.WriteLine("ROOMS");
			foreach (Room room in WorldWindow.region.rooms) {
				if (room is OffscreenRoom)
					continue;

				writer.Write($"{RoomNameCasing(room.Name)} : ");

				List<(string, bool)> connections = roomDefaultStates[RoomNameCasing(room.Name)];

				for (int i = 0; i < room.roomShortcutEntrances.Count; i++) {
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

							if (mainCreature.TimelinesMatch(otherCreature)) {
								sameTimelineCreatures.Add(otherCreature);
								nonLineageCreatures[k] = null;
							}
						}

						if (mainCreature.timelineType != TimelineType.All) {
							writer.Write("(");
							if (mainCreature.timelineType == TimelineType.Except) {
								writer.Write("X-");
							}
							bool first2 = true;
							foreach (string timeline in mainCreature.timelines) {
								if (!first2)
									writer.Write(",");
								first2 = false;
								writer.Write(timeline);
							}
							writer.Write(")");
						}

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("OFFSCREEN : ");
						} else {
							writer.Write($"{RoomNameCasing(room.Name)} : ");
						}

						bool first = true;

						foreach (DenLineage creature in sameTimelineCreatures) {
							if (!first)
								writer.Write(", ");
							first = false;

							if (room == WorldWindow.region.offscreenDen) {
								writer.Write($"0-{ExportCreatureName(creature.type)}");
							} else {
								writer.Write($"{i + room.roomShortcutEntrances.Count}-{ExportCreatureName(creature.type)}");
							}
							if (!string.IsNullOrEmpty(creature.tag)) {
								if (creature.tag == "MEAN") {
									writer.Write($"-{{Mean:{creature.data}}}");
								} else if (creature.tag == "LENGTH") {
									if (creature.type == "polemimic") {
										writer.Write($"-{{{ (int)creature.data }}}");
									} else {
										writer.Write($"-{{{creature.data}}}");
									}
								} else if (creature.tag == "SEED") {
									writer.Write($"-{{Seed:{ (int)creature.data }}}");
								} else if (creature.tag == "RotType") {
									writer.Write($"-{{RotType:{ (int)creature.data }}}");
								} else {
									writer.Write($"-{{{creature.tag}}}");
								}
							}
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

						if (lineage.timelineType != TimelineType.All && lineage.timelines.Count > 0) {
							writer.Write("(");
							if (lineage.timelineType == TimelineType.Except) {
								writer.Write("X-");
							}
							bool first = true;
							foreach (string timeline in lineage.timelines) {
								if (!first)
									writer.Write(",");
								first = false;
								writer.Write(timeline);
							}
							writer.Write(")");
						}

						writer.Write("LINEAGE : ");

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("OFFSCREEN : ");
						} else {
							writer.Write($"{RoomNameCasing(room.Name)} : ");
						}

						if (room == WorldWindow.region.offscreenDen) {
							writer.Write("0 : ");
						} else {
							writer.Write($"{i + room.roomShortcutEntrances.Count} : ");
						}

						DenCreature current = creature;
						while (current != null) {
							writer.Write(string.IsNullOrEmpty(current.type) || current.count == 0 ? "NONE" : ExportCreatureName(current.type));

							if (!string.IsNullOrEmpty(current.tag)) {
								if (current.tag == "MEAN") {
									writer.Write($"-{{Mean:{current.data}}}");
								} else if (current.tag == "LENGTH") {
									if (current.type == "polemimic") {
										writer.Write($"-{{{ (int)current.data }}}");
									} else {
										writer.Write($"-{{{current.data}}}");
									}
								} else if (current.tag == "SEED") {
									writer.Write($"-{{Seed:{ (int)current.data }}}");
								} else if (current.tag == "RotType") {
									writer.Write($"-{{RotType:{ (int)current.data }}}");
								} else {
									writer.Write($"-{{{current.tag}}}");
								}
							}

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
					if (worm.timelineType != TimelineType.All) {
						writer.Write("(");
						if (worm.timelineType == TimelineType.Except) {
							writer.Write("X-");
						}
						bool first = true;
						foreach (string timeline in worm.timelines) {
							if (!first)
								writer.Write(",");
							first = false;
							writer.Write(timeline);
						}
						writer.Write(")");
					}

					writer.Write($"{RoomNameCasing(room.Name)} : {room.GarbageWormDenIndex}-{ExportCreatureName(worm.type)}");
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

		string mapPath = PathUtil.FindOrAssumeFile(WorldWindow.region.exportPath, $"map_image_{WorldWindow.region.acronym}.txt");
		Backup.File(mapPath);

		StreamWriter? mapFile = null;

		try {
			mapFile = new StreamWriter(mapPath, false);
		}
		catch (Exception) {
			Logger.Info($"Error creating map_image_{WorldWindow.region.acronym}.txt");
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

		int layerHeight = (int) (bottomRight.y - topLeft.y) + 20;
		int textureWidth = (int) (bottomRight.x - topLeft.x) + 20;
		int textureHeight = layerHeight * 3;

		byte[] imageData = new byte[textureWidth * textureHeight * 3];

		bool debugPadding = Settings.DEBUGVisibleOutputPadding;

		for (int y = 0; y < textureHeight; y++) {
			for (int x = 0; x < textureWidth; x++) {
				int i = (y * textureWidth + x) * 3;
				if (debugPadding) {
					if (x < 10 || (y % layerHeight) < 10 || x >= textureWidth - 10 || (y % layerHeight) >= layerHeight - 10) {
						imageData[i] = 0;
						imageData[i + 1] = 255;
						imageData[i + 2] = 255;
					}
					else {
						imageData[i] = 0;
						imageData[i + 1] = 255;
						imageData[i + 2] = 0;
					}
				}
				else {
					imageData[i] = 0;
					imageData[i + 1] = 255;
					imageData[i + 2] = 0;
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
			int layerYOffset = (2 - room.data.layer) * layerHeight + 10;

			mapFile?.WriteLine($"{RoomNameCasing(room.Name)}: {roomPosition.x + layerXOffset},{roomPosition.y + layerYOffset},{room.width},{room.height}");

			for (int ox = 0; ox < room.width; ox++) {
				for (int oy = 0; oy < room.height; oy++) {
					int targetX = roomPosition.x + ox + layerXOffset;
					int targetY = roomPosition.y + oy + layerYOffset;

					if (targetX < 0 || targetX >= textureWidth || targetY < 0 || targetY >= textureHeight)
						continue;

					int i = (targetY * textureWidth + targetX) * 3;
					uint tile = room.GetTile(ox, oy);
					uint tileType = tile % 16;
					uint tileData = tile / 16;

					byte r = 0, g = 0, b = 0;

					if (tileType == 0 || tileType == 4 || tileType == 5) {
						r = 255;
						g = 0;
					}
					else if (tileType == 1) {
						r = 0;
						g = 0;
					}
					else if (tileType == 2 || tileType == 3 || (tileData & 1) != 0 || (tileData & 2) != 0) {
						r = 153;
						g = 0;
					}

					if (r > 0 && oy >= room.height - room.data.waterHeight) {
						b = 255;
					}

					bool isBlack = r == 0 && g == 0 && b == 0;
					bool pixelIsGreen = imageData[i] == 0 && imageData[i + 2] == 0;

					if (!room.data.merge || !isBlack || pixelIsGreen) {
						imageData[i] = r;
						imageData[i + 1] = g;
						imageData[i + 2] = b;
					}
				}
			}
		}

		mapFile?.Dispose();

		Backup.File(outputPath);
		try {
			using Stream stream = File.OpenWrite(outputPath);
			ImageWriter writer = new ImageWriter();
			writer.WritePng(imageData, textureWidth, textureHeight, ColorComponents.RedGreenBlue, stream);
			Logger.Info("Image file exported");
		}
		catch (Exception e) {
			Logger.Error($"Exporting image failed: {e.Message}");
		}
	}

	private static void ExportRoomAttr(StreamWriter writer, string name, Dictionary<string, RoomAttractiveness> attrs) {
		writer.Write($"Room_Attr: {name}: ");
		foreach (KeyValuePair<string, RoomAttractiveness> attr in attrs) {
			writer.Write(ExportCreatureName(attr.Key) + "-");
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

			ExportRoomAttr(writer, RoomNameCasing(room.Name), room.data.attractiveness);
		}

		foreach (var item in WorldWindow.region.overrideSubregionColors) {
			writer.WriteLine($"//FloodForge|SubregionColorOverride|{item.Key}|{item.Value}");
		}
	}
}