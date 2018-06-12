#pragma once

using Sector = std::unordered_set<unsigned int>;

class SectorManager {
public:
	SectorManager();
	unsigned int PositionToSectorIndex(unsigned int x, unsigned int y);
	std::vector<Sector> GetNearSectors(unsigned int sectorIdx);
	bool UpdateSector(unsigned int id, unsigned int oldX, unsigned int oldY, unsigned int newX, unsigned int newY);
	bool AddToSector(unsigned int id, unsigned int x, unsigned int y);

private:
	std::vector<Sector> sectorList;
	std::mutex lock;
};