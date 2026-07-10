namespace FloodForge.Rendering;

public class Mesh() {
	public uint currentIndex = 0;
	public List<Vertex> vertices = [];
	public List<uint> indices = [];

	public void Clear() {
		this.vertices.Clear();
		this.indices.Clear();
		this.currentIndex = 0;
	}

	public void AddQuad(float xPos, float yPos, Themes.ThemeColor theme) {
		this.AddQuad(new Vector2(xPos, yPos), Vector2.One, theme);
	}

	public void AddQuad(Vector2 centerPosition, Themes.ThemeColor theme) {
		this.AddQuad(centerPosition, Vector2.One, theme);
	}

	public void AddQuad(float xPos, float yPos, float scale, Themes.ThemeColor theme) {
		this.AddQuad(new Vector2(xPos, yPos), Vector2.One * scale, theme);
	}

	public void AddQuad(Vector2 centerPosition, float scale, Themes.ThemeColor theme) {
		this.AddQuad(centerPosition, Vector2.One * scale, theme);
	}

	public void AddQuad(float xPos, float yPos, Vector2 scale, Themes.ThemeColor theme) {
		this.AddQuad(new(xPos, yPos), scale, theme);
	}

	public void AddQuad(Vector2 centerPosition, Vector2 scale, Themes.ThemeColor theme) {
		float x0 = centerPosition.x - (scale.x / 2);
		float x1 = centerPosition.x + (scale.x / 2);
		float y0 = centerPosition.y + (scale.y / 2);
		float y1 = centerPosition.y - (scale.y / 2);
		this.AddQuad(
			new Vertex(x0, y0, theme),
			new Vertex(x1, y0, theme),
			new Vertex(x1, y1, theme),
			new Vertex(x0, y1, theme)
		);
	}

	public void AddQuad(Vertex[] vertices) {
		this.AddQuad(vertices[0], vertices[1], vertices[2], vertices[3]);
	}

	public void AddQuad(Vertex a, Vertex b, Vertex c, Vertex d) {
		this.vertices.Add(a);
		this.vertices.Add(b);
		this.vertices.Add(c);
		this.vertices.Add(d);
		this.indices.Add(this.currentIndex + 0);
		this.indices.Add(this.currentIndex + 1);
		this.indices.Add(this.currentIndex + 2);
		this.indices.Add(this.currentIndex + 2);
		this.indices.Add(this.currentIndex + 3);
		this.indices.Add(this.currentIndex + 0);
		this.currentIndex += 4;
	}

	public void AddTriangle(Vertex a, Vertex b, Vertex c) {
		this.vertices.Add(a);
		this.vertices.Add(b);
		this.vertices.Add(c);
		this.indices.Add(this.currentIndex + 0);
		this.indices.Add(this.currentIndex + 1);
		this.indices.Add(this.currentIndex + 2);
		this.currentIndex += 3;
	}
}