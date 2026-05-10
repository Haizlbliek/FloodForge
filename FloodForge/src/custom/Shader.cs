using System.Numerics;
using System.Text.RegularExpressions;

namespace Custom;

public class Shader {
	public readonly uint shader;
	private readonly string path;
	private readonly Dictionary<string, int> _uniformLocations = [];

	private Shader(uint shader, string path) {
		this.shader = shader;
		this.path = path;
		this.PopulateUniforms();
	}

	public void Use() => Custom.gl.UseProgram(this.shader);
	public void Dispose() => Custom.gl.DeleteProgram(this.shader);

	private void PopulateUniforms() {
		Custom.gl.GetProgram(this.shader, ProgramPropertyARB.ActiveUniforms, out int uniformCount);

		for (uint i = 0; i < uniformCount; i++) {
			string name = Custom.gl.GetActiveUniform(this.shader, i, out _, out _);
			int location = Custom.gl.GetUniformLocation(this.shader, name);

			if (location != -1) {
				this._uniformLocations[name] = location;
			}
		}
	}

	public int GetUniformLocation(string name) {
		if (this._uniformLocations.TryGetValue(name, out int location))
			return location;

		location = Custom.gl.GetUniformLocation(this.shader, name);
		this._uniformLocations[name] = location;
		if (location == -1) {
			Console.WriteLine($"Warning: Uniform '{name}' not found in shader {this.path}.");
		}

		return location;
	}

	public void SetUniform(string name, int value) => Custom.gl.Uniform1(this.GetUniformLocation(name), value);
	public void SetUniform(string name, uint value) => Custom.gl.Uniform1(this.GetUniformLocation(name), value);
	public void SetUniform(string name, float value) => Custom.gl.Uniform1(this.GetUniformLocation(name), value);
	public void SetUniform(string name, double value) => Custom.gl.Uniform1(this.GetUniformLocation(name), value);

	public void SetUniform(string name, int[] values) => Custom.gl.Uniform1(this.GetUniformLocation(name), (ReadOnlySpan<int>)values);
	public void SetUniform(string name, uint[] values) => Custom.gl.Uniform1(this.GetUniformLocation(name), (ReadOnlySpan<uint>)values);
	public void SetUniform(string name, float[] values) => Custom.gl.Uniform1(this.GetUniformLocation(name), (ReadOnlySpan<float>)values);

	public void SetUniform(string name, int x, int y) => Custom.gl.Uniform2(this.GetUniformLocation(name), x, y);
	public void SetUniform(string name, int x, int y, int z) => Custom.gl.Uniform3(this.GetUniformLocation(name), x, y, z);
	public void SetUniform(string name, int x, int y, int z, int w) => Custom.gl.Uniform4(this.GetUniformLocation(name), x, y, z, w);

	public void SetUniform(string name, uint x, uint y) => Custom.gl.Uniform2(this.GetUniformLocation(name), x, y);
	public void SetUniform(string name, uint x, uint y, uint z) => Custom.gl.Uniform3(this.GetUniformLocation(name), x, y, z);
	public void SetUniform(string name, uint x, uint y, uint z, uint w) => Custom.gl.Uniform4(this.GetUniformLocation(name), x, y, z, w);

	public void SetUniform(string name, float x, float y) => Custom.gl.Uniform2(this.GetUniformLocation(name), x, y);
	public void SetUniform(string name, float x, float y, float z) => Custom.gl.Uniform3(this.GetUniformLocation(name), x, y, z);
	public void SetUniform(string name, float x, float y, float z, float w) => Custom.gl.Uniform4(this.GetUniformLocation(name), x, y, z, w);

	public void SetUniform(string name, double x, double y) => Custom.gl.Uniform2(this.GetUniformLocation(name), x, y);
	public void SetUniform(string name, double x, double y, double z) => Custom.gl.Uniform3(this.GetUniformLocation(name), x, y, z);
	public void SetUniform(string name, double x, double y, double z, double w) => Custom.gl.Uniform4(this.GetUniformLocation(name), x, y, z, w);

	public void SetUniform(string name, Vector2 value) => Custom.gl.Uniform2(this.GetUniformLocation(name), value.x, value.y);
	public void SetUniform(string name, Vector3 value) => Custom.gl.Uniform3(this.GetUniformLocation(name), value.x, value.y, value.z);
	public void SetUniform(string name, Vector4 value) => Custom.gl.Uniform4(this.GetUniformLocation(name), value.x, value.y, value.z, value.w);
	public unsafe void SetUniform(string name, Matrix4x4 value, bool transpose = true) => Custom.gl.UniformMatrix4(this.GetUniformLocation(name), 1, transpose, (float*)&value);


	private static string PreprocessSource(string filePath, List<string> fileMap, bool isRoot = true) {
		if (!fileMap.Contains(filePath)) fileMap.Add(filePath);
		int fileIndex = fileMap.IndexOf(filePath);

		string[] lines = File.ReadAllLines(filePath);
		string? directory = Path.GetDirectoryName(filePath);
		List<string> processedLines = [];
		int startIndex = 0;

		if (isRoot) {
			foreach ((string line, int i) in lines.Select((l, i) => (l, i))) {
				if (Regex.IsMatch(line, @"^\s*#version")) {
					processedLines.Add(line);
					processedLines.Add($"#line {i + 2} {fileIndex}");
					startIndex = i + 1;
					break;
				}
			}
		} else {
			processedLines.Add($"#line 1 {fileIndex}");
		}

		for (int i = startIndex; i < lines.Length; i++) {
			Match match = Regex.Match(lines[i], @"^\s*#include\s+""(.+)""\s*$");
			if (match.Success) {
				string includePath = Path.Combine(directory ?? "", match.Groups[1].Value);
				if (!File.Exists(includePath)) includePath = Path.GetFullPath(match.Groups[1].Value);
				if (!File.Exists(includePath)) throw new FileNotFoundException($"Include not found: {match.Groups[1].Value}");

				processedLines.Add(PreprocessSource(includePath, fileMap, false));
				processedLines.Add($"#line {i + 2} {fileIndex}");
			} else {
				processedLines.Add(lines[i]);
			}
		}
		return string.Join("\n", processedLines);
	}

	private static uint CompileShader(string path, ShaderType type) {
		List<string> fileMap = [];
		string source = PreprocessSource(path, fileMap);
		
		uint shader = Custom.gl.CreateShader(type);
		Custom.gl.ShaderSource(shader, source);
		Custom.gl.CompileShader(shader);

		Custom.gl.GetShader(shader, ShaderParameterName.CompileStatus, out int status);
		if (status != (int)GLEnum.True) {
			string rawLog = Custom.gl.GetShaderInfoLog(shader);
			string formattedLog = FormatShaderLog(rawLog, fileMap);
			throw new Exception($"Error compiling {type} at {path}:\n{formattedLog}");
		}
		return shader;
	}

	private static string FormatShaderLog(string log, List<string> fileMap) {
		return Regex.Replace(log, @"(\d+)\((\d+)\)|(\d+):(\d+)", m => {
			int fileIdx = int.Parse(m.Groups[1].Success ? m.Groups[1].Value : m.Groups[3].Value);
			string lineNum = m.Groups[2].Success ? m.Groups[2].Value : m.Groups[4].Value;

			if (fileIdx >= 0 && fileIdx < fileMap.Count) {
				return $"{fileMap[fileIdx]}:{lineNum}";
			}
			return m.Value;
		});
	}

	public static Shader Load(string path, ShaderType type) {
		uint shader = CompileShader(path, type);
		uint program = Custom.gl.CreateProgram();
		
		Custom.gl.AttachShader(program, shader);
		Custom.gl.LinkProgram(program);
		
		Custom.gl.GetProgram(program, ProgramPropertyARB.LinkStatus, out int status);
		if (status != (int)GLEnum.True) throw new Exception(Custom.gl.GetProgramInfoLog(program));

		Custom.gl.DetachShader(program, shader);
		Custom.gl.DeleteShader(shader);

		return new Shader(program, path);
	}

	public static Shader Load(string vertexPath, string fragmentPath, string? geometryPath = null) {
		uint v = CompileShader(vertexPath, ShaderType.VertexShader);
		uint f = CompileShader(fragmentPath, ShaderType.FragmentShader);
		uint? g = geometryPath != null ? CompileShader(geometryPath, ShaderType.GeometryShader) : null;

		uint program = Custom.gl.CreateProgram();
		Custom.gl.AttachShader(program, v);
		Custom.gl.AttachShader(program, f);
		if (g.HasValue)
			Custom.gl.AttachShader(program, g.Value);

		Custom.gl.LinkProgram(program);
		Custom.gl.GetProgram(program, ProgramPropertyARB.LinkStatus, out int status);
		if (status != (int) GLEnum.True)
			throw new Exception(Custom.gl.GetProgramInfoLog(program));

		Custom.gl.DetachShader(program, v);
		Custom.gl.DetachShader(program, f);
		Custom.gl.DeleteShader(v);
		Custom.gl.DeleteShader(f);
		if (g.HasValue) {
			Custom.gl.DetachShader(program, g.Value);
			Custom.gl.DeleteShader(g.Value);
		}

		return new Shader(program, $"{vertexPath}@{fragmentPath}{(geometryPath == null ? "" : $"@{geometryPath}")}");
	}
}
