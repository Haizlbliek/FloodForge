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
                    string path = "";
                    Vector2 pos = Vector2.Zero;
                    float scale = 1f;
                    float brightness = 1f;
                    bool lockImage = false;
                    bool drawUnderGrid = true;
                    foreach (string property in splitLine[1].Split("</b>")) {
                        string[] splitProperty = property.Split("</c>");
                        switch (splitProperty[0]) {
                            case "path":
                                path = splitProperty[1];
                            break;
                            case "pos":
                                string[] vector = splitProperty[1].Split(';');
                                pos = new Vector2(float.Parse(vector[0]), float.Parse(vector[1]));
                            break;
                            case "scale":
                                scale = float.Parse(splitProperty[1]);
                            break;
                            case "brightness":
                                brightness = float.Parse(splitProperty[1]);
                            break;
                            case "lock":
                                lockImage = splitProperty[1] == "1";
                            break;
                            case "under":
                                drawUnderGrid = splitProperty[1] == "1";
                            break;
                        }
                    }
                    if (!path.IsNullOrEmpty()) {
                        WorldWindow.referenceImages.Add(new (path) {
                            Position = pos,
                            Scale = scale,
                            brightness = brightness,
                            lockImage = lockImage,
                            drawUnderGrid = drawUnderGrid
                        });
                    }
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
                newFile.Add($"REFIMAGE</a>"
                + $"path</c>{image.imagePath}</b>"
                + $"pos</c>{image.Position.x};{image.Position.y}</b>"
                + $"scale</c>{image.Scale}</b>"
                + $"lock</c>{(image.lockImage?"1":"0")}</b>"
                + $"under</c>{(image.drawUnderGrid?"1":"0")}</b>"
                + $"brightness</c>{image.brightness}");
            }
            newFile.Add($"ENDREGION");
        }

        File.WriteAllLines(persistentDataPath, newFile);
    }
}