using Silk.NET.Input;

namespace FloodForge;

public static class Keys {
	private static HashSet<Key> lastKeys = [];
	private static readonly HashSet<Key> keys = [];
	private static readonly HashSet<Key> checkedKeys = [Key.ShiftLeft, Key.ShiftRight, Key.ControlLeft, Key.ControlRight, Key.AltLeft, Key.AltRight];

	public static void End() {
		lastKeys = [.. keys];
	}

	public static void Press(Key key) {
		checkedKeys.Add(key);
		keys.Add(key);
	}

	public static void Release(Key key) {
		checkedKeys.Add(key);
		keys.Remove(key);
	}

	public static bool Pressed(Key key) {
		if (!checkedKeys.Contains(key)) {
			checkedKeys.Add(key);
			lastKeys.Add(key);
		}

		return keys.Contains(key);
	}

	public static bool JustPressed(Key key) {
		if (!checkedKeys.Contains(key)) {
			checkedKeys.Add(key);
			lastKeys.Add(key);
		}

		return keys.Contains(key) && !lastKeys.Contains(key);
	}

	public static bool Modifier(Modifiers key) {
		return key switch {
			Modifiers.Shift => keys.Contains(Key.ShiftLeft) || keys.Contains(Key.ShiftRight),
			Modifiers.Control => keys.Contains(Key.ControlLeft) || keys.Contains(Key.ControlRight),
			Modifiers.Alt => keys.Contains(Key.AltLeft) || keys.Contains(Key.AltRight),
			_ => false,
		};
	}

	public static char ParseCharacter(char character, bool shiftPressed, bool capsPressed) {
		if (shiftPressed) {
			character = character switch {
				'1' => '!',
				'2' => '@',
				'3' => '#',
				'4' => '$',
				'5' => '%',
				'6' => '^',
				'7' => '&',
				'8' => '*',
				'9' => '(',
				'0' => ')',
				'`' => '~',
				'-' => '_',
				'=' => '+',
				'[' => '{',
				']' => '}',
				';' => ':',
				'\'' => '"',
				'\\' => '|',
				',' => '<',
				'.' => '>',
				'/' => '?',
				_ => character
			};
		}

		return (shiftPressed || capsPressed)
			? char.ToUpper(character)
			: char.ToLower(character);
	}

	public enum Modifiers {
		Shift,
		Control,
		Alt,
		Meta,
		Caps
	}
}