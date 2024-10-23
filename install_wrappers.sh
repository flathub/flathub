#!/usr/bin/bash

for b in dash{d,-cli,-qt,-tx,-wallet}
do
  echo '#!/usr/bin/bash' > /app/bin/$b
  echo "exec /app/bin/internal/$b" '-datadir="${XDG_DATA_HOME}" "$@"' >> /app/bin/$b
  chmod 744 /app/bin/$b
done
