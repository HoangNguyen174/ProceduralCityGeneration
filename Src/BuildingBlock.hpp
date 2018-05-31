#ifndef BUILDING_BLOCK_H
#define BUILDING_BLOCK_H

#include "Building.hpp"

class BuildingBlock
{
	public:
		float						m_maxHeight;
		unsigned char				m_widthX;
		unsigned char				m_depthY;
		Vector3						m_center;
		Vector3						m_bottomLeft;
		std::vector<Building*>		m_buildings;

	public:
		BuildingBlock( const Vector3& bottomLeft, float maxHeight, float widthX, float depthY );
		~BuildingBlock();
		void GenerateBuildings();

	private:
		int GetBuildingOverLapIndex( AABB2 building );
};

#endif