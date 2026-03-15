using System;
using System.Collections.Generic;
using Silk.NET.Maths;
using Silk.NET.OpenGL;

namespace Custom;

public static class Custom {
	public static GL gl = null!;

	public static void Initialize(GL gl) {
		Custom.gl = gl;
	}

	public static T[] ToArray<T>(this Matrix4X4<T> matrix) where T : unmanaged, IFormattable, IEquatable<T>, IComparable<T> {
		return [
			matrix.M11, matrix.M12, matrix.M13, matrix.M14,
			matrix.M21, matrix.M22, matrix.M23, matrix.M24,
			matrix.M31, matrix.M32, matrix.M33, matrix.M34,
			matrix.M41, matrix.M42, matrix.M43, matrix.M44
		];
	}

	public static IEnumerator<T> GetEnumerator<T>(this Matrix4X4<T> matrix) where T : unmanaged, IFormattable, IEquatable<T>, IComparable<T> {
		yield return matrix.M11;
		yield return matrix.M12;
		yield return matrix.M13;
		yield return matrix.M14;
		yield return matrix.M21;
		yield return matrix.M22;
		yield return matrix.M23;
		yield return matrix.M24;
		yield return matrix.M31;
		yield return matrix.M32;
		yield return matrix.M33;
		yield return matrix.M34;
		yield return matrix.M41;
		yield return matrix.M42;
		yield return matrix.M43;
		yield return matrix.M44;
	}

	public static void UseProgram(this Silk.NET.OpenGL.GL gl, Shader program) {
		gl.UseProgram(program.shader);
	}

	public static int GetUniformLocation(this Silk.NET.OpenGL.GL gl, Shader program, string name) {
		return gl.GetUniformLocation(program.shader, name);
	}

	public static uint GetAttribLocation(this Silk.NET.OpenGL.GL gl, Shader program, string name) {
		return (uint)gl.GetAttribLocation(program.shader, name);
	}

	public static void Toggle<T>(this HashSet<T> set, T value) {
		if (set.Contains(value)) {
			set.Remove(value);
		} else {
			set.Add(value);
		}
	}
}