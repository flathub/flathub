
#!/bin/bash

# export 
echo "exporting flatpak local repo..."
flatpak build-export repo builddir

# attempt to run it
echo "atempting to run the flatpak..."
flatpak run dev.overlayed.Overlayed