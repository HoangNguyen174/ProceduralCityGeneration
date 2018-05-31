#include "buildingBlock.hpp"

BuildingBlock::BuildingBlock( const Vector3& bottomLeft, float maxHeight, float widthX, float depthY )
{
	m_bottomLeft = bottomLeft;
	m_maxHeight = maxHeight;
	m_widthX = widthX;
	m_depthY = depthY;
	m_center =  Vector3( widthX * 0.5f, depthY * 0.5f, 0.f ) + bottomLeft;
	GenerateBuildings();
}

BuildingBlock::~BuildingBlock()
{

}

void BuildingBlock::GenerateBuildings()
{
	int remainingSpaceOnX = m_widthX;
	int remainingSpaceOnY = m_depthY;
	
	Vector3 buildingCenter;
	Vector3 bottomLeft = m_bottomLeft;

	float blockLeft = m_bottomLeft.x + m_widthX;
	float blockTop = m_bottomLeft.y + m_depthY;

	int widthCount = 0;
	int depthCount = 0;

	int advanceY = 0;
	int maxSizeInARow = 0;

	int maxSize;

	while( remainingSpaceOnY > BUILDING_MIN_FOOTPRINT_SIZE )
	{
		maxSize = BUILDING_MAX_FOOTPRINT_SIZE;

		if( remainingSpaceOnY < BUILDING_MAX_FOOTPRINT_SIZE )
			maxSize = remainingSpaceOnY;

		while( remainingSpaceOnX > BUILDING_MIN_FOOTPRINT_SIZE )
		{
			if( remainingSpaceOnX < BUILDING_MAX_FOOTPRINT_SIZE )
				maxSize = remainingSpaceOnX;

			unsigned char footprintSize = MathUtilities::GetRandomNumber( BUILDING_MIN_FOOTPRINT_SIZE, maxSize );

			if( maxSizeInARow < footprintSize )
				maxSizeInARow = footprintSize;

			if( widthCount == 0 )
				advanceY = footprintSize;

			buildingCenter.x = bottomLeft.x + static_cast<float>( footprintSize ) * 0.5f;
			buildingCenter.y = bottomLeft.y + static_cast<float>( footprintSize ) * 0.5f;

			int roll = MathUtilities::GetRandomNumber( 0 , 1 );
			BuildingType type;

			if( roll == 1 )
				type = MODERN_ROUND_BUILDING;
			 else
				type = BLOCKY_BUILDING;

			Building* building = new Building( type, buildingCenter, footprintSize, m_maxHeight );
			m_buildings.push_back( building );

			bottomLeft.x += footprintSize;
			remainingSpaceOnX = blockLeft - bottomLeft.x;

			widthCount++;
		}

		remainingSpaceOnX = m_widthX;
		bottomLeft.x = m_bottomLeft.x;
		bottomLeft.y += maxSizeInARow;
		remainingSpaceOnY = blockTop - bottomLeft.y;
		widthCount = 0;
		maxSizeInARow = 0;
		depthCount++;
	}

}

int BuildingBlock::GetBuildingOverLapIndex( AABB2 building )
{
	for( unsigned int i = 0; i < m_buildings.size(); i++ )
	{
		Vector3 bPos = m_buildings[i]->m_center;
		Vector2 bMin = Vector2( bPos.x - m_buildings[i]->m_footprintSize, bPos.y - m_buildings[i]->m_footprintSize );
		Vector2 bMax = Vector2( bPos.x + m_buildings[i]->m_footprintSize, bPos.y + m_buildings[i]->m_footprintSize );

		AABB2 createdBuilding( bMin, bMax );

		if( building.IsCollideWithAABB( createdBuilding ) )
			return i;
	}
	return -1;
}