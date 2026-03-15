namespace FloodForge.World;

public class DenCreature {
	public string type;
	public int count;
	public string tag;
	public float data;

	public DenCreature? lineageTo = null;
	public float lineageChance = 0f;

	public DenCreature(string type, int count, string tag, float data) {
		this.type = type;
		this.count = count;
		this.tag = tag;
		this.data = data;
	}
}

public class DenLineage : DenCreature {
	public TimelineType timelineType = TimelineType.All;
	public HashSet<string> timelines = [];

	public DenLineage(string type, int count, string tag, float data) : base(type, count, tag, data) {
	}

	public bool TimelinesMatch(DenLineage other) {
		if (this.timelineType != other.timelineType) return false;
		if (this.timelineType == TimelineType.All) return true;

		return this.timelines.SetEquals(other.timelines);
	}
}

public class Den {
	public List<DenLineage> creatures = [];
}

public class GarbageWormDen {
	public string type = "";
	public int count;
	public HashSet<string> timelines = [];
	public TimelineType timelineType = TimelineType.All;

	public GarbageWormDen() {
	}

	public bool TimelinesMatch(GarbageWormDen other) {
		if (this.timelineType != other.timelineType) return false;
		if (this.timelineType == TimelineType.All) return true;

		return this.timelines.SetEquals(other.timelines);
	}
}