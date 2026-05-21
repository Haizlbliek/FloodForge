using Silk.NET.Input;

namespace FloodForge;

public static class Keys {
	private static readonly int MaxKeyIndex = ((int[])Enum.GetValues(typeof(Key))).Max();
	private static readonly bool[] currentKeys = new bool[MaxKeyIndex + 1];
	private static readonly bool[] lastKeys = new bool[MaxKeyIndex + 1];

	public static void End() {
		Array.Copy(currentKeys, lastKeys, currentKeys.Length);
	}

	public static void Press(Key key) {
		if (key != Key.Unknown) currentKeys[(int)key] = true;
	}

	public static void Release(Key key) {
		if (key != Key.Unknown) currentKeys[(int)key] = false;
	}

	public static bool Pressed(Key key) {
		return key != Key.Unknown && currentKeys[(int)key];
	}

	public static bool JustPressed(Key key) {
		if (key == Key.Unknown) return false;
		int index = (int)key;
		return currentKeys[index] && !lastKeys[index];
	}

	public static bool JustReleased(Key key) {
		if (key == Key.Unknown) return false;
		int index = (int)key;
		return !currentKeys[index] && lastKeys[index];
	}

	public static bool Modifier(Modifiers modifier) {
		return modifier switch 
		{
			Modifiers.Shift => currentKeys[(int)Key.ShiftLeft] || currentKeys[(int)Key.ShiftRight],
			Modifiers.Control => currentKeys[(int)Key.ControlLeft] || currentKeys[(int)Key.ControlRight],
			Modifiers.Alt => currentKeys[(int)Key.AltLeft] || currentKeys[(int)Key.AltRight],
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