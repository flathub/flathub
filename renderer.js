let mediaElement = document.getElementById("mediaPlayer");


function loadMedia(fileURL) {
  dialogOverlay.style.display = 'none';

  mediaElement.oncanplay = null;

  if (mediaElement) {
    mediaElement.src = fileURL; // ✅ Safe, properly encoded URL
    mediaElement.oncanplay = () => {
      if (autoplayCheckbox && autoplayCheckbox.checked) {
        mediaElement.play().catch(error => console.warn("Playback issue:", error));
      }
    };
  }
}

window.addEventListener('DOMContentLoaded', () => {
const dropArea = document.createElement('div');
dropArea.style.position = 'fixed';
dropArea.style.top = '0';
dropArea.style.left = '0';
dropArea.style.width = '100vw';
dropArea.style.height = '100vh';
dropArea.style.display = 'none'; // hidden by default
dropArea.style.zIndex = '0';     // behind everything
dropArea.style.opacity = '0';    // invisible
document.body.appendChild(dropArea);

let dragCounter = 0; // track nested dragenter/dragleave events


window.addEventListener('dragenter', (e) => {
    if (e.dataTransfer.types.includes('Files')) {
        dragCounter++;
        e.preventDefault();
        e.stopPropagation();
    }
});

window.addEventListener('dragleave', (e) => {
    if (e.dataTransfer.types.includes('Files')) {
        dragCounter--;
        if (dragCounter <= 0) dragCounter = 0;
        e.preventDefault();
        e.stopPropagation();
    }
});

window.addEventListener('dragover', (e) => {
    if (e.dataTransfer.types.includes('Files')) {
        e.preventDefault(); // allow drop
        e.stopPropagation();
    }
});


let previousDropURL = null; // Store last Object URL
window.previousDropURL = previousDropURL

window.addEventListener('drop', e => {
    e.preventDefault();
    e.stopPropagation();

    const file = e.dataTransfer.files[0];
    if (!file) return;

    // Destroy existing HLS/DASH instances if they exist
    if (window.hls) {
        window.hls.destroy();
        window.hls = null;
    }
    if (window.dash) {
        window.dash.reset();
        window.dash = null;
    }

    // Clear previously loaded subtitles
    const tracks = mediaElement.getElementsByTagName('track');
    for (let i = tracks.length - 1; i >= 0; i--) {
        tracks[i].remove();
    }

    // Revoke previous Object URL
    if (previousDropURL) {
        URL.revokeObjectURL(previousDropURL);
        window.previousDropURL = previousDropURL;
    }

    // Revoke previous file picker Object URL
    if (window.objectURL) {
        URL.revokeObjectURL(window.objectURL);
    }

    // Create a new Object URL
    const fileURL = URL.createObjectURL(file);
    mediaElement.src = fileURL;

    mediaElement.load();
      // Autoplay if checkbox is checked
    if (autoplayCheckbox.checked) {
        mediaElement.play().catch(err => console.warn(err));
    }

    // Store for future cleanup
    previousDropURL = fileURL;

    // Hide file dialog if applicable
    if (dialogOverlay) dialogOverlay.style.display = 'none';
});


});

// Handle submit subtitle URL
function clearSubtitles() {
  const tracks = mediaElement.getElementsByTagName('track');
  for (let i = tracks.length - 1; i >= 0; i--) {
    tracks[i].remove();
  }
}

// Validate media URL
function isSafeURL(fileURL) {
  try {
    const url = new URL(fileURL);
    return url.protocol === "file:";
  } catch (error) {
    return false;
  }
}

// Load addon script dynamically
function loadAddon(fileURL) {
  // Avoid duplicate scripts
  if (document.querySelector(`script[data-addon="${fileURL}"]`)) return;

  const script = document.createElement('script');
  script.src = fileURL;
  script.type = 'text/javascript';
  script.async = false; // optional, depends on your needs
  script.setAttribute('data-addon', fileURL);

  document.head.appendChild(script);

  console.log(`addon loaded: ${fileURL}`)
  alert("Addon loaded successfully");
}

// Unload addon script by removing the <script> tag
function unloadAddon(fileURL) {
  const script = document.querySelector(`script[data-addon="${fileURL}"]`);
  if (script) {
    script.remove();
    console.log(`addon unloaded: ${fileURL}`)
    alert("Addon unloaded successfully");
  } else {
    console.warn(`No addon script found for: ${fileURL}`);
  }
}


// ✅ Listen for events from main process securely
window.electron.receive("play-media", (fileURL) => {
  if (isSafeURL(fileURL)) {
    clearSubtitles()
    if (window.hls) {
      window.hls.destroy()
      window.hls = null
    }
    if (window.dash) {
      window.dash.reset()
      window.dash = null
    }
    loadMedia(fileURL);
  } else {
    console.warn("Blocked unsafe media URL:", fileURL);
  }
});

// Listen for load-addon message
window.electron.receive("load-addon", (fileURL) => {
  if (isSafeURL(fileURL)) {
    loadAddon(fileURL);
  } else {
    console.warn("Blocked unsafe script URL:", fileURL);
    alert("There was an issue loading your addon: it may be unsafe");
  }
});

// Listen for unload-addon message
window.electron.receive("unload-addon", (fileURL) => {
    unloadAddon(fileURL);
});





