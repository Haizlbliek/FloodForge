using System.Diagnostics.CodeAnalysis;

namespace FloodForge.SettingTypes;

public abstract class SettingType {
	protected static Dictionary<Type, Dictionary<string, SettingType>> allPairs = [];
	protected string Key = null!;
}

public class SettingType<T> : SettingType, IParsable<T> where T : SettingType<T>, new() {
	protected static Dictionary<string, T> pairs = [];

	public SettingType() {
		if (!SettingType.allPairs.ContainsKey(typeof(T))) {
			SettingType.allPairs[typeof(T)] = [];
		}
	}

	public static T Parse(string s, IFormatProvider? provider) {
		return pairs[s.ToLowerInvariant().Replace(" ", "")];
	}

	public static bool TryParse(string? s, IFormatProvider? provider, [MaybeNullWhen(false)] out T result) {
		result = null;

		return s != null && pairs.TryGetValue(s.ToLowerInvariant().Replace(" ", ""), out result);
	}

	protected static T Of(string key) {
		T t = new T() { Key = key };
		pairs.Add(key.ToLowerInvariant().Replace(" ", ""), t);
		SettingType.allPairs[typeof(T)][key.ToLowerInvariant().Replace(" ", "")] = t;
		return t;
	}
}