#include "webview.h"
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInt, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main() {
#endif
  webview::webview w(true, nullptr);
  w.set_title("Photopea");
  w.set_size(1920, 1080, WEBVIEW_HINT_NONE);
  w.navigate("https://photopea.com");
  w.run();
  return 0;

}
