document.addEventListener('DOMContentLoaded', () => {

    const dialogOverlay = document.getElementById('dialogOverlay');
    const chooseFileBtn = document.getElementById('chooseFileBtn');
    const enterUrlBtn = document.getElementById('enterUrlBtn');
    const fileInput = document.getElementById('fileInput');
    const mediaPlayer = document.getElementById('mediaPlayer');
    const playPauseBtn = document.getElementById('playPauseBtn');
    const seekBar = document.getElementById('seekBar');
    const timeDisplay = document.getElementById('timeDisplay');
    const volumeBar = document.getElementById('volumeBar');
    const settingsBtn = document.getElementById('settingsBtn');
    const settingsPanel = document.getElementById('settingsPanel');
    const autoplayCheckbox = document.getElementById('autoplayCheckbox');
    const loopCheckbox = document.getElementById('loopCheckbox');
    const saveSettingsBtn = document.getElementById('saveSettingsBtn');
    const urlDialogOverlay = document.getElementById('urlDialogOverlay');
    const settingsDialogOverlay = document.getElementById('settingsDialogOverlay');
    const urlInput = document.getElementById('urlInput');
    const submitUrlBtn = document.getElementById('submitUrlBtn');
    const cancelUrlBtn = document.getElementById('cancelUrlBtn');
    const ccBtn = document.getElementById('ccBtn'); // CC button
    const volumeBtn = document.getElementById("volumeBtn")
    const subtitlesOverlay = document.getElementById('subtitlesOverlay');
    const subtitlesInput = document.getElementById('subtitlesInput');
    const submitSubtitlesBtn = document.getElementById('submitSubtitlesBtn');
    const cancelSubtitlesBtn = document.getElementById('cancelSubtitlesBtn');
    const customControls = document.getElementById('customControls');
    let hls = null
    let player = null
    window.hls = hls; 
    window.dash = player;

    async function detectStreamType(url) {
  try {
    const response = await fetch(url, { method: 'HEAD' });
    const contentType = response.headers.get('Content-Type') || '';

    const isHLS = url.toLowerCase().endsWith('.m3u8') ||
      contentType.includes('application/vnd.apple.mpegurl') ||
      contentType.includes('application/x-mpegURL');

    const isDASH = url.toLowerCase().endsWith('.mpd') ||
      contentType.includes('application/dash+xml');

    return { isHLS, isDASH, contentType };
  } catch (err) {
    console.error("Failed to detect stream type:", err);
    return { isHLS: false, isDASH: false, contentType: null };
  }
}

    // Update media volume when the slider is moved
  volumeBar.addEventListener("input", function () {
    mediaPlayer.volume = volumeBar.value;
  });

  // Sync slider with media volume (in case it's changed programmatically)
  mediaPlayer.addEventListener("volumechange", function () {
    volumeBar.value = mediaPlayer.volume;
    if (mediaPlayer.muted || mediaPlayer.volume === 0) {
      volumeBtn.textContent = "🔇";
    } else {
      volumeBtn.textContent = "🔊";
    }
  });

async function addSubtitles(url) {
  // Remove existing tracks and revoke any previous blob URLs
  const existingTracks = mediaPlayer.getElementsByTagName('track');
  for (let i = existingTracks.length - 1; i >= 0; i--) {
    const track = existingTracks[i];
    if (track.src.startsWith('blob:')) URL.revokeObjectURL(track.src);
    track.remove();
  }

  // Fetch subtitle content
  let text = '';
  try {
    const res = await fetch(url);
    text = await res.text();
  } catch (err) {
    console.error('Failed to fetch subtitles:', err);
    return;
  }

  // Detect format
  const firstLine = text.split(/\r?\n/)[0].trim();
  const format = firstLine.startsWith('WEBVTT') || url.toLowerCase().endsWith('.vtt') ? 'vtt' : 'srt';

  // Determine track source
  let trackSrc = url;
  if (format === 'srt') {
    // Convert SRT → VTT
    text = 'WEBVTT\n\n' + text
      .replace(/\r+/g, '')
      .replace(/^\s+|\s+$/g, '')
      .split('\n')
      .map(line => line.replace(/(\d+):(\d+):(\d+),(\d+)/g, '$1:$2:$3.$4'))
      .join('\n');

    // Create Blob URL for converted subtitles
    const blob = new Blob([text], { type: 'text/vtt' });
    trackSrc = URL.createObjectURL(blob);
  }

  // Create and append track
  const track = document.createElement('track');
  track.kind = 'subtitles';
  track.label = 'English';
  track.srclang = 'en';
  track.src = trackSrc;
  track.default = true;
  mediaPlayer.appendChild(track);

  // Force enable immediately
  setTimeout(() => {
    for (let t of mediaPlayer.textTracks) t.mode = 'disabled';
    track.track.mode = 'showing';
  }, 50);
}



/*// Handle submit subtitle URL
function clearSubtitles() {
  const tracks = mediaPlayer.getElementsByTagName('track');
  for (let i = tracks.length - 1; i >= 0; i--) {
    tracks[i].remove();
  }
}*/

function clearSubtitles() {
  const tracks = mediaPlayer.getElementsByTagName('track');
  for (let i = tracks.length - 1; i >= 0; i--) {
    const track = tracks[i];
    if (track.src.startsWith('blob:')) {
      URL.revokeObjectURL(track.src); // free memory leaks from SRT to VTT converted subtitles
    }
    track.remove();
  }
}


// Use this function when a new video is loaded

submitSubtitlesBtn.addEventListener('click', () => {
  const subtitleUrl = subtitlesInput.value;
  if (subtitleUrl) {
    addSubtitles(subtitleUrl);
    subtitlesOverlay.style.display = 'none';
    subtitlesInput.value = '';
  }
});



    let autoplayEnabled = true;
    let loopEnabled = false;

    // Handle submit URL button in custom dialog
submitUrlBtn.addEventListener('click', async () => {
  let url = urlInput.value;

  // Check if URL is a valid URL and doesn't contain "http" or "https"
  if (url && !url.startsWith('http') && !url.startsWith('https')) {
    // Assuming it's a URL and needs the protocol added
    url = 'http://' + url;  // You can also choose 'https://' if preferred
  }

const { isHLS, isDASH } = await detectStreamType(url);
  

  if (url) {
    clearSubtitles();
    if (hls !== null) {
      hls.destroy()
      hls = null
      window.hls = hls
    }
    if (player !== null) {
      player.reset()
      player = null
      window.dash = player
    }
    if (url.toLowerCase().endsWith('.m3u8') || url.toLowerCase().endsWith('.m3u') || isHLS) {
      // HLS stream
      if (Hls.isSupported()) {
        mediaPlayer.style.display = 'flex'; // Hide the native video player
        hls = new Hls();
        window.hls = hls
        mediaPlayer.pause();
        hls.loadSource(url);
        hls.attachMedia(mediaPlayer);
        hls.on(Hls.Events.MANIFEST_PARSED, function() {
          if (autoplayCheckbox.checked) {
            mediaPlayer.play();
          }
          urlInput.value = "";
          customControls.style.display = 'flex';
        });
        window.hls = hls
      } else {
        alert("Your device doesn't support HLS.");
        customControls.style.display = 'flex';
        urlInput.value = "";
      }
    } else if (url.toLowerCase().endsWith('.mpd') || isDASH) {
      mediaPlayer.style.display = 'flex'; // Hide the native video player
      mediaPlayer.pause();
      player = dashjs.MediaPlayer().create();
      window.dash = player
      // MPEG-DASH stream
      player.initialize(mediaPlayer, url, true);
      customControls.style.display = 'flex';
      urlInput.value = "";
      if (autoplayCheckbox.checked) {
        mediaPlayer.play();
      }
      window.dash = player
    } else {
      mediaPlayer.style.display = 'flex'; // Hide the native video player
      mediaPlayer.pause();
      mediaPlayer.src = url;
      customControls.style.display = 'flex';
      urlInput.value = "";
      if (autoplayCheckbox.checked) {
        mediaPlayer.play();
      }
    }
    urlDialogOverlay.style.display = 'none';
    dialogOverlay.style.display = 'none';
  }
});



    // Handle CC button to show subtitle modal
    ccBtn.addEventListener('click', () => {
      subtitlesOverlay.style.display = 'block';
    });



    // Handle cancel subtitle modal
    cancelSubtitlesBtn.addEventListener('click', () => {
      subtitlesOverlay.style.display = 'none';
    });


    // Show the dialog on page load
    window.onload = function () {
      dialogOverlay.style.display = 'block';
    };



    // Handle "Choose a File" button
    chooseFileBtn.addEventListener('click', () => {
      fileInput.click();
    });

    mediaPlayer.addEventListener("volumechange", function () {
      if (mediaPlayer.muted) {
        volumeBtn.textContent = "🔇"
      } else if (mediaPlayer.volume === 0) {
        volumeBtn.textContent = "🔊"
      }
    });
    


   
    let previousObjectURL = null; // Store the last Object URL
    window.objectURL = previousObjectURL

    fileInput.addEventListener('change', (event) => {
      if (hls !== null) {
        hls.destroy()
        hls = null
        window.hls = hls
      }
      if (player !== null) {
        player.reset()
        player = null
        window.dash = player
      }
        const file = event.target.files[0];
        if (!file) return;
    
        clearSubtitles(); // Remove any previously loaded subtitles
    
        // Revoke the previous Object URL if it exists
        if (previousObjectURL) {
            URL.revokeObjectURL(previousObjectURL);
            window.objectURL = previousObjectURL
        }

        // Revoke previous file picker Object URL
    if (window.previousDropURL) {
        URL.revokeObjectURL(window.previousDropURL);
    }
    
        // Create a new Object URL for the selected file
        const fileURL = URL.createObjectURL(file);
        mediaPlayer.src = fileURL;
        mediaPlayer.load();
        if (autoplayCheckbox.checked) {
        mediaPlayer.play();
        }
    
        // Store the new Object URL for future cleanup
        previousObjectURL = fileURL;
        fileInput.value = "";
    
        // Hide dialog after selecting a file
        dialogOverlay.style.display = 'none';
    });
    

    // Handle "Enter a URL" button
    enterUrlBtn.addEventListener('click', () => {
      urlDialogOverlay.style.display = 'block';
    });

    // Handle cancel button in URL dialog
    cancelUrlBtn.addEventListener('click', () => {
      urlDialogOverlay.style.display = 'none';
    });

    // Handle custom play/pause button
playPauseBtn.addEventListener('click', () => {
  if (mediaPlayer.paused) {
    mediaPlayer.play();
    playPauseBtn.textContent = 'Pause';
  } else {
    mediaPlayer.pause();
    playPauseBtn.textContent = 'Play';
  }
});

// Sync button with the video player when it is paused manually
mediaPlayer.addEventListener('pause', () => {
  playPauseBtn.textContent = 'Play';
});

// Sync button with the video player when it is played
mediaPlayer.addEventListener('play', () => {
  playPauseBtn.textContent = 'Pause';
});

// Volume button toggling mute/unmute
volumeBtn.addEventListener('click', () => {
  if (mediaPlayer.muted || mediaPlayer.volume == 0) {
    mediaPlayer.muted = false;
    volumeBtn.textContent = '🔊'; // Unmute icon
  } else {
    mediaPlayer.muted = true;
    volumeBtn.textContent = '🔇'; // Mute icon
  }
});


// Handle URL input on Enter key
urlInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') {
    submitUrlBtn.click();
  }
});

// Handle Subtitles input on Enter key
subtitlesInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') {
    submitSubtitlesBtn.click();
  }
});

// Handle URL submission


    // Update seek bar and time display
    mediaPlayer.addEventListener('timeupdate', () => {
      seekBar.max = mediaPlayer.duration || 0;
      seekBar.value = mediaPlayer.currentTime;
      const current = formatTime(mediaPlayer.currentTime);
      const total = formatTime(mediaPlayer.duration);
      timeDisplay.textContent = `${current} / ${total}`;
    });

    // Seek media
    seekBar.addEventListener('input', () => {
      mediaPlayer.currentTime = seekBar.value;
    });

    // Handle volume
    volumeBar.addEventListener('input', () => {
      mediaPlayer.volume = volumeBar.value;
      if (volumeBar.value == 0 || mediaPlayer.volume == 0) {
        volumeBtn.textContent = "🔇";
      } else {
        volumeBtn.textContent = "🔊";
      }
    });

    // Show settings panel
    settingsBtn.addEventListener('click', () => {
      settingsDialogOverlay.style.display = 'block';
      settingsPanel.style.display = 'block';
    });

    saveSettingsBtn.addEventListener('click', () => {
      autoplayEnabled = autoplayCheckbox.checked;
      loopEnabled = loopCheckbox.checked;
      mediaPlayer.autoplay = autoplayEnabled;
      mediaPlayer.loop = loopEnabled;
      
      const controlsEnabled = document.getElementById('controlsCheckbox').checked;
      const colorsEnabled = document.getElementById('colorsCheckbox').checked;
      mediaPlayer.controls = controlsEnabled;
      if (colorsEnabled) {
        mediaPlayer.style.filter = "contrast(1.1) saturate(1.15) brightness(1.03)";
      } else {
        mediaPlayer.style.filter = "";
      }

      settingsPanel.style.display = 'none';
      settingsDialogOverlay.style.display = 'none';
    });
    

// End of first event listener for DOM content loaded

    // Format time
    function formatTime(time) {
      const minutes = Math.floor(time / 60) || 0;
      const seconds = Math.floor(time % 60) || 0;
      return `${minutes}:${seconds.toString().padStart(2, '0')}`;
    }

    const showDialogBtn = document.getElementById('showDialogBtn');
    const hideDialogBtn = document.getElementById('hideDialogBtn');
    const fullscreenBtn = document.getElementById('fullscreenBtn');

    // Show dialog
    showDialogBtn.addEventListener('click', () => {
      dialogOverlay.style.display = 'block';
    });

    // Hide dialog
    hideDialogBtn.addEventListener('click', () => {
      dialogOverlay.style.display = 'none';
    });

    // Fullscreen functionality
    fullscreenBtn.addEventListener('click', () => {
      if (!document.fullscreenElement) {
        mediaPlayer.requestFullscreen();
      } else {
        document.exitFullscreen();
      }
    });


// End of code and second event listener for DOM content loaded

});
