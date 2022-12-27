#!/usr/bin/bash

echo '#!/usr/bin/bash' > /app/bin/blackmore-qt
echo "exec /app/bin/internal/blackmore-qt" '-datadir="${XDG_DATA_HOME}" "$@"' >> /app/bin/blackmore-qt
chmod 744 /app/bin/blackmore-qt
