using Cstm = Custom.Custom;

namespace FloodForge;

public static class Immediate {
	private const uint MAX_VERTICES = 1024;
	private const uint MAX_INDICES = 1024;
	private const int MATRIX_COUNT = 2;

	public enum PrimitiveType {
		POINTS,
		LINES,
		LINE_STRIP,
		LINE_LOOP,
		TRIANGLES,
		TRIANGLE_FAN,
		QUADS,
	}

	public enum EMatrixMode {
		PROJECTION = 0,
		MODELVIEW = 1,
	}

	private struct VertexData {
		public float x, y, z, u, v, r, g, b, a;
	}

	private struct MatrixStack {
		public Stack<Matrix4X4<float>> stack;
		public Matrix4X4<float> cur;
	}

	private static class DrawState {
		public static VertexData[] batchVertices = new VertexData[MAX_VERTICES];
		public static uint[] batchIndices = new uint[MAX_INDICES];

		public static uint vertexCount = 0;
		public static uint indexCount = 0;
		public static uint currentIndex = 0;

		public static uint vertexArray = 0;
		public static uint vtxBuffer = 0;
		public static uint idxBuffer = 0;
		public static uint gpuProgram = 0;
		public static int mvpUniform = 0;
		public static int texUniform = 0;
		public static uint activeProgram = 0;

		public static uint placeholderTexture = 0;
		public static uint activeTexture = 0;

		public static PrimitiveType curPrim = PrimitiveType.POINTS;
		public static GLEnum curGlPrim = GLEnum.Points;
		public static bool dirty = false;
		public static bool drawActive = false;

		public static MatrixStack[] mats = new MatrixStack[MATRIX_COUNT];
		public static int matIdx = 0;

		public static float u = 0, v = 0, r = 1, g = 1, b = 1, a = 1;

		public static VertexData[] verts = new VertexData[4];
		public static uint active_idx0 = 0;
		public static uint active_idx1 = 0;
	}

	public static bool flushOnEnd = true;

	public static unsafe void Initialize() {
		DrawState.vertexCount = 0;
		DrawState.indexCount = 0;
		DrawState.currentIndex = 0;
		DrawState.dirty = false;
		DrawState.drawActive = false;

		for (int i = 0; i < MATRIX_COUNT; i++) {
			DrawState.mats[i].stack = new Stack<Matrix4X4<float>>();
			DrawState.mats[i].cur = Matrix4X4<float>.Identity;
		}

		Span<uint> vao = stackalloc uint[1];
		Cstm.gl.GenVertexArrays(vao);
		DrawState.vertexArray = vao[0];
		Cstm.gl.BindVertexArray(DrawState.vertexArray);

		Span<uint> buffers = stackalloc uint[2];
		Cstm.gl.GenBuffers(buffers);
		DrawState.vtxBuffer = buffers[0];
		DrawState.idxBuffer = buffers[1];

		Cstm.gl.BindBuffer(GLEnum.ArrayBuffer, DrawState.vtxBuffer);
		Cstm.gl.BufferData(GLEnum.ArrayBuffer, (nuint)(MAX_VERTICES * sizeof(VertexData)), null, GLEnum.StreamDraw);

		Cstm.gl.BindBuffer(GLEnum.ElementArrayBuffer, DrawState.idxBuffer);
		Cstm.gl.BufferData(GLEnum.ElementArrayBuffer, MAX_INDICES * sizeof(uint), null, GLEnum.StreamDraw);
		Cstm.gl.VertexAttribPointer(0, 3, GLEnum.Float, false, (uint)sizeof(VertexData), 0);
		Cstm.gl.EnableVertexAttribArray(0);
		Cstm.gl.VertexAttribPointer(1, 2, GLEnum.Float, false, (uint)sizeof(VertexData), (void*)(3 * sizeof(float)));
		Cstm.gl.EnableVertexAttribArray(1);
		Cstm.gl.VertexAttribPointer(2, 4, GLEnum.Float, false, (uint)sizeof(VertexData), (void*)(5 * sizeof(float)));
		Cstm.gl.EnableVertexAttribArray(2);

		Cstm.gl.BindVertexArray(0);
		string vertexShaderSource = @"
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTexCoord;
layout(location=2) in vec4 aColor;
out vec4 color;
out vec2 texCoord;
uniform mat4 uMvp;
void main() {
	gl_Position = uMvp * vec4(aPos, 1.0);
	color = aColor;
	texCoord = aTexCoord;
}";

		string fragmentShaderSource = @"
#version 330 core
in vec4 color;
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
void main() {
	fragColor = texture(uTexture, texCoord) * color;
}";

		uint vtxShader = Cstm.gl.CreateShader(ShaderType.VertexShader);
		Cstm.gl.ShaderSource(vtxShader, vertexShaderSource);
		Cstm.gl.CompileShader(vtxShader);

		Cstm.gl.GetShader(vtxShader, ShaderParameterName.CompileStatus, out int vtxStatus);
		if (vtxStatus == 0) {
			string log = Cstm.gl.GetShaderInfoLog(vtxShader);
			Logger.Error($"Vertex shader compilation failed: {log}");
		}

		uint fragShader = Cstm.gl.CreateShader(ShaderType.FragmentShader);
		Cstm.gl.ShaderSource(fragShader, fragmentShaderSource);
		Cstm.gl.CompileShader(fragShader);

		Cstm.gl.GetShader(fragShader, ShaderParameterName.CompileStatus, out int fragStatus);
		if (fragStatus == 0) {
			string log = Cstm.gl.GetShaderInfoLog(fragShader);
			Logger.Error($"Fragment shader compilation failed: {log}");
		}

		if (vtxStatus != 0 && fragStatus != 0) {
			DrawState.gpuProgram = Cstm.gl.CreateProgram();
			Cstm.gl.AttachShader(DrawState.gpuProgram, vtxShader);
			Cstm.gl.AttachShader(DrawState.gpuProgram, fragShader);
			Cstm.gl.LinkProgram(DrawState.gpuProgram);

			Cstm.gl.GetProgram(DrawState.gpuProgram, GLEnum.LinkStatus, out int linkStatus);
			if (linkStatus == 0) {
				string log = Cstm.gl.GetProgramInfoLog(DrawState.gpuProgram);
				Logger.Error($"Shader linking failed: {log}");
			}

			DrawState.mvpUniform = Cstm.gl.GetUniformLocation(DrawState.gpuProgram, "uMvp");
			DrawState.texUniform = Cstm.gl.GetUniformLocation(DrawState.gpuProgram, "uTexture");
			DrawState.activeProgram = DrawState.gpuProgram;
		}

		Cstm.gl.DeleteShader(vtxShader);
		Cstm.gl.DeleteShader(fragShader);

		uint whitePixel = 0xFFFFFFFF;
		Span<uint> placeholderTex = stackalloc uint[1];
		Cstm.gl.GenTextures(placeholderTex);
		DrawState.placeholderTexture = placeholderTex[0];

		Cstm.gl.BindTexture(GLEnum.Texture2D, DrawState.placeholderTexture);
		int wrapS = (int) GLEnum.ClampToEdge;
		Cstm.gl.TexParameterI(GLEnum.Texture2D, GLEnum.TextureWrapS, ref wrapS);
		int wrapT = (int) GLEnum.ClampToEdge;
		Cstm.gl.TexParameterI(GLEnum.Texture2D, GLEnum.TextureWrapT, ref wrapT);
		int magFilter = (int) GLEnum.Nearest;
		Cstm.gl.TexParameterI(GLEnum.Texture2D, GLEnum.TextureMagFilter, ref magFilter);
		int minFilter = (int) GLEnum.Nearest;
		Cstm.gl.TexParameterI(GLEnum.Texture2D, GLEnum.TextureMinFilter, ref minFilter);

		Span<uint> pixelData = [whitePixel];
		Cstm.gl.TexImage2D(GLEnum.Texture2D, 0, InternalFormat.Rgba, 1, 1, 0, GLEnum.Rgba, GLEnum.UnsignedByte, pixelData);

		DrawState.activeTexture = DrawState.placeholderTexture;

		DrawState.matIdx = (int)EMatrixMode.PROJECTION;
	}

	public static void Cleanup() {
		Span<uint> vao = [DrawState.vertexArray];
		Cstm.gl.DeleteVertexArrays(vao);

		Span<uint> buffers = [DrawState.vtxBuffer, DrawState.idxBuffer];
		Cstm.gl.DeleteBuffers(buffers);

		Span<uint> texture = [DrawState.placeholderTexture];
		Cstm.gl.DeleteTextures(texture);

		Span<uint> program = [DrawState.gpuProgram];
		Cstm.gl.DeleteProgram(DrawState.gpuProgram);

		DrawState.vertexArray = 0;
		DrawState.vtxBuffer = 0;
		DrawState.idxBuffer = 0;
		DrawState.gpuProgram = 0;
		DrawState.placeholderTexture = 0;
	}

	public static unsafe void Flush() {
		DrawState.dirty = false;
		if (DrawState.vertexCount == 0) return;

		Cstm.gl.BindVertexArray(DrawState.vertexArray);

		Cstm.gl.BindBuffer(GLEnum.ArrayBuffer, DrawState.vtxBuffer);
		unsafe {
			fixed (VertexData* verts = DrawState.batchVertices) {
				Cstm.gl.BufferSubData(GLEnum.ArrayBuffer, 0, (nuint)(DrawState.vertexCount * sizeof(VertexData)), verts);
			}
		}

		Cstm.gl.BindBuffer(GLEnum.ElementArrayBuffer, DrawState.idxBuffer);
		unsafe {
			fixed (uint* indices = DrawState.batchIndices) {
				Cstm.gl.BufferSubData(GLEnum.ElementArrayBuffer, 0, DrawState.indexCount * sizeof(uint), indices);
			}
		}

		Cstm.gl.UseProgram(DrawState.activeProgram);

		if (flushOnEnd) {
			Matrix4X4<float> modelView = DrawState.mats[(int)EMatrixMode.MODELVIEW].cur;
			Matrix4X4<float> proj = DrawState.mats[(int)EMatrixMode.PROJECTION].cur;
			Matrix4X4<float> mvp = proj * modelView;

			float[] mvpArray = mvp.ToArray();
			unsafe {
				fixed (float* mvpPtr = mvpArray) {
					Cstm.gl.UniformMatrix4(DrawState.mvpUniform, 1, false, mvpPtr);
				}
			}
		}
		else {
			float[] projArray = DrawState.mats[(int)EMatrixMode.PROJECTION].cur.ToArray();
			unsafe {
				fixed (float* projPtr = projArray) {
					Cstm.gl.UniformMatrix4(DrawState.mvpUniform, 1, false, projPtr);
				}
			}
		}
		Cstm.gl.ActiveTexture(GLEnum.Texture0);
		Cstm.gl.BindTexture(GLEnum.Texture2D, DrawState.activeTexture);
		Cstm.gl.Uniform1(DrawState.texUniform, 0);
		Cstm.gl.DrawElements(DrawState.curGlPrim, DrawState.indexCount, GLEnum.UnsignedInt, (void*)0);

		DrawState.vertexCount = 0;
		DrawState.indexCount = 0;
		DrawState.currentIndex = 0;
	}

	public static void UseTexture(uint textureId) {
		if (textureId == 0) {
			DrawState.activeTexture = DrawState.placeholderTexture;
		}
		else {
			DrawState.activeTexture = textureId;
		}
	}

	public static void UseTexture(Texture? texture) {
		UseTexture(texture?._id ?? 0);
	}

	public static void UseProgram(uint programId) {
		if (programId == 0) {
			DrawState.activeProgram = DrawState.gpuProgram;
			Cstm.gl.UseProgram(0);
		}
		else {
			DrawState.activeProgram = programId;
			Cstm.gl.UseProgram(programId);
		}
	}

	private static void CheckCapacity(uint newVertices, uint numIndices) {
		if (DrawState.vertexCount + newVertices >= MAX_VERTICES || DrawState.indexCount + numIndices >= MAX_INDICES) {
			Flush();
		}
	}

	private static void BeginDraw(uint requiredCapacity, uint numIndices, GLEnum newDrawMode) {
		CheckCapacity(requiredCapacity, numIndices);

		if (DrawState.curGlPrim != newDrawMode) {
			Flush();
			DrawState.curGlPrim = newDrawMode;
		}
	}

	private static void PushVertex(VertexData vtxData, bool cpuTransform) {
		if (cpuTransform) {
			Matrix4X4<float> mat = DrawState.mats[(int)EMatrixMode.MODELVIEW].cur;

			float x = vtxData.x;
			float y = vtxData.y;
			float z = 0.0f;
			float w = 1.0f;

			float x0 = x * mat.M11 + y * mat.M21 + z * mat.M31 + w * mat.M41;
			float y0 = x * mat.M12 + y * mat.M22 + z * mat.M32 + w * mat.M42;
			float z0 = x * mat.M13 + y * mat.M23 + z * mat.M33 + w * mat.M43;
			float w0 = x * mat.M14 + y * mat.M24 + z * mat.M34 + w * mat.M44;

			vtxData.x = x0 / w0;
			vtxData.y = y0 / w0;
			vtxData.z = z0;
		}

		DrawState.batchVertices[DrawState.vertexCount++] = vtxData;
	}

	private static void PushIndex(uint idx) {
		DrawState.batchIndices[DrawState.indexCount++] = idx;
	}

	private static void ProcessVertex(Vector3D<float> pos, bool cpuTransform) {
		if (!DrawState.drawActive) {
			Logger.Error("ERROR: Immediate.Vertex called before Immediate.Begin.");
			return;
		}

		VertexData curVert = new VertexData {
			x = pos.X,
			y = pos.Y,
			z = pos.Z,
			u = DrawState.u,
			v = DrawState.v,
			r = DrawState.r,
			g = DrawState.g,
			b = DrawState.b,
			a = DrawState.a
		};

		DrawState.verts[DrawState.active_idx0++] = curVert;

		switch (DrawState.curPrim) {
			case PrimitiveType.POINTS:
				BeginDraw(1, 1, GLEnum.Points);
				PushVertex(DrawState.verts[0], cpuTransform);
				PushIndex(DrawState.currentIndex++);
				DrawState.active_idx0 = 0;
				break;

			case PrimitiveType.LINES:
				if (DrawState.active_idx0 >= 2) {
					BeginDraw(2, 2, GLEnum.Lines);

					PushVertex(DrawState.verts[0], cpuTransform);
					PushVertex(DrawState.verts[1], cpuTransform);
					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex++);

					DrawState.active_idx0 = 0;
				}
				break;

			case PrimitiveType.LINE_STRIP:
			case PrimitiveType.LINE_LOOP:
				if (DrawState.active_idx0 == 1) {
					BeginDraw(1, 0, GLEnum.Lines);

					DrawState.active_idx1 = DrawState.currentIndex;
					PushVertex(DrawState.verts[0], cpuTransform);
				}
				else {
					BeginDraw(1, 2, GLEnum.Lines);

					PushVertex(DrawState.verts[1], cpuTransform);
					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex);
					DrawState.active_idx0--;
				}
				break;

			case PrimitiveType.TRIANGLES:
				if (DrawState.active_idx0 >= 3) {
					BeginDraw(3, 3, GLEnum.Triangles);

					PushVertex(DrawState.verts[0], cpuTransform);
					PushVertex(DrawState.verts[1], cpuTransform);
					PushVertex(DrawState.verts[2], cpuTransform);

					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex++);

					DrawState.active_idx0 = 0;
				}
				break;

			case PrimitiveType.TRIANGLE_FAN:
				if (DrawState.active_idx0 > 2) {
					BeginDraw(3, 3, GLEnum.Triangles);
					PushVertex(DrawState.verts[0], cpuTransform);
					PushVertex(DrawState.verts[1], cpuTransform);
					PushVertex(DrawState.verts[2], cpuTransform);
					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex++);
					PushIndex(DrawState.currentIndex++);
					DrawState.verts[1] = DrawState.verts[2];

					DrawState.active_idx0 = 2;
				}
				break;

			case PrimitiveType.QUADS:
				if (DrawState.active_idx0 >= 4) {
					BeginDraw(4, 6, GLEnum.Triangles);

					PushVertex(DrawState.verts[0], cpuTransform);
					PushVertex(DrawState.verts[1], cpuTransform);
					PushVertex(DrawState.verts[2], cpuTransform);
					PushVertex(DrawState.verts[3], cpuTransform);

					uint idx = DrawState.currentIndex;
					PushIndex(idx + 0);
					PushIndex(idx + 1);
					PushIndex(idx + 2);
					PushIndex(idx + 2);
					PushIndex(idx + 3);
					PushIndex(idx + 0);
					DrawState.currentIndex = idx + 4;

					DrawState.active_idx0 = 0;
				}
				break;
		}
	}

	public static void Begin(PrimitiveType primType) {
		if (DrawState.drawActive) {
			Logger.Error("ERROR: Immediate.Begin called when Immediate operation was already active.");
			return;
		}

		if (DrawState.dirty)
			Flush();

		DrawState.drawActive = true;
		DrawState.active_idx0 = 0;
		DrawState.curPrim = primType;
	}

	public static void Vertex(Vector3D<float> pos) {
		ProcessVertex(pos, !flushOnEnd);
	}

	public static void Vertex(float x, float y, float z) {
		Vertex(new Vector3D<float>(x, y, z));
	}

	public static void Vertex(float x, float y) {
		Vertex(new Vector3D<float>(x, y, 0.0f));
	}

	public static void TexCoord(Vector2D<float> uv) {
		DrawState.u = uv.X;
		DrawState.v = uv.Y;
	}

	public static void TexCoord(float u, float v) {
		DrawState.u = u;
		DrawState.v = v;
	}

	public static void Color(Vector4D<float> col) {
		DrawState.r = col.X;
		DrawState.g = col.Y;
		DrawState.b = col.Z;
		DrawState.a = col.W;
	}

	public static void Color(float r, float g, float b, float a) {
		DrawState.r = r;
		DrawState.g = g;
		DrawState.b = b;
		DrawState.a = a;
	}

	public static void Color(float r, float g, float b) {
		Color(r, g, b, 1.0f);
	}

	public static void Color(Color color) {
		Color(color.r, color.g, color.b, color.a);
	}

	public static void Alpha(float alpha) {
		DrawState.a = alpha;
	}

	public static void End() {
		if (!DrawState.drawActive) {
			Logger.Error("ERROR: Immediate.End called without an active Immediate operation.");
			return;
		}

		if (DrawState.curPrim == PrimitiveType.LINE_LOOP) {
			BeginDraw(1, 2, GLEnum.Lines);
			PushIndex(DrawState.currentIndex++);
			PushIndex(DrawState.active_idx1);
		}

		if (flushOnEnd || DrawState.dirty)
			Flush();

		DrawState.drawActive = false;
	}
	private static void MatrixChange() {
		if (DrawState.matIdx == (int)EMatrixMode.PROJECTION)
			DrawState.dirty = true;
	}

	private static void ValidateMatrixOperation() {
		if (DrawState.drawActive) {
			Logger.Error("ERROR: Attempt to modify render matrix with a Immediate operation already active.");
		}
	}

	public static Matrix4X4<float> GetMatrix(EMatrixMode mode) {
		return DrawState.mats[(int)mode].cur;
	}

	public static void MatrixMode(EMatrixMode mode) {
		ValidateMatrixOperation();
		DrawState.matIdx = (int)mode;
		MatrixChange();
	}

	public static void LoadIdentity() {
		ValidateMatrixOperation();
		DrawState.mats[DrawState.matIdx].cur = Matrix4X4<float>.Identity;
		MatrixChange();
	}

	public static void LoadMatrix(Matrix4X4<float> mat) {
		ValidateMatrixOperation();
		DrawState.mats[DrawState.matIdx].cur = mat;
		MatrixChange();
	}

	public static void MultMatrix(Matrix4X4<float> mat) {
		ValidateMatrixOperation();
		DrawState.mats[DrawState.matIdx].cur = mat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}

	public static void PopMatrix() {
		ValidateMatrixOperation();
		DrawState.mats[DrawState.matIdx].cur = DrawState.mats[DrawState.matIdx].stack.Pop();
		MatrixChange();
	}

	public static void PushMatrix() {
		ValidateMatrixOperation();
		DrawState.mats[DrawState.matIdx].stack.Push(DrawState.mats[DrawState.matIdx].cur);
		MatrixChange();
	}

	public static void Rotate(float angle, Vector3D<float> axis) {
		ValidateMatrixOperation();
		Matrix4X4<float> rotMat = Matrix4X4.CreateFromAxisAngle(axis, angle);
		DrawState.mats[DrawState.matIdx].cur = rotMat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}

	public static void Rotate(float angle, float x, float y, float z) {
		Rotate(angle, new Vector3D<float>(x, y, z));
	}

	public static void Translate(Vector3D<float> vec) {
		ValidateMatrixOperation();
		Matrix4X4<float> transMat = Matrix4X4.CreateTranslation(vec);
		DrawState.mats[DrawState.matIdx].cur = transMat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}

	public static void Translate(Vector2D<float> delta) {
		Translate(new Vector3D<float>(delta.X, delta.Y, 0.0f));
	}

	public static void Translate(float x, float y) {
		Translate(new Vector2D<float>(x, y));
	}

	public static void Translate(float x, float y, float z) {
		Translate(new Vector3D<float>(x, y, z));
	}

	public static void Scale(Vector3D<float> vec) {
		ValidateMatrixOperation();
		Matrix4X4<float> scaleMat = Matrix4X4.CreateScale(vec);
		DrawState.mats[DrawState.matIdx].cur = scaleMat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}

	public static void Scale(Vector2D<float> factor) {
		Scale(new Vector3D<float>(factor.X, factor.Y, 1.0f));
	}

	public static void Scale(float x, float y) {
		Scale(new Vector2D<float>(x, y));
	}

	public static void Scale(float x, float y, float z) {
		Scale(new Vector3D<float>(x, y, z));
	}

	public static void Ortho(float x, float y, float scaleX, float scaleY) {
		Ortho(-scaleX + x, scaleX + x, -scaleY + y, scaleY + y, 0f, 1f);
	}

	public static void Ortho(float left, float right, float bottom, float top, float near, float far) {
		ValidateMatrixOperation();
		Matrix4X4<float> mat = Matrix4X4.CreateOrthographic(right - left, top - bottom, near, far);
		mat.M41 = -(right + left) / (right - left);
		mat.M42 = -(top + bottom) / (top - bottom);
		mat.M43 = -(far + near) / (far - near);

		DrawState.mats[DrawState.matIdx].cur = mat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}

	public static void Perspective(float fov, float aspect, float near, float far) {
		ValidateMatrixOperation();
		Matrix4X4<float> mat = Matrix4X4.CreatePerspectiveFieldOfView(fov, aspect, near, far);
		DrawState.mats[DrawState.matIdx].cur = mat * DrawState.mats[DrawState.matIdx].cur;
		MatrixChange();
	}
}
