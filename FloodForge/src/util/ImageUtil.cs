namespace FloodForge.Utils;

public static class ImageUtil {
	public static void WritePsd(string outputPath, byte[] rawData, int width, int height) {
		using FileStream fs = File.Create(outputPath);
		using BinaryWriter bw = new BinaryWriter(fs);
		bw.Write(['8', 'B', 'P', 'S']);
		bw.Write(AsBigEndian((short) 1));
		bw.Write(new byte[6]);
		bw.Write(AsBigEndian((short) 3));
		bw.Write(AsBigEndian(height));
		bw.Write(AsBigEndian(width));
		bw.Write(AsBigEndian((short) 8));
		bw.Write(AsBigEndian((short) 3));
		bw.Write(AsBigEndian(0));
		bw.Write(AsBigEndian(0));
		bw.Write(AsBigEndian(0));
		bw.Write(AsBigEndian((short) 0));

		int pixelCount = width * height;
		byte[] rChannel = new byte[pixelCount];
		byte[] gChannel = new byte[pixelCount];
		byte[] bChannel = new byte[pixelCount];

		for (int i = 0; i < pixelCount; i++) {
			int srcIndex = i * 3;
			rChannel[i] = rawData[srcIndex];
			gChannel[i] = rawData[srcIndex + 1];
			bChannel[i] = rawData[srcIndex + 2];
		}

		bw.Write(rChannel);
		bw.Write(gChannel);
		bw.Write(bChannel);
	}

	private static byte[] AsBigEndian(int value) {
		byte[] bytes = BitConverter.GetBytes(value);
		if (BitConverter.IsLittleEndian)
			Array.Reverse(bytes);
		return bytes;
	}

	private static byte[] AsBigEndian(short value) {
		byte[] bytes = BitConverter.GetBytes(value);
		if (BitConverter.IsLittleEndian)
			Array.Reverse(bytes);
		return bytes;
	}
}