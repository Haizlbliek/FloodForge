using Stride.Core;
using Stride.Core.Extensions;
using FloodForge.Popups;

namespace FloodForge.World;

public static class NameChanger {
	public static void ChangeRoomName(Room room, string newName) {
		
	}
	
	public static void ChangeRegionAcronym(string newAcronym, bool deleteSourceFiles, bool deleteExistingFiles) {
		string oldAcronym = WorldWindow.region.acronym;
		Logger.Info($"Initiation acronymChange - {oldAcronym} -> {newAcronym} ; {(deleteSourceFiles ? "replace" : "copy")}");
		//Logger.Info("getting new paths");
		string newExportPath = WorldWindow.region.roomsPath[..(WorldWindow.region.roomsPath.IndexOfReverse('\\') + 1)] + newAcronym;
		string newRoomsPath = newExportPath + "-rooms";
		//Logger.Info("newExportPath = " + newExportPath);
		//Logger.Info("newRoomsPath = " + newRoomsPath);

        if (deleteExistingFiles) {
            Logger.Info("Deleting existing files.");
            if (Directory.Exists(newExportPath)) Directory.Delete(newExportPath, true);
            if (Directory.Exists(newRoomsPath)) Directory.Delete(newRoomsPath, true);
        }

		List<(string from, string to)> roomFilesToTransfer = [];
		Dictionary<string, string> nameConversions = [];
		List<(string from, string to)> roomsSubDirectoriesToTransfer = [];
		Logger.Info($"Analysing {oldAcronym}-rooms");

		foreach (Room room in WorldWindow.region.rooms) {
			string oldName = room.name;
			string newName = oldName;
			//Logger.Info($"checking room {oldName}");
			string[] splitName = oldName.Split('_');
			if (splitName[0].Equals(oldAcronym, StringComparison.InvariantCultureIgnoreCase)) {
				newName = newAcronym + oldName[oldName.IndexOf('_')..];
				//Logger.Info($"regular -> {newName}");
			}
			else if (splitName[0].Equals("GATE", StringComparison.InvariantCultureIgnoreCase)) {
				newName = "GATE_";
				if (splitName[1].Equals(oldAcronym, StringComparison.InvariantCultureIgnoreCase))
					newName += $"{newAcronym}_{splitName[2]}";
				else if (splitName[2].Equals(oldAcronym, StringComparison.InvariantCultureIgnoreCase))
					newName += $"{splitName[1]}_{newAcronym}";
				//Logger.Info($"GATE -> {newName}");
			}
			else if (splitName[0].Equals("OffscreenDen" + oldAcronym, StringComparison.InvariantCultureIgnoreCase)) {
				newName = "OffscreenDen" + newAcronym;
				//Logger.Info($"OffscreenDen -> {newName}");
			}
			else {
				//Logger.Info($"UNKNOWN -> {newName}");
			}
			room.name = newName;
			nameConversions.Add(oldName, newName);
			if (room is not OffscreenRoom) {
				int roomFinalBackslash = room.path.IndexOfReverse('\\');
				if (room.path[(roomFinalBackslash + 1)..].Equals(oldName + ".txt", StringComparison.InvariantCultureIgnoreCase)) {
					string roomPath = room.path[..roomFinalBackslash];
					if (roomPath.Equals(WorldWindow.region.roomsPath, StringComparison.InvariantCultureIgnoreCase))
						roomPath = newRoomsPath;
					room.path = roomPath + '\\' + newName;
					//Logger.Info($"new Path: {room.path}");
				}
				else {
					//Logger.Info($"Same Path: {room.path}");
				}
            }
        }

		string[] roomsOriginalFiles = Directory.Exists(WorldWindow.region.roomsPath) ? Directory.GetFileSystemEntries(WorldWindow.region.roomsPath) : [];

		bool encounteredAtypicalFiles = false;

        //Logger.Info($"Checking originalFiles");
        foreach (string path in roomsOriginalFiles) {
            int pathFinalBackslash = path.IndexOfReverse('\\');
            string fileName = path[(pathFinalBackslash + 1)..];
            //Logger.Info($" - checking file {fileName}");
            string[] splitFileName = fileName.Split('_');
            if (splitFileName[0] == oldAcronym) {
                //Logger.Info($" -> starts with {oldAcronym}");
                string newName = newAcronym + fileName[splitFileName[0].Length..];
                roomFilesToTransfer.Add((path, newRoomsPath + '\\' + newName));
                //Logger.Info($" <- added");
            }
            else {
                encounteredAtypicalFiles = true;
                if (Directory.Exists(path)) {
                    roomsSubDirectoriesToTransfer.Add((path, path.Replace(WorldWindow.region.roomsPath, newRoomsPath, StringComparison.InvariantCultureIgnoreCase)));
                    //Logger.Info($" <- directory added");
                }
                else {
                    roomFilesToTransfer.Add((path, newRoomsPath + '\\' + fileName));
                    //Logger.Info($" <- added");
                }
            }
        }        

		Logger.Info($"Analysing {oldAcronym}");
		string[] worldOriginalFiles = Directory.Exists(WorldWindow.region.exportPath) ? Directory.GetFileSystemEntries(WorldWindow.region.exportPath) : [];
		List<(string from, string to, int type)> worldFilesToTransfer = [];
		List<(string from, string to)> worldSubDirectoriesToTransfer = [];
		foreach (string file in worldOriginalFiles) {
			//Logger.Info($"Checking file {file}");
			if (file.Equals(WorldWindow.region.exportPath + '\\' + "properties.txt", StringComparison.InvariantCultureIgnoreCase)) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "properties.txt", 0));
				//Logger.Info($" <- add 0");
			}
			else if (file == (WorldWindow.region.exportPath + '\\' + "map_" + oldAcronym + ".txt")) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "map_" + newAcronym + ".txt", 1));
				//Logger.Info($" <- add 1");
			}
			else if (file == (WorldWindow.region.exportPath + '\\' + "map_image_" + oldAcronym + ".txt")) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "map_image_" + newAcronym + ".txt", 2));
				//Logger.Info($" <- add 2");
			}
			else if (file == (WorldWindow.region.exportPath + '\\' + "map_" + oldAcronym + ".png")) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "map_" + newAcronym + ".png", 3));
				//Logger.Info($" <- add 3");
			}
			else if (file == (WorldWindow.region.exportPath + '\\' + "world_" + oldAcronym + ".txt")) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "world_" + newAcronym + ".txt", 4));
				//Logger.Info($" <- add 4");
			}
			else if (file == (WorldWindow.region.exportPath + '\\' + "displayname.txt")) {
				worldFilesToTransfer.Add((file, newExportPath + '\\' + "displayname.txt", 5));
				//Logger.Info($" <- add 5");
			}
			else {
				encounteredAtypicalFiles = true;
				if (Directory.Exists(file)) {
					worldSubDirectoriesToTransfer.Add((file, file.Replace(WorldWindow.region.exportPath, newExportPath)));
					//Logger.Info($" <- add directory");
				}
				else {
					worldFilesToTransfer.Add((file, file.Replace(WorldWindow.region.exportPath, newExportPath), -1));
					//Logger.Info($" <- add -1");
				}
			}
		}

		Logger.Info($"Creating {newAcronym} folder!");
		Directory.CreateDirectory(newExportPath);

		Logger.Info($"! Applying files in worldFilesToTransfer");
		foreach ((string from, string to, int type) in worldFilesToTransfer) {
			//Logger.Info($"- type: {type}");
			switch (type) {
				case 0:
					// properties.txt
					string[] propertiesFile = File.ReadAllLines(from);
					List<string> newPropertiesFile = [];
					foreach (string line in propertiesFile) {
						if (line.StartsWith("Room_Attr: ")) {
							string[] splitLine = line.Split(':');
							if(splitLine.Length > 0) {
								if (nameConversions.TryGetValue(splitLine[1].Trim(), out string? newName)) {
									string newLine = "Room_Attr: " + newName + ':' + splitLine[2];
									newPropertiesFile.Add(newLine);
									//Logger.Info($"- converted line {line} to {"Room_Attr: " + newName + ':' + splitLine[2]}");
									continue;
								}
							}
						}
						newPropertiesFile.Add(line);
					}
					//Logger.Info($" <- Writing new file to {to}");
					File.WriteAllLines(to, newPropertiesFile);
				break;
				case 1:
					// map_XX.txt
					string[] mapRegionFile = File.ReadAllLines(from);
					List<string> newMapAcronymFile = [];
					foreach (string line in mapRegionFile) {
						if (line.StartsWith("//")) {
							string data = line.Split(';')[^1];
							string[] splitData = data.Split('|');
							string[] newData = new string[splitData.Length];
							for (int i = 0; i < splitData.Length; i++) {
								if (nameConversions.TryGetValue(splitData[i], out string? newName))
									newData[i] = newName;
								else
									newData[i] = splitData[i];
							}
						}
						else if (line.StartsWith("Connection: ")) {
							string[] splitConnectionData = line["Connection: ".Length..].Split(',');
							if (nameConversions.TryGetValue(splitConnectionData[0], out string? newName1))
								splitConnectionData[0] = newName1;
							else if (nameConversions.TryGetValue(splitConnectionData[1], out string? newName2))
								splitConnectionData[1] = newName2;
							string newData = "";
							foreach (string item in splitConnectionData) {
								newData += (newData != "" ? "," : "") + item;
							}
							newMapAcronymFile.Add("Connection: " + newData);
							//Logger.Info($"- converted line {line} to {"Connection: " + newData}");
						}
						else {
							string[] item = line.Split(':');
							if (item.Length > 1) {
								if(nameConversions.TryGetValue(item[0], out string? newName)) {
									newMapAcronymFile.Add(newName + ':' + item[1]);
									//Logger.Info($"- converted line {line} to {newName + item[1]}");
									continue;
								}
							}
						}
						newMapAcronymFile.Add(line);
					}
					//Logger.Info($" <- Writing new file to {to}");
					File.WriteAllLines(to, newMapAcronymFile);
				break;
				case 2:
					// map_image_XX.txt
					string[] mapImageAcronymFile = File.ReadAllLines(from);
					List<string> newMapImageAcronymFile = [];
					foreach (string line in mapImageAcronymFile) {
						string[] splitLine = line.Split(':');
						if (nameConversions.TryGetValue(splitLine[0], out string? newName)) {
							newMapImageAcronymFile.Add(newName + ':' + splitLine[1]);
							//Logger.Info($"- converted line {line} to {newName + ':' + splitLine[1]}");
							continue;
						}
						newMapImageAcronymFile.Add(line);
					}
					//Logger.Info($" <- Writing new file to {to}");
					File.WriteAllLines(to, newMapImageAcronymFile);
				break;
				case 4:
					// world_XX.txt
					string[] worldAcronymFile = File.ReadAllLines(from);
					List<string> newWorldAcronymFile = [];
					bool conditionalLinks = false;
					bool rooms = false;
					bool creatures = false;
					foreach (string line in worldAcronymFile) {
						if (line.StartsWith("CONDITIONAL LINKS")) {
							conditionalLinks = true;
						}
						else if (line.StartsWith("END CONDITIONAL LINKS")) {
							conditionalLinks = false;
						}
						else if (line.StartsWith("ROOMS")) {
							rooms = true;
						}
						else if (line.StartsWith("END ROOMS")) {
							rooms = false;
						}
						else if (line.StartsWith("CREATURES")) {
							creatures = true;
						}
						else if (line.StartsWith("END CREATURES")) {
							creatures = false;
						}
						else if (!line.IsNullOrEmpty()) {
							if (conditionalLinks) {
								string[] separatedLine = line.Split(" : ");
								string newLine = "";
								foreach (string part in separatedLine) {
									if (newLine != "")
										newLine += " : ";
									if (nameConversions.TryGetValue(part, out string? newName)) {
										newLine += newName;
										continue;
									}
									newLine += part;
								}
								newWorldAcronymFile.Add(newLine);
								//Logger.Info($"- converted line {line} to {newLine}");
								continue;
							}
							else if (rooms) {
								string[] separatedLine = line.Split(" : ");
								string newLine = "";
								foreach (string linePart in separatedLine) {
									if (newLine != "")
										newLine += " : ";
									string[] splitPart = linePart.Split(", ");
									if (splitPart.Length > 1) {
										string unSplitPart = "";
										foreach (string part in splitPart) {
											if (unSplitPart != "")
												unSplitPart += ", ";
											if (nameConversions.TryGetValue(part, out string? newName)) {
												unSplitPart += newName;
												continue;
											}
											unSplitPart += part;
										}
										newLine += unSplitPart;
									}
									else {
										if (nameConversions.TryGetValue(linePart, out string? newName)) {
											newLine += newName;
											continue;
										}
										newLine += linePart;
									}
								}
								newWorldAcronymFile.Add(newLine);
								//Logger.Info($"- converted line {line} to {newLine}");
								continue;
							}
							else if (creatures) {
								string[] separatedLine = line.Split(" : ");
								string newLine = "";
								if (separatedLine[0] == "LINEAGE") {
									if (nameConversions.TryGetValue(separatedLine[1], out string? newName)) {
										newLine = separatedLine[0] + " : " + newName;
										foreach (string part in separatedLine[2..]) {
											newLine += " : " + part;
										}
									}
									else {
										newLine = line;
									}
								}
								else if (nameConversions.TryGetValue(separatedLine[0], out string? newName)) {
									newLine = newName;
									foreach (string part in separatedLine[1..]) {
										newLine += " : " + part;
									}
								}
								else {
									newLine = line;
								}
								newWorldAcronymFile.Add(newLine);
								//Logger.Info($"- converted line {line} to {newLine}");
								continue;
							}
						}
						newWorldAcronymFile.Add(line);
					}
					//Logger.Info($" <- Writing new file to {to}");
					File.WriteAllLines(to, newWorldAcronymFile);
				break;
				case 3:
				case 5:
				default:
					// map_XX.png, displayname.txt, misc
					//Logger.Info($" <- Copying file to {to}");
					File.Copy(from, to, true);
				break;
			}
		}
		
		Logger.Info($"! Applying Directories in unmanagedWorldDirectories");
		foreach ((string from, string to) in worldSubDirectoriesToTransfer) {
			Stack<string> pathsToExplore = new();
			pathsToExplore.Push(from);
			while (pathsToExplore.Count > 0) {
				string currentPath = pathsToExplore.Pop();
				if (Directory.Exists(currentPath)) {
					//Logger.Info($" - creating directory");
					Directory.CreateDirectory(currentPath.Replace(from, to));
					foreach (string subPath in Directory.EnumerateFiles(currentPath)) {
						pathsToExplore.Push(subPath);
					}
					foreach (string subPath in Directory.EnumerateDirectories(currentPath)) {
						pathsToExplore.Push(subPath);
					}
				}
				else {
					//Logger.Info($" - applying file");
					File.Copy(currentPath, currentPath.Replace(from, to), true);
				}
			}
		}

		Logger.Info($"Creating {newAcronym}-rooms folder!");
		Directory.CreateDirectory(newRoomsPath);

		Logger.Info($"! Applying files in roomFilesToTransfer");
		foreach ((string from, string to) in roomFilesToTransfer) {
			File.Copy(from, to);
		}
		
		Logger.Info($"! Applying Directories in roomsSubDirectoriesToTransfer");
		foreach ((string from, string to) in roomsSubDirectoriesToTransfer) {
			Stack<string> pathsToExplore = new();
			pathsToExplore.Push(from);
			while (pathsToExplore.Count > 0) {
				string currentPath = pathsToExplore.Pop();
				if (Directory.Exists(currentPath)) {
					//Logger.Info($" - creating directory");
					Directory.CreateDirectory(currentPath.Replace(from, to));
					foreach (string subPath in Directory.EnumerateFiles(currentPath)) {
						pathsToExplore.Push(subPath);
					}
					foreach (string subPath in Directory.EnumerateDirectories(currentPath)) {
						pathsToExplore.Push(subPath);
					}
				}
				else {
					//Logger.Info($" - applying file");
					File.Copy(currentPath, currentPath.Replace(from, to));
				}
			}
		}

		if (deleteSourceFiles) {
			Logger.Info($"BACKING UP FILES!");
			Directory.GetFileSystemEntries(WorldWindow.region.roomsPath, "", searchOption: SearchOption.AllDirectories).ForEach(f => FloodForge.Backup.File(f));
			Directory.GetFileSystemEntries(WorldWindow.region.exportPath, "", searchOption: SearchOption.AllDirectories).ForEach(f => FloodForge.Backup.File(f));
			//Logger.Info($"FINISHED BACKING UP FILES!");
			Logger.Info($"! DELETING ORIGINAL FILES");
			Directory.Delete(WorldWindow.region.exportPath, true);
			Directory.Delete(WorldWindow.region.roomsPath, true);
			//Logger.Info($"! FINISHED DELETING ORIGINAL FILES");
		}

		Logger.Info($"Setting new paths");
		WorldWindow.region.exportPath = newExportPath;
		WorldWindow.region.roomsPath = newRoomsPath;
		WorldWindow.region.acronym = newAcronym;
		Logger.Info($"Setting new world as recent file.");
		RecentFiles.AddPath(newExportPath + $"\\world_{newAcronym}.txt");
		
		Logger.Info($"Updating PersistentData");
		PersistentData.GetPersistentData(oldAcronym);
		PersistentData.StorePersistentData(newAcronym);
		if (deleteSourceFiles) {
			PersistentData.RemovePersistentData(oldAcronym);
		}

		Logger.Info($"AcronymChange complete.");
		PopupManager.Add(new InfoPopup("Acronym successfully changed.\n" + (deleteSourceFiles ? "Original region deleted.\n" : "All files copied.\n") + "<s:1>Note - Modify and Regions.txt files are unchanged." + (encounteredAtypicalFiles ? "\n<s:1>Note - non-typical file contents are unchanged" : "")));
	}
}