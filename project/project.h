/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  DISPLAY                          1
#define  DISPLAY_CANVAS                   2       /* control type: canvas, callback function: canvafunction */
#define  DISPLAY_RESTART                  3       /* control type: command, callback function: (none) */
#define  DISPLAY_SAVE                     4       /* control type: command, callback function: SAVEFILEFUNC */
#define  DISPLAY_SCORE                    5       /* control type: numeric, callback function: (none) */
#define  DISPLAY_DISPLAY_TIMER            6       /* control type: timer, callback function: MoveBalloon */

#define  MAIN                             2
#define  MAIN_QUITBUTTON                  2       /* control type: command, callback function: QuitCallback */
#define  MAIN_STARTBUT                    3       /* control type: command, callback function: startgamefunc */
#define  MAIN_SCORES                      4       /* control type: numeric, callback function: (none) */
#define  MAIN_GAMELEVEL                   5       /* control type: binary, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK canvafunction(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MoveBalloon(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SAVEFILEFUNC(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK startgamefunc(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif