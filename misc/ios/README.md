# Smokin' Guns — build iOS

Client iOS avec contrôles tactiles et multijoueur LAN.

## Prérequis

- macOS avec Xcode et le SDK iPhoneOS
- Dossiers `baseq3/`, `smokinguns/` et `ui/` à la racine du dépôt
- Compte Apple Developer pour installer sur appareil

## Ouvrir dans Xcode

```bash
open misc/ios/SmokinGuns.xcodeproj
```

Dans **Signing & Capabilities**, choisissez votre équipe de développement, puis sélectionnez un iPhone/iPad connecté et lancez (⌘R).

Le projet exécute `misc/ios/build-ios.sh` (compilation via Make + packaging `.app`).

## Build en ligne de commande

```bash
chmod +x misc/ios/*.sh
./misc/ios/setup_sdl2.sh   # une fois, si libSDL2-ios.a est absent
./misc/ios/build-ios.sh
```

Produit : `build/ios/SmokinGuns.app`

## Réseau LAN

- `NSLocalNetworkUsageDescription` est dans `Info.plist`
- Host : menu **Create Server** (listen server)
- Les autres clients se connectent à l’IP LAN de l’hôte
