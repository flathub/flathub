#!/usr/bin/bash

echo '#!/usr/bin/bash' > /app/bin/peercoin-qt
echo "exec /app/bin/internal/peercoin-qt" '-datadir="${XDG_DATA_HOME}" "$@"' >> /app/bin/peercoin-qt
chmod 744 /app/bin/peercoin-qt
