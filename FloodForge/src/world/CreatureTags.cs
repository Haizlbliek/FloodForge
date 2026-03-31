namespace FloodForge.World;

// LATER: Make dynamic
public static class CreatureTags {
	private static readonly string[] lizards = [ "blacklizard", "bluelizard", "cyanlizard", "greenlizard", "pinklizard", "redlizard", "whitelizard", "yellowlizard", "salamander", "eellizard", "spitlizard", "trainlizard", "zooplizard", "basilisklizard", "blizzardlizard", "indigolizard" ];
	private static readonly string[] centipedes = [ "centipede", "centiwing", "redcentipede", "smallcentipede", "aquacenti" ];

	public static readonly Dictionary<string, Tag> tags = [];

	public static int Count => tags.Count;

	public static Tag POLEMIMIC_LENGTH = AddTag(new Tag("POLEMIMIC_LENGTH", new DisplayType.IntSlider(0, 32), [ "polemimic" ]));
	public static Tag CENTIPEDE_LENGTH = AddTag(new Tag("CENTIPEDE_LENGTH", new DisplayType.FloatSlider(0.1f, 1f), centipedes));
	public static Tag Mean             = AddTag(new Tag("Mean",             new DisplayType.FloatSlider(-1f, 1f), lizards));
	public static Tag RotType          = AddTag(new Tag("RotType",          new DisplayType.IntSlider(0, 3), lizards));
	public static Tag Seed             = AddTag(new Tag("Seed",             DisplayType.InputSignedInteger));
	public static Tag NamedAttr        = AddTag(new Tag("NamedAttr",        DisplayType.InputString));
	public static Tag Voidsea          = AddTag(new Tag("Voidsea",          DisplayType.None, [ "redlizard", "redcentipede", "bigspider", "daddylonglegs", "brotherlonglegs", "terrorlonglegs", "bigeel", "cyanlizard" ]));
	public static Tag Winter           = AddTag(new Tag("Winter",           DisplayType.None, [ "bigspider", "spitterspider", "yeek", ..lizards ]));
	public static Tag AlternateForm    = AddTag(new Tag("AlternateForm",    DisplayType.None));
	public static Tag Ignorecycle      = AddTag(new Tag("Ignorecycle",      DisplayType.None));
	public static Tag Lavasafe         = AddTag(new Tag("Lavasafe",         DisplayType.None));
	public static Tag Night            = AddTag(new Tag("Night",            DisplayType.None));
	public static Tag PreCycle         = AddTag(new Tag("PreCycle",         DisplayType.None));
	public static Tag Ripple           = AddTag(new Tag("Ripple",           DisplayType.None));
	public static Tag Slayer           = AddTag(new Tag("Slayer",           DisplayType.None, [ "tardigrade" ]));
	public static Tag TentacleImmune   = AddTag(new Tag("TentacleImmune",   DisplayType.None));

	private static Tag AddTag(Tag tag) {
		tags.Add(tag.id, tag);
		return tag;
	}

	public static Tag GetOrCreate(string id) {
		if (tags.TryGetValue(id, out Tag tag))
			return tag;

		return AddTag(new Tag(id, DisplayType.None));
	}

	public static IEnumerable<Tag> Tags(string creature) => tags.Values.Where(t => t.Supports(creature));

	public class DisplayType : IEquatable<DisplayType> {
		public static DisplayType None = new DisplayType("None");
		public static DisplayType InputSignedInteger = new DisplayType("InputSignedInt");
		public static DisplayType InputSignedFloat = new DisplayType("InputSignedFloat");
		public static DisplayType InputUnsignedInteger = new DisplayType("InputUnsignedInt");
		public static DisplayType InputUnsignedFloat = new DisplayType("InputUnsignedFloat");
		public static DisplayType InputString = new DisplayType("InputString");

		private readonly string id;

		private DisplayType(string id) {
			this.id = id;
		}

		public virtual void Clamp(DenCreature.Tag tag) {}

		bool IEquatable<DisplayType>.Equals(DisplayType? other) {
			return this.id == other?.id;
		}

		public class IntSlider : DisplayType {
			public readonly int min, max;

			public override void Clamp(DenCreature.Tag tag) {
				if (tag is not DenCreature.IntegerTag intTag)
					return;

				intTag.data = Math.Clamp(intTag.data, this.min, this.max);
			}

			public IntSlider(int min, int max) : base("IntSlider") {
				this.min = min;
				this.max = max;
			}
		}

		public class FloatSlider : DisplayType {
			public readonly float min, max;

			public override void Clamp(DenCreature.Tag tag) {
				if (tag is not DenCreature.FloatTag floatTag)
					return;

				floatTag.data = Mathf.Clamp(floatTag.data, this.min, this.max);
			}

			public FloatSlider(float min, float max) : base("FloatSlider") {
				this.min = min;
				this.max = max;
			}
		}
	}

	public readonly struct Tag : IEquatable<Tag> {
		public readonly string id;
		public readonly string[] supports;
		public readonly DisplayType displayType;

		public Tag(string id, DisplayType displayType, string[]? supports = null) {
			this.id = id;
			this.supports = supports ?? [];
			this.displayType = displayType;
		}

		public bool Equals(Tag other) {
			return this.id == other.id;
		}

		public static bool operator ==(Tag a, Tag b) {
			return a.Equals(b);
		}

		public static bool operator !=(Tag a, Tag b) {
			return !(a == b);
		}

		public override bool Equals(object? obj) {
			return obj is Tag other && this.Equals(other);
		}

		public override int GetHashCode() {
			return this.id?.GetHashCode() ?? 0;
		}

		public readonly bool Supports(string creature) {
			return this.supports.Length == 0 || this.supports.Contains(creature);
		}
	}
}