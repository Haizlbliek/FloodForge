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

	public static Tag CreateFrom(Mods.Tag cTag) {
		if (cTag.displayType == Mods.DisplayType.InputSignedFloat || cTag.displayType == Mods.DisplayType.InputUnsignedFloat) {
			return new FloatTag(cTag, 0f);
		}
		else if (cTag.displayType == Mods.DisplayType.InputSignedInteger || cTag.displayType == Mods.DisplayType.InputUnsignedInteger) {
			return new IntegerTag(cTag, 0);
		}
		else if (cTag.displayType == Mods.DisplayType.InputString) {
			return new StringTag(cTag, "");
		}
		else if (cTag.displayType is Mods.DisplayType.FloatSlider) {
			return new FloatTag(cTag, 0f);
		}
		else if (cTag.displayType is Mods.DisplayType.IntSlider) {
			return new IntegerTag(cTag, 0);
		}

		return new Tag(cTag);
	}

	public class Tag {
		public readonly Mods.Tag id;

		public Tag(Mods.Tag id) {
			this.id = id;
		}

		public virtual Tag Clone() {
			return new Tag(this.id);
		}
	}

	public class FloatTag : Tag {
		public float data;

		public FloatTag(Mods.Tag id, float data) : base(id) {
			this.data = data;
		}

		public override FloatTag Clone() {
			return new FloatTag(this.id, this.data);
		}
	}

	public class IntegerTag : Tag {
		public int data;

		public IntegerTag(Mods.Tag id, int data) : base(id) {
			this.data = data;
		}

		public override IntegerTag Clone() {
			return new IntegerTag(this.id, this.data);
		}
	}

	public class StringTag : Tag {
		public string data;

		public StringTag(Mods.Tag id, string data) : base(id) {
			this.data = data;
		}

		public override StringTag Clone() {
			return new StringTag(this.id, this.data);
		}
	}
}

public class DenLineage : DenCreature {
	public Timeline timeline = new ();
	public ConditionalPopup? conditionalPopup;

	public DenLineage(string type, int count) : base(type, count) {
	}
}

public class Den {
	public List<DenLineage> creatures = [];
}

public class GarbageWormDen {
	public string type = "";
	public int count;
	public Timeline timeline = new();

	public GarbageWormDen() {
	}
}