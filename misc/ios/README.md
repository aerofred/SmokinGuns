# Smokin' Guns — build iOS (dev)

Client iOS avec listen server (host de partie in-app), multijoueur LAN, contrôles tactiles.

## Prérequis

- macOS avec Xcode et SDK iPhoneOS
- Dossiers `baseq3/` et `smokinguns/` à la racine du projet (déjà présents localement, ignorés par git)
- Compte Apple Developer pour installer sur appareil

## Build rapide

```bash
# 1) SDL2 pour iOS (une fois)
chmod +x misc/ios/setup_sdl2.sh misc/ios/build-ios.sh misc/ios/copy_game_data.sh
./misc/ios/setup_sdl2.sh

# 2) Binaire + .app
./misc/ios/build-ios.sh
```

Produit : `build/ios/SmokinGuns.app`

## Xcode

Ouvrir `misc/ios/SmokinGuns.xcodeproj` pour signer et déployer sur un iPhone/iPad.
Le projet exécute `build-ios.sh` puis installe l’app sur l’appareil.

## Réseau LAN

- `NSLocalNetworkUsageDescription` est dans `Info.plist`
- Host : menu **Create Server** (listen server, `dedicated 0`)
- Les autres clients se connectent à l’IP LAN affichée (console `net_ip` ou réglages Wi‑Fi)
- Garder l’app au premier plan pendant l’hébergement (pas de serveur en arrière-plan)

## Technique

- Moteur : QVM (`vm_* = 2`), renderer GL1 + couche OpenGL ES 2 (`gles_immediate.c`)
- Pas de `ioq3ded` : `BUILD_SERVER=0`
- SDL 2, pas de SDL 1.2
