# org.apache.netbeans
Flatpak for Apache Netbeans (https://netbeans.apache.org/)

## How to build Apache Netbeans

```
flatpak-builder --repo=repo flatpakbuildir org.apache.netbeans.json --force-clean
```

## Add Apache Netbeans repo to remote

```
flatpak remote-add --user mynetbeans repo
```

## How to install Apache Netbeans from flatpak

```
flatpak install --user mynetbeans org.apache.netbeans
```

## How to run Apache Netbeans

```
flatpak run org.apache.netbeans
```

## How tu uninstall Apache Netbeans

```
flatpak uninstall --user org.apache.netbeans
```
