#include "BuildingFacadeTexture.hpp"
#include "World.hpp"

BuildingFacadeTexture::BuildingFacadeTexture()
{
	m_shaderProgram = new OpenGLShaderProgram( "building texture", "./Data/Shader/BuildingTextureVertexShader.glsl", "./Data/Shader/BuildingTextureFragmentShader.glsl" );
	m_textureSize = 1024;
	m_windowInARow = 64;
	m_vboId = 0;
	m_windowVertexList.reserve( m_windowInARow * m_windowInARow * 4 );
	m_windows.resize( m_windowInARow * m_windowInARow, UNLIT );
	for( int i = 0; i < m_windows.size(); i++ )
		m_windows[i] = UNLIT;
	m_buildingFacadeFBO = FBO::CreateOrGetFBOByName( "Building Facade Texture" , m_textureSize, m_textureSize );
	m_noiseTexture = Texture::CreateOrGetTexture( "./Data/Texture/noise1.jpg" );

	GenerateWindowLight();
	GenerateVertexList();
	CreateVBO();
	Render();
}

BuildingFacadeTexture::~BuildingFacadeTexture()
{
}

void BuildingFacadeTexture::GenerateWindowLight()
{
	int windowIndex = 0;
	float randNum;

	for( windowIndex = 0; windowIndex < m_windows.size(); windowIndex++ )
	{
		randNum = MathUtilities::GetRandomFloatZeroToOne();

		if( randNum < 0.05f )
			m_windows[windowIndex] = FULL;
		else if( randNum >= 0.25f && randNum <= 0.5f )
			m_windows[windowIndex] = MEDIUM;
		else
			m_windows[windowIndex] = UNLIT;
	}

	int neighborIndexList[8];
	float chanceToLitOnARow = 0.3f;
	bool litThisRow = false;

	float roll = 0.f;
	int count = 0;

	for( windowIndex = 0; windowIndex < m_windows.size(); windowIndex++ )
	{
		if( ( windowIndex % m_windowInARow ) == 0 )
		{
			roll = MathUtilities::GetRandomFloatZeroToOne();

			if( roll < chanceToLitOnARow )
			{
				litThisRow = true;
				count++;
			}
		}

		if( !litThisRow )
			continue;

		Get8NeighborWindowIndex(  windowIndex, neighborIndexList );

		int westNeighborIndex = neighborIndexList[ WEST_CARDINAL_DIRECTION ];
		int eastNeighborIndex = neighborIndexList[ EAST_CARDINAL_DIRECTION ];

		if( m_windows[ windowIndex ] == FULL )
		{
				float chanceToLit = 0.75f;

				randNum = MathUtilities::GetRandomFloatZeroToOne();

				if( randNum < chanceToLit )
				{
					if( westNeighborIndex > 0 )
						m_windows[ westNeighborIndex ] = FULL;
					if( eastNeighborIndex > 0 )
						m_windows[ eastNeighborIndex ] = FULL;
				}
		}

		if( ( windowIndex + 1 ) % m_windowInARow == 0 )
			litThisRow = false;
	}

	int k = count;
}

void BuildingFacadeTexture::Get8NeighborWindowIndex( int windowIndex, int indexList[8] )
{
	TileCoords2D tileCoords = ConvertTileIndexToTileCoords2D( windowIndex, m_windowInARow, m_windowInARow );

	if( tileCoords.y <= 0 )
		indexList[SOUTH_CARDINAL_DIRECTION] = -1;
	else 
		indexList[SOUTH_CARDINAL_DIRECTION] = windowIndex - m_windowInARow;

	if( tileCoords.y >= m_windowInARow - 1)
		indexList[NORTH_CARDINAL_DIRECTION] = -1;
	else
		indexList[NORTH_CARDINAL_DIRECTION] = windowIndex + m_windowInARow;

	if( tileCoords.x <= 0 )
		indexList[WEST_CARDINAL_DIRECTION] = -1;
	else
		indexList[WEST_CARDINAL_DIRECTION] = windowIndex - 1;

	if( tileCoords.x >= m_windowInARow - 1 )
		indexList[EAST_CARDINAL_DIRECTION] = -1;
	else
		indexList[EAST_CARDINAL_DIRECTION] = windowIndex + 1;

	if( indexList[NORTH_CARDINAL_DIRECTION] == -1 )
	{	
		indexList[NORTH_EAST_SEMICARDINAL_DIRECTION] = -1;
		indexList[NORTH_WEST_SEMICARDINAL_DIRECTION] = -1;
	}
	else
	{
		if( indexList[EAST_CARDINAL_DIRECTION] == -1 )
			indexList[NORTH_EAST_SEMICARDINAL_DIRECTION] = -1;
		else
			indexList[NORTH_EAST_SEMICARDINAL_DIRECTION] = indexList[NORTH_CARDINAL_DIRECTION] + 1;

		if( indexList[WEST_CARDINAL_DIRECTION] == -1 )
			indexList[NORTH_WEST_SEMICARDINAL_DIRECTION] = -1;
		else
			indexList[NORTH_WEST_SEMICARDINAL_DIRECTION] = indexList[NORTH_CARDINAL_DIRECTION] - 1;
	}

	if( indexList[SOUTH_CARDINAL_DIRECTION] == -1 )
	{	
		indexList[SOUTH_EAST_SEMICARDINAL_DIRECTION] = -1;
		indexList[SOUTH_WEST_SEMICARDINAL_DIRECTION] = -1;
	}
	else
	{
		if( indexList[EAST_CARDINAL_DIRECTION] == -1 )
			indexList[SOUTH_EAST_SEMICARDINAL_DIRECTION] = -1;
		else
			indexList[SOUTH_EAST_SEMICARDINAL_DIRECTION] = indexList[SOUTH_CARDINAL_DIRECTION] + 1;

		if( indexList[WEST_CARDINAL_DIRECTION] == -1 )
			indexList[SOUTH_WEST_SEMICARDINAL_DIRECTION] = -1;
		else
			indexList[SOUTH_WEST_SEMICARDINAL_DIRECTION] = indexList[SOUTH_CARDINAL_DIRECTION] - 1;
	}
}

void BuildingFacadeTexture::GenerateVertexList()
{
	int windowSize = m_textureSize / m_windowInARow;

	SimpleVertex3D vert;

	int border = 4;

	for( int y = 0; y < m_windowInARow; y++ )
	{
		for( int x = 0; x < m_windowInARow; x++ )
		{
			int index = x + m_windowInARow * y;

			switch( m_windows[index] )
			{
				case FULL:
					vert.m_color = RGBColor::White();
					break;
				case MEDIUM:
					vert.m_color = RGBColor( 0.2f, 0.2f, 0.2f, 1.f );
					break;
				case UNLIT:
					vert.m_color = RGBColor( 0.0f, 0.0f, 0.0f, 1.f );
					break;
			}

			Vector2 top;
			top.x = MathUtilities::GetRandomFloatZeroToOne();
			top.y = MathUtilities::GetRandomFloatZeroToOne();
			float scale = 0.03f;

			vert.m_position = Vector3( x * windowSize + border, y * windowSize + border, 0.f );
			vert.m_texCoords = top;
			m_windowVertexList.push_back( vert );

			vert.m_position.x += windowSize - border;
			vert.m_texCoords.x += scale;
			m_windowVertexList.push_back( vert );

			vert.m_position.y += windowSize - border;
			vert.m_texCoords.y += scale;
			m_windowVertexList.push_back( vert );

			vert.m_position.x -= windowSize - border;
			vert.m_texCoords.x -= scale;
			m_windowVertexList.push_back( vert );
		}
	}
		
}

void BuildingFacadeTexture::CreateVBO()
{
	if( m_vboId == 0 )
	{
		GraphicManager::s_render->GenerateBuffer( 1, &m_vboId );
	}
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboId );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( SimpleVertex3D ) * m_windowVertexList.size(), m_windowVertexList.data(), GL_STATIC_DRAW );

	m_numVertex = m_windowVertexList.size();
}

void BuildingFacadeTexture::Render()
{
	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, m_buildingFacadeFBO->m_fboID );
	GraphicManager::s_render->ClearColor( 0.f, 0.f, 0.f, 1.f );
 	GraphicManager::s_render->ClearDepth( 1.f );
 	GraphicManager::s_render->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	GraphicManager::s_render->Enable( GL_TEXTURE_2D );

	m_shaderProgram->UseShaderProgram();

	glViewport( 0, 0, m_textureSize, m_textureSize );
	Matrix44 orthoMatrix = Matrix44::CreateOrthoMatrix( 0, m_textureSize, 0, m_textureSize, 0.0f, 1.f );
	m_shaderProgram->SetMat4UniformValue( "u_mvp", orthoMatrix );
	m_shaderProgram->SetIntUniformValue( "u_noiseTexture", 0 );

	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, m_noiseTexture->m_openglTextureID );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboId );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_position ) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_color ) );
	glVertexAttribPointer( VERTEX_ATTRIB_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_texCoords ) );

	GraphicManager::s_render->DrawArray( GL_QUADS ,0 , m_numVertex );

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );

	m_shaderProgram->DisableShaderProgram();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	GraphicManager::s_render->Disable( GL_TEXTURE_2D );

	glViewport( 0, 0, WINDOW_PHYSICAL_WIDTH, WINDOW_PHYSICAL_HEIGHT );
}

