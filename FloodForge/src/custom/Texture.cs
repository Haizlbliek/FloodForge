using StbImageSharp;

namespace Custom;

public class Texture : IDisposable {
	public readonly uint _id;
	public readonly uint width;
	public readonly uint height;

	private Texture(uint id, uint width, uint height) {
		this._id = id;
		this.width = width;
		this.height = height;
	}

	public static unsafe Texture Load(string path, TextureWrapMode wrapMode = TextureWrapMode.Repeat, TextureMinFilter minFilter = TextureMinFilter.Nearest, TextureMagFilter magFilter = TextureMagFilter.Nearest) {
		using Stream stream = File.OpenRead(path);
		ImageResult result = ImageResult.FromStream(stream, ColorComponents.RedGreenBlueAlpha);

		Texture texture = new Texture(Custom.gl.GenTexture(), (uint) result.Width, (uint) result.Height);

		Custom.gl.BindTexture(TextureTarget.Texture2D, texture._id);

		fixed (byte* ptr = result.Data) {
			Custom.gl.TexImage2D(TextureTarget.Texture2D, 0, InternalFormat.Rgba, (uint) result.Width, (uint) result.Height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, ptr);
		}

		Custom.gl.TexParameter(GLEnum.Texture2D, GLEnum.TextureWrapS, (int) wrapMode);
		Custom.gl.TexParameter(GLEnum.Texture2D, GLEnum.TextureWrapT, (int) wrapMode);
		Custom.gl.TexParameter(GLEnum.Texture2D, GLEnum.TextureMinFilter, (int) minFilter);
		Custom.gl.TexParameter(GLEnum.Texture2D, GLEnum.TextureMagFilter, (int) magFilter);

		Custom.gl.BindTexture(TextureTarget.Texture2D, 0);

		return texture;
	}

	public static RawImage LoadRawImage(string path) {
		using Stream stream = File.OpenRead(path);
		ImageResult result = ImageResult.FromStream(stream, ColorComponents.RedGreenBlueAlpha);

		return new RawImage(result.Width, result.Height, result.Data);
	}

	public void Dispose() {
		Custom.gl.DeleteTexture(this._id);
		GC.SuppressFinalize(this);
	}
}