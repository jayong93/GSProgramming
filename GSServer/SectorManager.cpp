#include "stdafx.h"
#include "SectorManager.h"
#include "../Share/Shares.h"

SectorManager::SectorManager()
{
	auto sectorNum = int(std::ceil((double)BOARD_W / (double)VIEW_SIZE) * std::ceil((double)BOARD_H / (double)VIEW_SIZE));
	sectorList.resize(sectorNum);
}

unsigned int SectorManager::PositionToSectorIndex(unsigned int x, unsigned int y)
{
	const auto sectorX = x / VIEW_SIZE;
	const auto sectorY = y / VIEW_SIZE;
	return sectorY * int(std::ceil(BOARD_W / (double)VIEW_SIZE)) + sectorX;
}

std::vector<Sector> SectorManager::GetNearSectors(unsigned int sectorIdx)
{
	std::vector<Sector> nearSectors;
	const auto horiSectorNum = int(std::ceil(BOARD_W / (double)VIEW_SIZE));
	const auto vertSectorNum = int(std::ceil(BOARD_H / (double)VIEW_SIZE));
	const int sectorX = sectorIdx % horiSectorNum;
	const int sectorY = sectorIdx / horiSectorNum;
	const auto sectorMinX = max(sectorX - 1, 0);
	const auto sectorMaxX = min(sectorX + 1, horiSectorNum - 1);
	const auto sectorMinY = max(sectorY - 1, 0);
	const auto sectorMaxY = min(sectorY + 1, vertSectorNum - 1);

	std::shared_lock<std::shared_timed_mutex> lg{ this->lock };
	for (auto i = sectorMinY; i <= sectorMaxY; ++i) {
		for (auto j = sectorMinX; j <= sectorMaxX; ++j) {
			nearSectors.emplace_back(sectorList[i*horiSectorNum + j]);
		}
	}
	return nearSectors;
}

bool SectorManager::UpdateSector(unsigned int id, unsigned int oldX, unsigned int oldY, unsigned int newX, unsigned int newY)
{
	std::unique_lock<decltype(this->lock)> lg{ this->lock };
	auto oldIdx = this->PositionToSectorIndex(oldX, oldY);
	if (oldIdx < 0 || oldIdx >= sectorList.size()) return false;
	auto newIdx = this->PositionToSectorIndex(newX, newY);
	if (newIdx < 0 || newIdx >= sectorList.size()) return false;

	auto& oldSector = sectorList[oldIdx];
	auto& newSector = sectorList[newIdx];

	auto it = oldSector.find(id);
	if (it == oldSector.end()) return false;
	
	oldSector.erase(it);
	newSector.emplace(id);

	return true;
}

bool SectorManager::AddToSector(unsigned int id, unsigned int x, unsigned int y)
{
	std::unique_lock<decltype(this->lock)> lg{ this->lock };
	auto idx = this->PositionToSectorIndex(x, y);
	if (idx < 0 || idx >= sectorList.size()) return false;

	auto& sector = sectorList[idx];
	
	auto it = sector.find(id);
	if (it != sector.end()) return false;

	sector.emplace(id);
	return true;
}
