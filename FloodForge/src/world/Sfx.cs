using Silk.NET.OpenAL;

namespace FloodForge;

public static class Sfx {
	private static AL _al = null!;
	private static ALContext _alc = null!;
	private static unsafe Device* _device;
	private static unsafe Context* _context;

	// Use a list to track active sources and their buffers
	private static readonly List<uint> ActiveSources = new();
	// Cache buffers by path to avoid re-loading the same file from disk
	private static readonly Dictionary<string, uint> BufferCache = new();

	public static unsafe void Initialize() {
		_al = AL.GetApi();
		_alc = ALContext.GetApi();

		_device = _alc.OpenDevice("");
		_context = _alc.CreateContext(_device, null);
		_alc.MakeContextCurrent(_context);
	}

	public static void Play(string filePath) {
		// 1. Get or Load the buffer
		if (!BufferCache.TryGetValue(filePath, out uint buffer)) {
			buffer = LoadWav(filePath);
			BufferCache[filePath] = buffer;
		}

		// 2. Cleanup finished sources before starting a new one
		CleanupFinishedSources();

		// 3. Create a new source for this specific playback instance
		uint source = _al.GenSource();
		_al.SetSourceProperty(source, SourceInteger.Buffer, buffer);
		_al.SourcePlay(source);

		ActiveSources.Add(source);
	}

	private static void CleanupFinishedSources() {
		for (int i = ActiveSources.Count - 1; i >= 0; i--) {
			_al.GetSourceProperty(ActiveSources[i], GetSourceInteger.SourceState, out int state);
			if ((SourceState) state == SourceState.Stopped) {
				_al.DeleteSource(ActiveSources[i]);
				ActiveSources.RemoveAt(i);
			}
		}
	}

	public static unsafe void Cleanup() {
		foreach (var source in ActiveSources) {
			_al.SourceStop(source);
			_al.DeleteSource(source);
		}
		foreach (var buffer in BufferCache.Values) {
			_al.DeleteBuffer(buffer);
		}

		_alc.MakeContextCurrent(null);
		_alc.DestroyContext(_context);
		_alc.CloseDevice(_device);
	}

	private static unsafe uint LoadWav(string path) {
		using var stream = File.OpenRead(path);
		using var reader = new BinaryReader(stream);

		// Simple WAV Header Parsing
		reader.ReadBytes(12); // RIFF...WAVE
		reader.ReadBytes(4);  // fmt
		reader.ReadInt32();   // chunk size
		reader.ReadInt16();   // format tag
		int channels = reader.ReadInt16();
		int sampleRate = reader.ReadInt32();
		reader.ReadInt32();   // byte rate
		reader.ReadInt16();   // block align
		int bitsPerSample = reader.ReadInt16();

		// Skip to data chunk
		while (new string(reader.ReadChars(4)) != "data") { }
		int dataSize = reader.ReadInt32();
		byte[] data = reader.ReadBytes(dataSize);

		var format = (channels, bitsPerSample) switch {
			(1, 8) => BufferFormat.Mono8,
			(1, 16) => BufferFormat.Mono16,
			(2, 8) => BufferFormat.Stereo8,
			(2, 16) => BufferFormat.Stereo16,
			_ => throw new NotSupportedException("Unsupported format")
		};

		uint buffer = _al.GenBuffer();
		fixed (byte* pData = data) {
			_al.BufferData(buffer, format, pData, dataSize, sampleRate);
		}
		return buffer;
	}
}
