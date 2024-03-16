/*
 * Entry point for Handmade here
 *
 * Will try to implement as c only and not pull in any cpp stuff ... 
 *      we'll see how that pans out
 */

#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    return MessageBox(NULL, "This is Handmade Hero", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
}
