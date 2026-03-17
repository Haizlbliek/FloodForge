using System.Diagnostics;
using System.IO;
using Silk.NET.Core.Native;
using Stride.Core.Extensions;

namespace FloodForge.Popups;

public class MarkdownPopup : Popup {
	protected float scroll;
	protected float targetScroll;
	protected float maxScroll;
	protected MDItem[] lines = [];
	protected Texture[] textures = [];

	public MarkdownPopup(string path) {
		this.bounds = new Rect(-0.8f, -0.8f, 0.8f, 0.8f);
		Main.Scroll += this.Scroll;
		this.LoadFile(path);
	}

	protected void LoadFile(string path) {
		if (!File.Exists(path)) {
			Logger.Warn("No file found '" + path + "'");
			this.Close();
			return;
		}

		List<MDItem> lines = [];
		List<Texture> images = [];

		int addNewline = 0;
		string[] texts = File.ReadAllLines(path);
		foreach (string oText in texts) {
			string text = oText;

			MDType type = MDType.Text;
			if (text.IsNullOrEmpty()) {
				if (addNewline == 1) {
					addNewline = 2;
				} else {
					addNewline = 0;
				}

				continue;
			}

			if (text.StartsWith("# ")) {
				type = MDType.H1;
				text = text[2..];
				addNewline = 0;
			}
			else if (text.StartsWith("## ")) {
				type = MDType.H2;
				text = text[3..];
				addNewline = 0;
			}
			else if (text.StartsWith("### ")) {
				type = MDType.H3;
				text = text[4..];
				addNewline = 0;
			}
			else if (text.StartsWith("> ")) {
				if (addNewline == 2) {
					lines.Add(new MDItem(MDType.Text, []));
				}
				addNewline = 1;

				type = MDType.Quote;
				text = text[2..];
			}
			else if (text.StartsWith("---") && text[..'-'].Length == 0 || text.StartsWith("***") && text[..'*'].Length == 0 || text.StartsWith("___") && text[..'_'].Length == 0) {
				lines.Add(new MDItem(MDType.HorizontalRule, []));
				addNewline = 0;
				continue;
			}
			else if (text.StartsWith("$")) {
				lines.Add(new MDItem(MDType.Texture, []));
				images.Add(Texture.Load("assets/" + text[1..]));
				addNewline = 1;
				continue;
			}
			else {
				if (addNewline == 2) {
					lines.Add(new MDItem(MDType.Text, []));
				}
				addNewline = 1;
			}

			lines.Add(new MDItem(type, this.ParseStyledText(text)));
		}

		this.lines = [..lines];
		this.textures = [..images];
	}

	protected MDStyledText[] ParseStyledText(string text) {
		Stack<MDStyledText> result = [];

		bool bold = false, italic = false, underline = false, strikethrough = false, code = false, inLink = false;
		string current = "";

		text += " ";
		for (int i = 0; i < text.Length - 1; i++) {
			char c = text[i];

			if (c == '\\') {
				current += text[i + 1];
				i++;
			}
			else if (code) {
				if (c == '`') {
					code = false;
					result.Push(new MDStyledText(current, code: true));
					current = "";
				} else {
					current += c;
				}
			}
			else {
				if (c == '`') {
					result.Push(new MDStyledText(current, italic, bold, underline, strikethrough, false));
					current = "";
					code = true;
				}
				else if (c == '*' && text[i + 1] == '*') {
					result.Push(new MDStyledText(current, italic, bold, underline, strikethrough, false));
					current = "";
					bold = !bold;
					i++;
				}
				else if (c == '~' && text[i + 1] == '~') {
					result.Push(new MDStyledText(current, italic, bold, underline, strikethrough, false));
					current = "";
					strikethrough = !strikethrough;
					i++;
				}
				else if (c == '_' && text[i + 1] == '_') {
					result.Push(new MDStyledText(current, italic, bold, underline, strikethrough, false));
					current = "";
					underline = !underline;
					i++;
				}
				else if (c == '*' || c == '_') {
					result.Push(new MDStyledText(current, italic, bold, underline, strikethrough, false));
					current = "";
					italic = !italic;
				}
				else if (c == '[') {
					result.Push(new MDStyledText(current + " ", italic, bold, underline, strikethrough, false));
					current = "";
					inLink = true;
				}
				else if (c == ']') {
					if (!inLink) {
						current += ']';
						continue;
					}

					result.Push(new MDStyledText(current));
				}
				else if (c == '(') {
					if (!inLink) {
						current += '(';
						continue;
					}

					current = "";
				}
				else if (c == ')') {
					if (!inLink) {
						current += ')';
						continue;
					}

					MDStyledText last = result.Pop();
					result.Push(new MDStyledText(last.text, italic, bold, true, strikethrough, false, current ));
					current = "";
					inLink = false;
				}
				else {
					current += c;
				}
			}
		}

		if (!current.IsNullOrEmpty()) {
			result.Push(new MDStyledText(current));
		}

		return result.Where(x => x.text.Length > 0).Reverse().ToArray();
	}

	protected void Scroll(float x, float y) {
		if (!this.hovered || this.minimized) return;

		this.targetScroll -= y * 0.1f;
		this.ClampScroll();
	}

	public override void Close() {
		Main.Scroll -= this.Scroll;

		foreach (Texture texture in this.textures) {
			texture.Dispose();
		}

		base.Close();
	}

	public override void Draw() {
		base.Draw();

		if (this.minimized) return;

		float padding = 0.01f;
		int width = Program.window.FramebufferSize.X;
		int height = Program.window.FramebufferSize.Y;
		Program.gl.Enable(EnableCap.ScissorTest);
		Program.gl.Scissor(
			(int) (((this.bounds.x0 + padding) / Main.screenBounds.x + 1f) * 0.5f * width),
			(int) (((this.bounds.y0 + padding) / Main.screenBounds.y + 1f) * 0.5f * height),
			(uint) ((this.bounds.x1 - this.bounds.x0 - padding * 2f) / Main.screenBounds.x * 0.5f * width),
			(uint) ((this.bounds.y1 - this.bounds.y0 - padding * 2f - 0.05f) / Main.screenBounds.y * 0.5f * height)
		);

		this.scroll += (this.targetScroll - this.scroll) * (1f - MathF.Pow(1f - Settings.PopupScrollSpeed, Program.Delta * 60f));

		float x = this.bounds.x0;
		float y = 0.75f + this.bounds.y0 + 0.8f + this.scroll;
		int currentImage = 0;

		Color textColor = Themes.Text;
		foreach (MDItem item in this.lines) {
			switch (item.type) {
				case MDType.Text: {
					this.WriteLine(textColor, item.text, x + 0.02f, y, 0.03f);
					y -= 0.05f;
					break;
				}

				case MDType.Quote: {
					Immediate.Color(textColor * 0.7f);
					Immediate.Begin(Immediate.PrimitiveType.LINES);
					Immediate.Vertex(-0.77f, y);
					Immediate.Vertex(-0.77f, y - 0.04f);
					Immediate.End();

					this.WriteLine(textColor, item.text, x + 0.02f, y, 0.03f);
					y -= 0.05f;
					break;
				}

				case MDType.H1: {
					y -= 0.02f;
					this.WriteLine(textColor, item.text, x + 0.03f, y, 0.08f);
					y -= 0.11f;
					break;
				}

				case MDType.H2: {
					y -= 0.02f;
					this.WriteLine(textColor, item.text, x + 0.02f, y, 0.05f);
					y -= 0.08f;
					break;
				}

				case MDType.H3: {
					y -= 0.01f;
					this.WriteLine(textColor * 0.95f, item.text, x + 0.02f, y, 0.04f);
					y -= 0.06f;
					break;
				}

				case MDType.HorizontalRule: {
					y -= 0.03f;
					Immediate.Color(textColor * 0.7f);
					Immediate.Begin(Immediate.PrimitiveType.LINES);
					Immediate.Vertex(this.bounds.x0 + 0.01f, y);
					Immediate.Vertex(this.bounds.x1 - 0.01f, y);
					Immediate.End();
					y -= 0.03f;
					break;
				}

				case MDType.Texture: {
					Texture texture = this.textures[currentImage++];
					float h = 1.58f / texture.width * texture.height;
					y -= 0.03f;
					Immediate.Color(1f, 1f, 1f);
					Immediate.UseTexture(texture);
					UI.FillRect(UVRect.FromSize(x + 0.02f, y - height, 1.58f, height));
					Immediate.UseTexture(0);
					y -= 0.03f;
					break;
				}
			}
		}

		this.maxScroll = -(y - this.scroll);

		Program.gl.Disable(EnableCap.ScissorTest);
	}

	protected void WriteLine(Color textColor, MDStyledText[] line, float x, float y, float size) {
		foreach (MDStyledText seg in line) {
			Immediate.Color(textColor * 0.85f);

			if (seg.bold) {
				UI.font.separationScale = 1.1f;
			}

			float width = UI.font.Measure(seg.text, size).x;

			Matrix4X4<float> matrix = Matrix4X4.CreateTranslation(0f, y - size, 0f);
			if (seg.italic) {
				Immediate.Color(textColor * 0.8f);
				matrix.M21 = 0.2f;
			}
			Immediate.PushMatrix();
			Immediate.MultMatrix(matrix);

			if (seg.code) {
				x += 0.02f;
				UI.StrokeRect(x - 0.01f, -0.005f, x + width + 0.01f, size + 0.005f);
			}

			if (seg.bold) {
				float p = size / UI.font.baseSize * 1.5f;
				Immediate.Color(textColor);
				UI.font.Write(seg.text, x + p, size, size);
				UI.font.Write(seg.text, x - p, size, size);
			}
			UI.font.Write(seg.text, x, size, size);

			if (seg.strikethrough) {
				UI.Line(x, size * 0.4f, x + width, size * 0.4f, 0.1f);
			}

			if (seg.underline) {
				UI.Line(x, -0.01f, x + width, -0.01f, 0.1f);
			}

			if (seg.url != null) {
				if (Mouse.JustLeft && Mouse.X >= x && Mouse.X < x + width && Mouse.Y <= y && Mouse.Y >= y - size) {
					Process.Start(new ProcessStartInfo() { FileName = seg.url, UseShellExecute = true });
				}
			}

			if (seg.code) {
				x += 0.02f;
			}

			Immediate.PopMatrix();

			x += width;

			UI.font.separationScale = 1f;
		}
	}

	protected void ClampScroll() {
		if (this.targetScroll >= this.maxScroll) {
			this.targetScroll = this.maxScroll;
			if (this.scroll >= this.maxScroll - 0.03f) {
				this.scroll = this.maxScroll - 0.03f;
			}
		}

		if (this.targetScroll < 0) {
			this.targetScroll = 0;
			if (this.scroll < 0.03f) {
				this.scroll = 0.03f;
			}
		}
	}

	protected readonly struct MDItem {
		public readonly MDType type;
		public readonly MDStyledText[] text;

		public MDItem(MDType type, MDStyledText[] text) {
			this.type = type;
			this.text = text;
		}
	}

	protected readonly struct MDStyledText {
		public readonly string text;
		public readonly bool italic = false;
		public readonly bool bold = false;
		public readonly bool underline = false;
		public readonly bool strikethrough = false;
		public readonly bool code = false;
		public readonly string? url = null;

		public MDStyledText(string text, bool italic = false, bool bold = false, bool underline = false, bool strikethrough = false, bool code = false, string? url = null) {
			this.text = text;
			this.italic = italic;
			this.bold = bold;
			this.underline = underline;
			this.strikethrough = strikethrough;
			this.code = code;
			this.url = url;
		}
	}

	protected enum MDType {
		Text,
		H1,
		H2,
		H3,
		Quote,
		HorizontalRule,
		Texture
	}
}