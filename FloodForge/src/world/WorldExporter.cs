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

	private static void ParseConditionalLinkConnection(TextWriter writer, Room room, Connection connection, List<string> timelines, Dictionary<string, List<(Room? first, bool second)>> state, List<(Room? first, bool second)> defaultState) {
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

		string stringifiedConditions = "";
		bool first = true;
		foreach (string condition in connection.preProcessorConditions) {
			if (!first)
				stringifiedConditions += ',';
			first = false;
			stringifiedConditions += condition;
		}
		if (!string.IsNullOrEmpty(stringifiedConditions))
			stringifiedConditions = $"{{{stringifiedConditions}}}";

		foreach (string timeline in connection.timeline.timelines) {
			if (!state.ContainsKey(timeline)) {
				state[timeline] = [.. defaultState];
				timelines.Add(timeline);
			}

			if (connection.timeline.timelineType == TimelineType.Only) {
				writer.Write($"{stringifiedConditions}{timeline} : {RoomNameCasing(room.name)} : ");

				if (state[timeline][connectionId].first == null) {
					int disconnectedBefore = 0;
					for (int i = 0; i < connectionId; i++) {
						if (defaultState[i].first == null)
							disconnectedBefore++;
					}
					writer.Write(disconnectedBefore + 1);
				}
				else {
					writer.Write(state[timeline][connectionId].first);
				}
				writer.WriteLine($" : {RoomNameCasing(otherRoom.name)}");

				if (otherRoom != state[timeline][connectionId].first) {
					state[timeline][connectionId] = (otherRoom, true);
				}
			}
			else if (connection.timeline.timelineType == TimelineType.Except) {
				foreach (string otherTimeline in timelines) {
					if (otherTimeline == timeline) {
						continue;
					}
					if (!state[otherTimeline][connectionId].second) {
						continue;
					}

					writer.Write($"{stringifiedConditions}{otherTimeline} : {RoomNameCasing(room.name)} : ");
					if (state[otherTimeline][connectionId].first == null) {
						int disconnectedBefore = 0;
						for (int i = 0; i < connectionId; i++) {
							if (state[otherTimeline][i].first == null)
								disconnectedBefore++;
						}
						writer.Write(disconnectedBefore + 1);
					}
					else {
						writer.Write(state[otherTimeline][connectionId].first);
					}
					writer.WriteLine($" : {RoomNameCasing(otherRoom.name)}");
				}

				writer.Write($"{stringifiedConditions}{timeline} : {RoomNameCasing(room.name)} : ");
				if (state[timeline][connectionId].second) {
					if (state[timeline][connectionId].first == null) {
						int disconnectedBefore = 0;
						for (int i = 0; i < connectionId; i++) {
							if (state[timeline][i].first == null)
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
				writer.WriteLine($" : {(defaultState[connectionId].first == null ? "DISCONNECTED" : defaultState[connectionId].first)}");

				if (otherRoom != defaultState[connectionId].first) {
					defaultState[connectionId] = (otherRoom, false);
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

			Dictionary<string, List<(Room?, bool)>> roomDefaultStates = [];

			Logger.Info("- Conditional Links");
			StringBuilder conditionalLinksBuffer = new StringBuilder();
			using (StreamWriter tempWriter = new StreamWriter(new MemoryStream(), Encoding.UTF8, 1024, leaveOpen: true))
			using (StringWriter stringWriter = new StringWriter(conditionalLinksBuffer)) {
				Dictionary<string, List<(Room?, bool)>> tempStates = [];

				foreach (Room room in WorldWindow.region.rooms) {
					if (room is OffscreenRoom)
						continue;

					List<string> timelines = [];
					Dictionary<string, List<(Room?, bool)>> state = [];
					List<(Room?, bool)> defaultState = [];
					for (int i = 0; i < room.roomExits.Count; i++) {
						defaultState.Add((null, false));
					}

					foreach (Connection connection in room.connections) {
						if (connection.timeline.timelineType != TimelineType.All)
							continue;

						if (connection.roomA == room) {
							defaultState[(int) connection.roomAExitID] = (connection.roomB, false);
						}
						else {
							defaultState[(int) connection.roomBExitID] = (connection.roomA, false);
						}
					}

					foreach (Connection connection in room.connections) {
						if (connection.timeline.timelineType != TimelineType.Except || connection.timeline.timelines.Count == 0)
							continue;

						ParseConditionalLinkConnection(stringWriter, room, connection, timelines, state, defaultState);
					}

					foreach (Connection connection in room.connections) {
						if (connection.timeline.timelineType != TimelineType.Only || connection.timeline.timelines.Count == 0)
							continue;

						ParseConditionalLinkConnection(stringWriter, room, connection, timelines, state, defaultState);
					}

					foreach (Connection connection in room.connections) {
						if (connection.timeline.timelineType != TimelineType.All)
							continue;
						
						ParseConditionalLinkConnection(stringWriter, room, connection, timelines, state, defaultState);
					}

					roomDefaultStates[RoomNameCasing(room.name)] = defaultState;

					if ((room.timeline.timelineType == TimelineType.All || room.timeline.timelines.Count == 0) && room.preProcessorConditions.Length == 0) {
						continue;
					}

					Timeline virtualTimeline = room.timeline;

					foreach (Room replacingRoom in room.replacingRooms) {
						Timeline resultingTimeline = replacingRoom.timeline.Inverted().And(room.timeline.Inverted()).Inverted(); // this is cursed but should work
						virtualTimeline = resultingTimeline;
					}

					if ((virtualTimeline.timelineType == TimelineType.All || virtualTimeline.timelines.Count == 0) && room.preProcessorConditions.Length == 0) {
						continue;
					}

					if (room.preProcessorConditions.Length != 0) {
						stringWriter.Write("{");
						bool first1 = true;
						foreach (string preProcessor in room.preProcessorConditions) {
							if (!first1)
								stringWriter.Write(",");
							first1 = false;
							stringWriter.Write(preProcessor);
						}
						stringWriter.Write("}");
					}

					if (virtualTimeline.timelineType == TimelineType.All || virtualTimeline.timelines.Count == 0) {
						continue;
					}

					bool first = true;
					foreach (string timeline in virtualTimeline.timelines) {
						if (!first)
							stringWriter.Write(",");
						first = false;
						stringWriter.Write(timeline);
					}

					stringWriter.Write(" : ");
					if (room.replacedRoom != null)
						stringWriter.Write($"REPLACEROOM : {room.replacedRoom.name}");
					else
						stringWriter.Write((virtualTimeline.timelineType == TimelineType.Only) ? "EXCLUSIVEROOM" : "HIDEROOM");
					stringWriter.WriteLine($" : {RoomNameCasing(room.name)}");
				}
			}

			if (conditionalLinksBuffer.Length > 0) {
				writer.WriteLine("CONDITIONAL LINKS");
				writer.Write(conditionalLinksBuffer.ToString());
				writer.WriteLine("END CONDITIONAL LINKS");
				writer.WriteLine();
			}

			Logger.Info("- Rooms");
			writer.WriteLine("ROOMS");

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
					writer.WriteLine();
				}
				
				isFirstRoom = false;
				wasGate = isGate;
				lastSubregion = room.data.subregion;

				writer.Write($"{FancyRoomCasing(room)} : ");

				List<(Room?, bool)> connections = roomDefaultStates[RoomNameCasing(room.name)];

				for (int i = 0; i < room.roomExits.Count; i++) {
					if (i > 0) writer.Write(", ");

					writer.Write(connections[i].Item1 == null ? "DISCONNECTED" : FancyRoomCasing(connections[i].Item1!));
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
							writer.Write(text);
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
							writer.Write(text);
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

					if (worm.preProcessorConditions.Length != 0) {
						writer.Write("{");
						bool first = true;
						foreach (string preProcessor in worm.preProcessorConditions) {
							if (!first)
								writer.Write(",");
							first = false;
							writer.Write(preProcessor);
						}
						writer.Write("}");
					}

					writer.Write($"{RoomNameCasing(room.name)} : {room.GarbageWormDenIndex}-{Mods.ExportCreatureName(worm.type)}");
					if (worm.count > 1)
						writer.Write($"-{worm.count}");
					writer.WriteLine();
				}
			}

			writer.Write(WorldWindow.region.extraWorldCreatures);
			writer.WriteLine("END CREATURES");

			Logger.Info("- Bat migration blockages");

			IOrderedEnumerable<Room> sortedMigrationRooms = WorldWindow.region.rooms
				.Where(room => room is not OffscreenRoom && room.data.blockedBatMigration)
				.OrderBy(room => room.data.tags.Contains("GATE") ? 0 : 1)
				.ThenBy(room => room.data.subregion)
				.ThenBy(room => room.data.tags.Contains("SHELTER") ? 0 : 1)
				.ThenBy(room => room.data.cameras.Count)
				.ThenBy(room => room.name, StringComparer.OrdinalIgnoreCase);

			if (sortedMigrationRooms.Any()) {
				writer.WriteLine();
				writer.WriteLine("BAT MIGRATION BLOCKAGES");
				foreach (Room room in sortedMigrationRooms) {
					writer.WriteLine($"{FancyRoomCasing(room)}");
				}
				writer.WriteLine("END BAT MIGRATION BLOCKAGES");
			}

			writer.Write(WorldWindow.region.extraWorld);
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

		Backup.File(outputPath);

		File.WriteAllLines(outputPath, [WorldWindow.region.displayName]);
	}
}