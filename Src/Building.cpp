#include "Building.hpp"
#include "CityManager.hpp"
#include "World.hpp"

std::string		Building::m_signName[5] = { "GUILDHALL", "BLIZZARD", "UBISOFT" , "SQUARENIX", "CAPCOM"  };

Building::Building( BuildingType type, const Vector3& center, unsigned char footprintSize, unsigned char height )
{
	m_center = center;
	m_floorIteration = 0;
	m_isLightOn = 1;
	m_lightOffPeriod = MathUtilities::GetRandomNumber( 10, 20 );
	m_lightOffDuration = MathUtilities::GetRandomNumber( 3, 5 );
	m_lightOffTime = 0.f;
	m_accumulateTime = 1.f;

	unsigned char maxHeight = MathUtilities::Max2( height, MIN_HEIGHT );
	unsigned char minHeight = MathUtilities::Min2( height, MIN_HEIGHT );

	m_height = MathUtilities::GetRandomNumber( minHeight, maxHeight );

	m_isVboDirty = true;
	m_vboId = 0;
	m_numVertex = 0;
	m_type = type;
	m_footprintSize = footprintSize;
	m_roundBuildingBodyVertexNum = 0;

	int rand = MathUtilities::GetRandomNumber( 0, 2 );

	if( rand == 0 )
		m_buildingLightColor = RGBColor( 0.25f, 1.f, 0.3f, 1.f );
	else if( rand == 1 )
		m_buildingLightColor = RGBColor( 0.75f, 0.75f, 0.f, 1.f );
	else if( rand == 2 )
		m_buildingLightColor = RGBColor( 0.88f, 0.85f, 0.9f, 1.f );

	m_buildingSign = nullptr;

	switch( m_type )
	{
		case BLOCKY_BUILDING: GenerateBlockyBuilding();
							  break;
		case MODERN_ROUND_BUILDING : GenerateModernRoundBuilding();
									 break;

	}
}
	
EnginePolygon Building::GenerateRegularPolygon( const Vector3& center, float radius, unsigned short numSide, float orientationDegree )
{
	SimpleVertex3D vertex;
	EnginePolygon polygon;

	if( numSide < 3 || radius < 0.f )
		return polygon;

	float orientationRadian = MathUtilities::DegToRad( orientationDegree );
	
	vertex.m_color = RGBColor::Red();

	for( int i = 0; i < numSide; i++ )
	{
		float angle = i * 2 * PI / numSide;
		vertex.m_position.x = radius * cos( angle + orientationRadian ) + center.x;
		vertex.m_position.y = radius * sin( angle + orientationRadian ) + center.y;
		vertex.m_position.z = center.z;

		polygon.m_vertexList.push_back( vertex );
	}

	return polygon;
}

void Building::CreateVBO()
{
	if( m_vboId == 0 )
	{
		GraphicManager::s_render->GenerateBuffer( 1, &m_vboId );
	}

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboId );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( SimpleVertex3D ) * m_vertexList.size(), m_vertexList.data(), GL_STATIC_DRAW );

	m_numVertex = m_vertexList.size();

	m_isVboDirty = false;
}

void Building::RenderVBO()
{
	CityManager::s_buildingShaderProgram->UseShaderProgram();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, CityManager::s_buildingFacadeTexture->m_buildingFacadeFBO->m_fboColorTextureID );

	CityManager::s_buildingShaderProgram->SetVec3UniformValue( "u_buildingLightColor", Vector3( m_buildingLightColor.m_red, m_buildingLightColor.m_green, m_buildingLightColor.m_blue ) );
	CityManager::s_buildingShaderProgram->SetIntUniformValue( "u_isLightOn", m_isLightOn );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_NORMALS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboId );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_position ) );
	glVertexAttribPointer( VERTEX_ATTRIB_NORMALS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_normal ) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_color ) );
	glVertexAttribPointer( VERTEX_ATTRIB_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_texCoords ) );

	if( m_type == BLOCKY_BUILDING )
		GraphicManager::s_render->DrawArray( GL_QUADS ,0 , m_numVertex );
	else if( m_type == MODERN_ROUND_BUILDING )
	{
		GraphicManager::s_render->DrawArray( GL_TRIANGLE_STRIP ,0 , m_roundBuildingBodyVertexNum );
		GraphicManager::s_render->DrawArray( GL_TRIANGLE_FAN , m_roundBuildingBodyVertexNum + 1, m_numVertex );
	}

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_NORMALS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );
}

void Building::Render()
{
	if( m_buildingSign != nullptr )
		m_buildingSign->Render();

	if( m_isVboDirty )
		CreateVBO();

	RenderVBO();
}

Orientation Building::CalcOrientation( const Vector3& p1, const Vector3& p2, const Vector3& p3 )
{
	int val = ( p2.y - p1.y ) * ( p3.x - p2.x ) - 
			  ( p2.x - p1.x ) * ( p3.y - p2.y );

	if( val == 0 ) 
		return COLINEAR;

	return ( val > 0 ) ? CLOCKWISE : COUNTER_CLOCKWISE;
}

void Building::GenerateBlockyBuilding()
{
	unsigned char minHeight = 1;

	unsigned char iteration = 10;
	unsigned char currentIteration = 0;
		
	float currentHeight = static_cast<float>( m_height );

	float offsetMax = m_footprintSize * 0.5f;

	float footprintRight  = m_center.x + m_footprintSize * 0.5f;
	float footprintLeft   = m_center.x - m_footprintSize * 0.5f;
 	float footprintTop    = m_center.y + m_footprintSize * 0.5f;
 	float footprintBottom = m_center.y - m_footprintSize * 0.5f;

	bool skip = false;

	float blockLeftMax = 0.f;
	float blockRightMax = 0.f;
	float blockTopMax = 0.f;
	float blockBottomMax = 0.f;

	while(1)
	{
		skip = false;
		if( currentHeight < minHeight  )
			break;
		if( currentIteration >= iteration )
			break;

		float offsetXFromCenter = MathUtilities::GetRandomFloatInRange( 0.f, offsetMax );
		float offsetYFromCenter = MathUtilities::GetRandomFloatInRange( 0.f, offsetMax ); 

		Vector3 bottomLeft = Vector3( m_center.x - offsetXFromCenter, m_center.y - offsetYFromCenter, 0.f );

		float widthX = MathUtilities::GetRandomFloatInRange( abs( m_center.x - bottomLeft.x ) , abs( bottomLeft.x - footprintRight ) );
		float depthY = MathUtilities::GetRandomFloatInRange( abs( m_center.y - bottomLeft.y ), abs( bottomLeft.y - footprintTop ) );

		float leftToCenter = abs ( bottomLeft.x - m_center.x );
		float rightToCenter = abs ( bottomLeft.x + widthX - m_center.x );
		float bottomToCenter = abs( bottomLeft.y - m_center.y );
		float topToCenter = abs( bottomLeft.x + depthY - m_center.y );

		//if new block is hidden, skip
		if( leftToCenter <= blockLeftMax &&
			rightToCenter <= blockRightMax &&
			bottomToCenter <= blockBottomMax &&
			topToCenter <= blockTopMax )
				skip = true;

		//if new block cover previous block, skip
		if( currentIteration != 0 && 
			leftToCenter > blockLeftMax && 
			rightToCenter > blockRightMax &&
			bottomToCenter > blockBottomMax &&
		    topToCenter > blockTopMax )
				skip = true;

		float diffenceTolerance = 0.05f;
		//if wall of block is in the same position with previous one, skip to avoid z-fighting
		//or the different is too small
		if( abs( leftToCenter - blockLeftMax ) <= diffenceTolerance || 
			abs( rightToCenter - blockRightMax ) <= diffenceTolerance  ||
			abs( bottomToCenter - blockBottomMax ) <= diffenceTolerance  || 
			abs( topToCenter - blockTopMax ) <= diffenceTolerance )
				skip = true;

		if( !skip )
		{
			blockLeftMax = MathUtilities::Max2( leftToCenter, blockLeftMax );
			blockRightMax = MathUtilities::Max2( rightToCenter, blockRightMax );
			blockBottomMax = MathUtilities::Max2( bottomToCenter, blockBottomMax );
			blockTopMax = MathUtilities::Max2( topToCenter, blockTopMax );

			currentHeight = MathUtilities::GetRandomFloatInRange( currentHeight - 2.f, currentHeight );

			if( currentHeight < minHeight )
				currentHeight = minHeight;

			ConstructBlock( bottomLeft, widthX, depthY, currentHeight, RGBColor( 0.9f, 0.9f, 0.9f, 1.f) );

			//construct little block above
			bottomLeft.z += currentHeight;
			bottomLeft.x -= 0.05f;
			bottomLeft.y -= 0.05f;

			if( currentIteration != 0 )
			{
				ConstructBlock( bottomLeft, widthX + 0.05f * 2, depthY + 0.05f * 2, .1f, RGBColor( 0.0f, 0.0f, 0.0f, 0.4f) );
			}
			else
			{
				GenerateBlockyBuildingTop( bottomLeft, widthX + 0.05f * 2, depthY + 0.05f * 2 );
			}
		}

		unsigned char scale = 1;
		currentIteration++;
		currentHeight -= MathUtilities::GetRandomFloatZeroToOne() * scale;
	}	
}

void Building::CalcBuildingSignPositionAndRotation( const Vector3& bottomLeft, float width, float depth )
{
	int cardinalDirection = MathUtilities::GetRandomNumber( 0, 3 );
	Vector3 position = bottomLeft;
	float offsetFromWall = 0.05f;

	std::string name = m_signName[ MathUtilities::GetRandomNumber( 0, 4 ) ];

	m_buildingSign = new GraphicText( Vector3( 0.f, 0.f, 0.f), Vector4( 0.f, 0.f, 1.f, -90.f ), Vector3( 0.1f, 0.1f, 1.f ), m_buildingLightColor, nullptr, nullptr, 5, World::s_camera3D, name );

	if( cardinalDirection == NORTH_CARDINAL_DIRECTION )
	{
		position.x += width;
		position.y += depth;
		m_buildingSign->m_bottomLeft = position;
		m_buildingSign->m_bottomLeft.y += offsetFromWall;
		m_buildingSign->m_cellHeight = 1.f;
		m_buildingSign->m_rotateAxisAndAngleDeg = Vector4( 0.f, 0.f, 1.f, 180.f );
	}
	else if( cardinalDirection == SOUTH_CARDINAL_DIRECTION )
	{
		m_buildingSign->m_bottomLeft = position;
		m_buildingSign->m_bottomLeft.y -= offsetFromWall;
		m_buildingSign->m_cellHeight = 1.f;
		m_buildingSign->m_rotateAxisAndAngleDeg = Vector4( 0.f, 0.f, 1.f, 0.f );
	}
	else if( cardinalDirection == WEST_CARDINAL_DIRECTION )
	{
		position.x += width;
		m_buildingSign->m_bottomLeft = position;
		m_buildingSign->m_bottomLeft.x += offsetFromWall;
		m_buildingSign->m_cellHeight = 1.f;
		m_buildingSign->m_rotateAxisAndAngleDeg = Vector4( 0.f, 0.f, 1.f, 90.f );
	}
	else if( cardinalDirection == EAST_CARDINAL_DIRECTION )
	{
		position.y += depth;
		m_buildingSign->m_bottomLeft = position;
		m_buildingSign->m_bottomLeft.x -= offsetFromWall;
		m_buildingSign->m_cellHeight = 1.f;
		m_buildingSign->m_rotateAxisAndAngleDeg = Vector4( 0.f, 0.f, 1.f, 270.f );
	}


}

void Building::GenerateBlockyBuildingTop( const Vector3& bottomLeft, float widthX, float depthY )
{
	if( widthX * depthY < 6 )
		return;

	unsigned char maxIter = 4;

	unsigned char iter = MathUtilities::GetRandomNumber( 1, maxIter );
	float offset = 0.05f;
	Vector3 copy = bottomLeft;
	float width = widthX;
	float depth = depthY;
	float thickness = 0.f;

	CalcBuildingSignPositionAndRotation( bottomLeft, width, depth );

	for( unsigned int i = 0; i < iter; i++ )
	{
		thickness = MathUtilities::GetRandomFloatInRange( 0.5f, 1.f );
		ConstructBlock( copy, width, depth, thickness, RGBColor( 0.0f, 0.0f, 0.0f, 0.1f) );
		offset += MathUtilities::GetRandomFloatInRange( 0.1f, 0.2f );
		copy.x += offset;
		copy.y += offset;
		copy.z += thickness;
		width -= 2 * offset;
		depth -= 2 * offset;
		thickness -= 0.05f;
	}

}

void Building::ConstructBlock( const Vector3& bottomLeft, float width, float depth, float height, const RGBColor& color )
{
	SimpleVertex3D vert;
	float windowScale = 0.05f;

	vert.m_color = color;

	//front
	vert.m_normal = Vector3( 0.f, -1.f, 0.f );
	vert.m_position = bottomLeft;
	vert.m_texCoords = Vector2( 0.f, 1.f );
	m_vertexList.push_back( vert );
	
	vert.m_position.x += width; 
	vert.m_texCoords = Vector2( width * windowScale, 1.f );
	m_vertexList.push_back( vert );

	vert.m_position.z += height;
	vert.m_texCoords = Vector2( width * windowScale, abs( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );

	vert.m_position.x -= width;
	vert.m_texCoords = Vector2( 0.f, abs( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );

	
	//right
	vert.m_normal = Vector3( 1.f, 0.f, 0.f );
	vert.m_position = Vector3( bottomLeft.x + width, bottomLeft.y, bottomLeft.z );
	vert.m_texCoords = Vector2( 0.f, 1.f );
	m_vertexList.push_back( vert );

	vert.m_position.y += depth; 
	vert.m_texCoords = Vector2( depth * windowScale, 1.f );
	m_vertexList.push_back( vert );

	vert.m_position.z += height;
	vert.m_texCoords = Vector2( depth * windowScale, abs( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );

	vert.m_position.y -= depth;
	vert.m_texCoords = Vector2( 0.f, abs( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );

	//back
	vert.m_normal = Vector3( 0.f, 1.f, 0.f );
	vert.m_position = bottomLeft;
	vert.m_position.y += depth;
	vert.m_texCoords = Vector2( width * windowScale, 1.f );
	m_vertexList.push_back( vert );
	
	vert.m_position.z += height;
	vert.m_texCoords = Vector2( width * windowScale, abs( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );
	
	vert.m_position.x += width;
	vert.m_texCoords = Vector2( 0.f, abs ( 1.f - height * windowScale ) );
	m_vertexList.push_back( vert );
	
	vert.m_position.z -= height;
	vert.m_texCoords = Vector2( 0.f, 1.f );
	m_vertexList.push_back( vert );

	//left
	vert.m_normal = Vector3( -1.f, 0.f, 0.f );
	vert.m_position = bottomLeft;
	vert.m_texCoords = Vector2( 0.f, 1.f );	
	m_vertexList.push_back( vert );
	
	vert.m_position.z += height;
	vert.m_texCoords = Vector2( 0.f, abs( 1.f - height * windowScale ) );	
	m_vertexList.push_back( vert );
	
	vert.m_position.y += depth;
	vert.m_texCoords = Vector2( depth * windowScale, abs( 1.f - height * windowScale ) );	
	m_vertexList.push_back( vert );
	
	vert.m_position.z -= height;
	vert.m_texCoords = Vector2( depth * windowScale, 1.f );	
	m_vertexList.push_back( vert );
	
	vert.m_texCoords = Vector2( 0.f, 0.f );
	//bottom
	vert.m_normal = Vector3( 0.f, 0.f, -1.f );
	vert.m_position = bottomLeft;
	m_vertexList.push_back( vert );
	
	vert.m_position.y += depth;
	m_vertexList.push_back( vert );
	
	vert.m_position.x += width;
	m_vertexList.push_back( vert );
	
	vert.m_position.y -= depth;
	m_vertexList.push_back( vert );
	
	//top
	vert.m_normal = Vector3( 0.f, 0.f, 1.f );
	vert.m_position = Vector3( bottomLeft.x, bottomLeft.y, bottomLeft.z + height );
	m_vertexList.push_back( vert );
	
	vert.m_position.x += width;
	m_vertexList.push_back( vert );
	
	vert.m_position.y += depth;
	m_vertexList.push_back( vert );
	
	vert.m_position.x -= width;
	m_vertexList.push_back( vert );
}

void Building::ConstructCylinder( const Vector3& center, float radius, int sideNum )
{
	SimpleVertex3D vert;

	float theta =  0.f;
	float delta = 2 * PI / sideNum;

	unsigned char maxNumCut = 3;
	unsigned char numCut = MathUtilities::GetRandomNumber( 0, maxNumCut );
	unsigned char cutCount = 0;
	float cutChance = 1.f / sideNum;
	cutChance = 0.3f;

	float tu = 0.f;
	float uStep = 1 / sideNum;

	int cutIndexArr[3] = { -1, -1, -1 };
	int cutIndex = 0;

	for( int i = 0; i <= sideNum ; i++ )
	{
		float roll = MathUtilities::GetRandomFloatZeroToOne();
		
		if( roll <= cutChance && cutCount < maxNumCut && ( sideNum - i > 6 ) )
		{
			delta = PI * 0.5f;
			i += 5;
			cutCount++;
			uStep = 5.f / sideNum;
			cutIndexArr[cutIndex] = i;
			cutIndex++;
		}
		else
		{
			delta = 2 * PI / sideNum;
			uStep = 1.f / sideNum;
		}

		vert.m_color = RGBColor( 0.9f, 0.9f, 0.9f, 1.0f );
		vert.m_position.x = radius * cos( theta ) + center.x;
		vert.m_position.y = radius * sin( theta ) + center.y;
		vert.m_position.z = m_height;

		vert.m_normal = Vector3(  vert.m_position.x - center.x, vert.m_position.y  - center.y, 0.f );
		vert.m_normal = vert.m_normal.Normalize();

		vert.m_texCoords = Vector2( tu, 1.f );
		m_vertexList.push_back( vert );

		vert.m_position.z = 0.f;

		vert.m_texCoords = Vector2( tu, 0.f );
		m_vertexList.push_back( vert );

		theta += delta;

		tu += uStep;

		m_roundBuildingBodyVertexNum += 2;
	}
	

	theta = 0.f;

	vert.m_position = Vector3( m_center.x, m_center.y, m_height );
	vert.m_normal = Vector3( 0.f, 0.f, 1.f );
	vert.m_color = RGBColor( 0.f, 0.f, 0.f, 0.1f );
	m_vertexList.push_back( vert );

	for( int i = 0; i <= sideNum; i++ )
	{
		delta = 2 * PI / sideNum;
		vert.m_position.x = radius * cos( theta ) + center.x;
		vert.m_position.y = radius * sin( theta ) + center.y;
		vert.m_position.z = m_height;
		m_vertexList.push_back( vert );
		
// 		for( int j = 0; j < 3; j++ )
// 		{
// 			if( i == cutIndexArr[j] )
// 			{
// 				i += 6;
// 				delta = 0.5f * PI;
// 				cutIndexArr[j] = -1;
// 				break;
// 			}
// 		}

		theta += delta;
	}
}

void Building::Update( float elapsedTime )
{
// 	m_accumulateTime += elapsedTime;
// 
// 	if( ( int(m_accumulateTime) % m_lightOffPeriod == 0 ) && m_isLightOn != 0 )
// 		m_isLightOn = 0;
// 
// 	if( m_isLightOn == 0 )
// 		m_lightOffTime += elapsedTime; 
// 
// 	if( m_lightOffTime > m_lightOffDuration )
// 	{
// 		m_isLightOn = 1;
// 		m_lightOffTime = 0.f;
// 	}
}
void Building::GenerateClassicBuilding()
{

}

void Building::GenerateModernRoundBuilding()
{
	int radius = m_footprintSize * 0.5f;

	ConstructCylinder( m_center, radius, 24 );
}






