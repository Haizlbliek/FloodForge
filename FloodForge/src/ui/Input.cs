using Silk.NET.GLFW;
using Silk.NET.Input;

namespace FloodForge;

public static class Input {
	private static readonly int MaxKeyIndex = ((int[]) Enum.GetValues(typeof(Key))).Max();
	private static readonly bool[] currentKeys = new bool[MaxKeyIndex + 1];
	private static readonly bool[] lastKeys = new bool[MaxKeyIndex + 1];

	public static void End() {
		Array.Copy(currentKeys, lastKeys, currentKeys.Length);
	}

	public static void Press(Key key) {
		if (key != Key.Unknown)
			currentKeys[(int) key] = true;
	}

	public static void Release(Key key) {
		if (key != Key.Unknown)
			currentKeys[(int) key] = false;
	}

	private static bool Pressed(Key key) {
		return key != Key.Unknown && currentKeys[(int) key];
	}

	private static bool JustPressed(Key key) {
		if (key == Key.Unknown)
			return false;

		int index = (int) key;
		return currentKeys[index] && !lastKeys[index];
	}

	public static bool ModifiersPressed(Modifier modifier) {
		return modifier switch {
			Modifier.Shift => currentKeys[(int) Key.ShiftLeft] || currentKeys[(int) Key.ShiftRight],
			Modifier.Control => currentKeys[(int) Key.ControlLeft] || currentKeys[(int) Key.ControlRight],
			Modifier.Alt => currentKeys[(int) Key.AltLeft] || currentKeys[(int) Key.AltRight],
			_ => false,
		};
	}

	private static bool ModifiersPressed(KeyModifiers modifiers, KeyModifiers ignoreModifiers) {
		bool pressed = true;

		if ((modifiers & KeyModifiers.Shift) > 0)
			pressed = pressed && ModifiersPressed(Modifier.Shift);
		else if ((ignoreModifiers & KeyModifiers.Shift) == 0)
			pressed = pressed && !ModifiersPressed(Modifier.Shift);

		if ((modifiers & KeyModifiers.Control) > 0)
			pressed = pressed && ModifiersPressed(Modifier.Control);
		else if ((ignoreModifiers & KeyModifiers.Control) == 0)
			pressed = pressed && !ModifiersPressed(Modifier.Control);

		if ((modifiers & KeyModifiers.Alt) > 0)
			pressed = pressed && ModifiersPressed(Modifier.Alt);
		else if ((ignoreModifiers & KeyModifiers.Alt) == 0)
			pressed = pressed && !ModifiersPressed(Modifier.Alt);

		if ((modifiers & KeyModifiers.Super) > 0)
			pressed = pressed && ModifiersPressed(Modifier.Super);
		else if ((ignoreModifiers & KeyModifiers.Super) == 0)
			pressed = pressed && !ModifiersPressed(Modifier.Super);

		return pressed;
	}

	public static bool Pressed(Keys keybinding) {
		foreach ((Key key, KeyModifiers modifiers, KeyModifiers ignoreModifiers) in keybinding.inputs) {
			if (Pressed(key) && ModifiersPressed(modifiers, ignoreModifiers))
				return true;
		}

		return false;
	}

	public static bool JustPressed(Keys keybinding) {
		foreach ((Key key, KeyModifiers modifiers, KeyModifiers ignoreModifiers) in keybinding.inputs) {
			if (JustPressed(key) && ModifiersPressed(modifiers, ignoreModifiers))
				return true;
		}

		return false;
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

	public enum Modifier {
		Shift,
		Control,
		Alt,
		Super
	}
}