
/*
 * 	The philosophy of menus: 
 *
 * 	A menu is any number of items which have a display and an 
 * 	associated set of key bindings. 
 *
 * 	Typically, the display will be a label and/or possibly an image, 
 * 	while the bound commands will be focus movement for the arrows, 
 * 	and a single executable command bound to enter. 
 *
 * 	Menus can be used to browse things: 
 * 		- files
 * 		- resources (maps, models, sounds, music, textures, etc.)
 * 	and menus can also be used to choose current mode (play, edit, 
 * 	demo, etc.)
 */
 
// General TODO: move from initializing menus here to doing so in a 
// script file. 

#include "recalc.h"


#define MAIN_MENU 0
#define MAP_MENU 1

/* 
    The menu commands array. Every menu that is activated 
    populates this array with commands that it wants to enable. 
*/
void (* menu_commands[320])(void *) ; 

void EditMap(void) ;
void LoadMapNames(bool toConsole) ;

Menu* CurrentMenu = NULL ;      // Tracks which menu is currently in use. Possibly none. 
Menu mainmenu ;
Menu mapmenu ;

void menu_select(void*) 
{
    playsound(0) ;
    CurrentMenu->select() ;
}

void menu_go_back(void*)
{
    if (CurrentMenu->PreviousMenu)
    {
        Menu* OldMenu = CurrentMenu ;
        CurrentMenu = CurrentMenu->PreviousMenu ;
        OldMenu->PreviousMenu = NULL ;
    }
    else    // Cancel menu
    {
        EditNewMap() ; //TODO: replace this with something
    }

}

void menu_down(void*)
{
    playsound(0) ;
    CurrentMenu->menu_down() ;
    //printf("\nexecuted menu down. \n") ;
}
void menu_up(void*)
{
    playsound(0) ;
    CurrentMenu->menu_up() ;
    //printf("\nexecuted menu up. \n") ;
}

Menu* GetCurrentMenu()
{
    return CurrentMenu ;
}

/*
*/
void initialize_menu()
{
}

/*
    Used for the Edit Maps menu. 
    
    Every entry is named by the map is refers to, 
    and its command is this function, which opens 
    the map in question and then enables editing, 
    and removes the menu. 
*/
void EditMap(void)
{
    // Check if the map exists. 
    LoadWorld(CurrentMenu->items[CurrentMenu->CurrentMenuItem].name) ;
    
    Engine& e = GetEngine() ;
    e.menu = false ;
    e.editing = true ;
    e.physics = true ;
    e.paused = false ;
    e.rendering = true ;
    setcommands(NULL) ;
}

/*
    This function is used to pass names of maps to the maps menu. 
    
    Or, when debugging, check the contents of the maps directory 
    in the console. 
*/
//vector<MenuItem> MapMenuItems ;
vector<char*> mapfiles ;
void LoadMapNames(bool toConsole)
{
    bool refreshflagged = false ;
    if (mapfiles.length()==0 || refreshflagged)
    {
        mapfiles.deletearrays() ;
        listdir("data/maps", NULL, mapfiles) ;
        listdir("data/user_maps", NULL, mapfiles) ;
        
        
        loopv(mapfiles) 
        {
            if (strcmp(mapfiles[i], ".")==0)    continue; 
            if (strcmp(mapfiles[i], "..")==0)   continue ;

            if (toConsole)
            {
                printf("\nfile: ") ;
                printf("\t%s", mapfiles[i]) ;
            }
            else
            {
                MenuItem mapMenuItem   = {"", EditMap} ;
                strcpy(&mapMenuItem.name[0], mapfiles[i]) ;
                mapmenu.add(mapMenuItem) ;
            }
            
            char msg[128] ;
            sprintf(msg, "map listed: %s", mapfiles[i]) ;
            GetConsole().message(msg) ;
        }
    }
}


/*
*/
void EditNewMap()
{
    Engine& e = GetEngine() ;
    e.menu = false ;
    e.editing = true ;
    e.physics = true ;
    e.paused = false ;
    e.rendering = true ;
    setcommands(NULL) ;
}


void MenuQuit()
{
    Quit(0) ;
}





/*
    Function: ActivateMainMenu

    The rule for menus: every time a menu is activated, it replaces any currently 
    active menu. 

    If there is no menu currently active, it pauses the engine (always?) and then 
    pushes itself on top of the render stack. 
*/
void ActivateMainMenu()
{
    // Add main menu render function to 
    mapmenu.PreviousMenu = NULL ;
    CurrentMenu = &mainmenu ;
    
    Engine& e = GetEngine() ;
    e.menu = true ;
    e.editing = false ;
    e.physics = false ;
    
    setcommands(menu_commands) ;
}

/*
void ToggleMainMenu()
{
    Engine& e = GetEngine() ;
    e.menu = e.menu ;
    e.editing = !e.editing ;
    e.physics = !e.physics ;
    
}
*/

//
//  MAPS MENU
//

/*
    DESCRIPTION: 
        The Map Menu Shows maps available for loading. Activating it means 
        we place this menu's render function on top of the render stack, 
        and we set the input handlers to reflect this menu's behaviors (mostly 
        just browse the menu and either select something or exit). 
*/
void ActivateMapMenu() 
{
    mapmenu.PreviousMenu = CurrentMenu ;
    CurrentMenu = &mapmenu ;
    
    Engine& e = GetEngine() ;
    e.menu = true ;
    e.editing = false ;
    e.physics = false ;
    setcommands(menu_commands) ;
    
    // Loop through available map names and populate our menu with these. 
    LoadMapNames(false) ;
}


//
//  MAIN MENU
//

// Menu Items to populate Main Menu
MenuItem MIeditnewmap = { "Edit New Map",    EditNewMap } ;
MenuItem MIeditmap    = { "Edit Map",        ActivateMapMenu } ;
MenuItem MInewgame    = { "New Game",        NULL } ;
MenuItem MIloadgame   = { "Load Game",       NULL } ;
MenuItem MIsavegame   = { "Save Game",       NULL } ;
MenuItem MIexitrecalc = { "Exit Recalc",     MenuQuit } ;





void init_menus() 
{
    printf("\n[MENU::init_menus] called... ") ;
    mainmenu.activate = &ActivateMainMenu ;
   // mapmenu.activate = &ActivateMapMenu ;

    // Main menu
    mainmenu.add( MIeditnewmap ) ;
    mainmenu.add( MIeditmap ) ;
    mainmenu.add( MInewgame ) ;
    mainmenu.add( MIloadgame ) ;
    mainmenu.add( MIsavegame ) ;
    mainmenu.add( MIexitrecalc ) ;

    // Edit map menu (show available maps)
    // Load game menu (show available games to load)
    // Save game menu

    // Menu commands. the main menu is active when the 
    // program starts. 
    menu_commands[SDLK_RETURN] = &menu_select ;
    menu_commands[SDLK_ESCAPE] = &menu_go_back ;
    menu_commands[SDLK_e] = &menu_select ;
    menu_commands[SDLK_DOWN] = &menu_down ;
    menu_commands[SDLK_s] = &menu_down ;
    menu_commands[SDLK_UP] = &menu_up ;
    menu_commands[SDLK_w] = &menu_up ;
    //menu_commands[SDLK_BACKQUOTE] = &menu_up ;

    ActivateMainMenu() ; 
    
    printf("done. ") ;
}


//typedef void (**commandptr)(void*) ;
commandptr GetMenuCommands() 
{
    return menu_commands ;
}



