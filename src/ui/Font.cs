using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using Silk.NET.GLFW;
using Silk.NET.Maths;
using Silk.NET.OpenGL;

namespace FloodForge;

public class Font {
	private static uint _vao;
	private static uint _vbo;

	public static unsafe void Initialize() {
		_vao = Program.gl.GenVertexArray();
		_vbo = Program.gl.GenBuffer();

		Program.gl.BindVertexArray(_vao);
		Program.gl.BindBuffer(BufferTargetARB.ArrayBuffer, _vbo);

		uint stride = 4 * sizeof(float);
		Program.gl.EnableVertexAttribArray(0);
		Program.gl.VertexAttribPointer(0, 2, VertexAttribPointerType.Float, false, stride, (void*) 0);
		Program.gl.EnableVertexAttribArray(1);
		Program.gl.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, stride, (void*) (2 * sizeof(float)));

		Program.gl.BindVertexArray(0);
	}

	public readonly Dictionary<uint, Character> characters = [];
	public uint textureWidth;
	public uint textureHeight;
	public uint baseSize;
	public Texture texture;

	public float separationScale = 1f;

	public Font(string name) {
		this.LoadData("assets/fonts/" + name + ".txt");
		this.texture = Texture.Load("assets/fonts/" + name + ".png", minFilter: TextureMinFilter.Linear);
	}

	public void LoadData(string path) {
		if (!File.Exists(path)) {
			Console.WriteLine("File not found");
			return;
		}

		string[] lines = File.ReadAllLines(path);
		int idx = lines[1].IndexOf("base=");
		this.baseSize = uint.Parse(lines[1][(idx + 5)..lines[1].IndexOf(" ", idx)]);
		idx = lines[1].IndexOf("scaleW=");
		this.textureWidth = uint.Parse(lines[1][(idx + 7)..lines[1].IndexOf(" ", idx)]);
		idx = lines[1].IndexOf("scaleH=");
		this.textureHeight = uint.Parse(lines[1][(idx + 7)..lines[1].IndexOf(" ", idx)]);

		for (int i = 4; i < lines.Length; i++) {
			this.LoadCharacter(lines[i]);
		}
	}

	public void LoadCharacter(string line) {
		string[] parts = line.Split(' ');
		if (parts[0] != "char") return;

		this.characters.Add((uint) GetValue(parts[1]), new Character() {
			x = (uint) GetValue(parts[2]),
			y = (uint) GetValue(parts[3]),
			width = (uint) GetValue(parts[4]),
			height = (uint) GetValue(parts[5]),
			xOffset = GetValue(parts[6]),
			yOffset = GetValue(parts[7]),
			xAdvance = GetValue(parts[8])
		});
	}

	public static int GetValue(string pair) {
		return int.Parse(pair[(pair.IndexOf('=') + 1)..]);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public void Write(string text, float x, float y, float textSize) {
		this.Write(text, x, y, textSize, Center.None);
	}

	public Vector2 Measure(string text, float textSize) {
		float scale = textSize / this.baseSize;
		float width = 0;
		float currentLineWidth = 0;
		float height = this.baseSize * scale;

		foreach (char c in text) {
			if (c == '\n') {
				width = Math.Max(width, currentLineWidth);
				currentLineWidth = 0;
				height += this.baseSize * scale;
				continue;
			}

			if (!this.characters.TryGetValue(c, out var ch)) continue;
			currentLineWidth += ch.xAdvance * scale * this.separationScale;
		}
		
		return new Vector2(Math.Max(width, currentLineWidth), height);
	}

	public void Write(string text, float startX, float startY, float textSize, Center center) {
		float scale = textSize / this.baseSize;
		Vector2 size = this.Measure(text, textSize);

		if (center.HasFlag(Center.X)) {
			startX -= size.x / 2f;
		}
		if (center.HasFlag(Center.Y)) {
			startY += size.y / 2f;
		}

		float cursorX = startX;
		float cursorY = startY;

		bool reenableBlend = Program.gl.IsEnabled(EnableCap.Blend);
		Program.gl.Enable(EnableCap.Blend);
		Immediate.UseTexture(this.texture);
		Immediate.Begin(Immediate.PrimitiveType.QUADS);

		foreach (char c in text) {
			if (c == '\n') {
				cursorX = startX;
				cursorY += this.baseSize * scale;
				continue;
			}

			if (!this.characters.TryGetValue(c, out var ch)) continue;

			float x = cursorX + (ch.xOffset * scale);
			float y = cursorY - (ch.yOffset * scale);
			float w = ch.width * scale;
			float h = ch.height * scale;

			float u0 = (float) ch.x / this.textureWidth;
			float v0 = (float) ch.y / this.textureHeight;
			float u1 = (float) (ch.x + ch.width) / this.textureWidth;
			float v1 = (float) (ch.y + ch.height) / this.textureHeight;

			Immediate.TexCoord(u0, v0); Immediate.Vertex(x, y);
			Immediate.TexCoord(u1, v0); Immediate.Vertex(x + w, y);
			Immediate.TexCoord(u1, v1); Immediate.Vertex(x + w, y - h);
			Immediate.TexCoord(u0, v1); Immediate.Vertex(x, y - h);

			cursorX += ch.xAdvance * scale * this.separationScale;
		}

		Immediate.End();
		Immediate.UseTexture(0);

		if (!reenableBlend) Program.gl.Disable(EnableCap.Blend);
	}

	public struct Character {
		public uint x;
		public uint y;
		public uint width;
		public uint height;
		public int xOffset;
		public int yOffset;
		public int xAdvance;
	}

	// LATER: Make this not this ugly struct... This is horrible.
	public readonly struct Center {
		private readonly int _value;
		private Center(int value) => this._value = value;

		public static readonly Center None = new Center(0);
		public static readonly Center X = new Center(1);
		public static readonly Center Y = new Center(2);
		public static readonly Center XY = new Center(3);

		public static Center operator |(Center left, Center right) => new Center(left._value | right._value);
		public static Center operator &(Center left, Center right) => new Center(left._value & right._value);

		public bool HasFlag(Center flag) => (this._value & flag._value) == flag._value;
	}


	[StructLayout(LayoutKind.Sequential)]
	public struct TexturedVertex {
		public float x, y, u, v;

		public TexturedVertex(float x, float y, float u, float v) {
			this.x = x;
			this.y = y;
			this.u = u;
			this.v = v;
		}
	}
}