using System.ComponentModel.DataAnnotations;
using System.Threading;
using Silk.NET.Input;
using Silk.NET.SDL;

namespace FloodForge;

public static class UI {
	private static Rect? clipRect = null;

	public const uint LINE_NONE = 0;
	public const uint LINE_START = 1;
	public const uint LINE_END = 2;
	public const uint LINE_BOTH = 3;

	public static Font font = null!;
	public static Font rodondo = null!;

	public static Texture ui = null!;

	public static bool CanClick => !Mouse.Disabled && (clipRect?.Inside(Mouse.X, Mouse.Y) ?? true);

	public static void Initialize() {
		Font.Initialize();
		font = new Font(Main.AprilFools ? "ComicSand" : "rainworld");
		rodondo = new Font(Main.AprilFools ? "ComicSand" : "Rodondo");

		ui = Texture.Load("assets/ui.png");
	}

	public static void KeyPress(Key key) {
		if (CurrentEditable is not TextInputEditable editable) return;
		if (key == Key.Enter) {
			editable.submitted = true;
			UpdateTextInput(editable);
			CurrentEditable = null;
			return;
		}

		if (key == Key.Left) {
			selectIndex = Math.Max(selectIndex - 1, 0);
			selectTime = 0;
		}

		if (key == Key.Right) {
			selectIndex = Math.Min(selectIndex + 1, editable.value.Length);
			selectTime = 0;
		}

		char write = (char) 0;
		if (editable.type == TextInputEditable.Type.Text) {
			if ((int) key >= 33 && (int) key <= 126) {
				write = Keys.ParseCharacter(
					(char) key,
					Keys.Modifier(Keymod.Shift),
					Keys.Modifier(Keymod.Caps)
				);
			}

			if (key == Key.Space) {
				write = ' ';
			}

			if (write != 0 && editable.bannedLetters.Contains(write)) {
				write = (char) 0;
			}
		}
		else {
			if (editable.type == TextInputEditable.Type.SignedFloat || editable.type == TextInputEditable.Type.SignedInteger) {
				if (key == Key.Minus) {
					write = '-';
				}
			}

			if (editable.type == TextInputEditable.Type.SignedFloat || editable.type == TextInputEditable.Type.UnsignedFloat) {
				if (key == Key.Period) {
					write = '.';
				}
			}

			if (key >= Key.Number0 && key <= Key.Number9) {
				write = (char) key;
			}
		}

		if (key == Key.Backspace) {
			write = (char) 1;
		}

		if (write == 0) return;
		if (write == 1) {
			selectTime = 0;
			if (selectIndex != 0) {
				editable.value = $"{editable.value[..(selectIndex - 1)]}{editable.value[selectIndex..]}";
				selectIndex--;
			}

			return;
		}
		editable.value = $"{editable.value[..selectIndex]}{write}{editable.value[selectIndex..]}";
		selectIndex++;
	}

	public static void FillRect(float x0, float y0, float x1, float y1) {
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.Vertex(x0, y0);
		Immediate.Vertex(x1, y0);
		Immediate.Vertex(x1, y1);
		Immediate.Vertex(x0, y1);
		Immediate.End();
	}

	public static void FillRect(Rect rect) {
		FillRect(rect.x0, rect.y0, rect.x1, rect.y1);
	}

	public static void FillRect(UVRect rect) {
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.TexCoord(rect.uv0.x, rect.uv0.y); Immediate.Vertex(rect.x0, rect.y1);
		Immediate.TexCoord(rect.uv1.x, rect.uv1.y); Immediate.Vertex(rect.x1, rect.y1);
		Immediate.TexCoord(rect.uv2.x, rect.uv2.y); Immediate.Vertex(rect.x1, rect.y0);
		Immediate.TexCoord(rect.uv3.x, rect.uv3.y); Immediate.Vertex(rect.x0, rect.y0);
		Immediate.End();
	}

	public static void StrokeRect(float x0, float y0, float x1, float y1) {
		Immediate.Begin(Immediate.PrimitiveType.LINE_LOOP);
		Immediate.Vertex(x0, y0);
		Immediate.Vertex(x1, y0);
		Immediate.Vertex(x1, y1);
		Immediate.Vertex(x0, y1);
		Immediate.End();
	}

	public static void StrokeRect(Rect rect) {
		StrokeRect(rect.x0, rect.y0, rect.x1, rect.y1);
	}

	public static void StrokeRect(UVRect rect) {
		StrokeRect(rect.x0, rect.y0, rect.x1, rect.y1);
	}

	public static void StrokeRect(float x0, float y0, float x1, float y1, float thickness) {
		Line(x0, y0, x1, y0, thickness);
		Line(x1, y0, x1, y1, thickness);
		Line(x1, y1, x0, y1, thickness);
		Line(x0, y1, x0, y0, thickness);
	}

	public static void StrokeRect(Rect rect, float thickness) {
		StrokeRect(rect.x0, rect.y0, rect.x1, rect.y1, thickness);
	}

	public static void Line(float x0, float y0, float x1, float y1) {
		Immediate.Begin(Immediate.PrimitiveType.LINES);
		Immediate.Vertex(x0, y0);
		Immediate.Vertex(x1, y1);
		Immediate.End();
	}

	public static void Line(Vector2 a, Vector2 b) {
		Immediate.Begin(Immediate.PrimitiveType.LINES);
		Immediate.Vertex(a.x, a.y);
		Immediate.Vertex(b.x, b.y);
		Immediate.End();
	}

	public static void Line(float x0, float y0, float x1, float y1, float thickness, uint fancyEnds = LINE_BOTH) {
		thickness /= 64f;

		float angle = MathF.Atan2(y1 - y0, x1 - x0);

		float a0x = x0 + Mathf.Cos(angle - Mathf.PI_2) * thickness;
		float a0y = y0 + Mathf.Sin(angle - Mathf.PI_2) * thickness;
		float b0x = x0 + Mathf.Cos(angle + Mathf.PI_2) * thickness;
		float b0y = y0 + Mathf.Sin(angle + Mathf.PI_2) * thickness;
		float a1x = x1 + Mathf.Cos(angle - Mathf.PI_2) * thickness;
		float a1y = y1 + Mathf.Sin(angle - Mathf.PI_2) * thickness;
		float b1x = x1 + Mathf.Cos(angle + Mathf.PI_2) * thickness;
		float b1y = y1 + Mathf.Sin(angle + Mathf.PI_2) * thickness;

		float c0x = x0 + Mathf.Cos(angle + Mathf.PI) * thickness;
		float c0y = y0 + Mathf.Sin(angle + Mathf.PI) * thickness;
		float c1x = x1 + Mathf.Cos(angle) * thickness;
		float c1y = y1 + Mathf.Sin(angle) * thickness;

		Immediate.Begin(Immediate.PrimitiveType.TRIANGLES);

		Immediate.Vertex(a0x, a0y);
		Immediate.Vertex(a1x, a1y);
		Immediate.Vertex(b0x, b0y);

		Immediate.Vertex(a1x, a1y);
		Immediate.Vertex(b1x, b1y);
		Immediate.Vertex(b0x, b0y);

		if ((fancyEnds & LINE_START) > 0) {
			Immediate.Vertex(a0x, a0y);
			Immediate.Vertex(b0x, b0y);
			Immediate.Vertex(c0x, c0y);
		}

		if ((fancyEnds & LINE_END) > 0) {
			Immediate.Vertex(a1x, a1y);
			Immediate.Vertex(b1x, b1y);
			Immediate.Vertex(c1x, c1y);
		}

		Immediate.End();
	}

	public static void Line(Vector2 a, Vector2 b, float thickness, uint fancyEnds = LINE_BOTH) {
		Line(a.x, a.y, b.x, b.y, thickness, fancyEnds);
	}

	public static void FillCircle(float x, float y, float radius, int resolution) {
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		for (int i = 0; i <= resolution; i++) {
			float r = i / (float) resolution * Mathf.TAU;

			if (i != 0) {
				Immediate.Vertex(x + Mathf.Cos(r) * radius, y + Mathf.Sin(r) * radius);
				Immediate.Vertex(x, y);
			}

			if (i != resolution) {
				Immediate.Vertex(x, y);
				Immediate.Vertex(x + Mathf.Cos(r) * radius, y + Mathf.Sin(r) * radius);
			}
		}
		Immediate.End();
	}

	public static void FillCircle(Vector2 pos, float radius, int resolution) {
		FillCircle(pos.x, pos.y, radius, resolution);
	}

	public static void StrokeCircle(float x, float y, float radius, int resolution) {
		Immediate.Begin(Immediate.PrimitiveType.LINE_LOOP);
		for (int i = 0; i < resolution; i++) {
			float r = i / (float) resolution * Mathf.TAU;
			Immediate.Vertex(x + Mathf.Cos(r) * radius, y + Mathf.Sin(r) * radius);
		}
		Immediate.End();
	}

	public static void StrokeCircle(Vector2 pos, float radius, int resolution) {
		StrokeCircle(pos.x, pos.y, radius, resolution);
	}

	public static ButtonResponse Button(Rect rect, ButtonMods? mods = null) {
		mods ??= new ButtonMods();
		bool highlight = CanClick && rect.Inside(Mouse.Pos);

		Immediate.Color(mods.disabled ? Themes.ButtonDisabled : Themes.Button);
		FillRect(rect);

		Immediate.Color(mods.disabled ? Themes.Border : ((highlight || mods.selected) ? Themes.BorderHighlight : Themes.Border));
		StrokeRect(rect);

		return new ButtonResponse(highlight && Mouse.JustLeft && !mods.disabled, highlight);
	}

	public static ButtonResponse TextButton(string text, Rect rect, TextButtonMods? mods = null) {
		mods ??= new TextButtonMods();
		bool can = CanClick;
		bool highlight = can && rect.Inside(Mouse.Pos);

		Immediate.Color(mods.disabled ? Themes.ButtonDisabled : Themes.Button);
		FillRect(rect);

		Immediate.Color(mods.textColor ?? (mods.disabled ? Themes.TextDisabled : (mods.selected ? Themes.TextHighlight : Themes.Text)));
		UI.font.Write(text, rect.CenterX, rect.CenterY, 0.03f, Font.Center.XY);

		Immediate.Color(mods.disabled ? Themes.Border : ((highlight || mods.selected) ? Themes.BorderHighlight : Themes.Border));
		StrokeRect(rect);

		return new ButtonResponse(highlight && Mouse.JustLeft && !mods.disabled, highlight);
	}

	public static ButtonResponse TextureButton(UVRect rect, TextureButtonMods? mods = null) {
		mods ??= new TextureButtonMods();
		bool can = CanClick;
		bool highlight = can && rect.Inside(Mouse.Pos);

		Immediate.Color(mods.disabled ? Themes.ButtonDisabled : Themes.Button);
		FillRect(rect);

		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(mods.texture);
		Immediate.Color(mods.textureColor);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);
		Immediate.TexCoord(rect.uv0.x, rect.uv0.y);
		Immediate.Vertex(Mathf.Lerp(rect.x1, rect.x0, 0.5f + mods.textureScale.x * 0.5f), Mathf.Lerp(rect.y1, rect.y0, 0.5f + mods.textureScale.y * 0.5f));
		Immediate.TexCoord(rect.uv1.x, rect.uv1.y);
		Immediate.Vertex(Mathf.Lerp(rect.x0, rect.x1, 0.5f + mods.textureScale.x * 0.5f), Mathf.Lerp(rect.y1, rect.y0, 0.5f + mods.textureScale.y * 0.5f));
		Immediate.TexCoord(rect.uv2.x, rect.uv2.y);
		Immediate.Vertex(Mathf.Lerp(rect.x0, rect.x1, 0.5f + mods.textureScale.x * 0.5f), Mathf.Lerp(rect.y0, rect.y1, 0.5f + mods.textureScale.y * 0.5f));
		Immediate.TexCoord(rect.uv3.x, rect.uv3.y);
		Immediate.Vertex(Mathf.Lerp(rect.x1, rect.x0, 0.5f + mods.textureScale.x * 0.5f), Mathf.Lerp(rect.y0, rect.y1, 0.5f + mods.textureScale.y * 0.5f));
		Immediate.End();
		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);

		Immediate.Color(mods.disabled ? Themes.Border : ((highlight || mods.selected) ? Themes.BorderHighlight : Themes.Border));
		StrokeRect(rect);

		return new ButtonResponse(highlight && Mouse.JustLeft && !mods.disabled, highlight);
	}

	public static ButtonResponse CheckBox(Rect rect, ref bool value) {
		ButtonResponse response = TextureButton(new UVRect(rect.x0, rect.y0, rect.x1, rect.y1).UV(value ? 0.75f : 0.5f, 0.25f, value ? 1f : 0.75f, 0.5f));

		if (response.clicked) {
			value = !value;
		}

		return response;
	}

	public static TextInputResponse TextInput(Rect rect, TextInputEditable editable, TextInputMods? mods = null) {
		mods ??= new TextInputMods();
		bool selected = CurrentEditable == editable;
		bool highlight = CanClick && rect.Inside(Mouse.Pos) && !mods.disabled;
		bool submitted = editable.submitted;
		editable.submitted = false;

		Immediate.Color(mods.disabled ? Themes.ButtonDisabled : Themes.Button);
		UI.FillRect(rect);

		Immediate.Color(mods.disabled ? Themes.TextDisabled : (selected ? Themes.TextHighlight : Themes.Text));
		UI.font.Write(editable.value, rect.x0 + 0.01f, rect.CenterY, 0.03f, Font.Center.Y);
		if (selected && UI.selectTime < 30) {
			float width = UI.font.Measure(editable.value[0..UI.selectIndex], 0.03f).x;
			Immediate.Begin(Immediate.PrimitiveType.LINES);
			Immediate.Vertex(rect.x0 + 0.012f + width, rect.CenterY - 0.02f);
			Immediate.Vertex(rect.x0 + 0.012f + width, rect.CenterY + 0.02f);
			Immediate.End();
		}

		Immediate.Color((!mods.disabled && (highlight || selected)) ? Themes.BorderHighlight : Themes.Border);
		UI.StrokeRect(rect);

		if (highlight && Mouse.JustLeft && !mods.disabled) {
			if (selected) {
				UI.UpdateTextInput(editable);
				UI.CurrentEditable = null;
				submitted = true;
			}
			else {
				UI.selectTime = 0;
				UI.CurrentEditable = editable;
				UI.selectIndex = editable.value.Length;
			}
		}

		return new TextInputResponse(UI.CurrentEditable == editable, highlight, submitted);
	}

	public static void CenteredTexture(Texture texture, float x, float y, float scale) {
		Immediate.Color(1f, 1f, 1f);
		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(texture);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);

		float ratio = (texture.width / (float) texture.height + 1f) * 0.5f;
		float uvx = 1f / ratio;
		float uvy = ratio;
		if (uvx < 1f) {
			uvy /= uvx;
			uvx = 1f;
		}
		if (uvy < 1f) {
			uvx /= uvy;
			uvy = 1f;
		}
		uvx *= 0.5f;
		uvy *= 0.5f;

		float centerX = x + 0.5f;
		float centerY = y - 0.5f;
		Immediate.TexCoord(0.5f - uvx, 0.5f + uvy); Immediate.Vertex(centerX - scale * 0.5f, centerY - scale * 0.5f);
		Immediate.TexCoord(0.5f + uvx, 0.5f + uvy); Immediate.Vertex(centerX + scale * 0.5f, centerY - scale * 0.5f);
		Immediate.TexCoord(0.5f + uvx, 0.5f - uvy); Immediate.Vertex(centerX + scale * 0.5f, centerY + scale * 0.5f);
		Immediate.TexCoord(0.5f - uvx, 0.5f - uvy); Immediate.Vertex(centerX - scale * 0.5f, centerY + scale * 0.5f);

		Immediate.End();
		Immediate.UseTexture(0);
		Program.gl.Disable(EnableCap.Blend);
	}

	public static void CenteredUV(Texture texture, ref UVRect rect) {
		float ratio = (texture.width / (float) texture.height + 1f) * 0.5f;
		float uvx = 1f / ratio;
		float uvy = ratio;
		if (uvx < 1f) {
			uvy /= uvx;
			uvx = 1f;
		}
		if (uvy < 1f) {
			uvx /= uvy;
			uvy = 1f;
		}
		uvx *= 0.5f;
		uvy *= 0.5f;

		rect.UV(0.5f - uvx, 0.5f + uvy, 0.5f + uvx, 0.5f - uvy);
	}

	// LATER: Add variant that also clips GL
	public static void Clip(Rect rect) {
		clipRect = rect;
	}

	public static void ClearClip() {
		clipRect = null;
	}


	public class ButtonMods {
		public bool disabled = false;
		public bool selected = false;
	}

	public class TextButtonMods : ButtonMods {
		public Color? textColor = null;

		public TextButtonMods(Color? textColor = null) {
			this.textColor = textColor;
		}
	}

	public class TextureButtonMods : ButtonMods {
		public Texture texture;
		public Color textureColor = Color.White;
		public Vector2 textureScale = Vector2.One;

		public TextureButtonMods(Texture? texture = null, Color? textureColor = null, Vector2? textureScale = null) {
			this.texture = texture ?? UI.ui;
			this.textureColor = textureColor ?? Color.White;
			this.textureScale = textureScale ?? Vector2.One;
		}
	}

	public class TextInputMods {
		public bool disabled = false;
	}

	public readonly struct ButtonResponse {
		public readonly bool clicked, hovered;

		public ButtonResponse(bool clicked, bool hovered) {
			this.clicked = clicked;
			this.hovered = hovered;
		}

		public static implicit operator bool(ButtonResponse button) {
			return button.clicked;
		}
	}

	public class TextInputResponse {
		public readonly bool focused, hovered, submitted;

		public TextInputResponse(bool focused, bool hovered, bool submitted) {
			this.focused = focused;
			this.hovered = hovered;
			this.submitted = submitted;
		}
	}


	private static Editable? CurrentEditable = null;
	private static int selectTime = 0;
	private static int selectIndex = 0;

	public static void Update() {
		if (CurrentEditable == null) {
			selectTime = 0;
			return;
		}

		selectTime = (selectTime + 1) % 60;
	}

	private static void UpdateTextInput(TextInputEditable edit) {
		bool neg = edit.value.Contains('-');

		edit.value = edit.type switch {
			TextInputEditable.Type.SignedFloat or TextInputEditable.Type.UnsignedFloat
				=> float.TryParse(edit.value, out float f) && (edit.type == TextInputEditable.Type.SignedFloat || !neg)
					? f.ToString($"F{edit.floatDecimalCount}")
					: $"0.{new string('0', edit.floatDecimalCount)}",

			TextInputEditable.Type.SignedInteger or TextInputEditable.Type.UnsignedInteger
				=> int.TryParse(edit.value, out int i) && (edit.type == TextInputEditable.Type.SignedInteger || !neg)
					? i.ToString()
					: "0",

			_ => edit.value
		};
	}

	public static void Delete(Editable editable) {
		if (CurrentEditable == editable) {
			CurrentEditable = null;
		}
	}

	public class Editable {
		public bool submitted;
		public bool Focused => UI.CurrentEditable == this;
	}

	public class TextInputEditable : Editable {
		public Type type;
		public int floatDecimalCount;
		public string value;
		public string bannedLetters = "";

		public TextInputEditable(Type type, string value, int floatDecimalCount = 1) {
			this.type = type;
			this.value = value;
			this.floatDecimalCount = floatDecimalCount;
		}

		public void SetValue(int value) {
			this.value = value.ToString();
		}

		public void SetValue(float value) {
			this.value = value.ToString($"F{this.floatDecimalCount}");
		}

		public void Submit() {
			if (UI.CurrentEditable != this) return;

			UI.UpdateTextInput(this);
			UI.Delete(this);
			this.submitted = true;
		}

		public enum Type {
			Text,
			UnsignedFloat,
			SignedFloat,
			UnsignedInteger,
			SignedInteger,
		}
	}
}