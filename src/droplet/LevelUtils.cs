namespace FloodForge.Droplet;

public static class LevelUtils {
	public static void CreateLevelFiles(string directory, string roomName, int width, int height, bool fillLayer1, bool fillLayer2, bool createCameras) {
		string filePath = Path.Combine(directory, roomName + ".txt");

		using StreamWriter geo = new StreamWriter(filePath);

		geo.WriteLine(roomName);
		geo.WriteLine($"{width}*{height}|-1|0");
		geo.WriteLine("0.0000*1.0000|0|0");

		if (createCameras) {
			int screenWidth = (int) Math.Round((width + 4.0) / 52.0);
			int screenHeight = (int) Math.Round((height + 5.0) / 40.0);

			for (int y = 0; y < screenHeight; y++) {
				for (int x = 0; x < screenWidth; x++) {
					if (x != 0 || y != 0)
						geo.Write("|");
					geo.Write($"{-212 + x * 1024},{-34 + 768 * y}");
				}
			}

			if (screenHeight <= 0 && screenWidth <= 0)
				geo.WriteLine("-212,-34");
			else
				geo.WriteLine();
		}
		else {
			geo.WriteLine("-220,-50");
		}

		geo.WriteLine("Border: Passable");
		geo.WriteLine("\n\n\n\n0\n");

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				geo.Write(fillLayer1 ? "1|" : (fillLayer2 ? "0,6|" : "0|"));
			}
		}
		geo.WriteLine();
	}

	public static List<Vector2i> Line(int x0, int y0, int x1, int y1) {
		List<Vector2i> points = [];

		int dx = Math.Abs(x1 - x0);
		int dy = Math.Abs(y1 - y0);
		int sx = (x0 < x1) ? 1 : -1;
		int sy = (y0 < y1) ? 1 : -1;
		int error = dx - dy;

		while (true) {
			points.Add(new Vector2i(x0, y0));

			if (x0 == x1 && y0 == y1)
				break;

			int e2 = 2 * error;
			if (e2 > -dy) {
				error -= dy;
				x0 += sx;
			}
			if (e2 < dx) {
				error += dx;
				y0 += sy;
			}
		}

		return points;
	}
}