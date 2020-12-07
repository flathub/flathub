build-commands:
  # Install npm dependencies
  - npm install --prefix=main --offline --cache=/run/build/pocket-browser/npm-cache/
  # Bundle app and dependencies
  - mkdir -p /app/main /app/bin
  - cp -ra main/* /app/main/
  # Install app wrapper
  - install install.sh /app/bin/
