#include "World.hpp"
#include "SoulStoneEngine/JobSystem/MemoryPoolManager.hpp"

World*			theWorld = nullptr;
WorldCoords2D	g_mouseWorldPosition;
WorldCoords2D	g_mouseScreenPosition;
bool			g_isLeftMouseDown;
bool			g_isRightMouseDown;
bool			g_isHoldingShift;
bool			g_isQuitting = false;
Matrix44		g_viewMatrix;
Camera3D*		World::s_camera3D = nullptr;
FBO*			World::s_fbo = nullptr;

World::World()
{
	Initialize();
	m_buildingTexture = new BuildingFacadeTexture();
}

World::~World()
{
}

void World::InitializeGraphic()
{
	GraphicManager::Instance()->Initialize();
	GraphicManager::Instance()->CreateOpenGLRender( "Main Render" );
	GraphicManager::Instance()->ActivateRender( "Main Render" );

	float aspect = ( WINDOW_VIRTUAL_WIDTH / WINDOW_VIRTUAL_HEIGHT );
	float fovX = 70.f;
	float fovY = (fovX / aspect);
	float zNear = 0.1f;
	float zFar = 1000.f;
	s_camera3D = Camera3D::CreateOrGetCameraByName( "Main Camera" );
	s_camera3D->SetupPerspectiveProjection( fovY, aspect, zNear, zFar );
	s_camera3D->m_cameraPosition = Vector3( 50.f, 50.f, 20.f );
}

void World::Initialize()
{
	InitializeGraphic();
	InitializeTime();
	g_theConsole->InitializeConsole();
	m_mouseClickColor = RGBColor( 0.f,0.f,0.f, 0.f );
	m_mouseClickRadius = 0.f;
	m_mouseClickFlashTime = 0.f;

	m_cityManager = new CityManager();

	for(int i = 0; i < 256; i++)
	{
		IsKeyDownLastFrame[i] = IsKeyDownKeyboard[i];
	}
	m_renderWorldOriginAxes = true;

	//GraphicManager::s_render->CreateInfinitePointLight( Vector3( 25.f, 25.f, 25.0), Vector3( 0.f, 0.f, -1.f), RGBColor( 1.f, 1.f, 1.f, 1.f ) );
	//GraphicManager::s_render->CreateDirectionalLight( Vector3( 0.f, 0.f, 25.0), Vector3( 1.f, 1.f, -1.f), RGBColor( 1.f, 1.f, 1.f, 1.f ), 0.5f );

	s_fbo = FBO::CreateOrGetFBOByName( "Main Screen FBO" );
	m_fboShaderProgram = new OpenGLShaderProgram( "Display FBO Shader", "./Data/Shader/FBOVertexShader.glsl","./Data/Shader/FBOFragmentShader.glsl" );
}

void World::ApplyCameraTransform()
{
	s_camera3D->ApplyCameraTranform();
	glLoadMatrixf( s_camera3D->GetProjectionViewMatrix().m_matrix );
}

bool World::ProcessKeyDownEvent(HWND , UINT wmMessageCode, WPARAM wParam, LPARAM )
{
	unsigned char asKey = (unsigned char) wParam;
	switch( wmMessageCode )
	{
		case WM_KEYDOWN:
			IsKeyDownKeyboard[asKey] = true;
			return true;
			break;

		case WM_KEYUP:
			IsKeyDownKeyboard[asKey] = false;
			return true;
			break;
	}
	return true;
}

Vector2 World::GetMouseSinceLastChecked()
{
	POINT centerCursorPos = { 800, 450 };
	POINT cursorPos;
	GetCursorPos( &cursorPos );
	SetCursorPos( centerCursorPos.x, centerCursorPos.y );
	Vector2i mouseDeltaInts( cursorPos.x - centerCursorPos.x, cursorPos.y - centerCursorPos.y );
	Vector2 mouseDeltas( (float) mouseDeltaInts.x, (float) mouseDeltaInts.y );
	return mouseDeltas;
}

void World::OpenOrCloseConsole()
{
	if( IsKeyDownKeyboard[ VK_OEM_3 ] && IsKeyDownKeyboard[ VK_OEM_3 ] != IsKeyDownLastFrame[ VK_OEM_3 ] )
	{
		if( g_theConsole->m_isOpen == false )
			g_theConsole->m_isOpen = true;
		else
			g_theConsole->m_isOpen = false;
	}
}

void World::Update(float elapsedTime)
{
	OpenOrCloseConsole();

	if( g_theConsole->m_isOpen )
	{
		elapsedTime = 0.f;
	}
	else
	{
		s_camera3D->Update( elapsedTime );
		UpdateFromKeyboard();
	}
	
	m_cityManager->Update( elapsedTime );
//	GraphicManager::s_render->SetUniformValueOfLightToShader( CityManager::s_buildingShaderProgram->m_shaderProgramID );

// 	static float hackTime = 0.f;
// 	hackTime += elapsedTime;
// 	GraphicManager::s_render->m_lightList[0]->m_position.x = 30.f * cos( hackTime ) + 30.f;

	for(int i = 0; i < 256; i++)
	{
		IsKeyDownLastFrame[i] = IsKeyDownKeyboard[i];
	}
	g_isLeftMouseDown = false;
	g_isRightMouseDown = false;
}

void World::Render()
{
	GraphicManager::s_render->BindFrameBuffer( GL_FRAMEBUFFER, 0 );
	GraphicManager::s_render->ClearColor( 1.f, 1.f, 1.f, 1.f );
	GraphicManager::s_render->ClearDepth( 1.f );
	GraphicManager::s_render->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	GraphicManager::s_render->EnableDepthMask();
	GraphicManager::s_render->Enable( GL_DEPTH_TEST );

 	ApplyCameraTransform();

	m_cityManager->Render();

	if( m_renderWorldOriginAxes )
		RenderWorldAxes();

 	if( g_theConsole->m_isOpen )
 		g_theConsole->Render();
 	

#ifdef USE_MEMORY_MANAGER
	GetMemoryPoolManager()->PrintMemoryStatisticToScreen();
#endif
}


void World::RenderWorldAxes()
{
	GraphicManager::s_render->DisableAllTextureUnit();
	GraphicManager::s_render->DisableShaderProgram();
	GraphicManager::s_render->Disable( GL_DEPTH_TEST );
	GraphicManager::s_render->Disable( GL_TEXTURE_2D );
	GraphicManager::s_render->LineWidth( 1.f );
	GraphicManager::s_render->Enable( GL_LINE_SMOOTH );

	GraphicManager::s_render->PushMatrix();
	GraphicManager::s_render->Scalef(1.f,1.f,1.f);

	GraphicManager::s_render->BeginDraw(GL_LINES);
	{
		GraphicManager::s_render->Color4f(1.f,0.f,0.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(1.f,0.f,0.f);

		GraphicManager::s_render->Color4f(0.f,1.f,0.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(0.f,1.f,0.f);

		GraphicManager::s_render->Color4f(0.f,0.f,1.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,1.f);
	}
	GraphicManager::s_render->EndDraw();

	GraphicManager::s_render->Enable(GL_DEPTH_TEST);
	GraphicManager::s_render->LineWidth(3.f);
	GraphicManager::s_render->BeginDraw(GL_LINES);
	{
		GraphicManager::s_render->Color4f(1.f,0.f,0.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(1.f,0.f,0.f);

		GraphicManager::s_render->Color4f(0.f,1.f,0.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(0.f,1.f,0.f);

		GraphicManager::s_render->Color4f(0.f,0.f,1.f,1.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,0.f);
		GraphicManager::s_render->Vertex3f(0.f,0.f,1.f);
	}
	GraphicManager::s_render->EndDraw();
	GraphicManager::s_render->PopMatrix();

	GraphicManager::s_render->Color4f(1.f,1.f,1.f,1.f);
	GraphicManager::s_render->LineWidth(1.f);
	GraphicManager::s_render->Enable(GL_DEPTH_TEST);
}

void World::DrawCursor()
{
	GraphicManager::s_render->PushMatrix();
	GraphicManager::s_render->Translated( g_mouseWorldPosition.x, g_mouseWorldPosition.y, 0.f );
	GraphicManager::s_render->Scalef( .1f, .1f, .1f );
	GraphicManager::s_render->LineWidth( 3.f );
	GraphicManager::s_render->Color3f( 1.f, 0.3f, 0.9f );
	GraphicManager::s_render->BeginDraw( GL_LINES );
	GraphicManager::s_render->Vertex3f( 1.f, 1.f, 0.f );
	GraphicManager::s_render->Vertex3f( -1.f, -1.f, 0.f );
	GraphicManager::s_render->Vertex3f( -1.f, 1.f, 0.f );
	GraphicManager::s_render->Vertex3f( 1.f, -1.f, 0.f );
	GraphicManager::s_render->EndDraw();
	GraphicManager::s_render->PopMatrix();
}

bool World::ProcessMouseDownEvent(HWND, UINT wmMessageCode, WPARAM, LPARAM)
{
	switch( wmMessageCode )
	{
		case WM_LBUTTONDOWN:
			g_isLeftMouseDown = true;	
			break;

		case WM_LBUTTONUP:
			g_isLeftMouseDown = false;	
			break;

		case WM_RBUTTONDOWN:
			g_isRightMouseDown = true;
			break;

		case WM_RBUTTONUP:
			g_isRightMouseDown = false;	
			break;
	}
	return true;
}

void World::RenderFlashCursor()
{
	GraphicManager::s_render->Draw2DFilledCircle( g_mouseWorldPosition, m_mouseClickRadius, m_mouseClickColor );
}

void World::UpdateFromKeyboard()
{
	if( IsKeyDownKeyboard[ 'O' ] && IsKeyDownKeyboard[ 'O' ] != IsKeyDownLastFrame[ 'O' ] )
	{
		if(m_renderWorldOriginAxes == false)
			m_renderWorldOriginAxes = true;
		else
			m_renderWorldOriginAxes = false;
	}

	if( IsKeyDownKeyboard[ 'Q' ] )
	{
		g_theConsole->ExecuteCommand( "Quit", "Quit Program." );
	}
}

void World::RenderFBOToScreen()
{
	std::vector<Vertex3D> fboVertexArray;
	Vertex3D vertex;
	Matrix44 orthoMatrix;

	int fboColorTextureUniformLoc = glGetUniformLocation( m_fboShaderProgram->m_shaderProgramID, "u_colorTexture");
	int fboDepthTextureUniformLoc = glGetUniformLocation( m_fboShaderProgram->m_shaderProgramID, "u_depthTexture");
	int fboWordToScreenMatrixUniformLoc = glGetUniformLocation( m_fboShaderProgram->m_shaderProgramID, "u_WorldToScreenMatrix");
	int fboTimeUniformLoc = glGetUniformLocation( m_fboShaderProgram->m_shaderProgramID, "u_time");

	orthoMatrix = Matrix44::CreateOrthoMatrix( 0.0, WINDOW_PHYSICAL_WIDTH, 0.0, WINDOW_PHYSICAL_HEIGHT, 0.0, 1.0 );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearDepth( 1.f );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_TEXTURE_2D );
	glDepthMask( GL_FALSE );

	glUseProgram( m_fboShaderProgram->m_shaderProgramID );

	static float hackTime = 0.f;
	hackTime += 0.017f;

	glUniform1i( fboColorTextureUniformLoc, 0 );
	glUniform1i( fboDepthTextureUniformLoc, 1 );
	glUniformMatrix4fv( fboWordToScreenMatrixUniformLoc, 1, GL_FALSE,  orthoMatrix.m_matrix );
	glUniform1f( fboTimeUniformLoc, hackTime );

	GraphicManager::s_render->Enable( GL_TEXTURE_2D );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, s_fbo->m_fboColorTextureID );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, s_fbo->m_fboBufferDepthTextureID );

	vertex.m_color = RGBColor::White();
	vertex.m_texCoords = Vector2( 0.0f, 0.0f ); 
	vertex.m_position = Vector3( 0.f, 0.f, 0.f );
	fboVertexArray.push_back(vertex);

	vertex.m_texCoords = Vector2( 1.0f, 0.0f ); 
	vertex.m_position = Vector3( 1600.f, 0.f, 0.f );
	fboVertexArray.push_back(vertex);

	vertex.m_texCoords = Vector2( 1.0f, 1.0f ); 
	vertex.m_position = Vector3( 1600.f , 900.f, 0.f  );
	fboVertexArray.push_back(vertex);

	vertex.m_texCoords = Vector2( 0.f, 1.f ); 
	vertex.m_position = Vector3( 0.f, 900.f, 0.f );
	fboVertexArray.push_back(vertex);

	static unsigned int id = 0;
	GraphicManager::s_render->GenerateBuffer(1,&id);
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, id );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( Vertex3D ) * fboVertexArray.size(), fboVertexArray.data(), GL_STATIC_DRAW );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, id );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex3D ), (const GLvoid*) offsetof(Vertex3D,m_position) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( Vertex3D ), (const GLvoid*) offsetof(Vertex3D,m_color) );
	glVertexAttribPointer( VERTEX_ATTRIB_TEXCOORDS, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex3D ), (const GLvoid*) offsetof(Vertex3D,m_texCoords) );

	GraphicManager::s_render->DrawArray( GL_QUADS, 0, 4 );

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_TEXCOORDS );
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );
	GraphicManager::s_render->Disable( GL_TEXTURE_2D );
	GraphicManager::s_render->Disable( GL_DEPTH_TEST );

	glUseProgram( 0 );

	glDisable( GL_TEXTURE_2D );
	glEnable( GL_DEPTH_TEST );
}


