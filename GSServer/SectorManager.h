#pragma once

using Sector = std::unordered_set<WORD>;

class SectorManager {
public:
	SectorManager();
	unsigned int PositionToSectorIndex(WORD x, WORD y);
	std::vector<Sector> GetNearSectors(WORD sectorIdx);
	bool UpdateSector(WORD id, WORD oldX, WORD oldY, WORD newX, WORD newY);
	bool AddToSector(WORD id, WORD x, WORD y);
	bool RemoveFromSector(WORD id, WORD x, WORD y);

private:
	std::vector<Sector> sectorList;
	std::mutex lock;
};