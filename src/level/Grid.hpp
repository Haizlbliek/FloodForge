#pragma once

#include <vector>
#include <set>
#include <utility>
#include <iostream>
#include <cstdint>

#include "History.hpp"

struct Tile {
	uint8_t tile;
	std::set<uint8_t> data;

	Tile(uint8_t tile)
	 : tile(tile),
	   data(std::set<uint8_t>{}) {
		// Just a placeholder
	}

	Tile(uint8_t tile, std::set<uint8_t> data)
	 : tile(tile),
	   data(data) {
		// Just a placeholder
	}
};

class Grid {
	public:
		Grid(unsigned int width, unsigned int height)
		 : width(width),
		   height(height) {
			for (int i = 0; i < width * height; i++) {
				tiles.push_back(new Tile{ 0 });
			}
		}

		~Grid() {
			for (int i = 0; i < width * height; i++) {
				delete tiles.back();
				tiles.pop_back();
			}
		}

		uint8_t getTile(unsigned int x, unsigned int y) {
			if (x >= width) return 0;
			if (y >= height) return 0;

			return tiles[x + y * width]->tile;
		}

		std::set<uint8_t> getData(unsigned int x, unsigned int y) {
			if (x >= width || y >= height)
				return std::set<uint8_t>{};

			return tiles[x + y * width]->data;
		}

		bool hasData(unsigned int x, unsigned int y, uint8_t data) {
			if (x >= width) return false;
			if (y >= height) return false;

			Tile *tile = tiles[x + y * width];

			return tile->data.count(data) > 0;
		}

		void setTile(unsigned int x, unsigned int y, uint8_t tile) {
			if (x >= width) return;
			if (y >= height) return;

			tiles[x + y * width]->tile = tile;

			History::unsavedChanges = true;
		}

		void clearData(unsigned int x, unsigned int y) {
			if (x >= width) return;
			if (y >= height) return;

			Tile *tile = tiles[x + y * width];

			tile->data.clear();

			History::unsavedChanges = true;
		}

		void addData(unsigned int x, unsigned int y, uint8_t data) {
			if (x >= width) return;
			if (y >= height) return;

			Tile *tile = tiles[x + y * width];

			tile->data.insert(data);

			History::unsavedChanges = true;
		}

		void removeData(unsigned int x, unsigned int y, uint8_t data) {
			if (x >= width) return;
			if (y >= height) return;

			Tile *tile = tiles[x + y * width];

			tile->data.erase(data);

			History::unsavedChanges = true;
		}

		bool inBoundary(unsigned int x, unsigned int y) {
			if (x < 12) return false;
			if (x >= width - 12) return false;
			if (y < 3) return false;
			if (y >= height - 5) return false;

			return true;
		}

		void resize(unsigned int width, unsigned int height) {
			std::vector< Tile* > newTiles;

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					if (x >= this->width || y >= this->height) {
						newTiles.push_back(new Tile{ 0 });
						continue;
					}

					Tile *tile = tiles[x + y * this->width];
					newTiles.push_back(new Tile(tile->tile, tile->data));
				}
			}

			for (int i = 0; i < this->width * this->height; i++) {
				delete tiles.back();
				tiles.pop_back();
			}

			this->width = width;
			this->height = height;
			tiles = newTiles;
			History::unsavedChanges = true;
		}

		unsigned int Width() {
			return width;
		}

		unsigned int Height() {
			return height;
		}

	private:
		unsigned int width;
		unsigned int height;
		std::vector< Tile* > tiles;
};