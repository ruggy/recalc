/*
    This is the code that deals with textures in Recalc. 




*/
#include "recalc.h"

// Lol.
struct Texture
{
    int glID ;
} ;


/*
    This is a singleton, buddy. 
*/
struct RecalcTextures
{
    // main textures: all basic solid geometric surface textures
    // Note that this reference id will be used for either texarrays or texatlases
    GLuint T_mt ;   
} ;

RecalcTextures  recalctextures ;


vector<char*> texturenames ;

/*
    This function depends on init scripts having run. 

    Recalc tracks its available textures by first running through all its 
    config scripts, a process which will gather all texture names listed in the 
    configs. Armed with these names, LoadAllTextures then creates a proper-sized 
    GL_TEXTURE_2D_ARRAY with every file name available. 

    If any of the files aren't found: cry and exit. 
*/
void LoadAllTextures()
{
    if (texturenames.length()==0) return ;

    // First: create the texture array, giving it the correct size based on 
    // the number of names in the texturenames array. 
   
    // Now, fetch every texture from disk and send its data to the OpenGL 
    // driver/GPU. 
    loopv(texturenames)
    {
    }
}

/*
*/
GLuint SetMainTextures()
{
//    glGenTexT_mt
}

/*
*/
GLuint& GetMainTextures()
{
    return recalctextures.T_mt ;
}




