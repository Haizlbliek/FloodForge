using TextCopy;

namespace FloodForge;

public static class Clipboard {
	public static string Content {
		get => ClipboardService.GetText() ?? "";
		set => ClipboardService.SetText(value);
	}
}