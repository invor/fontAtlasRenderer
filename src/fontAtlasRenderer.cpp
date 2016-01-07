#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glowl/glowl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <locale>
#include <codecvt>

#include <iostream>
#include <fstream>
#include <sstream>
#include <array>


void writeFontAtlasTexture()
{

}

/**
 * Function to simply read the string of a shader source file from disk
 */
const std::string readShaderFile(const char* const path)
{
	std::ifstream inFile( path, std::ios::in );

	std::ostringstream source;
	while( inFile.good() ) {
		int c = inFile.get();
		if( ! inFile.eof() ) source << (char) c;
	}
	inFile.close();

	return source.str();
}

/**
 * Load a shader program
 * \attribute vs_path Path to vertex shader source file
 * \attribute fs_path Path to fragement shader source file
 * \attribute attributes Vertex shader input attributes (i.e. vertex layout)
 * \return Returns the handle of the created GLSL program
 */
std::shared_ptr<GLSLProgram> createShaderProgram(const char* const vs_path, const char* const fs_path, std::vector<const char* const> attributes)
{
	/* Create a shader program object */
	auto shader_prgm = std::make_shared<GLSLProgram>();
	shader_prgm->init();

	/* Set the location (i.e. index) of the attribute (basically the input variable) in the vertex shader.
	 * The vertices intended to be used with this program will have to match that index in their
	 * attribute decription, so that a connection between the vertex data and the shader input can be made.
	 */
	for(int i=0; i<attributes.size(); i++)
		shader_prgm->bindAttribLocation(i,attributes[i]);

	/* Read the shader source files */
	std::string vs_source = readShaderFile(vs_path);

	if(!shader_prgm->compileShaderFromString(&vs_source, GL_VERTEX_SHADER)){ std::cout<<shader_prgm->getLog();};

	/* Load, compile and attach fragment shader */
	std::string fs_source = readShaderFile(fs_path);

	if(!shader_prgm->compileShaderFromString(&fs_source,GL_FRAGMENT_SHADER)){ std::cout<<shader_prgm->getLog();};

	/* Link program */
	if(!shader_prgm->link()){ std::cout<<shader_prgm->getLog();};

	return shader_prgm;
}

int main(int argc, char*argv[])
{
	/////////////////////////////////////
	// overly simple command line parsing
	/////////////////////////////////////

	std::string font_path;

	int i=1;
    if (argc < 3) {
		std::cout<<"Supply a ttf file with -ff <graph.gl>"<<std::endl; return 0;
	}
	while(i<argc)
	{
		if(argv[i] == (std::string) "-ff")
		{
			i++;
			if(i<argc) { font_path = argv[i]; i++; }
			else { std::cout<<"Missing parameter for -ff"<<std::endl; return 0; }
		}
		else
		{
			i++;
		}
	}


	//////////////////////////////////////
	// Freetype Initialization
	//////////////////////////////////////

	FT_Library ft_lib;
    FT_Face face;

	if(FT_Init_FreeType(&ft_lib) != 0)
		return 0;
	//TODO Error Handling

	if(FT_New_Face(ft_lib,font_path.c_str(),0,&face) != 0)
		return 0;
	//TODO Error Handling


	//TODO Compute neccessary window size based on font

	FT_Set_Pixel_Sizes(face, 150.0, 0.0);
	const FT_GlyphSlot glyph = face->glyph;
	//FT_Load_Char(face, 'M', FT_LOAD_RENDER);

	// assuming monospace font, 14 charaters per line and a one character wide border around the atlas
	unsigned int character_width = (face->size->metrics.max_advance >> 6);
	//unsigned int character_width = 100;
	unsigned int character_height = (face->size->metrics.height >> 6);;
	unsigned int window_width = (16 * character_width);
	unsigned int window_height = (6 * character_height);

	unsigned int character_max_y = (face->size->metrics.ascender >> 6);

	/////////////////////////////////////
	// Window and OpenGL Context creation
	/////////////////////////////////////

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
	{
		std::cout<<"Failed to initialize glfw."<<std::endl;
        return -1;
	}

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "Simple Font Atlas Renderer", NULL, NULL);
    if (!window)
    {
		std::cout<<"Failed to create window."<<std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

	/*	Initialize glew */
	//glewExperimental = GL_TRUE;
	GLenum error = glewInit();
	if( GLEW_OK != error)
	{
		std::cout<<"-----\n"
				<<"The time is out of joint - O cursed spite,\n"
				<<"That ever I was born to set it right!\n"
				<<"-----\n"
				<<"Error: "<<glewGetErrorString(error);
		return false;
	}
	/* Apparently glewInit() causes a GL ERROR 1280, so let's just catch that... */
	glGetError();

	/////////////////////////////////////
	// Graphics Resources
	/////////////////////////////////////

	// Create fullscreen quad mesh
	Mesh m_fullscreenQuad;

	std::array< float, 20 > vertex_array = {{ -1.0,-1.0,0.0,0.0,0.0,
											-1.0,1.0,0.0,0.0,1.0,
											1.0,1.0,0.0,1.0,1.0,
											1.0,-1.0,0.0,1.0,0.0 }};

	std::array< GLuint, 6 > index_array = {{ 0,1,2,2,0,3 }};

	if(!(m_fullscreenQuad.bufferDataFromArray(vertex_array,index_array,GL_TRIANGLES))) return false;
	m_fullscreenQuad.setVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,4*5,(GLvoid*) 0);
	m_fullscreenQuad.setVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*5,(GLvoid*) (4*3) );

	std::shared_ptr<GLSLProgram> glyp_prgm = createShaderProgram("../src/glyph_v.glsl","../src/glyph_f.glsl",{"v_position","v_uv"});
	std::shared_ptr<GLSLProgram> fullscreen_prgm = createShaderProgram("../src/fullscreen_v.glsl","../src/fullscreen_f.glsl",{"v_position","v_uv"});

	std::shared_ptr<FramebufferObject> atlas_fbo = std::make_shared<FramebufferObject>(window_width,window_height);
	atlas_fbo->createColorAttachment(GL_R8,GL_RED,GL_UNSIGNED_BYTE);

	Mesh glyph_mesh;
	char data = 255;
	Texture2D glyph_tx2D("glyph_tx2D",GL_R8,1,1,GL_RED,GL_UNSIGNED_BYTE,&data);
	glyph_tx2D.texParameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glyph_tx2D.texParameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	//////////////////////////////////////////////////////
	// Render font atlas to fbo
	//////////////////////////////////////////////////////

	//	std::vector<std::string> atlas_rows;
	//	
	//	atlas_rows.push_back("ABCDEFGHIJKLMN");
	//	atlas_rows.push_back("OPQRSTUVWXYZab");
	//	atlas_rows.push_back("cdefghijklmnop");
	//	atlas_rows.push_back("qrstuvwxyz1234");
	//	atlas_rows.push_back("567890&@.,?!'\"");
	//	atlas_rows.push_back("\"()*-_ßöäü");

	// Using u16string, hoping to gain support for umlauts
	std::vector<std::u16string> u16_atlas_rows;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;

	u16_atlas_rows.push_back(utf16conv.from_bytes( "ABCDEFGHIJKLMN" ));
	u16_atlas_rows.push_back(utf16conv.from_bytes( "OPQRSTUVWXYZab" ));
	u16_atlas_rows.push_back(utf16conv.from_bytes( "cdefghijklmnop" ));
	u16_atlas_rows.push_back(utf16conv.from_bytes( "qrstuvwxyz1234" ));
	u16_atlas_rows.push_back(utf16conv.from_bytes( "567890&@.,?!'\"" ));
	u16_atlas_rows.push_back(utf16conv.from_bytes( "\"()*-_ßöäüÖÄÜ" ));

	//TOOD Bind fbo and stuff
	atlas_fbo->bind();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, atlas_fbo->getWidth(), atlas_fbo->getHeight());

	glyp_prgm->use();

	glActiveTexture(GL_TEXTURE0);
	glyph_tx2D.bindTexture();
	glyp_prgm->setUniform("glyph_tx2D",0);

	// scaling to [-1,1]
	float sx = 2.0/(float)window_width;
	float sy = 2.0/(float)window_height;

	// start point
	float x = -1.0 - (face->bbox.xMin >> 6)*sx + character_width*sx;
	float y = 1.0 - character_max_y*sy;

	std::cout<<"xMin: "<<(face->bbox.xMin >> 6)<<std::endl;

	//FT_Set_Pixel_Sizes(face, 900.0/14.0, 0.0);

	//const FT_GlyphSlot glyph = face->glyph;

	 glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for(auto& row : u16_atlas_rows)
	{
		for(auto c : row)
		{
			//TODO render character/glyph

			if(FT_Load_Char(face, c, FT_LOAD_RENDER) != 0)
				continue;

			 glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                     glyph->bitmap.width, glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE, glyph->bitmap.buffer);

			//glyph_tx2D.reload(glyph->bitmap.width,glyph->bitmap.rows,glyph->bitmap.buffer);
			//glyph_tx2D.bindTexture();

			const float vx = x + glyph->bitmap_left * sx;
			const float vy = y + glyph->bitmap_top * sy;
			const float w = glyph->bitmap.width * sx;
			const float h = glyph->bitmap.rows * sy;

			std::array<float,24> glyph_vertices =
						{{vx    , vy    , 0.0, 0.0,
						  vx    , vy - h, 0.0, 1.0,
						  vx + w, vy    , 1.0, 0.0,
						  vx + w, vy - h, 1.0, 1.0}};

			std::array< GLuint, 6 > glyph_indices = {{ 0,1,2,2,3,1 }};

			glyph_mesh.bufferDataFromArray(glyph_vertices,glyph_indices,GL_TRIANGLES);
			glyph_mesh.setVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*4,0);
			glyph_mesh.setVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*4,(GLvoid*) (4*2) );

			glyph_mesh.draw();

			x += character_width * sx;
		}

		//TODO shift row position
		x = -1.0 - (face->bbox.xMin >> 6)*sx + character_width*sx;;
		y -= 1.0 * character_height*sy;
	}

	 glPixelStorei(GL_UNPACK_ALIGNMENT, 4);


	//////////////////////////////////////////////////////
	// Render Loop - Simply displays fbo until window is closed
	//////////////////////////////////////////////////////

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window_width, window_height);

		fullscreen_prgm->use();

		glActiveTexture(GL_TEXTURE0);
		atlas_fbo->bindColorbuffer(0);
		fullscreen_prgm->setUniform("input_tx2D",0);

		m_fullscreenQuad.draw();

		 /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
	}

	//TODO write texture to file
}