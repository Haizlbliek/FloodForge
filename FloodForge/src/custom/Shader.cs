namespace Custom;

public class Shader {
	public readonly uint shader;

	private Shader(uint shader) {
		this.shader = shader;
	}

	public void Dispose() {
		Custom.gl.DeleteProgram(this.shader);
	}

	public static Shader Load(string vertexPath, string fragmentPath, string? geometryPath = null) {
		uint vertexShader = Custom.gl.CreateShader(ShaderType.VertexShader);
		Custom.gl.ShaderSource(vertexShader, File.ReadAllText(vertexPath));
		Custom.gl.CompileShader(vertexShader);

		Custom.gl.GetShader(vertexShader, ShaderParameterName.CompileStatus, out int vertexStatus);
		if (vertexStatus != (int) GLEnum.True) {
			throw new Exception(Custom.gl.GetShaderInfoLog(vertexShader));
		}


		uint fragmentShader = Custom.gl.CreateShader(ShaderType.FragmentShader);
		Custom.gl.ShaderSource(fragmentShader, File.ReadAllText(fragmentPath));
		Custom.gl.CompileShader(fragmentShader);

		Custom.gl.GetShader(fragmentShader, ShaderParameterName.CompileStatus, out int fragmentStatus);
		if (fragmentStatus != (int) GLEnum.True) {
			throw new Exception(Custom.gl.GetShaderInfoLog(fragmentShader));
		}


		uint? geometryShader = null;
		if (geometryPath != null) {
			geometryShader = Custom.gl.CreateShader(ShaderType.GeometryShader);
			Custom.gl.ShaderSource(geometryShader.Value, File.ReadAllText(geometryPath));
			Custom.gl.CompileShader(geometryShader.Value);

			Custom.gl.GetShader(geometryShader.Value, ShaderParameterName.CompileStatus, out int geometryStatus);
			if (geometryStatus != (int) GLEnum.True) {
				throw new Exception(Custom.gl.GetShaderInfoLog(geometryShader.Value));
			}
		}


		uint shaderProgram = Custom.gl.CreateProgram();
		Custom.gl.AttachShader(shaderProgram, vertexShader);
		Custom.gl.AttachShader(shaderProgram, fragmentShader);
		if (geometryShader.HasValue) {
			Custom.gl.AttachShader(shaderProgram, geometryShader.Value);
		}
		Custom.gl.LinkProgram(shaderProgram);
		Custom.gl.GetProgram(shaderProgram, ProgramPropertyARB.LinkStatus, out int linkStatus);
		if (linkStatus != (int) GLEnum.True) {
			throw new Exception(Custom.gl.GetProgramInfoLog(shaderProgram));
		}

		Custom.gl.DetachShader(shaderProgram, vertexShader);
		Custom.gl.DetachShader(shaderProgram, fragmentShader);
		Custom.gl.DeleteShader(vertexShader);
		Custom.gl.DeleteShader(fragmentShader);

		if (geometryShader.HasValue) {
			Custom.gl.DetachShader(shaderProgram, geometryShader.Value);
			Custom.gl.DeleteShader(geometryShader.Value);
		}

		return new Shader(shaderProgram);
	}
}