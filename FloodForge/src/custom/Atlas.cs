using FloodForge;

namespace Custom;

public class UVAtlas {
	private UVAtlasElement[] elements;

	public UVAtlas(UVAtlasElement[] elements) {
		this.elements = elements;
	}

	public UVCoordinates UV(string ID) {
		foreach (UVAtlasElement atlasElement in this.elements) {
			if (atlasElement.elementID.Equals(ID, StringComparison.InvariantCultureIgnoreCase)) {
				return atlasElement.elementUVs;
			}
		}
		Logger.Info($"Failed to get UV's for ID: {ID}");
		return new(0, 0, 1, 1);
	}

	public class UVAtlasElement {
		public string elementID;
		public UVCoordinates elementUVs;
		public UVAtlasElement(string elementID, UVCoordinates elementUVs) {
			this.elementID = elementID;
			this.elementUVs = elementUVs;
		}
	}
		
	public struct UVCoordinates(float u0, float v0, float u1, float v1) {
		public float u0 = u0;
		public float v0 = v0;
		public float u1 = u1;
		public float v1 = v1;
	}
}