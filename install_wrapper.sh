#!/usr/bin/bash

echo '#!/usr/bin/bash' > /app/bin/goldcoin-qt
echo "exec /app/bin/internal/goldcoin-qt" '-datadir="${XDG_DATA_HOME}" "$@"' >> /app/bin/goldcoin-qt
chmod 744 /app/bin/goldcoin-qt
