#!/bin/sh
exec java -jar /app/bin/ZoopPC-Server-jvm.jar

# Definir permissões para o arquivo .jar
echo "Definindo permissões para ZoopPC-Server.jar..."
chmod 644 ZoopPC-Server-jvm.jar
# Definir permissões para o script de execução
echo "Definindo permissões para run-java.sh..."
chmod +x run-java.sh

echo "Permissões definidas com sucesso."
