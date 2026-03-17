import React from "react";
import ReactDOM from "react-dom/client";
import "@/lib/i18n";
import App from "./App";
import { FullscreenPlayerView } from "@/components/Player/FullscreenPlayerView";
import "@/styles/globals.css";

const isFullscreenPlayer =
  new URLSearchParams(window.location.search).get("mode") ===
  "fullscreen-player";

ReactDOM.createRoot(document.getElementById("root") as HTMLElement).render(
  <React.StrictMode>
    {isFullscreenPlayer ? <FullscreenPlayerView /> : <App />}
  </React.StrictMode>,
);
