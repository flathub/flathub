(function loadDevToolsScript() {
  const DDR_DART_APP_ATTRIBUTE = 'data-ddr-dart-app';
  // Note: Changes to the DEBUGGER_PANEL_NAME and INSPECTOR_PANEL_NAME
  // must be reflected in `tool/update_dev_files.dart` as well.
  const DEBUGGER_PANEL_NAME = 'Dart Debugger';
  const INSPECTOR_PANEL_NAME = 'Flutter Inspector';

  let debuggerCreated = false;
  let inspectorCreated = false;
  let checkDartCount = 0;
  let checkFlutterCount = 0;

  chrome.devtools.network.onNavigated.addListener(createDebuggerPanelIfDartApp)
  const checkDartAppInterval = setInterval(createDebuggerPanelIfDartApp, 1000)
  createDebuggerPanelIfDartApp()

  function createDebuggerPanelIfDartApp() {
    if (debuggerCreated || checkDartCount++ > 20) {
      clearInterval(checkDartAppInterval);
      return;
    }

    checkIsDartApp();
  }

  function checkIsDartApp() {
    // TODO(elliette): Remove the DDR data attribute check when we are ready to launch externally,
    // and instead replace it with the following: !!window.$dartAppId 
    // Note: we must remove the useContentScriptContext option as well.
    chrome.devtools.inspectedWindow.eval(
      `document.documentElement.hasAttribute("${DDR_DART_APP_ATTRIBUTE}")`,
      { useContentScriptContext: true },
      function (isDartApp) {
        if (!isDartApp) return;

        chrome.devtools.panels.create(
          DEBUGGER_PANEL_NAME, '', 'debugger_panel.html'
        );
        debuggerCreated = true;
        createInspectorPanelIfFlutterApp();
      });
  }

  function createInspectorPanelIfFlutterApp() {
    const checkFlutterAppInterval = setInterval(function () {
      if (inspectorCreated || checkFlutterCount++ > 10) {
        clearInterval(checkFlutterAppInterval);
        return;
      }

      // The following value is loaded asynchronously, which is why
      // we check for it every 1 second:
      chrome.devtools.inspectedWindow.eval(
        '!!window._flutter_web_set_location_strategy',
        function (isFlutterWeb) {
          if (isFlutterWeb) {
            chrome.devtools.panels.create(
              INSPECTOR_PANEL_NAME, '', 'inspector_panel.html'
            );
            inspectorCreated = true;
          }
        }
      );
    }, 1000)
  }
}());

