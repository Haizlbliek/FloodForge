using Stride.Core.Extensions;

namespace FloodForge.World;

public static class PersistentData {
    public static string persistentDataPath = "";

    public static void Initialize() {
        persistentDataPath = "assets/persistentdata.txt";
    }

    public static void GetPersistentData(string acronym) {
        if (!File.Exists(persistentDataPath)) {
            Logger.Info("persistentData.txt not found");
            return;
        }

        bool isRegion = false;
        foreach (string line in File.ReadAllLines(persistentDataPath)) {
            if (line.IsNullOrEmpty())
                continue;
            if (line.StartsWith("ENDREGION")) {
                isRegion = false;
                continue;
            }
            if (line.StartsWith("REGION") && line.Split("</a>")[^1] == acronym) {
                isRegion = true;
                Logger.Info($"Found persistentData for region {acronym}");
                continue;
            }
            
            if (isRegion) {
                string[] splitLine = line.Split("</a>");
                if(splitLine[0] == "REFIMAGE") {
                    string[] properties = splitLine[1].Split("</b>");
                    string[] position = properties[1].Split(';');
					ReferenceImage newImage = new ReferenceImage(properties[0]) {
						Position = new Vector2(float.Parse(position[0]), float.Parse(position[1])),
						Scale = float.Parse(properties[2]),
						lockImage = properties[3] == "1"
					};
                    WorldWindow.referenceImages.Add(newImage);
				}
            }
        }
    }

    public static void StorePersistentData() {
        string acronym = WorldWindow.region.acronym;

        string[] file = [];
        if(File.Exists(persistentDataPath))
            file = File.ReadAllLines(persistentDataPath);
        bool isRegion = false;
        List<string> newFile = [];
        foreach (string line in file) {
            if (line.StartsWith("REGION") && line.Split("</a>")[^1] == acronym)
                isRegion = true;
            if(!isRegion && line != "") newFile.Add(line);
            if (line.StartsWith("ENDREGION"))
                isRegion = false;
        }

        if(WorldWindow.referenceImages.Count != 0) {
            newFile.Add($"REGION</a>{acronym}");
            foreach (ReferenceImage image in WorldWindow.referenceImages) {
                string imagePath = image.imagePath;
                Vector2 imagePosition = image.Position;
                float imageScale = image.Scale;
                bool lockImage = image.lockImage;
                newFile.Add($"REFIMAGE</a>{imagePath}</b>{imagePosition.x};{imagePosition.y}</b>{imageScale}</b>{(lockImage?"1":"0")}");
            }
            newFile.Add($"ENDREGION");
        }

        File.WriteAllLines(persistentDataPath, newFile);
    }
}