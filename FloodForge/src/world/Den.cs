namespace FloodForge.World;

public class DenCreature {
	public string type;
	public int count;
	public List<Tag> tags = [];

	public DenCreature? lineageTo = null;
	public float lineageChance = 0f;

	public DenCreature(string type, int count) {
		this.type = type;
		this.count = count;
	}

	public DenCreature(string type, int count, IEnumerable<Tag> tags) {
		this.type = type;
		this.count = count;
		foreach (Tag tag in tags) {
			this.tags.Add(tag.Clone());
		}
	}

	public DenCreature(DenCreature clone) : this(clone.type, clone.count, clone.tags) {
	}

	public void AddTag(Tag tag) {
		this.tags.Add(tag);
	}

	public static Tag CreateFrom(CreatureTags.Tag cTag) {
		if (cTag.displayType == CreatureTags.DisplayType.InputSignedFloat || cTag.displayType == CreatureTags.DisplayType.InputUnsignedFloat) {
			return new FloatTag(cTag, 0f);
		}
		else if (cTag.displayType == CreatureTags.DisplayType.InputSignedInteger || cTag.displayType == CreatureTags.DisplayType.InputUnsignedInteger) {
			return new IntegerTag(cTag, 0);
		}
		else if (cTag.displayType == CreatureTags.DisplayType.InputString) {
			return new StringTag(cTag, "");
		}
		else if (cTag.displayType is CreatureTags.DisplayType.FloatSlider) {
			return new FloatTag(cTag, 0f);
		}
		else if (cTag.displayType is CreatureTags.DisplayType.IntSlider) {
			return new IntegerTag(cTag, 0);
		}

		return new Tag(cTag);
	}

	public class Tag {
		public readonly CreatureTags.Tag id;

		public Tag(CreatureTags.Tag id) {
			this.id = id;
		}

		public virtual Tag Clone() {
			return new Tag(this.id);
		}
	}

	public class FloatTag : Tag {
		public float data;

		public FloatTag(CreatureTags.Tag id, float data) : base(id) {
			this.data = data;
		}

		public override FloatTag Clone() {
			return new FloatTag(this.id, this.data);
		}
	}

	public class IntegerTag : Tag {
		public int data;

		public IntegerTag(CreatureTags.Tag id, int data) : base(id) {
			this.data = data;
		}

		public override IntegerTag Clone() {
			return new IntegerTag(this.id, this.data);
		}
	}

	public class StringTag : Tag {
		public string data;

		public StringTag(CreatureTags.Tag id, string data) : base(id) {
			this.data = data;
		}

		public override StringTag Clone() {
			return new StringTag(this.id, this.data);
		}
	}
}

public class DenLineage : DenCreature {
	public TimelineType timelineType = TimelineType.All;
	public HashSet<string> timelines = [];

	public DenLineage(string type, int count) : base(type, count) {
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