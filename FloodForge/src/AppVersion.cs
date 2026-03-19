using System.Text.RegularExpressions;

namespace FloodForge;

public readonly partial struct AppVersion : IComparable<AppVersion> {
	public int Major { get; }
	public int Minor { get; }
	public int Build { get; }
	public int Beta { get; }

	[GeneratedRegex(@"v(?<numbers>[\d\.]+)(\s*beta\s*(?<beta>\d+))?", RegexOptions.IgnoreCase)]
	private static partial Regex VersionRegex();

	public AppVersion(string versionString) {
		string clean = versionString.Trim();
		Match match = VersionRegex().Match(clean);

		string[] numbers = match.Groups["numbers"].Value.Split('.');
		this.Major = numbers.Length > 0 ? int.Parse(numbers[0]) : 0;
		this.Minor = numbers.Length > 1 ? int.Parse(numbers[1]) : 0;
		this.Build = numbers.Length > 2 ? int.Parse(numbers[2]) : 0;

		string betaVal = match.Groups["beta"].Value;
		this.Beta = string.IsNullOrEmpty(betaVal) ? int.MaxValue : int.Parse(betaVal);
	}

	public int CompareTo(AppVersion other) {
		if (this.Major != other.Major)
			return this.Major.CompareTo(other.Major);
		if (this.Minor != other.Minor)
			return this.Minor.CompareTo(other.Minor);
		if (this.Build != other.Build)
			return this.Build.CompareTo(other.Build);
		return this.Beta.CompareTo(other.Beta);
	}

	public override string ToString() =>
		$"v{this.Major}.{this.Minor}.{this.Build}" + (this.Beta == int.MaxValue ? "" : $" beta {this.Beta}");

	public static bool operator ==(AppVersion left, AppVersion right) => left.CompareTo(right) == 0;
	public static bool operator !=(AppVersion left, AppVersion right) => left.CompareTo(right) != 0;
	public static bool operator <(AppVersion left, AppVersion right) => left.CompareTo(right) < 0;
	public static bool operator >(AppVersion left, AppVersion right) => left.CompareTo(right) > 0;
	public static bool operator <=(AppVersion left, AppVersion right) => left.CompareTo(right) <= 0;
	public static bool operator >=(AppVersion left, AppVersion right) => left.CompareTo(right) >= 0;

	public override bool Equals(object? obj) => obj is AppVersion other && this == other;
	public override int GetHashCode() => HashCode.Combine(this.Major, this.Minor, this.Build, this.Beta);
}
