using TextCopy;

namespace FloodForge;

public static class Clipboard {
	public static string Get() {
		return ClipboardService.GetText() ?? "";
	}
}