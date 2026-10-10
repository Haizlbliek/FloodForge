using Silk.NET.GLFW;
using Silk.NET.Input;

namespace FloodForge;

public readonly struct Keys {
	public static Keys Undo = Of(Key.Z, KeyModifiers.Control, KeyModifiers.Super);
	public static Keys Redo = Of(Key.Y, KeyModifiers.Control).With(Key.Z, KeyModifiers.Shift | KeyModifiers.Control);
	public static Keys Copy = Of(Key.C, KeyModifiers.Control);
	public static Keys Cut = Of(Key.X, KeyModifiers.Control);
	public static Keys Paste = Of(Key.V, KeyModifiers.Control);
	public static Keys DisableSnap = Of(KeyModifiers.Alt);

	public static Keys Accept = Of(Key.Enter);
	public static Keys Cancel = Of(Key.Escape);
	public static Keys ToggleFullscreen = Of(Key.F11);
	public static Keys ToggleDevTools = Of(Key.F3);

	public static Keys OpenSplash = Of(Key.S, KeyModifiers.Alt);
	public static Keys OpenTutorial = Of(Key.T, KeyModifiers.Alt);
	public static Keys Delete = Of(Key.X);
	public static Keys Search = Of(Key.F, KeyModifiers.Control);
	public static Keys MoveToFront = Of(Key.I);
	public static Keys ChangeSubregion = Of(Key.S);
	public static Keys ChangeTag = Of(Key.T);
	public static Keys ChangeLayer = Of(Key.L);
	public static Keys ToggleMerge = Of(Key.G);
	public static Keys ToggleWarpable = Of(Key.W);
	public static Keys ToggleVisibility = Of(Key.H);
	public static Keys ToggleBatMigrationBlockage = Of(Key.B);
	public static Keys ChangeCreatures = Of(Key.C);
	public static Keys ChangeAttractiveness = Of(Key.A);
	public static Keys ChangeConditionals = Of(Key.D);
	public static Keys EditRoom = Of(Key.R);
	public static Keys MassConnectRooms = Of(Key.O, KeyModifiers.Shift);
	public static Keys ShowHiddenReplaceRooms = Of(KeyModifiers.Alt);
	public static Keys ShowShortcuts = Of(KeyModifiers.Shift);
	public static Keys HighlightConnections = Of(KeyModifiers.Shift);
	public static Keys DecrementSwap = Of(KeyModifiers.Shift);
	public static Keys ForceDeleteSubregion = Of(KeyModifiers.Shift);
	public static Keys DevAndCanon = Of(KeyModifiers.Alt);
	public static Keys SoloLayer = Of(KeyModifiers.Shift);

	public static Keys SetWaterLevel = Of(Key.W);
	public static Keys TypeLeft = Of(Key.A);
	public static Keys TypeRight = Of(Key.D);
	public static Keys TypeUp = Of(Key.W);
	public static Keys TypeDown = Of(Key.S);
	public static Keys FloodFill = Of(Key.Q);
	public static Keys AddCamera = Of(Key.C);
	public static Keys Tab1 = Of(Key.Number1);
	public static Keys Tab2 = Of(Key.Number2);
	public static Keys Tab3 = Of(Key.Number3);
	public static Keys SelectionMode = Of(Key.E, KeyModifiers.Shift);
	public static Keys UnlockCameraAngles = Of(KeyModifiers.Shift);

	public readonly (Key key, KeyModifiers modifiers, KeyModifiers ignoreModifiers)[] inputs;

	public Keys(IEnumerable<(Key, KeyModifiers, KeyModifiers)> inputs) {
		this.inputs = [.. inputs];
	}

	public static Keys Of(Key key, KeyModifiers modifiers = 0, KeyModifiers ignoreModifiers = KeyModifiers.Super | KeyModifiers.Shift) {
		return new Keys([ (key, modifiers, ignoreModifiers) ]);
	}

	public static Keys Of(KeyModifiers key) {
		KeyModifiers ignore = KeyModifiers.Shift | KeyModifiers.Control | KeyModifiers.Alt | KeyModifiers.Super;

		if (key == KeyModifiers.Shift)
			return Of(Key.ShiftLeft, KeyModifiers.Shift, ignore).With(Key.ShiftRight, KeyModifiers.Shift, ignore);

		if (key == KeyModifiers.Alt)
			return Of(Key.AltLeft, KeyModifiers.Alt, ignore).With(Key.AltRight, KeyModifiers.Alt, ignore);

		if (key == KeyModifiers.Control)
			return Of(Key.ControlLeft, KeyModifiers.Control, ignore).With(Key.ControlRight, KeyModifiers.Control, ignore);

		if (key == KeyModifiers.Super)
			return Of(Key.SuperLeft, KeyModifiers.Super, ignore).With(Key.SuperRight, KeyModifiers.Super, ignore);

		throw new NotImplementedException();
	}

	public Keys With(Key key, KeyModifiers modifiers = 0, KeyModifiers ignoreModifiers = KeyModifiers.Super | KeyModifiers.Shift) {
		return new Keys([ ..this.inputs, (key, modifiers, ignoreModifiers) ]);
	}
}