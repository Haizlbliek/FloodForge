namespace FloodForge.World;

public class RoomVisuals {
	private readonly Room room;

	public bool terrainNeedsRefresh = true;
	public bool hasTerrain = false;

	public List<Vector2> terrain = [];
	public bool waterNeedsRefresh = true;
	public List<WaterSpot> water = [];

	public RoomVisuals(Room room) {
		this.room = room;
	}

	private static float SampleHandle(float a, float b, float c, float d, float t) {
		float it = 1f - t;
		return it * it * it * a + 3f * it * it * t * b + 3f * it * t * t * c + t * t * t * d;
	}

	private static float SampleTerrain(TerrainHandleObject left, TerrainHandleObject right, float x) {
		if (x < left.Middle.x) {
			return Mathf.Lerp(left.Middle.y, left.Left.y, Mathf.InverseLerp(x, left.Middle.x, left.Left.x));
		}
		if (x > right.Middle.x) {
			return Mathf.Lerp(right.Middle.y, right.Right.y, Mathf.InverseLerp(x, right.Middle.x, right.Right.x));
		}

		float leftPos = 0f;
		float rightPos = 1f;
		for (int i = 0; i < 16; i++) {
			float middlePos = (leftPos + rightPos) * 0.5f;
			if (SampleHandle(left.Middle.x, left.Right.x, right.Left.x, right.Middle.x, middlePos) < x) {
				leftPos = middlePos;
			}
			else {
				rightPos = middlePos;
			}
		}

		return SampleHandle(left.Middle.y, left.Right.y, right.Left.y, right.Middle.y, (leftPos + rightPos) * 0.5f);
	}

	private float SampleTerrain(float x) {
		if (!this.hasTerrain) return -10000f;

		float xa = MathF.Floor(x / 20f) * 20f;
		float xb = MathF.Ceiling(x / 20f) * 20f;
		float ya = this.terrain[Mathf.FloorToInt(xa / 20f)].y;
		float yb = this.terrain[Mathf.FloorToInt(xb / 20f)].y;
		return Mathf.Lerp(ya, yb, Mathf.InverseLerp(x, xa, xb));
	}

	private void RefreshTerrain() {
		this.terrain.Clear();
		TerrainHandleObject[] handles = [.. this.room.data.objects.OfType<TerrainHandleObject>()];

		if (handles.Length >= 2) {
			Array.Sort(handles, (a, b) => a.Middle.x.CompareTo(b.Middle.x));
			int handleIndex = 0;
			int segments = this.room.width + 1;
			for (float x = 0; x < segments * 20f; x += 20f) {
				while (handleIndex < handles.Length - 2 && handles[handleIndex + 1].Middle.x < x) {
					handleIndex++;
				}
				this.terrain.Add(new Vector2(x, SampleTerrain(handles[handleIndex], handles[handleIndex + 1], x)));
			}
			this.hasTerrain = true;
		}
		else {
			this.hasTerrain = false;
		}
	}

	private void RefreshWater() {
		this.water.Clear();
		if (this.room.data.waterHeight == -1) return;

		this.water.Add(new WaterSpot(new Vector2(0f, 0f), new Vector2(this.room.width * 20f, this.room.data.waterHeight * 20f + 10f)));
		foreach (AirPocketObject airPocket in this.room.data.objects.OfType<AirPocketObject>()) {
			WaterSpot spot = new WaterSpot(airPocket.nodes[0].position, airPocket.nodes[1].position);
			if (spot.size.x <= 0f || spot.size.y <= 0f) continue;

			for (int i = this.water.Count - 1; i >= 0; i--) {
				if (this.water[i].Intersects(spot)) {
					List<WaterSpot> splits = RoomVisuals.WaterSpot.Split(this.water[i], spot);
					if (splits.Count == 0) {
						this.water.RemoveAt(i);
					}
					else {
						this.water[i] = splits[0];
						for (int j = 1; j < splits.Count; j++) {
							this.water.Add(splits[j]);
						}
					}
				}
			}

			spot.size.y = MathF.Min(airPocket.nodes[2].position.y, spot.size.y);
			if (spot.size.y <= 0f) continue;
			this.water.Add(spot);
		}
	}

	public void Refresh() {
		if (this.terrainNeedsRefresh) {
			this.RefreshTerrain();
			this.terrainNeedsRefresh = false;
		}

		if (this.waterNeedsRefresh) {
			this.RefreshWater();
			this.waterNeedsRefresh = false;
		}
	}

	public bool Underwater(int x, int y) {
		Vector2 p = new Vector2(x, this.room.height - y) * 20f;
		try {
			WaterSpot? spot = this.water.First(s => s.Intersects(p));
			return spot != null;
		} catch (Exception) {
			return false;
		}
	}

	private int UnderTerrain(float x, float y) {
		float height = this.SampleTerrain(x);
		return height > y ? 1 : 0;
	}

	public bool UnderTerrain(int ox, int oy, out bool slope) {
		float x = ox * 20f + 10f, y = (this.room.height - oy) * 20f + 10f;
		float v = 4f;
		int count =
			this.UnderTerrain(x - v, y - v) +
			this.UnderTerrain(x + v, y - v) +
			this.UnderTerrain(x - v, y + v) +
			this.UnderTerrain(x + v, y + v);

		slope = count < 4;
		return count > 0;
	}

	public struct WaterSpot {
		public Vector2 pos;
		public Vector2 size;

		public WaterSpot(Vector2 pos, Vector2 size) {
			this.pos = pos;
			this.size = size;
		}

		public readonly bool Intersects(Vector2 p) {
			return p.x >= this.pos.x && p.x < this.pos.x + this.size.x && p.y >= this.pos.y && p.y < this.pos.y + this.size.y;
		}

		public readonly bool Intersects(WaterSpot other) {
			return this.pos.x <= other.pos.x + other.size.x && this.pos.y <= other.pos.y + other.size.y && this.pos.x + this.size.x >= other.pos.x && this.pos.y + this.size.y >= other.pos.y;
		}

		public static List<WaterSpot> Split(WaterSpot water, WaterSpot by) {
			WaterSpot left = new WaterSpot(water.pos, new Vector2(by.pos.x - water.pos.x, water.size.y));
			WaterSpot right = new WaterSpot(new Vector2(by.pos.x + by.size.x, water.pos.y), water.size - new Vector2(by.pos.x + by.size.x - water.pos.x, 0f));
			float middlePosX = MathF.Max(by.pos.x, water.pos.x);
			float middleSizeX = MathF.Min(by.pos.x + by.size.x, water.pos.x + water.size.x) - middlePosX;
			WaterSpot bottom = new WaterSpot(new Vector2(middlePosX, water.pos.y), new Vector2(middleSizeX, by.pos.y - water.pos.y));
			WaterSpot top = new WaterSpot(new Vector2(middlePosX, by.pos.y + by.size.y), new Vector2(middleSizeX, water.size.y - (by.pos.y + by.size.y - water.pos.y)));

			List<WaterSpot> values = [];
			if (left.size.x > 0f && left.size.y > 0f) values.Add(left);
			if (right.size.x > 0f && right.size.y > 0f) values.Add(right);
			if (bottom.size.x > 0f && bottom.size.y > 0f) values.Add(bottom);
			if (top.size.x > 0f && top.size.y > 0f) values.Add(top);
			return values;
		}
	}
}