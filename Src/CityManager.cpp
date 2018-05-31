#include "CityManager.hpp"
#include "World.hpp"
#include "SoulStoneEngine/Utilities/Noise.hpp"

BuildingFacadeTexture* CityManager::s_buildingFacadeTexture = nullptr;
OpenGLShaderProgram*   CityManager::s_buildingShaderProgram = nullptr;
OpenGLShaderProgram*   CityManager::s_bloomShaderProgram = nullptr;
int					   CityManager::s_cityWidthX = 0;
int					   CityManager::s_cityDepthY = 0;

CityManager::CityManager()
{
	s_buildingFacadeTexture = new BuildingFacadeTexture();
	s_buildingShaderProgram = new OpenGLShaderProgram( "Shader", "./Data/Shader/BuildingVertexShader.glsl", "./Data/Shader/BuildingFragmentShader.glsl" );
	s_bloomShaderProgram = new OpenGLShaderProgram( "Bloom Shader", "./Data/Shader/FBOVertexShader.glsl", "./Data/Shader/CombiningBloomFragmentShader.glsl" );

	m_streetTexture = Texture::CreateOrGetTexture( "./Data/Texture/streetTexture2.png" );
	m_isStreetVBOdirty = true;
	m_streetVBOid = 0;
	m_isRenderingGlow = false;
	m_numBlockX = 0;
	m_numBlockY = 0;

	m_colorFBO = FBO::CreateOrGetFBOByName( "City Color Pass" );
	m_glowFBO = FBO::CreateOrGetFBOByName( "City Glow Pass" );

	m_glowFBOHalfRes = FBO::CreateOrGetFBOByName( "City Glow Pass 1/2 Res", HALF_RESOLUTION_WIDTH, HALF_RESOLUTION_HEIGHT );
	m_glowFBOAFourthRes = FBO::CreateOrGetFBOByName( "City Glow Pass 1/4 Res", A_FOURTH_RESOLUTION_WIDTH, A_FOUTH_RESOLUTION_HEIGHT );

	m_hBlurFBO = FBO::CreateOrGetFBOByName( "Horizontal Blur" );
	m_vBlurFBO = FBO::CreateOrGetFBOByName( "Vertical Blur" );

	m_hBlurFBOHalfRes = FBO::CreateOrGetFBOByName( "Horizontal Blur 1/2 Res" );
	m_vBlurFBOHalfRes = FBO::CreateOrGetFBOByName( "Vertical Blur 1/2 Res" );

	m_hBlurFBOAFourthRes = FBO::CreateOrGetFBOByName( "Horizontal Blur 1/4 Res" );
	m_vBlurFBOAFourthRes = FBO::CreateOrGetFBOByName( "Vertical Blur 1/4 Res" );

	m_finalFBO = FBO::CreateOrGetFBOByName( "Final Pass" );

	m_bloomIntensity = 1.f;
	m_bloomSaturation = 1.f;
	m_originalIntensity = 1.f;
	m_originalSaturation = 1.f;

	GenerateBuildingBlocks();
	GenerateStreetVertices();
	GenerateCars();

	m_skyboxTexture = Texture::CreateOrGetTexture( "./Data/Texture/skybox.jpg" );
	m_skybox = new SkyBox( s_cityWidthX * 0.5f, 75.f, World::s_camera3D, m_skyboxTexture );
}

CityManager::~CityManager()
{
	delete s_buildingShaderProgram;
	delete s_buildingFacadeTexture;

	s_buildingShaderProgram = nullptr;
	s_buildingFacadeTexture = nullptr;
}

void CityManager::Update( float elapsedTime )
{
	for( int i = 0; i < m_cars.size(); i++ )
		m_cars[i]->Update( elapsedTime );

	for( int i = 0; i < m_buildingBlocks.size(); i++ )
	{
		for( int j = 0; j < m_buildingBlocks[i]->m_buildings.size(); j++ )
		{
			m_buildingBlocks[i]->m_buildings[j]->Update( elapsedTime );
		}
	}

	if( IsKeyDownKeyboard[ '5' ])
	{
		m_bloomIntensity = 1.f;
		m_bloomSaturation = 1.f;
		m_originalIntensity = 1.f;
		m_originalSaturation = 1.f;
	}

	if( IsKeyDownKeyboard[ '1' ] )
	{
		m_bloomIntensitySelected = true;
		m_originalIntensitySelected = false;
	}

	if( m_bloomIntensitySelected )
	{
		if( IsKeyDownKeyboard[ VK_UP ] )
		{
			m_bloomIntensity += 0.1f;
		}
		else if( IsKeyDownKeyboard[ VK_DOWN ] )
			m_bloomIntensity -= 0.1f;
	}

	if( IsKeyDownKeyboard[ '2' ] )
	{
		m_originalIntensitySelected = true;
		m_bloomIntensitySelected = false;
	}

	if( m_originalIntensitySelected  )
	{
		if( IsKeyDownKeyboard[ VK_UP ] )
		{
			m_originalIntensity += 0.1f;
		}
		else if( IsKeyDownKeyboard[ VK_DOWN ] )
			m_originalIntensity -= 0.1f;
	}
}

void CityManager::Render()
{
	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, m_colorFBO->m_fboID );
	GraphicManager::s_render->ClearColor( 0.f, 0.f, 0.f, 1.f );
	GraphicManager::s_render->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_isRenderingGlow = false;

	m_skybox->Render();
	RenderBlocks();
	RenderCars();
	RenderStreet();

	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, 0 );

// 	GraphicManager::RenderFBOToScreenOrFBO( nullptr, m_colorFBO, 0, Vector2( 0.f, 0.f ) );

	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, m_glowFBO->m_fboID );
	GraphicManager::s_render->ClearColor( 0.f, 0.f, 0.f, 1.f );
	GraphicManager::s_render->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_isRenderingGlow = true;

	RenderBlocks();
	RenderCars();
	RenderStreet();

	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, 0 );

	glViewport( 0.0, 0.0, HALF_RESOLUTION_WIDTH, HALF_RESOLUTION_HEIGHT );
	GraphicManager::RenderFBOToScreenOrFBO( m_glowFBOHalfRes, m_glowFBO, 0, Vector2( 0.f, 0.f ) );

	glViewport( 0.0, 0.0, A_FOURTH_RESOLUTION_WIDTH, A_FOUTH_RESOLUTION_HEIGHT );
	GraphicManager::RenderFBOToScreenOrFBO( m_glowFBOAFourthRes, m_glowFBO, 0, Vector2( 0.f, 0.f ) );

	glViewport( 0.0, 0.0, WINDOW_PHYSICAL_WIDTH, WINDOW_PHYSICAL_HEIGHT );

	GraphicManager::RenderFBOToScreenOrFBO( m_hBlurFBO, m_glowFBO,  1, Vector2( 1.f, 0.f ) );
	GraphicManager::RenderFBOToScreenOrFBO( m_vBlurFBO, m_hBlurFBO, 1, Vector2( 0.f, 1.f ) );

	GraphicManager::RenderFBOToScreenOrFBO( m_hBlurFBOHalfRes, m_glowFBOHalfRes,  1, Vector2( 1.f, 0.f ) );
	GraphicManager::RenderFBOToScreenOrFBO( m_vBlurFBOHalfRes, m_hBlurFBOHalfRes, 1, Vector2( 0.f, 1.f ) );

	GraphicManager::RenderFBOToScreenOrFBO( m_hBlurFBOAFourthRes, m_glowFBOAFourthRes,  1, Vector2( 1.f, 0.f ) );
	GraphicManager::RenderFBOToScreenOrFBO( m_vBlurFBOAFourthRes, m_hBlurFBOAFourthRes, 1, Vector2( 0.f, 1.f ) );

//	GraphicManager::RenderFBOToScreenOrFBO( nullptr, m_glowFBO, 0, Vector2( 0.f,0.f ) );
//	GraphicManager::RenderFBOToScreenOrFBO( m_verticalBlurFBO, nullptr, 0, Vector2( 0.f, 0.f ) );

//	GraphicManager::RenderFBOToScreenOrFBO( m_glowFBO, nullptr, 0, Vector2( 0.f, 0.f ) );
	CombiningBloom();
}

void CityManager::CombiningBloom()
{
	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, 0 );

	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearDepth( 1.f );
	glDisable( GL_DEPTH_TEST );
//	glEnable( GL_TEXTURE_2D );
	glDepthMask( GL_FALSE );

	Matrix44 orthoMatrix = Matrix44::CreateOrthoMatrix( 0.f, WINDOW_PHYSICAL_WIDTH, 0.f, WINDOW_PHYSICAL_HEIGHT, 0.f, 1.f );

	GraphicManager::s_render->UseShaderProgram( s_bloomShaderProgram->m_shaderProgramID );

	s_bloomShaderProgram->SetMat4UniformValue( "u_WorldToScreenMatrix", orthoMatrix.m_matrix );
	s_bloomShaderProgram->SetIntUniformValue( "u_originalTexture", 0 );
	s_bloomShaderProgram->SetIntUniformValue( "u_bloomTexture", 1 );
	s_bloomShaderProgram->SetIntUniformValue( "u_bloomTextureHalfRes", 2 );
	s_bloomShaderProgram->SetIntUniformValue( "u_bloomTextureAFourthRes", 3 );
	s_bloomShaderProgram->SetFloatUniformValue( "u_bloomIntensity", m_bloomIntensity );
	s_bloomShaderProgram->SetFloatUniformValue( "u_bloomSaturation", m_bloomSaturation );
	s_bloomShaderProgram->SetFloatUniformValue( "u_originalIntensity", m_originalIntensity );
	s_bloomShaderProgram->SetFloatUniformValue( "u_originalSaturation", m_originalSaturation );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_colorFBO->m_fboColorTextureID );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, m_vBlurFBO->m_fboColorTextureID );

	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, m_vBlurFBOHalfRes->m_fboColorTextureID );

	glActiveTexture( GL_TEXTURE3 );
	glBindTexture( GL_TEXTURE_2D, m_vBlurFBOAFourthRes->m_fboColorTextureID );

	static unsigned int id = 0;

	if( id == 0 )
		GraphicManager::s_render->GenerateBuffer( 1, &id );
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, id );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( SimpleVertex3D ) * 4, GraphicManager::s_screenVertices, GL_STATIC_DRAW );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, id );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_position ) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_color ) );
	glVertexAttribPointer( VERTEX_ATTRIB_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_texCoords ) );

	GraphicManager::s_render->DrawArray( GL_QUADS, 0, 4 );

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );

	GraphicManager::s_render->Disable( GL_TEXTURE_2D );
	GraphicManager::s_render->Disable( GL_DEPTH_TEST );

	GraphicManager::s_render->DisableShaderProgram();
}

void CityManager::RenderBlocks()
{
	s_buildingShaderProgram->UseShaderProgram();

// 	GraphicManager::s_render->ClearColor( 0.f, 0.f, 0.f, 1.f );
// 	GraphicManager::s_render->Clear( GL_COLOR_BUFFER_BIT );

	GraphicManager::s_render->Enable( GL_DEPTH_TEST );
	GraphicManager::s_render->Enable( GL_TEXTURE_2D );
	GraphicManager::s_render->Enable(GL_CULL_FACE);

	Matrix44 modelMatrix;

	s_buildingShaderProgram->SetMat4UniformValue( "u_vp", World::s_camera3D->GetProjectionViewMatrix() );
	s_buildingShaderProgram->SetMat4UniformValue( "u_modelMatrix", modelMatrix );
	s_buildingShaderProgram->SetIntUniformValue( "u_renderGlowPart", m_isRenderingGlow );
	s_buildingShaderProgram->SetIntUniformValue( "u_diffuseTexture", 0 );
	s_buildingShaderProgram->SetFloatUniformValue( "u_fogStartDistance", 100.f );
	s_buildingShaderProgram->SetFloatUniformValue( "u_fogEndDistance", 175.f );
	s_buildingShaderProgram->SetVec4UniformValue( "u_fogColor", Vector4( 0.f, 0.f, 0.f, 0.9f ) );
	s_buildingShaderProgram->SetVec3UniformValue( "u_cameraWorldPosition", World::s_camera3D->m_cameraPosition );

	for( int i = 0; i < m_buildingBlocks.size(); i++ )
	{
		for( int j = 0; j < m_buildingBlocks[i]->m_buildings.size(); j++ )
		{
			m_buildingBlocks[i]->m_buildings[j]->Render();
		}
	}

	GraphicManager::s_render->Disable( GL_TEXTURE_2D );

	s_buildingShaderProgram->DisableShaderProgram();
}

void CityManager::GenerateBuildingBlocks()
{
	m_numBlockX = MathUtilities::GetRandomNumber( 10, 10 );
	m_numBlockY = MathUtilities::GetRandomNumber( 10, 10 );

	std::vector< unsigned char > blocKSizeList;

	BlockInfo info;
	Vector2 bottomLeft;

	for( int y = 0; y < m_numBlockY; y++ )
	{
		unsigned char blockDepth = MathUtilities::GetRandomNumber( 25, 40 );
		unsigned char topOffset = MathUtilities::GetRandomNumber( 3, 6 );

		bottomLeft.x = 0.f;
		s_cityDepthY += blockDepth;

		for( int x = 0; x < m_numBlockX; x++ )
		{
			info.bottomLeft = bottomLeft;

			unsigned char blockWidth;
			unsigned char rightOffset;
			if( y == 0 )
			{
				blockWidth = MathUtilities::GetRandomNumber( 25, 40 );
				rightOffset = MathUtilities::GetRandomNumber( 3, 6 );
			}
			else
			{
				blockWidth = m_blocksInfo[x].widthX;
				rightOffset = m_blocksInfo[x].rightOffset;
			}

			info.widthX = blockWidth;
			info.depthY = blockDepth;
			info.topOffset = topOffset;
			info.rightOffset = rightOffset;

			if( x == 0 )
				s_cityWidthX += blockWidth;

			m_blocksInfo.push_back( info );
			
			bottomLeft.x += ( blockWidth + rightOffset );
		}

		bottomLeft.y += ( blockDepth + topOffset );
	}

	Vector3 position;
	for( int y = 0; y < m_numBlockY; y++ )
	{
		position.x = 0;
		for( int x = 0; x < m_numBlockX; x++ )
		{
			int index = x + y * m_numBlockX;

			unsigned char blockDepth = m_blocksInfo[index].depthY;
			unsigned char blockWidth = m_blocksInfo[index].widthX;

			float scaleX = ( cos( float(x) ) + 1.5f ) * 0.5f;
			float scaleY = ( cos( float(y) ) + 1.5f ) * 0.5f;
			float scale = scaleX + scaleY;
			scale = MathUtilities::Clamp(  0.f, 1.f, scale  );

			float blockHeight = MathUtilities::GetRandomFloatInRange( 10.f, 30.f ) * scale;

//			float blockHeight = ComputePerlinNoiseValueAtPosition2D( Vector2( x, y ), 1, 4, 5.f, 1.f ) + 20.f;
//			blockHeight *= scale;
			
			BuildingBlock* block = new BuildingBlock( position, blockHeight, blockWidth, blockDepth );

			position.x += ( blockWidth + m_blocksInfo[index].rightOffset );

			m_buildingBlocks.push_back( block );
		}
		position.y += ( m_blocksInfo[ y * m_numBlockX ].depthY + m_blocksInfo[ y * m_numBlockX ].topOffset );
	}
}

void CityManager::VisualizeBuildingBlocks()
{
	GraphicManager::s_render->DisableShaderProgram();
	GraphicManager::s_render->PushMatrix();
	GraphicManager::s_render->Disable(GL_DEPTH_TEST);
	GraphicManager::s_render->Disable(GL_TEXTURE_2D);
	GraphicManager::s_render->Scalef(1.f,1.f,1.f);
	GraphicManager::s_render->LineWidth(1.f);
	GraphicManager::s_render->Enable(GL_LINE_SMOOTH);

	GraphicManager::s_render->Color4f( 0.2f, 0.2f, 0.2f, 1.f );
	GraphicManager::s_render->BeginDraw(GL_QUADS);
	{
		for( unsigned int i = 0; i < m_buildingBlocks.size(); i++ )
		{	
			Vector3 pos = m_buildingBlocks[i]->m_bottomLeft;
			unsigned char width = m_buildingBlocks[i]->m_widthX;
			unsigned char height = m_buildingBlocks[i]->m_depthY;

			GraphicManager::s_render->Vertex3f( pos.x, pos.y, pos.z );
			GraphicManager::s_render->Vertex3f( pos.x + width, pos.y, pos.z  );
			GraphicManager::s_render->Vertex3f( pos.x + width, pos.y + height, pos.z  );
			GraphicManager::s_render->Vertex3f( pos.x, pos.y + height , pos.z  );
		}
	}
	GraphicManager::s_render->EndDraw();

	GraphicManager::s_render->PopMatrix();

	GraphicManager::s_render->Color4f(1.f,1.f,1.f,1.f);
	GraphicManager::s_render->LineWidth(1.f);
	GraphicManager::s_render->Enable(GL_DEPTH_TEST);
}

void CityManager::GenerateStreetVertices()
{
	int numBlockInARow =  sqrt( (float) m_blocksInfo.size() );

	Vector3 bottomLeft;
	int	advanceY;
	SimpleVertex3D vertex;


	vertex.m_normal.z = 1.f;
	vertex.m_color.m_alpha = 1.f;
	vertex.m_color.m_red = 1.f;
	vertex.m_color.m_green = 1.f;
	vertex.m_color.m_blue = 1.f;

	for( unsigned int i = 0; i < numBlockInARow; i++ )
	{
		for( unsigned int j = 0; j < numBlockInARow; j++ )
		{
			int index = j + i * numBlockInARow;
			BlockInfo block = m_blocksInfo[ index ];

			advanceY = ( block.depthY + block.topOffset );

			// right size street
			vertex.m_position = bottomLeft;
			vertex.m_position.x += block.widthX;
			vertex.m_texCoords = Vector2( 0.f, 1.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.x += block.rightOffset;
			vertex.m_texCoords = Vector2( 1.f, 1.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.y += block.depthY;
			vertex.m_texCoords = Vector2( 1.f, 0.f - 5.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.x -= block.rightOffset;
			vertex.m_texCoords = Vector2( 0.f, 0.f - 5.f );
			m_streetVertices.push_back( vertex );

			// top size street
			vertex.m_position = bottomLeft;
			vertex.m_position.y += block.depthY;
			vertex.m_texCoords = Vector2( 0.f, 0.f + 5.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.x += block.widthX;
			vertex.m_texCoords = Vector2( 0.f, 1.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.y += block.topOffset;
			vertex.m_texCoords = Vector2( 1.f, 1.f );
			m_streetVertices.push_back( vertex );

			vertex.m_position.x -= block.widthX;
			vertex.m_texCoords = Vector2( 1.f, 0.f + 5.f );
			m_streetVertices.push_back( vertex );

			bottomLeft.x += ( block.widthX + block.rightOffset );
		}
		bottomLeft.x = 0.f;
		bottomLeft.y += advanceY;
	}
}

void CityManager::RenderStreet()
{
	if( m_isStreetVBOdirty )
		CreateVBOForStreetVertices();

	Matrix44 modelMatrix;

	s_buildingShaderProgram->UseShaderProgram();

	s_buildingShaderProgram->SetMat4UniformValue( "u_vp", World::s_camera3D->GetProjectionViewMatrix() );
	s_buildingShaderProgram->SetMat4UniformValue( "u_modelMatrix", modelMatrix );
	s_buildingShaderProgram->SetIntUniformValue( "u_diffuseTexture", 0 );

	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, m_streetTexture->m_openglTextureID );

	GraphicManager::s_render->Enable( GL_DEPTH_TEST );
	GraphicManager::s_render->Enable( GL_TEXTURE_2D );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_NORMALS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_streetVBOid );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_position ) );
	glVertexAttribPointer( VERTEX_ATTRIB_NORMALS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_normal ) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_color ) );
	glVertexAttribPointer( VERTEX_ATTRIB_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_texCoords ) );

	GraphicManager::s_render->DrawArray( GL_QUADS ,0 , m_streetVertices.size() );

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_NORMALS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );

	GraphicManager::s_render->Disable( GL_TEXTURE_2D );

	s_buildingShaderProgram->DisableShaderProgram();
}

void CityManager::CreateVBOForStreetVertices()
{
	if( m_streetVBOid == 0 )
	{
		GraphicManager::s_render->GenerateBuffer( 1, &m_streetVBOid );
	}

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_streetVBOid );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( SimpleVertex3D ) * m_streetVertices.size(), m_streetVertices.data(), GL_STATIC_DRAW );

	m_isStreetVBOdirty = false;
}

void CityManager::GenerateCars()
{
	Car* car;
	int blocKIndex = 0;

	for( blocKIndex = 0; blocKIndex < m_numBlockX; blocKIndex++ )
	{
		int numCar = MathUtilities::GetRandomNumber( 75, 100 );
		BlockInfo block = m_blocksInfo[ blocKIndex ];
		Vector2 initPos;
		float left =  block.bottomLeft.x + block.widthX;
		float right = left + block.rightOffset;
		float middle = ( left + right ) * 0.5f;

		int roll = MathUtilities::GetRandomNumber( 0, 1 );
		int lineDirection = 0;
		RGBColor color;
		if( roll == 0 )
		{
			lineDirection = -1;
			color = RGBColor( 1.f, 0.5f, 0.5f, 1.f );
			color.m_alpha = 0.1f;
		}
		else
		{
			lineDirection = 1;
			color = RGBColor( 0.99f, 0.98f, 0.84f, 1.f );
			color.m_alpha = 0.1f;
		}

		for( int i = 0; i < numCar; i++ )
		{
			initPos.x = MathUtilities::GetRandomFloatInRange( left, right );
			initPos.y = MathUtilities::GetRandomNumber( 0.f, s_cityDepthY );
			Vector2 velocity = Vector2( 0, 2 );

			if( initPos.x < middle )
				velocity.y *= lineDirection;
			else
				velocity.y *= -lineDirection;

			Car* car = new Car( initPos, velocity, color );
			m_cars.push_back( car );
		}
	}

	blocKIndex = 0;
	for( int i = 0; i < m_numBlockY; i++ )
	{
		BlockInfo block = m_blocksInfo[ blocKIndex ];
		int numCar = MathUtilities::GetRandomNumber( 75, 100 );
		Vector2 initPos;
		float bottom =  block.bottomLeft.y + block.depthY;
		float top = bottom + block.topOffset;
		float middle = ( top + bottom ) * 0.5f;

		int roll = MathUtilities::GetRandomNumber( 0, 1 );
		int lineDirection = 0;
		RGBColor color;
		if( roll == 0 )
		{
			lineDirection = -1;
			color = RGBColor( 0.99f, 0.88f, 0.58f, 1.f );
			color.m_alpha = 0.1f;
		}
		else
		{
			lineDirection = 1;
			color = RGBColor( 0.99f, 0.98f, 0.84f, 1.f );
			color.m_alpha = 0.1f;
		}

		for( int i = 0; i < numCar; i++ )
		{
			initPos.x = MathUtilities::GetRandomNumber( 0.f, s_cityWidthX );
			initPos.y = MathUtilities::GetRandomFloatInRange( bottom, top );
			Vector2 velocity = Vector2( 2, 0 );

			if( initPos.y < middle )
				velocity.x *= lineDirection;
			else
				velocity.x *= -lineDirection;

			Car* car = new Car( initPos, velocity, color );
			m_cars.push_back( car );
		}
		blocKIndex += m_numBlockX;
	}

}

void CityManager::RenderCars()
{
	GraphicManager::s_render->Disable( GL_TEXTURE_2D );
	GraphicManager::s_render->Enable( GL_DEPTH_TEST );

	s_buildingShaderProgram->UseShaderProgram();

	s_buildingShaderProgram->SetMat4UniformValue( "u_vp", World::s_camera3D->GetProjectionViewMatrix() );

	for( int i = 0; i < m_cars.size(); i++ )
		m_cars[i]->Render();

	s_buildingShaderProgram->DisableShaderProgram();

	GraphicManager::s_render->Enable( GL_TEXTURE_2D );

}

