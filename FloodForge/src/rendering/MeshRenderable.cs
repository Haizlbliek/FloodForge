namespace FloodForge.Rendering;

[StructLayout(LayoutKind.Sequential)]
public class MeshRenderable {
	public Mesh mesh;
	protected uint _vao = 0;
	protected uint _vbo = 0;
	protected uint _ebo = 0;
	protected Dictionary<string, int> shaderVariableLocations = [];
	protected Shader shaderToUse;

	public unsafe MeshRenderable(Mesh mesh, Shader shaderToUse, VertexAttributeInformation[]? vertexAttributeInformation = null, string[]? shaderVariableLocations = null) {
		this.mesh = mesh;
		if (this._vao == 0) {
			this._vao = Program.gl.GenVertexArray();
			this._vbo = Program.gl.GenBuffer();
			this._ebo = Program.gl.GenBuffer();
		}

		Span<Vertex> vertices = CollectionsMarshal.AsSpan(this.mesh.vertices);
		Span<uint> indices = CollectionsMarshal.AsSpan(this.mesh.indices);

		Program.gl.BindVertexArray(this._vao);

		Program.gl.BindBuffer(BufferTargetARB.ArrayBuffer, this._vbo);
		fixed (Vertex* ptr = vertices) {
			Program.gl.BufferData(BufferTargetARB.ArrayBuffer, (nuint) (vertices.Length * sizeof(Vertex)), ptr, BufferUsageARB.StaticDraw);
		}

		Program.gl.BindBuffer(BufferTargetARB.ElementArrayBuffer, this._ebo);
		fixed (uint* ptr = indices) {
			Program.gl.BufferData(BufferTargetARB.ElementArrayBuffer, (nuint) (indices.Length * sizeof(uint)), ptr, BufferUsageARB.StaticDraw);
		}
		
		if(vertexAttributeInformation != null) {
			foreach (VertexAttributeInformation item in vertexAttributeInformation) {
				Program.gl.VertexAttribPointer(item.index, item.size, item.type, item.normalised, item.stride, item.pointer);
				Program.gl.EnableVertexAttribArray(item.index);
			}
		}
		
		this.shaderToUse = shaderToUse;
		Program.gl.UseProgram(shaderToUse);
		if(shaderVariableLocations != null) {
			foreach (string name in shaderVariableLocations) {
				this.shaderVariableLocations.Add(name, Program.gl.GetUniformLocation(shaderToUse, name));
			}
		}
		Program.gl.UseProgram(0);

		Program.gl.BindVertexArray(0);
		Program.gl.BindBuffer(BufferTargetARB.ArrayBuffer, 0);
		Program.gl.BindBuffer(BufferTargetARB.ElementArrayBuffer, 0);
	}

	public void UniformMatrix4(string name, bool transpose, ReadOnlySpan<float> value)
		=> Program.gl.UniformMatrix4(this.shaderVariableLocations[name], transpose, value);
	public void Uniform4(string name, float x, float y, float z, float w)
		=> Program.gl.Uniform4(this.shaderVariableLocations[name], x, y, z, w);
	public void Uniform1(string name, float val)
		=> Program.gl.Uniform1(this.shaderVariableLocations[name], val);

	public void PreDraw() {
		Program.gl.BindVertexArray(this._vao);
		Program.gl.UseProgram(this.shaderToUse);
	}

	public unsafe void DoDraw() {
		Program.gl.DrawElements(PrimitiveType.Triangles, (uint) this.mesh.indices.Count, DrawElementsType.UnsignedInt, (void*) 0);

		Program.gl.BindVertexArray(0);
		Program.gl.UseProgram(0);
	}

	// REVIEW - automatically calculate pointer based on int size (test if that can be reasonably derived first)
	public unsafe struct VertexAttributeInformation (uint index, int size, VertexAttribPointerType type, bool normalised, uint stride, void* pointer){
		public uint index = index;
		public int size = size;
		public VertexAttribPointerType type = type;
		public bool normalised = normalised;
		public uint stride = stride;
		public void* pointer = pointer;
	}
}

public readonly struct Vertex {
	public readonly float x, y;
	public readonly float r, g, b, a;

	public Vertex(float x, float y, Color color) {
		this.x = x;
		this.y = y;
		this.r = color.r;
		this.g = color.g;
		this.b = color.b;
		this.a = color.a;
	}

	public Vertex(float x, float y, float r, float g, float b, float a = 1f) {
		this.x = x;
		this.y = y;
		this.r = r;
		this.g = g;
		this.b = b;
		this.a = a;
	}
}